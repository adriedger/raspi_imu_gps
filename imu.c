#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>

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


void getIMUdata(double* heading){
        
    int gyro_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x29) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x28));
    int gyro_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x2B) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x2A));
    int gyro_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x2D) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x2C));
//        printf("%d, %d, %d\n", gyro_raw[0], gyro_raw[1], gyro_raw[2]);

    int acc_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x29) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x28));
    int acc_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x2B) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x2A));
    int acc_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x2D) << 8 | wiringPiI2CReadReg8(fd_acc_mag, 0x2C));
//        printf("%d, %d, %d\n", acc_raw[0], acc_raw[1], acc_raw[2]);

    int mag_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x08) | wiringPiI2CReadReg8(fd_acc_mag, 0x09) << 8);
    int mag_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x0A) | wiringPiI2CReadReg8(fd_acc_mag, 0x0B) << 8);
    int mag_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_acc_mag, 0x0C) | wiringPiI2CReadReg8(fd_acc_mag, 0x0D) << 8);
//    printf("%d, %d, %d\n", mag_raw[0], mag_raw[1], mag_raw[2]);
    *heading = atan2((double)mag_raw_x, (double)mag_raw_y)*180/PI;
    if(*heading < 0)
        *heading += 360;
}

