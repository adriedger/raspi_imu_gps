#include <stdint.h>
#include <math.h>
#include <wiringPiI2C.h>
#include "imu.h"

#define PI 3.14159265

int fd_gyro;
int fd_acc_mag;

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
        
    int gyro_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x29) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x28));
    int gyro_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x2B) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x2A));
    int gyro_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x2D) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x2C));

    int acc_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x29) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x28));
    int acc_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x2B) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x2A));
    int acc_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x2D) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x2C));

    int mag_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x08) | wiringPiI2CReadReg8(fd_acc_mag, 0x09) << 8);
    int mag_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x0A) | wiringPiI2CReadReg8(fd_acc_mag, 0x0B) << 8);
    int mag_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x0C) | wiringPiI2CReadReg8(fd_acc_mag, 0x0D) << 8);

    *heading = atan2((double)mag_raw_x, (double)mag_raw_y)*180/PI;
    if(*heading < 0)
        *heading += 360;
    //linear algebra normalization
    double acc_norm_x = acc_raw_x/sqrt(acc_raw_x * acc_raw_x + acc_raw_y * acc_raw_y + acc_raw_z * acc_raw_z);
    double acc_norm_y = acc_raw_y/sqrt(acc_raw_x * acc_raw_x + acc_raw_y * acc_raw_y + acc_raw_z * acc_raw_z);

    *pitch = asin(acc_norm_y);
    *roll = -(asin(acc_norm_x)/cos(*pitch));
/*
    double mag_tiltcomp_x = mag_raw_x * cos(*pitch) + mag_raw_z * sin(*pitch);
    double mag_tiltcomp_y = mag_raw_y * sin(*roll) * sin(*pitch) + mag_raw_y * cos(*roll) - mag_raw_z * sin(*roll) * cos(*pitch);
    *heading = atan2(mag_tiltcomp_x, mag_tiltcomp_y)*180/PI;
    if(*heading < 0)
        *heading += 360;
*/
}

