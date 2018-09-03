#include <stdio.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <wiringPiI2C.h>

#define LP 0.020
#define G_GAIN 0.070

int fd_gyro;
int fd_acc_mag;
double gyroX = 0;
double gyroY = 0;
double gyroZ = 0;

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

void Gyros(){

    int gyro_raw_x = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x29) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x28));
    int gyro_raw_y = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x2B) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x2A));
    int gyro_raw_z = (int16_t)(wiringPiI2CReadReg8(fd_gyro, 0x2D) << 8 | wiringPiI2CReadReg8(fd_gyro, 0x2C));

    double rate_gyr_x = (double)gyro_raw_x*G_GAIN; 
    double rate_gyr_y = (double)gyro_raw_y*G_GAIN;
    double rate_gyr_z = (double)gyro_raw_z*G_GAIN;

    double start = nanosec();

    gyroX += rate_gyr_x*LP;
    gyroY += rate_gyr_y*LP;    
    gyroZ += rate_gyr_z*LP;

    while(nanosec()-start < LP){
        usleep(100);
    }
}

int main(){
    enableIMU();
    int x = 0;
    while(x<1000){
        Gyros();
        x++;
    }
    printf("%.2f, %.2f, %.2f\n", gyroX, gyroY, gyroZ);
    return 0;
}
