#define prototipo1

#include <LiquidCrystal.h> //lcd library

// defines for setting and clearing register bits
#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

const long FREQCONTROLLER=50000; //Controller update rate = 50KHz
const long timeUpdateTemperature=500; //ms to update temperature sensor: T1->500ms-T2->500ms
const long timeUpdateLcd=500; //ms to update display
byte state=0; //0: stopped, 1: starting, 2: normal mode, 99: alarm mode (emergency stop)
String lastError; //error string
//float noseI;
//float noseCTRL;
unsigned int noseHz;
boolean debug=false;

// initialize the lcd library with the numbers of the interface pins
LiquidCrystal lcd(3,4,5,6,7,8);

class SENSOR //sensor connected to 1 analog pin
{
public:
	SENSOR (int pin, float scaleFactor, String name, float maximum);
	float getNew(); //new measurement
	float getLast(); //last measurement
private:
	int inputPin; //analog pin
	float factor; //scaling factor
	float value; //last measurement
	String sensorName; //id string
	float maxValue; //above this limit, fire the alarm
};

SENSOR::SENSOR (int pin, float scaleFactor, String name, float maximum)
{
	inputPin=pin;
	factor=scaleFactor;
	sensorName=name;
	maxValue=maximum;
}

float SENSOR::getNew()
{
	value=factor*analogRead(inputPin); //+0.99*value;
	if (value>maxValue) 
	{
		String errorMsg;
		char buffer1[5];
		char buffer2[5];
		dtostrf(value, 4, 2, buffer1);
		dtostrf(maxValue, 4, 2, buffer2);
		errorMsg=sensorName+"="+String(buffer1)+">"+String(buffer2);
		alarm(errorMsg);
	}
	return value;
}
float SENSOR::getLast()
{
	return value;
}

class SENSORRAW //sensor connected to 1 analog pin, no scaling, no floating point data
{
public:
	SENSORRAW (int pin, String name, int maximum);
	int getNew(); //new measurement
	int getLast(); //last measurement
private:
	int inputPin; //analog pin
	int value; //last measurement
	String sensorName; //id string
	int maxValue; //above this limit, fire the alarm
};

SENSORRAW::SENSORRAW (int pin, String name, int maximum)
{
	inputPin=pin;
	sensorName=name;
	maxValue=maximum;
}

int SENSORRAW::getNew()
{
	value=analogRead(inputPin); //+0.01*value;
	if (value>maxValue) 
	{
		String errorMsg;
		errorMsg=sensorName+"="+String(value)+">"+String(maxValue);
		alarm(errorMsg);
	}
	return value;
}
int SENSORRAW::getLast()
{
	return value;
}

class DIFSENSOR //sensor connected to 2 analog pins
{
public:
	DIFSENSOR (int pinPlus,int pinMinus, float scaleFactor, String name, float maximum);
	float getNew(); //new measurement
	float getLast(); //last measurement
private:
	int pin1; //p+ 
	int pin2; //p-
	float factor; //scaling factor
	float value; //last measurement
	String sensorName; //id string
	float maxValue; //above this limit, fire the alarm
};

DIFSENSOR::DIFSENSOR (int pinPlus,int pinMinus, float scaleFactor, String name, float maximum)
{
	pin1=pinPlus;
	pin2=pinMinus;
	factor=scaleFactor;
	sensorName=name;
	maxValue=maximum;
}

float DIFSENSOR::getNew()
{
	value=factor*(analogRead(pin1)-analogRead(pin2));
	if (value>maxValue) 
	{
		String errorMsg;
		char buffer1[5];
		char buffer2[5];
		dtostrf(value, 4, 2, buffer1);
		dtostrf(maxValue, 4, 2, buffer2);
		errorMsg=sensorName+"="+String(buffer1)+">"+String(buffer2);
		alarm(errorMsg);
	}
	return value;
}
float DIFSENSOR::getLast()
{
	return value;
}


