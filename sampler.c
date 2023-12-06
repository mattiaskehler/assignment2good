#include <stdbool.h>
#include "sampler.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <math.h>

#define USER_Button_Value "/sys/class/gpio/gpio72/value"
#define MAX_SAMPLES 1000 
#define A2D_FILE_VOLTAGE "/sys/bus/iio/devices/iio:device0/in_voltage1_raw"
#define A2D_VOLTAGE_REF_V 1.8
#define A2D_MAX_READING 4095

static long long Interval_Total = 0;
static long long Minumum_Interval = LLONG_MAX;
static long long Maximum_Interval = 0;
static double Averagae_Interval = 0.0;

static pthread_t Sample_Thread;
static pthread_t Analysis_Thread;
static pthread_mutex_t bufferMutex = PTHREAD_MUTEX_INITIALIZER;
static samplerDatapoint_t buff[MAX_SAMPLES];

static int Sample_Status = 0; 
static int Amount_Samples = 0;
static long long Samples_taken = 0;

static long long Samples_Analyzed = 0;
static double Voltage_Average = 0.0; 
static double Minimum_Voltage = 0.0;    
static double Maximum_Voltage = 0.0;     

static double Current_Volt_Avg = 0.0;
static double Threshold = 0.1; 
static double Hysteresis = 0.03; 
static bool Dips_Detected = false;

void *analysisFunction(void *arg);

struct AnalysisData analysisData;

struct AnalysisData Sampler_getAnalysisData(void)
{
    return analysisData;
}


static void sleepForMs(long long delayInMs)
{
    const long long NS_PER_MS = 1000 * 1000;
    const long long NS_PER_SECOND = 1000000000;
    long long delayNs = delayInMs * NS_PER_MS;
    int seconds = delayNs / NS_PER_SECOND;
    int nanoseconds = delayNs % NS_PER_SECOND;
    struct timespec reqDelay = {seconds, nanoseconds};
    nanosleep(&reqDelay, (struct timespec *)NULL);
}
int readFromFile(char *fileName)
{
    FILE *pFile = fopen(fileName, "r");
    if (pFile == NULL)
    {
        printf("ERROR: Unable to open file (%s) for read\n", fileName);
        exit(-1);
    }
    const int MAX_LENGTH = 1024;
    char rad[MAX_LENGTH];
    fgets(rad, MAX_LENGTH, pFile);
    fclose(pFile);
    int intValue = atoi(rad);
    return intValue;
}
static long long getTimeInNs(void)
{
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    long long seconds = spec.tv_sec;
    long long nanoSeconds = spec.tv_nsec;
    return seconds * 1000000000 + nanoSeconds;
}

double getVoltageValue()
{
    // Open file
    FILE *f = fopen(A2D_FILE_VOLTAGE, "r");
    if (!f)
    {
        printf("ERROR: Unable to open voltage input file. Check if the ADC cape is loaded.\n");
        exit(-1);
    }

    int a2dReading = 0;
    int itemsRead = fscanf(f, "%d", &a2dReading);
    if (itemsRead <= 0)
    {
        printf("ERROR: Unable to read values from voltage input file.\n");
        fclose(f);
        exit(-1);
    }

    fclose(f);

    double voltage = ((double)a2dReading / A2D_MAX_READING) * A2D_VOLTAGE_REF_V;

    return voltage;
}

static void *samplingFunction(void *arg)
{
    while (Sample_Status)
    {
        double voltage = getVoltageValue();

        pthread_mutex_lock(&bufferMutex);
        if (Amount_Samples < MAX_SAMPLES)
        {
            buff[Amount_Samples].sampleInV = voltage;
            buff[Amount_Samples].timestampInNanoS = getTimeInNs();
            Amount_Samples++;
            Samples_taken++;
        }
        pthread_mutex_unlock(&bufferMutex);

        sleepForMs(1);
    }

    pthread_exit(NULL);
}

