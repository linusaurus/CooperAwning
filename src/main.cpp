/* -----------------------
Awning Automation Program
author: r young
date: 4/28/2015
version : 2.6 - 11/5/2015-
version : 2.9 - 5/11/2017-
version : 3.0 - 5/24/2017-
version : 3.1 - 1/28/2021-
--------------------------
*/


#include<Sabertooth.h>
#include <StaticThreadController.h>
#include <Thread.h>
#include <ThreadController.h>
#include <Wire.h>
#include <LiquidCrystal_SR3W.h>
#include <LiquidCrystal_SR2W.h>
#include <LiquidCrystal_SR.h>
#include<LiquidCrystal_I2C.h>
#include <LiquidCrystal.h>
//#include <LCD.h>
#include <I2CIO.h>
#include <FastIO.h>

#define I2C_ADDR 0x27
#define BACKLIGHT_PIN 3
#define En_pin 2
#define Rw_pin 1
#define Rs_pin 0
#define D4_pin 4
#define D5_pin 5
#define D6_pin 6
#define D7_pin 7
#define PRGM_Version 3.0

LiquidCrystal_I2C lcd(I2C_ADDR,En_pin,Rw_pin,Rs_pin,D4_pin,D5_pin,D6_pin,D7_pin);
//--------------------------------------------------------------------------------

/* Pin definitions */
#define POT_PIN     A0
#define OPEN_LIMIT_SWITCH 10
#define CLOSED_LIMIT_SWITCH  8
#define OPEN_SWITCH  9
#define CLOSE_SWITCH  4
#define SAFE_SWITCH 7

//Configurable Limits

int OPEN_LIMIT = 328;
int CLOSED_LIMIT = 16;

char  pot[4];

//Starting Values
int potValue = 0;
boolean opened = false;
boolean closed = false;
boolean closeSwitchActive = false;
boolean openSwitchActive = false;
boolean safeToOperate = false;
int OPMODE=0;
int MSPEED=0;

//MainThreadController
ThreadController controll = ThreadController();

//Thread pointers

Thread* LimitSwitchThread = new Thread();
Thread* potReaderThread = new Thread();
Thread* PotMonitorThread = new Thread();
Thread* OpenSwitchThread = new Thread();
Thread* CloseSwitchThread = new Thread();
Thread* SafeSwitchThread = new Thread();
Sabertooth ST(128);

//Multi-tasking functions
void PotReader() 
{
	potValue = analogRead(POT_PIN);

	float smoothedValue = 0;
	float lastReadValue = potValue;
	float alpha = 0.50;
	// Filter the  from the potentiometer --
	smoothedValue = (alpha) * smoothedValue + (1 - alpha) * lastReadValue;

	potValue = smoothedValue;
	lcd.setCursor ( 0, 1 ); // go to the 2nd line
  sprintf(pot,"%3d",potValue);
	lcd.print(pot);
}

//Motor Monitoring Commands ---
void PotMonitor()
{
	if (potValue > OPEN_LIMIT )
	{
		lcd.setCursor ( 10, 1 ); // go to the 2nd line
		lcd.print("OPEN ");
		opened = true;
		closed = false;
		
	}
	if (potValue < CLOSED_LIMIT )
	{
		lcd.setCursor ( 10, 1 ); // go to the 2nd line
		lcd.print("CLOSE");
		closed = true;
		opened = false;
	}
					
}

//Switch Block-Turned off
void LimitSwitchMonitor()
{
		if (digitalRead(CLOSED_LIMIT_SWITCH)==HIGH)
		{						
			closed = true;
			lcd.setCursor ( 10, 1 ); // go to the 2nd line
			lcd.print("SWCLS");              
		}

		else if (digitalRead(OPEN_LIMIT_SWITCH)==HIGH)
		{
			
			opened = true;
			lcd.setCursor ( 10, 1 ); // go to the 2nd line
			lcd.print("SWOPN");                
		}				
		else if(digitalRead(OPEN_LIMIT_SWITCH)==LOW && digitalRead(CLOSED_LIMIT_SWITCH)==LOW)
		{				
			lcd.setCursor ( 10, 1 ); // go to the 2nd line
			lcd.print("+++++");	
			opened = false;
			closed= false;			
		}
}