class NTC5K25 //594-NTCLE203E3502JB0 NTC sensor with 1k pulldown resistor
{
public:
	NTC5K25(int inputPin, String name, int maximum);
	int getNew();
	int getLast();
private:
	int ntcPin;
	//analogRead table for NTC 5K@25º, for 5-145º range (5,25,45,65,85,105,125,145 ºC)
	int ntc5k25AN[8];
	float  ntc5k25Factor[7];
	int value; //last measurement
	String sensorName; //id string
	int maxValue; //above this limit, fire the alarm
};

NTC5K25::NTC5K25(int inputPin, String name, int maximum)
{
	ntcPin=inputPin; //arduino input attached to sensor
	//analog values table
	ntc5k25AN[0]=75;
	ntc5k25AN[1]=171;
	ntc5k25AN[2]=321;
	ntc5k25AN[3]=501;
	ntc5k25AN[4]=667;
	ntc5k25AN[5]=792;
	ntc5k25AN[6]=876;
	ntc5k25AN[7]=928;
	//factors table: 20/(ntc5k25AN[k+1]-ntc5k25AN[k])
	ntc5k25Factor[0]=0.20887;
	ntc5k25Factor[1]=0.13268;
	ntc5k25Factor[2]=0.11107;
	ntc5k25Factor[3]=0.12075;
	ntc5k25Factor[4]=0.16018;
	ntc5k25Factor[5]=0.23808;
	ntc5k25Factor[6]=0.38157;
	sensorName=name;
	maxValue=maximum;

}

int NTC5K25::getNew()
//access sensor
//  +5V------|NTC|---|1K|-----
//                 |         _|_
//            arduino pin
//
//sensor and resistor form a voltage divider. temperature is obtained
//by interpolating in lookup table
{
	int analog; //analogRead from the input pin
	register byte counter;
	analog=analogRead(ntcPin);
	//Serial.print ("analog=");
	//Serial.println (analog);
	for (counter=0;counter<7;counter++) 
	{
		if (analog>=ntc5k25AN[counter] && analog<=ntc5k25AN[counter+1]) //interpolation
		{
			value=(analog-ntc5k25AN[counter])*ntc5k25Factor[counter]+20*counter+5;
			break;
		}
	}
	if (counter>7)
	{
		//value out of table
		if (analog<ntc5k25AN[0]) //lower than 5ºC 
		{
			value= -200;
		} else 
			{
				value= 666; //hotter than 145 ºC 
		}
	}
	if (value>maxValue) 
	{
		String errorMsg;
		errorMsg=sensorName+"="+String(value)+">"+String(maxValue);
		alarm(errorMsg);
	}
	return value;
}

int NTC5K25::getLast()
{
	return value;
}

class PID //implements PID controller with AntiWindup protection
{
public:
	PID(float P, float I, float D, float KB, float H, float L);
	double ctrlStep(float setpoint, float measurement);
private:
	//controller parameters
	float ctrlP; //proportional gain
	float ctrlI; //integral gain
	float ctrlD; //derivative gain
	float ctrlKB; //anti-windup term
	float ctrlH; //higher limit
	float ctrlL; //lower limit
	long timePrev; //previous time
	float outputSatPrev; //previous controller output previous to saturation
	float outputPrev; //previous controller output
	float errorPrev; //previous error
	float errorAwPrev; //previous error with antiwindup compensation
	float IPrev; // previous integral term
};

PID::PID(float P, float I, float D, float KB, float H, float L)
{
	//controller parameters
	ctrlP=P;
	ctrlI=I;
	ctrlD=D;
	ctrlKB=KB;
	ctrlH=H;
	ctrlL=L;
	//previous values init
	timePrev=0;
	outputSatPrev=0;
	outputPrev=0;
	errorPrev=0;
	errorAwPrev=0;
	IPrev=0;
}

