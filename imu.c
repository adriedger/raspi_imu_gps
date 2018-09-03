#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <wiringPiI2C.h>
#include "imu.h"

//#define PI 3.14159265
#define LP 0.020 //[s/loop] loop period 20ms
#define AA 0.97 //complimentary filter constant
#define G_GAIN 0.070 //[deg/s/LSB] taken from manual for 2000dsp
#define RADTODEG 180/M_PI

int fd_gyro;
int fd_acc_mag;

double nanosec(){
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    return tv.tv_sec + tv.tv_nsec*1e-9;
}

void enableIMU(){
    fd_gyro = wiringPiI2CSetup(0x6b);
    fd_acc_mag = wiringPiI2CSetup(0x1d);

    wiringPiI2CWriteReg8(fd_gyro, 0x20, 0b00001111); //enables gyro xyz
    wiringPiI2CWriteReg8(fd_gyro, 0x23, 0b00110000); //allows continuous update, 2000dps

    wiringPiI2CWriteReg8(fd_acc_mag, 0x20, 0b10000111); //enables accelerometer xyz at 400hz, continuous update
    wiringPiI2CWriteReg8(fd_acc_mag, 0x21, 0b01011000); //194hz anti-alias filter bandwidth, +-8g scale
    
    wiringPiI2CWriteReg8(fd_acc_mag, 0x24, 0b11110000); //enable temp sensor,high-resolution mag sensor, 50hz data rate
    wiringPiI2CWriteReg8(fd_acc_mag, 0x25, 0b01100000); //+-12 gauss
    wiringPiI2CWriteReg8(fd_acc_mag, 0x26, 0b00000000); //continuous update
}

void getIMUdata(double* heading, double* pitch, double* roll){
    //pitch up is +, roll right is +, yaw clockwise is +    
    int gyro_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x29) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x28));
    int gyro_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x2B) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x2A));
    int gyro_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x2D) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x2C));

    int acc_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x29) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x28));
    int acc_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x2B) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x2A));
    int acc_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x2D) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x2C));

    int mag_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x09) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x08));
    int mag_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x0B) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x0A));
    int mag_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x0D) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x0C));
    
//    printf("%d, %d, %d\n", mag_raw_x, mag_raw_y, mag_raw_z);

    double accXnorm = acc_raw_x/sqrt(acc_raw_x * acc_raw_x + acc_raw_y * acc_raw_y + acc_raw_z * acc_raw_z);
    double accYnorm = acc_raw_y/sqrt(acc_raw_x * acc_raw_x + acc_raw_y * acc_raw_y + acc_raw_z * acc_raw_z);
    
    double accPitch = asin(accXnorm); //in radians
    double accRoll = -asin(accYnorm/cos(accPitch));
//    printf("%.2f, %.2f\n", accPitch*RADTODEG, accRoll*RADTODEG);

    double magXcomp = mag_raw_x*cos(accPitch) + mag_raw_z*sin(accPitch);
    double magYcomp = mag_raw_x*sin(accRoll)*sin(accPitch) + mag_raw_y*cos(accRoll) - mag_raw_z*sin(accRoll)*cos(accPitch);//
//    printf("%.f, %.f\n", magXcomp, magYcomp);

    double magAccHeading = atan2((double)-magYcomp,(double)magXcomp);//
//    if(magAccHeading < 0)
//       magAccHeading += PI*2;
   
//    printf("%.1f\n", magAccHeading*RADTODEG);
//    double accPitch = (atan2((double)acc_raw_z,(double)acc_raw_x)+PI/2);// -PI/2 -> 0 -> PI*1.5
//    double accRoll = (atan2((double)acc_raw_z,(double)-acc_raw_y)+PI/2);// -PI/2 -> 0 -> PI*1.5 
    //makes accPitch/Roll between -180 -> 180 like gyro, or is gyro 0 -> 90?
//    if(accPitch > PI)
//        accPitch -= PI*2;
//    if(accRoll > PI)
//        accRoll -= PI*2;

    double rate_gyr_x = (double)gyro_raw_x*G_GAIN-(1.72/1000/LP); //converts raw to deg/s of rotation + calibration
    double rate_gyr_y = (double)gyro_raw_y*G_GAIN-(62.85/1000/LP);
    double rate_gyr_z = (double)gyro_raw_z*G_GAIN-(-4.40/1000/LP);

    double start = nanosec();
    *pitch = AA*(*pitch + rate_gyr_y*LP) + (1-AA) * accPitch*RADTODEG; //complimentary filter, gyro in short run, acc in long run
    *roll = AA*(*roll + rate_gyr_x*LP) + (1-AA) * accRoll*RADTODEG;
    *heading = AA*(*heading + rate_gyr_z*LP) + (1-AA) * magAccHeading*RADTODEG;

    while(nanosec()-start < LP){
        usleep(100);
    }
}
