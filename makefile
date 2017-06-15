CC = gcc
CFLAGS = -std=gnu11 -Wall -lwiringPi -lm


all: guidance


#imu.o: imu.c
#	$(CC) -c $^ $(CFLAGS)

guidance: main.c
	$(CC) -o $@ $^ $(CFLAGS) -lpthread

clean: 
	rm guidance