double PID::ctrlStep(float setpoint, float measurement)
//error     ---
//---------| P |---------------
//    |     ---                |
//	  |                        |
//	  |     ---    ---         |
//    |----| D |--| s |-------(+)
//	  |     ---    ---         |
//	  |                        |     -----
//	  |     ---         ---    |    |   _ |         control
//     ----| I |--(+)--|1/s|--(+)---| _/  |---------
//		    ---    |    ---       | |     |   |  
//                 |              |  -----    |
//			       |    ---       |           |
//				    ---| KB|-----(-)----------
//					    ---
{
	long time;
	float h; //time step
	float outputSat; //controller output previous to saturation
	float output; //controller output
	float error; 
	float errorAw; //error with antiwindup compensation
	float P; //proportional term
	float I; //integral term
	float D; //derivative term
	error=setpoint-measurement;
//	Serial.print("set=");
//	Serial.print(setpoint);
//	Serial.print(" meas=");
//	Serial.print(measurement);
//	Serial.print(" error=");
//	Serial.print(error,0);
	
	time=micros();
	
	h=(float)(time-timePrev)*0.000001;
//	Serial.print(" h=");
//	Serial.print(h);
	


	if (h==0) { //something is wrong
		timePrev=time;
		outputSatPrev=0;
		outputPrev=0;
		errorPrev=0;
		errorAwPrev=0;
		IPrev=0;
		alarm ("PID Error");
		return 0;
	} 

	
	P=ctrlP*error;
//	Serial.print(" P=");
//	Serial.print(P);	
	D=ctrlD*(error-errorPrev)/h;
	
	errorAw=ctrlI*error+ctrlKB*(outputPrev-outputSatPrev);
//	Serial.print(" errorAw=");
//	Serial.print(errorAw,0);
	I=IPrev+0.5*h*(errorAw+errorAwPrev); //euler integration
//	Serial.print(" I=");
//	Serial.print(I,0);
	outputSat=P+I+D; 
//	Serial.print(" outputSat=");
//	Serial.print(outputSat,0);
	//check limits
	if (outputSat>ctrlH) {
		output=ctrlH;
	} else 	if (outputSat<ctrlL) {
		output=ctrlL; 
	} else {
		output=outputSat;
	}
//	Serial.print(" output=");
//	Serial.println(output);
	//saves actual values for next step
	timePrev=time;
	outputSatPrev=outputSat;
	outputPrev=output;
	errorPrev=error;
	errorAwPrev=errorAw;
	IPrev=I;
	return output;
}

class PICTRL //implements PI controller with AntiWindup protection, works with integer & long types only
{
public:
	PICTRL(float Kp, float Ki, long Kb, int H, int L);
	int ctrlStep(int error);
private:
	//controller parameters
	long Kp1024; //proportional gain, scaled 10 bits left
	long Ki1024; //integral gain, scaled 10 bits left
	long Kb; //antisaturation gain
	long H1024; //higher limit, scaled 10 bits left
	long L1024; //lower limit, scaled 10 bits left
	long timePrev; //previous time
	long outputSatPrev1024; //previous controller output previous to saturation, scaled 10 bits left
	long outputPrev1024; //previous controller output, scaled 10 bits left
	long IPrev1024; // previous integral term, scaled 10 bits left
};

PICTRL::PICTRL(float KP, float KI, long KB, int H, int L)
{
	//controller parameters
	Kp1024=KP*1024; //KP*1024 (10 bits left), proportional gain
	Ki1024=KI*1024; //KI*1024 (10 bits left), integral gain
	Kb=KB; //antisaturation gain, 10 bits left
	H1024=(long)H<<10; //H*1024 (10 bits left), max output
	L1024=(long)L<<10; //L*1024 (10 bits left), min output
	//previous values init
	timePrev=-100;
	outputSatPrev1024=0;
	outputPrev1024=0;
	IPrev1024=0;
}

