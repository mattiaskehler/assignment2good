all:
	arm-linux-gnueabihf-gcc -Wall -g -std=c99 -D _POSIX_C_SOURCE=200809L -Werror -Wshadow -pthread main.c sampler.c joystick.c LEDMatrix.c -o main
	cp main $(HOME)/cmpt433/public/assignment2
