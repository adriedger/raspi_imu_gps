#ifndef GPS_H
#define GPS_H

extern int no_gps_fix;

void openSerialPort();
void getGPSdata();
void closeSerialPort();

#endif