int PICTRL::ctrlStep(int error)
//error     ---
//---------| P |---------------
//    |     ---                |
//	  |                        |
//	  |                        |     -----
//	  |     ---         ---    |    |   _ |         control
//     ----| I |--(+)--|1/s|--(+)---| _/  |---------
//		    ---    |    ---       | |     |   |  
//                 |              |  -----    |
//			       |    ---       |           |
//				    ---| KB|-----(-)----------
//					    ---
{
	long time;
	long h; //time step
	long outputSat1024; //controller output previous to saturation
	long output1024; //controller output, scaled 10 bits left
	long errorAw1024; //error with antiwindup compensation
	long P1024; //proportional term, scaled 10 bits left
	long I1024; //integral term, scaled 10 bits left

	
	time=micros();
	//time=timePrev+222;

	h=time-timePrev; 
/*	if (h==0) { //something is wrong
		timePrev=time;
		outputSatPrev1024=0;
		outputPrev1024=0;
		IPrev1024=0;
		alarm ("PI Error");
		return 0;
	} */
	
	P1024=Kp1024*error;
	errorAw1024=Ki1024*error+Kb*(outputPrev1024-outputSatPrev1024);
	I1024=IPrev1024+((h*errorAw1024)>>20);
	outputSat1024=P1024+I1024; 
	//check limits
	if (outputSat1024>H1024) {
		output1024=H1024;
	} else 	if (outputSat1024<L1024) {
		output1024=L1024; 
	} else {
		output1024=outputSat1024;
	}
	//saves actual values for next step
	timePrev=time;
	outputSatPrev1024=outputSat1024;
	outputPrev1024=output1024;
	IPrev1024=I1024;
	
	if (debug)
	{
		Serial.print(" Kp1024=");
		Serial.print(Kp1024);
		Serial.print(" Ki1024=");
		Serial.print(Ki1024);
		Serial.print(" H1024=");
		Serial.print(H1024);
		Serial.print(" L1024=");
		Serial.print(L1024);
		Serial.print(" L1024="); 
		Serial.print(" IPrev1024=");
		Serial.print(IPrev1024);
		Serial.print(" error=");
		Serial.print(error);
		Serial.print(" Time=");
		Serial.print(time);
		Serial.print(" h=");
		Serial.print(h);
		Serial.print(" P1024=");
		Serial.print(P1024);	
		Serial.print(" errorAw1024=");
		Serial.print(errorAw1024);
		Serial.print(" I1024=");
		Serial.print(I1024);
		Serial.print(" outputSat1024=");
		Serial.print(outputSat1024);
		Serial.print(" output1024=");
		Serial.print(output1024);
		Serial.print("PI=");
		Serial.println(output1024>>10);
	}
	return output1024>>10;
}


PID ctrl(0.05,0.5,0,3,5,0); //P,I,D,KB,H,L
//PICTRL ctrld(0.1,40,0,160,0);
NTC5K25 temp1(12,"T1",50); //temperature sensor #1
NTC5K25 temp2(13,"T2",50); //temperature sensor #2
//DIFSENSOR currentInput(2,3,1); //input current
//DIFSENSOR currentPrim(4,5,1.25); //primary current
//DIFSENSOR voltageInput(6,7,0.88367); //input DC Bus voltage
SENSOR currentSec(11,0.0135,"Is",20); //secundary current, measured
//SENSORRAW currentRaw(6,"Ir",100); //secundary current, stimated from primary. no scaling
SENSOR voltageOutput(10,0.016,"Vo",21); //output voltage


void serialShowSensorData ()

{
	Serial.print (" IS=");
	Serial.print(currentSec.getLast ());
	//Serial.print (" IR=");
	//Serial.print(currentRaw.getLast ());
	Serial.print (" Vo=");
	Serial.print(voltageOutput.getLast ());
	Serial.print (" Temp1=");
	Serial.print(temp1.getLast ());
	Serial.print (" Temp2=");
	Serial.print(temp2.getLast ());
	Serial.print (" OCR2B=");
	Serial.println(OCR2B);
}

void fastADC12_5()
//adjust prescaler to get faster conversion (12.5KHz)
{
	// set prescale to 64
    sbi(ADCSRA,ADPS2) ;
    sbi(ADCSRA,ADPS1) ;
    cbi(ADCSRA,ADPS0) ;
}

void fastADC50()
//adjust prescaler to get faster conversion (50KHz)
{
	// set prescale to 16
    sbi(ADCSRA,ADPS2) ;
    cbi(ADCSRA,ADPS1) ;
    cbi(ADCSRA,ADPS0) ;
}

void fastADC100()
//adjust prescaler to get faster conversion (100KHz)
{
	// set prescale to 8
    cbi(ADCSRA,ADPS2) ;
    sbi(ADCSRA,ADPS1) ;
    sbi(ADCSRA,ADPS0) ;
}

