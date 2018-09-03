#include <stdio.h>
#include <wiringPi.h>
#include <softPwm.h>

#define SERVO 18

void startServo(){
     softPwmCreate(SERVO, 0, 100);
}

void headingControl(double delta){
    int i, j;
    for(i=5, j=80; i<26 && j>-81; i++, j=j-8){
        if(j <= delta && delta < j+9){
            printf("%d, %f\n", i, delta);
            softPwmWrite(SERVO, i);
        }
    }
}
    
void stopServo(){
    pinMode(SERVO, OUTPUT);
}
