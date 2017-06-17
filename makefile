CC = gcc
CFLAGS = -std=gnu11 -Wall -lwiringPi -lm -lpthread

all: guidance


guidance: main.o imu.o
	$(CC) -o $@ $^ $(CFLAGS) 

clean: 
	rm guidance *.o