void lcdInit()
{
	//set up the LCD's number of columns and rows:
	lcd.begin(16, 2);
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("LCD OK");
}

void pwmInit()
//timer2 is used to provide pwm output. pwm and controller will 
//both work at the same frequency

{
	//define timer2 pwm pins as outputs
	pinMode (10, OUTPUT);
	pinMode (9, OUTPUT);
	//Timer2 setup
	//
	//TCCR2A
	//COM2A1 COM2A0 COM2B1 COM2B0 ----- ----- WGM21 WGM20
	//   0      1      1      0     0     0     1     1
	//
	//TCCR2B
	//FOC2A   FOC2B  -----  ----- WGM22  CS22  CS21  CS20
	//   0      0      0      0     1     0     0     1
	//
	//WGM22   WGM21  WGM20
	//   1      1      1  Fast PWM, Top=OCR2A, Update of UCR2B at Bottom, TOV Flag set on Top
	//
	//COM2A1 COM2A0
	//   0      1    WGM22 = 1: Toggle OC2A on Compare Match
	//
	//COM2B1 COM2B0
	//   1      0    Clear OCB2 on Compare Match (non-inverting mode)  prototype 1, 1.1
	//   1      1    Set OCB2 on Compare Match (inverting mode)        prototype 1.2
	//  
	//  CS22   CS21   CS20
	//   0      0      1   N=1: CLK (16MHz)/1 (no prescaling)
	//
	//FOC2A, FOC2B unused in PWM mode
	//PWM Freq=16000000/(2*N*(OCR2A+1))

	#ifndef prototipo12
		TCCR2A = _BV(COM2A0) | _BV(COM2B1) | _BV(WGM21) | _BV(WGM20); //non-inverting pwm output
	#else
		TCCR2A = _BV(COM2A0) | _BV(COM2B1) |_BV(COM2B0) | _BV(WGM21) | _BV(WGM20); //prototype 1.2 inverts pwm output
	#endif
	TCCR2B = _BV(WGM22) | _BV(CS20);
	OCR2A = 16000000/(2*FREQCONTROLLER)-1; //OCR2A sets frequency
	OCR2B = 0;
}

void pwmStop()
//stop timer2

{
	//Timer2 setup
	//
	//TCCR2A
	//COM2A1 COM2A0 COM2B1 COM2B0 ----- ----- WGM21 WGM20
	//   0      0      0      0     0     0     0     0
	//
	//TCCR2B
	//FOC2A   FOC2B  -----  ----- WGM22  CS22  CS21  CS20
	//   0      0      0      0     0     0     0     0
	//
	//WGM22   WGM21  WGM20
	//   0      0        Normal Mode of Operation
	//
	//COM2A1 COM2A0
	//   0      0    WGM22 = 0: Normal port operation, OC2B disconnected
	//
	//COM2B1 COM2B0
	//   0      0    Normal port operation, OC2B disconnected
	//
	//  CS22   CS21   CS20
	//   0      0      0   No clock source (Timer stopped)
	//

	TCCR2A = 0;
	TCCR2B = 0;
}

void serialInit()
{
	Serial.begin(38400);
}

void lcdShowSensorData ()

{
	static long timeLastDisplay=0;
	long timeActual;
	timeActual=millis();
	if ((timeActual-timeLastDisplay)>timeUpdateLcd || timeActual<timeLastDisplay) //time to read temperature
	{
		lcd.clear();
		lcd.setCursor(0, 0);
		lcd.print ("Is=");
		lcd.print(currentSec.getLast(),2);
		//lcd.print (" Ir=");
		//lcd.print(currentRaw.getLast());
		lcd.print ("V=");
		lcd.print(voltageOutput.getLast (),1);

		//lcd.print(" C=");
		//lcd.print(noseCTRL);
		//lcd.print(" H=");
		//lcd.print(noseHz);



		lcd.setCursor(0, 1);
		//lcd.print("R=");
		//lcd.print(voltageOutput.getLast ()/currentSec.getLast(),1);
		lcd.print ("T1=");
		lcd.print(temp1.getLast ());
		lcd.print ("T2=");
		lcd.print(temp2.getLast ());
		lcd.print (" O=");
		lcd.print (OCR2B);
		timeLastDisplay=timeActual;
	}
}

