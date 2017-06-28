#include <stdio.h>
#include <stdlib.h>
#include <signal.h> //for SIGINT and SIGTERM
#include <wiringPi.h>
#include <softPwm.h>
#include <math.h> //for round() stuff
#include <pthread.h> //threads
#include <unistd.h> //unix standard library, for read()
#include <string.h> //for strcopy stuff
#include <errno.h> //stderr
#include "imu.h"
#include "gps.h"

#define PI 3.14159265
#define GREEN_LED1 5
#define GREEN_LED2 6
#define GREEN_LED3 13
#define GREEN_LED4 16
#define NOFIX_LED 20
#define GPSFIX_LED 26

volatile int keepRunning = 1;

double* latArr;//main, create, loop
double* lonArr;//main, create, loop

void intHandler(int dummy) {
	keepRunning = 0;
}

void createDestinationArray(){
    FILE* fd;
    char buffer[24];
    int i = 0;
    latArr = malloc(sizeof(double));
    lonArr = malloc(sizeof(double));
    if( (fd = fopen("ddCoords.txt", "r")) );
    else{
        printf("Coordinates file open Error: %s\n", strerror(errno));
        exit(1);
    }
    while(fgets(buffer, sizeof(buffer), fd) != NULL){
        latArr[i] = strtod(strtok(strtok(buffer, "\n"), ","), NULL);
        lonArr[i] = strtod(strtok(NULL, ","), NULL);
        printf("%.4f %.4f,",latArr[i], lonArr[i]);//
        i++; 
        latArr = realloc(latArr, (i+1)*sizeof(double));
        lonArr = realloc(lonArr, (i+1)*sizeof(double));
    }
    printf("\n");//
    fclose(fd);
}

void* strobe(){
    int modDir = 0;
    int counter = 0;
    while(keepRunning){
        softPwmWrite(GPSFIX_LED, counter);
        digitalWrite(NOFIX_LED, no_gps_fix);
        if(no_gps_fix == 0){
            if(modDir == 0){
                if(counter == 100){
                    modDir = 1;
                    counter--;
                }
                else
                    counter++;
            }
            else{
                if(counter == 0){
                    modDir = 0;
                    counter++;
                }
                else
                    counter--;
            }
            usleep(10000);
        }
        else{
            counter = 0;
        }
    }
    pthread_exit(NULL);
}

void* loop(){
    int current_dest = 0;
    
    double sGPSLat = 0;
    double sGPSLon = 0;
    double sGPSAlt = 0;
    double sGPSSpeed = 0;
    double sGPSHeading = 0;

    double sIMUHeading = 0;
    double sIMUPitch = 0;
    double sIMURoll = 0;
    
    double* PsGPSLat = &sGPSLat;
    double* PsGPSLon = &sGPSLon;
    double* PsGPSAlt = &sGPSAlt;
    double* PsGPSSpeed = &sGPSSpeed;
    double* PsGPSHeading = &sGPSHeading;

    double* PsIMUHeading = &sIMUHeading;    
    double* PsIMUPitch = &sIMUPitch;    
    double* PsIMURoll = &sIMURoll;    

    while(keepRunning){
        getGPSdata(PsGPSLat, PsGPSLon, PsGPSAlt, PsGPSSpeed, PsGPSHeading);
                
        double deltaLat = latArr[current_dest]-sGPSLat;
        double deltaLon = lonArr[current_dest]-sGPSLon;
        double bearing_to_dest = atan2(deltaLon, deltaLat)*180/PI;
        if(bearing_to_dest < 0)
            bearing_to_dest += 360;

        getIMUdata(PsIMUHeading, PsIMUPitch, PsIMURoll);

        int sLat = round(sGPSLat * 10000);
        int sLon = round(sGPSLon * 10000);
        int dLat = round(latArr[current_dest] * 10000);
        int dLon = round(lonArr[current_dest] * 10000);
        if(current_dest < sizeof(latArr)){
	        if((dLat-2 <= sLat && sLat <= dLat+2) && (dLon-2 <= sLon && sLon <= dLon+2)){
                if(current_dest == 0)
                    digitalWrite(GREEN_LED1, 1);
                if(current_dest == 1)
                    digitalWrite(GREEN_LED2, 1);
                if(current_dest == 2)
                    digitalWrite(GREEN_LED3, 1);
                if(current_dest == 3)
                    digitalWrite(GREEN_LED4, 1);
	            current_dest++;
	        }
        }
        else{
            keepRunning = 1;
        }
        //keeprunning, gpsfix, currentdest, lat, lon, alt, speed, heading, bearing, heading, pitch, roll
        printf("%d,%d,%d,%.4f,%.4f,%.1f,%.2f,%.1f,%.1f,%.1f,%.1f,%.1f\n",keepRunning,no_gps_fix,current_dest,sGPSLat,sGPSLon,sGPSAlt,sGPSSpeed,sGPSHeading
                ,bearing_to_dest,sIMUHeading,sIMUPitch,sIMURoll);
    }
    pthread_exit(NULL);
}


int main(int argc, char* argv[]){
    pthread_t threads[2];
    pthread_attr_t attr;
    int rc;
    void* status;
        
	signal(SIGINT, intHandler);
	signal(SIGTERM, intHandler); 
    
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	wiringPiSetupGpio();
    pinMode(GREEN_LED1, OUTPUT);
    pinMode(GREEN_LED2, OUTPUT);
    pinMode(GREEN_LED3, OUTPUT);
    pinMode(GREEN_LED4, OUTPUT);
    pinMode(NOFIX_LED, OUTPUT);
    softPwmCreate(GPSFIX_LED, 0, 100);
    
    createDestinationArray();
    
    openSerialPort();

    enableIMU();

    rc = pthread_create(&threads[0], NULL, strobe, (void*)0);
    if(rc){
        printf("ERROR: RETURN CODE FROM PTHREAD_CREATE() IS %d\n", rc);
        exit(-1);
    }
    rc = pthread_create(&threads[1], NULL, loop, (void*)1);
    if(rc){
        printf("ERROR: RETURN CODE FROM PTHREAD_CREATE() IS %d\n", rc);
        exit(-1);
    }
    //this waits for threads to end before proceeding
    for(int t=0; t<2; t++){
        rc = pthread_join(threads[t], &status);
        if(rc){
            printf("ERROR: RETURN CODE FROM PTHREAD_JOIN() IS %s\n", strerror(rc));
            exit(-1);
        }
    }
    
    digitalWrite(GREEN_LED1, 0);
    digitalWrite(GREEN_LED2, 0);
    digitalWrite(GREEN_LED3, 0);
    digitalWrite(GREEN_LED4, 0);
    digitalWrite(NOFIX_LED, 0);
    pinMode(GPSFIX_LED, OUTPUT);
    free(latArr);
    free(lonArr);
    closeSerialPort();
	printf("Done\n");
	return 0;
}
