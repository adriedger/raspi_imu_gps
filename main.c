#include <stdio.h>
#include <stdlib.h>
#include <signal.h> //for SIGINT and SIGTERM
#include <wiringPi.h>
#include <softPwm.h>
#include <math.h> //for round() stuff
#include <fcntl.h> //file control definitions for open() i.e O_RDONLY
#include <unistd.h> //unix standard library, for read()
#include <string.h> //for strcopy stuff
#include <pthread.h> //threads
#include <errno.h> //stderr

#define PI 3.14159265
#define GREEN_LED1 5
#define GREEN_LED2 6
#define GREEN_LED3 13
#define GREEN_LED4 16
#define NOFIX_LED 20
#define GPSFIX_LED 26

volatile int keepRunning = 1;
int no_gps_fix = 0;//get_GPS, strobe, loop
int serialPort;//get_GPS, main
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
        printf("Coordinates file read Error: %s\n", strerror(errno));
        exit(1);
    }
    while(fgets(buffer, sizeof(buffer), fd) != NULL){
        latArr[i] = strtod(strtok(strtok(buffer, "\n"), ","), NULL);
        lonArr[i] = strtod(strtok(NULL, ","), NULL);
        i++; 
        latArr = realloc(latArr, (i+1)*sizeof(double));
        lonArr = realloc(lonArr, (i+1)*sizeof(double));
    }
    for(i=0; i<sizeof(latArr); i++)
        printf("Entry %d: %f, %f\n", i, latArr[i], lonArr[i]);
    fclose(fd);
}

void getSerialData(double* sGPSLat, double* sGPSLon, double* sGPSAlt, double* sGPSSpeed, double* sGPSHeading){ 
    int rc;
    char buffer[82];
    char *token;
    int i = 0, j = 0, latSign = 1, lonSign= 1;
    char mLat[10], mLon[10], dLat[5], dLon[5];
    char altitude[10], speed[10], heading[10];
    rc = read(serialPort, buffer, sizeof(buffer));
    if(rc < 0){
        printf("Serial port read failed\n");
        exit(1);
    }
    if(strncmp(buffer, "$GPGGA", 6) == 0){
       token = strtok(buffer, ",");
	   while(token != NULL){ 
	       switch(i++){
	           case 2:
	               strncpy(dLat, token, 2);
                   for(int n=0; n<7; n++){
                       mLat[n] = token[n+2];
                   }
	           case 3:
	               if(strcmp(token, "S") == 0)
	                  latSign = -1; 
	           case 4:
	               strncpy(dLon, token, 3);
                   for(int n=0; n<7; n++){
                       mLon[n] = token[n+3];
                   }
	           case 5:
	               if(strcmp(token, "W") == 0)
	                   lonSign = -1;
               case 9:
                   strcpy(altitude, token);
	       }
	       token = strtok(NULL, ",");
	   }
       if(i < 14){
           no_gps_fix = 1;
       }
       else{
           no_gps_fix = 0;
           *sGPSLat = (strtod(dLat, NULL) + (strtod(mLat, NULL) / 60)) * latSign;
           *sGPSLon = (strtod(dLon, NULL) + (strtod(mLon, NULL) / 60)) * lonSign;
           *sGPSAlt = strtod(altitude, NULL);
       }
   }
   if(strncmp(buffer, "$GPRMC", 6) == 0){
       token = strtok(buffer, ",");
	   while(token != NULL){
           switch(j++){
               case 7:
                   strcpy(speed, token);
               case 8:
                   strcpy(heading, token);
           }
	       token = strtok(NULL, ",");
       }
       if(j < 11){
           no_gps_fix = 1;
       }
       else{
           no_gps_fix = 0;
           *sGPSSpeed = strtod(speed, NULL);
           *sGPSHeading = strtod(heading, NULL);
       }
   }
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
    
    double* PsGPSLat = &sGPSLat;
    double* PsGPSLon = &sGPSLon;
    double* PsGPSAlt = &sGPSAlt;
    double* PsGPSSpeed = &sGPSSpeed;
    double* PsGPSHeading = &sGPSHeading;

    while(keepRunning){
        getSerialData(PsGPSLat, PsGPSLon, PsGPSAlt, PsGPSSpeed, PsGPSHeading);
                
        double deltaLat = latArr[current_dest]-sGPSLat;
        double deltaLon = lonArr[current_dest]-sGPSLon;
        double bearing_to_dest = atan((deltaLon)/(deltaLat))*180/PI;
        if((deltaLon > 0 && deltaLat < 0) || (deltaLon < 0 && deltaLat < 0))
            bearing_to_dest = 180 + bearing_to_dest;
        if(deltaLon < 0 && deltaLat > 0)
            bearing_to_dest = 360 + bearing_to_dest;

        printf("Lat: %.4f, Lon: %.4f, GPS_Altitude: %.1fm, GPS_Speed: %.2fkn, GPS_Heading: %.1f, Bearing_to_Dest: %.1f\n", 
                sGPSLat, sGPSLon, sGPSAlt, sGPSSpeed, sGPSHeading, bearing_to_dest);

        signed sLat = round(sGPSLat * 10000);
        signed sLon = round(sGPSLon * 10000);
        signed dLat = round(latArr[current_dest] * 10000);
        signed dLon = round(lonArr[current_dest] * 10000);
        if(current_dest < sizeof(latArr)){
	        if((dLat-2 <= sLat && sLat <= dLat+2) && (dLon-2 <= sLon && sLon <= dLon+2)){
	            printf("Reached Destination %d\n", current_dest+1);
                if(current_dest == 0)
                    digitalWrite(GREEN_LED1, 1);
                if(current_dest == 1)
                    digitalWrite(GREEN_LED2, 1);
                if(current_dest == 2)
                    digitalWrite(GREEN_LED3, 1);
                if(current_dest == 3)
                    digitalWrite(GREEN_LED4, 1);
	            current_dest++;
	            printf("Going to %.4f, %.4f next\n", latArr[current_dest], lonArr[current_dest]);
	        }
        }
        else{
            printf("Destinations reached, ending process...\n");
            keepRunning =1;
        }
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
    printf("Currently going to %.4f, %.4f\n", latArr[0], lonArr[0]);

    serialPort = open("/dev/ttyAMA0", O_RDONLY | O_NOCTTY);
    if(serialPort == -1){
        printf("Unable to open serial port\n");
        exit(1);
    }

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
    close(serialPort);
	printf("Done\n");
	return 0;
}
