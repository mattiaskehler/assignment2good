
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 

#define Y_DIRECTION_VOLTAGE "/sys/bus/iio/devices/iio:device0/in_voltage2_raw"
#define X_DIRECTION_VOLTAGE "/sys/bus/iio/devices/iio:device0/in_voltage3_raw"

double Joystick_readX()
{
// Open file
FILE *f = fopen(X_DIRECTION_VOLTAGE, "r");
if (!f) {
printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
printf(" Check /boot/uEnv.txt for correct options.\n");
exit(-1);
}
// Get reading
int a2dReading = 0;
int itemsRead = fscanf(f, "%d", &a2dReading);
if (itemsRead <= 0) {
printf("ERROR: Unable to read values from voltage input file.\n");
exit(-1);
}
// Close file
fclose(f);
return a2dReading;
}

double Joystick_readY()
{
// Open file
FILE *f = fopen(Y_DIRECTION_VOLTAGE, "r");
if (!f) {
printf("ERROR: Unable to open voltage input file. Cape loaded?\n");
printf(" Check /boot/uEnv.txt for correct options.\n");
exit(-1);
}
// Get reading
int a2dReading = 0;
int itemsRead = fscanf(f, "%d", &a2dReading);
if (itemsRead <= 0) {
printf("ERROR: Unable to read values from voltage input file.\n");
exit(-1);
}
// Close file
fclose(f);
return a2dReading;
}

int Joystick_dir() {
    double x_value = (Joystick_readX() / 4095.0) * 2 - 1;  
    double y_value = (Joystick_readY() / 4095.0) * 2 - 1;
    if (x_value < -0.5) {
        return 1;  
    }
    else if (x_value > 0.5) {
        return 2;  
    }
    else if (y_value < -0.5) {
        return 3;  
    }
    else if (y_value > 0.5) {
        return 4; 
    }
    else {
        return 0;  
    }
}