void SafeSwitchCheck()
{
        if (digitalRead(SAFE_SWITCH)==HIGH){ 
        safeToOperate = true;
        lcd.setCursor ( 12,0 ); // go to the 2nd line
        lcd.print("GO");
        }
        else if (digitalRead(SAFE_SWITCH)==LOW)
        { 
         safeToOperate = false;
         lcd.setCursor ( 12,0 ); // go to the 2nd line
         lcd.print("NO");
        } 
}
 
void OpenSwitchMonitor()
{
	if (digitalRead(OPEN_SWITCH)== HIGH && opened != true)
	{		
		MSPEED = 127;
    lcd.setCursor ( 4, 1 ); 
		lcd.print("OP-2");
		OPMODE = 2;

	}
	else if (digitalRead(OPEN_SWITCH)==HIGH &&  opened == true )
	{
		lcd.setCursor ( 4, 1 );
		lcd.print("OP-2"); 
		OPMODE = 0;

	}
	else if (digitalRead(OPEN_SWITCH)==LOW && digitalRead(CLOSE_SWITCH) == LOW)
	{		
		lcd.setCursor ( 4, 1 ); // go to the 2nd line
		lcd.print("OP-0");
		OPMODE = 0;		
	}

}

//This is basic control, the operate switch must be held down to open/close
void CloseSwitchMonitor()
{
	
	if (digitalRead(CLOSE_SWITCH)==HIGH &&  closed != true)
	{
	  	MSPEED = 127;
    	lcd.setCursor ( 4, 1 ); // go to the 2nd line
		lcd.print("OP-1");
		OPMODE = 1;
		
	}
	else if (digitalRead(CLOSE_SWITCH)==HIGH &&  closed == true)
	{
		lcd.setCursor ( 4, 1 ); // go to the 2nd line
		lcd.print("OP-1");
		OPMODE = 0;
		
	}

}

void setup(){
	
potReaderThread->onRun(PotReader);
potReaderThread->setInterval(20);

PotMonitorThread->onRun(PotMonitor);
PotMonitorThread->setInterval(20);

LimitSwitchThread->onRun(LimitSwitchMonitor);
LimitSwitchThread->setInterval(20);

OpenSwitchThread->onRun(OpenSwitchMonitor);
OpenSwitchThread->setInterval(20);

CloseSwitchThread->onRun(CloseSwitchMonitor);
CloseSwitchThread->setInterval(20);

SafeSwitchThread->onRun(SafeSwitchCheck);
SafeSwitchThread->setInterval(20);

// Adds the threads to the controller
controll.add(potReaderThread);
controll.add(PotMonitorThread);
controll.add(LimitSwitchThread);
controll.add(OpenSwitchThread);
controll.add(CloseSwitchThread);
controll.add(SafeSwitchThread);

ST.setRamping(50);
SabertoothTXPinSerial.begin(9600);

//Pin Assignments
pinMode(POT_PIN, INPUT);
pinMode(CLOSED_LIMIT_SWITCH,INPUT);
pinMode(OPEN_LIMIT_SWITCH,INPUT);
pinMode(CLOSE_SWITCH,INPUT);
pinMode(SAFE_SWITCH,INPUT_PULLUP);

lcd.setBacklightPin(BACKLIGHT_PIN,POSITIVE);
lcd.setBacklight(LOW);
lcd.begin (16, 2);
lcd.home (); 
lcd.print("AWNG v3.2 ->GO");

}
//////////////////////////////////////////////////////////////
/////////////// Super Loop //////////////////////////////////
/////////////////////////////////////////////////////////////

void loop(){
  
 
//Start multitasking
controll.run();

if(safeToOperate){    // insure that the safe switch is "released" => HIGH

    if (OPMODE == 0)
    {ST.motor(1,0);}
    	
    if (OPMODE == 1)
    {ST.motor(1,MSPEED);}
    	
    if (OPMODE == 2)
    {ST.motor(1,-MSPEED);}
	
    }
}