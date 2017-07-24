CC = gcc
CFLAGS = -std=gnu11 -Wall -lwiringPi -lm -lpthread
DEPS = imu.h gps.h

all: guidance

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

guidance: main.o imu.o gps.o
	$(CC) -o $@ $^ $(CFLAGS)
	touch out.txt

cal: calibrateIMU.c
	$(CC) -o $@ $^ $(CFLAGS)

clean: 
	rm guidance cal *.o out.txt
