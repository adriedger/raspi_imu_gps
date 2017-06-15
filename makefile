CC = gcc
CFLAGS = -std=gnu11 -Wall -lwiringPi -lm

all: guidance

guidance: main.c
	$(CC) -o $@ $^ $(CFLAGS) -lpthread

clean: 
	rm guidance
