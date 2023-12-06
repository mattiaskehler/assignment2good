#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include "sampler.h" // Include necessary headers

#define USER_Button_Value "/sys/class/gpio/gpio72/value"

// Define functions used for handling button press and program exit
void handleExit() {
    Sampler_stopSampling(); // Stop sampling threads or perform cleanup
    // Additional cleanup steps if needed

    printf("Exiting program...\n");
    exit(0);
}

int isUserButtonPressed() {
    FILE *buttonFile = fopen(USER_Button_Value, "r");
    if (buttonFile == NULL) {
        perror("Error opening button file");
        exit(EXIT_FAILURE);
    }

    char buttonState;
    fscanf(buttonFile, "%c", &buttonState);
    fclose(buttonFile);

    // Check the state read from the file
    // Return 1 if the button is pressed (based on the specific condition), 0 otherwise
    return (buttonState == '0'); // Example: '0' might indicate a pressed state
}

void signalHandler(int signal) {
    if (signal == SIGINT) {
        handleExit();
    }
}

int main() {
    // Register signal handler to handle Ctrl+C (SIGINT)
    signal(SIGINT, signalHandler);

    Sampler_startSampling(); // Start sampling

    while (1) {
        if (isUserButtonPressed()) {
            handleExit(); // Handle button press and exit
        }
        sleep(1); // Adjust sleep time as needed or use other suitable delay functions

        // Your existing code logic
        // Fetch data, display LED matrix, joystick controls, etc.
        // You can keep your existing code within this loop
    }

    return 0;
}