void lcdShowLastError()
{
	lcd.clear();
	lcd.setCursor(0, 0);
	lcd.print("ERROR!");
	lcd.setCursor(0, 1);
	lcd.print(lastError);
}

void alarm (String errorString) //enable alarm mode, stop PWM & store error string
{
	lastError=errorString;
	pwmStop(); //stop converter
	state=99; //set alarm mode
}

void adquire()
//read sensor data
{
	static long timeLastTemperatureRead=0;
	static boolean lastReadTemperatureWasT1=false; //self explaining
	float isValue;
	int irValue;
	float voValue;
	long timeActual;
	//Is & Vo are always read
	isValue=currentSec.getNew();
	//irValue=currentRaw.getNew ();
	voValue=voltageOutput.getNew(); 
	timeActual=millis();
	if ((timeActual-timeLastTemperatureRead)>timeUpdateTemperature || timeActual<timeLastTemperatureRead) //time to read temperature
	{
		if (lastReadTemperatureWasT1) temp2.getNew();
		else temp1.getNew();
		lastReadTemperatureWasT1=!lastReadTemperatureWasT1;
		timeLastTemperatureRead=timeActual;
	} 

}

void setup()
{
	fastADC12_5();
	lcdInit();
	serialInit();
}

void loop()
{
	float seno;
	float frecuencia=0.02;
	float control;
	float sensor;
	float consigna=5;
	unsigned int counter=0;
	long startTime;
	long timeStart2;
	long timeStart3;
	startTime=millis();
	timeStart2=millis();
	debug=false;
	//while (millis()-startTime<2000)
	while(1)
	{
		switch (state)
		{
			case 0: //stopped. start
				adquire();
				pwmInit();
				state=1; //start soft initialisation
				break;
			case 1: //starting. charge output capacitor slowly up to working voltage
				OCR2B=10;
				adquire();
				delay(50);
				adquire();
				state=2;
				if (voltageOutput.getLast()>0.3) 
				{
					timeStart3=millis();
					state=2;
				}
				break;
			case 2:
				//seno=-74*cos(2*3.1415*frecuencia*(millis()-startTime)/1000)+84;
				seno=10*sin(2*3.1415*frecuencia*(millis()-startTime)/1000)+20;
				//OCR2B=10;
				//lcd.setCursor(0, 0);
				//lcd.print("Test");
				//startTime=millis();
				//for (counter=0;counter<1000;counter++) sensor=difSensor.getNew();
				//frecuencia=counter/(double)(millis()-startTime)*1000;
				//Serial.print(frecuencia);
				//Serial.println ("Hz");
				//Serial.print("temp=");
				//Serial.println(sensor);
				//Serial.println("");
				//Serial.print("Seno=");
				//Serial.println(seno);
				//Serial.print(millis()-startTime);
				//Serial.print(" Seno=");
				//Serial.print(seno,0);
				control=8+32*ctrl.ctrlStep (consigna,voltageOutput.getLast());
			
				control=seno;
				//control=ctrld.ctrlStep (consigna-currentRaw.getLast());
				if (voltageOutput.getLast()>15) control=0;
				if (control>120) 
				{
					if (voltageOutput.getLast()>1) alarm("control>120");
					else OCR2B=0;
				} else OCR2B=control;
				//Serial.println(control);
				//if (control<20) OCR2B=control;
				//else OCR2B=0;

				//Serial.println(seno);
				adquire();
				
				//serialShowSensorData();
				counter++;
				if ((millis()-timeStart2)>1000) {
					lcdShowSensorData();
					timeStart2=millis();
					noseHz=counter;
					counter=0;
					//OCR2B=0;
					//while (1);
				}
				/*if ((millis()-timeStart3)>1700) {
					timeStart3=millis();
					if (consigna==8) consigna=8;
					else consigna=8;

				} */

				//delay(100);

				break;
			case 99:
				lcdShowLastError();
				delay(1000);
				break;
		}


	}
	//while (1);
  
}