#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include "LEDMatrix.h"

#define I2C_DEVICE_ADDRESS 0x70

 unsigned char intPatterns[10][8] = {
    {0x07, 0x05, 0x05, 0x05, 0x05, 0x05, 0x07, 0x00}, // 0
    {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x00}, // 1
    {0x07, 0x04, 0x04, 0x07, 0x01, 0x01, 0x07, 0x00}, // 2
    {0x07, 0x04, 0x04, 0x07, 0x04, 0x04, 0x07, 0x00}, // 3
    {0x05, 0x05, 0x05, 0x07, 0x04, 0x04, 0x04, 0x00}, // 4
    {0x07, 0x01, 0x01, 0x07, 0x04, 0x04, 0x07, 0x00}, // 5
    {0x07, 0x01, 0x01, 0x07, 0x05, 0x05, 0x07, 0x00}, // 6
    {0x07, 0x05, 0x05, 0x04, 0x04, 0x04, 0x04, 0x00}, // 7
    {0x07, 0x05, 0x05, 0x07, 0x05, 0x05, 0x07, 0x00}, // 8
    {0x07, 0x05, 0x05, 0x07, 0x04, 0x04, 0x07, 0x00}  // 9
};

 unsigned char floatPatterns[10][8] = {
    {0x07, 0x05, 0x05, 0x05, 0x05, 0x05, 0x07, 0x08}, // 0
    {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x08}, // 1
    {0x07, 0x04, 0x04, 0x07, 0x01, 0x01, 0x07, 0x08}, // 2
    {0x07, 0x04, 0x04, 0x07, 0x04, 0x04, 0x07, 0x08}, // 3
    {0x05, 0x05, 0x05, 0x07, 0x04, 0x04, 0x04, 0x08}, // 4
    {0x07, 0x01, 0x01, 0x07, 0x04, 0x04, 0x07, 0x08}, // 5
    {0x07, 0x01, 0x01, 0x07, 0x05, 0x05, 0x07, 0x08}, // 6
    {0x07, 0x05, 0x05, 0x04, 0x04, 0x04, 0x04, 0x08}, // 7
    {0x07, 0x05, 0x05, 0x07, 0x05, 0x05, 0x07, 0x08}, // 8
    {0x07, 0x05, 0x05, 0x07, 0x04, 0x04, 0x07, 0x08}  // 9
};

 void writeToDisplay(int display, unsigned char address, unsigned char value) {
    unsigned char buffer[2];
    buffer[0] = address;
    buffer[1] = value;

    int result = write(display, buffer, 2);
    if (result != 2) {
        perror("Failed to write to I2C register");
        exit(-1);
    }
}

 void updateDisplay(int display, unsigned char led[8]) {
    for (int i = 0; i < 16; i += 2) {
        writeToDisplay(display, i, led[i / 2]);
        writeToDisplay(display, i + 1, 0x00);
    }
}

 int initializeDisplay(char* bus, int address) {
    int display = open(bus, O_RDWR);
    if (display < 0) {
        printf("I2C DRV: Failed to open bus for read/write (%s)\n", bus);
        perror("Error:");
        exit(-1);
    }

    int result = ioctl(display, I2C_SLAVE, address);
    if (result < 0) {
        perror("Failed to set I2C device to slave address.");
        exit(-1);
    }
    return display;
}

 void initializeLED(unsigned char led[8], int number) {
    if (number < 0 || number > 99) {
        fprintf(stderr, "Invalid input for LED pattern: %d\n", number);
        exit(-1);
    }
    int tens = number / 10;
    int ones = number % 10;
    for (int i = 0; i < 8; i++) {
        led[i] = (intPatterns[ones][i] << 4) | intPatterns[tens][i];
    }
}

 void initializeFloatLED(unsigned char led[8], float f) {
    if (f < 0.0 || f > 9.9) {
        fprintf(stderr, "Invalid input for float LED pattern: %.2f\n", f);
        exit(-1);
    }
    int number = (int)(f * 10);
    int tens = number / 10;
    int ones = number % 10;
    for (int i = 0; i < 8; i++) {
        led[i] = (intPatterns[ones][i] << 4) | floatPatterns[tens][i];
    }
}

void displayNext(char* bus, void* number, DataType type) {
    unsigned char led[8];
    
    int display = initializeDisplay(bus, I2C_DEVICE_ADDRESS);
    writeToDisplay(display, 0x21, 0x00);
    writeToDisplay(display, 0x81, 0x00);

    if (type == INT) {
        initializeLED(led, *((int*)number));
    } else if (type == DOUBLE) {
        initializeFloatLED(led, *((double*)number));
    }
    
    updateDisplay(display, led);
    
    close(display);
}
