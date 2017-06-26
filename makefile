CC = gcc
CFLAGS = -std=gnu11 -Wall -lwiringPi -lm -lpthread
DEPS = imu.h

all: guidance

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

guidance: main.o imu.o
	$(CC) -o $@ $^ $(CFLAGS) 

clean: 
	rm guidance *.o
