#ifndef LEDMATRIX_H
#define LEDMATRIX_H

#define I2C_DEVICE_ADDRESS 0x70 

typedef enum {
    INT,
    DOUBLE
} DataType;

void writeToDisplay(int display, unsigned char address, unsigned char value);
void updateDisplay(int display, unsigned char led[8]);
int initializeDisplay(char* bus, int address);
void initializeLED(unsigned char led[8], int number);
void initializeFloatLED(unsigned char led[8], float f);
void displayNext(char* bus, void* number, DataType type);

#endif /* LEDMATRIX_H */