void *analysisFunction(void *arg)
{
    double previousAverage = 0.0;
    int dipCount = 0;
    printf("Starting to sample data...\n");

    while (Sample_Status)
    {
        sleepForMs(1000);

        int length;
        samplerDatapoint_t *extractedValues = Sampler_extractAllValues(&length);

        if (length > 0)
        {
            Samples_Analyzed += length;

            for (int i = 0; i < length; i++)
            {
                double weight = 0.001;
                Voltage_Average = (1.0 - weight) * previousAverage + weight * extractedValues[i].sampleInV;
                previousAverage = Voltage_Average;

                if (extractedValues[i].sampleInV < Minimum_Voltage || i == 0)
                {
                    Minimum_Voltage = extractedValues[i].sampleInV;
                }

                if (extractedValues[i].sampleInV > Maximum_Voltage || i == 0)
                {
                    Maximum_Voltage = extractedValues[i].sampleInV;
                }

                if (i > 0)
                {
                    long long intervalTime = extractedValues[i].timestampInNanoS - extractedValues[i - 1].timestampInNanoS;

                    Averagae_Interval = (Averagae_Interval * Interval_Total + intervalTime) / (Interval_Total + 1);
                    Interval_Total++;

                    if (intervalTime < Minumum_Interval)
                    {
                        Minumum_Interval = intervalTime;
                    }
                    if (intervalTime > Maximum_Interval)
                    {
                        Maximum_Interval = intervalTime;
                    }
                }

                Current_Volt_Avg = Voltage_Average;

                if (fabs(extractedValues[i].sampleInV - Current_Volt_Avg) >= Threshold)
                {
                    if (!Dips_Detected)
                    {
                        dipCount++;
                        Dips_Detected = true;
                    }
                }
                else if (Dips_Detected && fabs(extractedValues[i].sampleInV - Current_Volt_Avg) < Hysteresis)
                {
                    Dips_Detected = false;
                }
            }

            printf("Analysis results:\n");
            printf("Number of samples analyzed: %lld\n", Samples_Analyzed);
            printf("Interval time (ms): (%7f, %7f) avg=%.7f\n", (double)Minumum_Interval / 1000000, (double)Maximum_Interval / 1000000, Averagae_Interval / 1000000.0);
            printf("Samples Voltage (V): (%.2f, %.2f) avg=%.2f\n", Minimum_Voltage, Maximum_Voltage, Voltage_Average);
            printf("Number of dips detected: %d\n", dipCount);
            printf("\n");

            analysisData.minVoltage = Minimum_Voltage;
            analysisData.maxVoltage = Maximum_Voltage;
            analysisData.avgVoltage = Voltage_Average;
            analysisData.dips = dipCount;
            analysisData.samples = Samples_Analyzed;
            analysisData.minInterval = (double)Minumum_Interval / 1000000;
            analysisData.maxInterval = (double)Maximum_Interval / 1000000;
            analysisData.avgInterval = Averagae_Interval / 1000000.0;

            free(extractedValues);
            Minumum_Interval = LLONG_MAX;
            Maximum_Interval = 0;
            Averagae_Interval = 0.0;
            Samples_Analyzed = 0;
            Interval_Total = 0;
            dipCount = 0;
        }
    }

    pthread_exit(NULL);
}
void Sampler_startSampling(void)
{
    if (!Sample_Status)
    {
        Sample_Status = 1;
        pthread_create(&Sample_Thread, NULL, samplingFunction, NULL);
        pthread_create(&Analysis_Thread, NULL, analysisFunction, NULL); 
        printf("Sampling thread started.\n");
    }
}

void Sampler_stopSampling(void)
{
    if (Sample_Status)
    {
        Sample_Status = 0;
        pthread_join(Sample_Thread, NULL);
        pthread_join(Analysis_Thread, NULL); 
        printf("Sampling thread stopped.\n");
    }
}

samplerDatapoint_t *Sampler_extractAllValues(int *length)
{
    pthread_mutex_lock(&bufferMutex);

    samplerDatapoint_t *copyBuffer = malloc(Amount_Samples * sizeof(samplerDatapoint_t));
    memcpy(copyBuffer, buff, Amount_Samples * sizeof(samplerDatapoint_t));

    *length = Amount_Samples;

    Amount_Samples = 0;

    pthread_mutex_unlock(&bufferMutex);

    return copyBuffer;
}

/*
made but not used
int Sampler_getAmount_SamplesInHistory(void)
{
    pthread_mutex_lock(&bufferMutex);
    int count = Amount_Samples;
    pthread_mutex_unlock(&bufferMutex);

    return count;
}

long long Sampler_getSamples_taken(void)
{
    return Samples_taken;
}
*/
