/**********************************************************************
* Filename    : BtnAndLed.c
* Description : Controlling an led by button.
* Author      : Robot
* E-mail      : support@sunfounder.com
* website     : www.sunfounder.com
* Date        : 2014/08/27
**********************************************************************/
#include <wiringPi.h>
#include <stdio.h>

#define LedPin    3
#define ButtonPin 1
#define BeepPin 0

int main(void)
{
	if(wiringPiSetup() == -1){ //when initialize wiring failed,print messageto screen
		printf("setup wiringPi failed !");
		return 1; 
	}
	
	pinMode(LedPin, OUTPUT); 
	pinMode(ButtonPin, INPUT);
	pinMode(BeepPin, OUTPUT);   //set GPIO0 output

	pullUpDnControl(ButtonPin, PUD_UP);  //pull up to 3.3V,make GPIO1 a stable level
	while(1){
		digitalWrite(LedPin, HIGH);
		if(digitalRead(ButtonPin) == 0){ //indicate that button has pressed down
			digitalWrite(LedPin, LOW);   //led on
			digitalWrite(BeepPin, LOW);  //beep on
		}
		else
			digitalWrite(BeepPin, HIGH); //beep off
	}

	return 0;
}

