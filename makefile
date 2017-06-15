CC = gcc
CFLAGS = -std=gnu11 -Wall -lwiringPi -lm


all: guidance


#imu.o: imu.c
#	$(CC) -c $^ $(CFLAGS)

guidance: main.o imu.o
	$(CC) -o $@ $^ $(CFLAGS) -lpthread

clean: 
	rm guidance *.o
