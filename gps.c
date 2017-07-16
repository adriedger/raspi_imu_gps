#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h> //file control definitions for open() i.e O_RDONLY
#include <unistd.h> //unix standard library, for read()
#include <string.h> //for strcopy stuff
#include <errno.h>
#include "gps.h"

int serialPort;
int no_gps_fix = 0;//get_GPS, strobe, loop

void openSerialPort(){
    serialPort = open("/dev/ttyAMA0", O_RDONLY | O_NOCTTY | O_NDELAY | O_NONBLOCK);
    if(serialPort == -1){
        fprintf(stderr, "SERIAL PORT OPEN() ERROR: %s\n", strerror(errno));
        exit(1);               
    }
}

void closeSerialPort(){
    close(serialPort);
}


void getGPSdata(double* sGPSLat, double* sGPSLon, double* sGPSAlt, double* sGPSSpeed, double* sGPSHeading){ 
    char buffer[82];
    char* token;
    int i = 0;
    int latSign = 1;
    int lonSign= 1;
    char Lat[10];
    char Lon[10];
    char dLat[5];
    char dLon[5];
    char mLat[10];
    char mLon[10];
    char altitude[10];
    char speed[10];
    char heading[10];
    read(serialPort, buffer, sizeof(buffer));

    if(strncmp(buffer, "$GPGGA", 6) == 0){
       token = strtok(buffer, ",");
	   while(token != NULL){ 
	       switch(i++){
	           case 2:
	               strcpy(Lat, token);
	           case 3:
	               if(strcmp(token, "S") == 0)
	                  latSign = -1; 
	           case 4:
	               strcpy(Lon, token);
	           case 5:
	               if(strcmp(token, "W") == 0)
	                   lonSign = -1;
               case 9:
                   strcpy(altitude, token);
	       }
	       token = strtok(NULL, ",");
	   }
//       printf("%s,%s\n", Lat, Lon);
       if(i < 14){
           no_gps_fix = 1;
       }
       else{
           no_gps_fix = 0;

           strncpy(dLat, Lat, 2);
           strncpy(dLon, Lon, 3);
           dLat[2] = '\0';
           dLon[3] = '\0';
           for(int n = 0; n < 7; n++){
               mLat[n] = Lat[n+2];
               mLon[n] = Lon[n+3];
           }
//           printf("%s,%s,%s,%s\n", dLat, mLat, dLon, mLon);
           *sGPSLat = (strtod(dLat, NULL) + (strtod(mLat, NULL) / 60)) * latSign;
           *sGPSLon = (strtod(dLon, NULL) + (strtod(mLon, NULL) / 60)) * lonSign;
           *sGPSAlt = strtod(altitude, NULL);
       }
   }
   if(strncmp(buffer, "$GPRMC", 6) == 0){
       token = strtok(buffer, ",");
	   while(token != NULL){
           switch(i++){
               case 7:
                   strcpy(speed, token);
               case 8:
                   strcpy(heading, token);
           }
	       token = strtok(NULL, ",");
       }
       if(i < 11){
           no_gps_fix = 1;
       }
       else{
           no_gps_fix = 0;
           *sGPSSpeed = strtod(speed, NULL);
           *sGPSHeading = strtod(heading, NULL);
       }
   }
}

