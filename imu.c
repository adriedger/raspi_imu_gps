#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <wiringPiI2C.h>
#include "imu.h"

#define PI 3.14159265
#define LP 0.02 //[s/loop] loop period 20ms
#define AA 0.97 //complimentary filter constant
#define G_GAIN 0.070 //[deg/s/LSB] taken from manual for 2000dsp

int fd_gyro;
int fd_acc_mag;
//double gyroPitch = 0;
//double gyroRoll = 0;

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

    double start = nanosec();
        
    int gyro_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x29) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x28));
    int gyro_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x2B) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x2A));
    int gyro_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x2D) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x2C));

    int acc_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x29) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x28));
    int acc_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x2B) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x2A));
    int acc_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x2D) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x2C));

    int mag_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x09) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x08));
    int mag_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x0B) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x0A));
    int mag_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x0D) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x0C));
    
    *heading = atan2((double)-mag_raw_y, (double)mag_raw_x)*180/PI;
    if(*heading < 0)
        *heading += 360;

    double rate_gyr_x = (double)gyro_raw_x*G_GAIN + .20; //converts raw to deg/s of rotation + calibration
    double rate_gyr_y = (double)gyro_raw_y*G_GAIN - 3.06;
    double rate_gyr_z = (double)gyro_raw_z*G_GAIN + 1;

//    gyroPitch += rate_gyr_y*LP;
//    gyroRoll += rate_gyr_x*LP;    
//    printf("%.2f, %.2f, %.2f\n", rate_gyr_x, rate_gyr_y, rate_gyr_z);
//    printf("Gyro %.2f, %.2f\n", gyroPitch, gyroRoll);

//    double acc_norm_x = acc_raw_x/sqrt(acc_raw_x * acc_raw_x + acc_raw_y * acc_raw_y + acc_raw_z * acc_raw_z);
//    double acc_norm_y = acc_raw_y/sqrt(acc_raw_x * acc_raw_x + acc_raw_y * acc_raw_y + acc_raw_z * acc_raw_z);

//    *pitch = asin(acc_norm_x);
//    *roll = -(asin(acc_norm_y)/cos(*pitch));

    double accPitch = (atan2((double)acc_raw_z,(double)acc_raw_x)+PI/2)*180/PI; //+ PI is to add 90 or 180 degrees to reading to make it 0-360
    double accRoll = (atan2((double)acc_raw_y,(double)acc_raw_z)+PI)*180/PI; //same
    if(accPitch > 180){
        accPitch -= 360;
    }
    if(accRoll > 180){
        accRoll -= 360;
    }
    // need to make accPitch/Roll between -180/180 like gyro instead of 0/360
    *pitch = AA*(*pitch + rate_gyr_y * LP) + (1-AA) * accPitch; //complimentary filter
    *roll = AA*(*roll + rate_gyr_x * LP) + (1-AA) * accRoll;

//    printf("Acc %.2f, %.2f\n", accPitch, accRoll);

    while(nanosec()-start < LP){
        usleep(100);
    }
}

