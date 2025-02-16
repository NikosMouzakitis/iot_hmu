/*
		comments: 
	
    a) 	use of analogWrite() on motor pins 
       	instead of digitalWrite() determines 
        the speed of the respective motor
        
    b)  on ultrasonic measurement if value is more than 10cm,
        means (0.55 * 1 * 0.9) ~= 0.5 500lt as as we want
        we need to use fill water functionality.
*/


// Include the LiquidCrystal_I2C library
#include <LiquidCrystal_I2C.h>

// for servo motor
#include <Servo.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>


#define TEMP_PIN  A0  
#define PHOTO_PIN  A1
#define WATERING_SERVO_PIN  3
#define FILL_WATER_SERVO_PIN  11  
#define US_PING		2

Servo FillWater;
Servo Watering;
// Create a new instance of the LiquidCrystal_I2C class
LiquidCrystal_I2C lcd(0x20, 16, 2);

// Define a custom degree character
byte Degree[] = {
  B00111,
  B00101,
  B00111,
  B00000,
  B00000,
  B00000,
  B00000,
  B00000
};

//Motor A
const int motorPin1  = 5;  // Pin 14 of L293
const int motorPin2  = 6;  // Pin 10 of L293
//Motor B
const int motorPin3  = 10; // Pin  7 of L293
const int motorPin4  = 9;  // Pin  2 of L293

//true because simulation starts at night time.
bool watered_today = true; //track if we have watered this day
float tempC;	//global temperature
int last_round_photo;
bool ongoing_filling= false;
bool ongoing_watering= false;


// DAY = 1 NIGHT = 0
int isDay(void)
{
 	/*with 4.7KOhm resistor
      when we get vals above 865 
      treat is as a day */
	int photo = analogRead(PHOTO_PIN);
    if(photo >= 865)
      return 1;
  	return 0;
}
void motorA_clockwise()
{
 digitalWrite(motorPin1,HIGH);
 digitalWrite(motorPin2,LOW);  
}

void motorA_rev_clockwise()
{
 digitalWrite(motorPin1,LOW);
 digitalWrite(motorPin2,HIGH);  
}
void motorA_stop()
{
 digitalWrite(motorPin1,LOW);
 digitalWrite(motorPin2,LOW);  
}

void motorB_clockwise()
{
 digitalWrite(motorPin3,HIGH);
 digitalWrite(motorPin4,LOW);  
}

void motorB_rev_clockwise()
{
 digitalWrite(motorPin3,LOW);
 digitalWrite(motorPin4,HIGH);  
}
void motorB_stop()
{
 digitalWrite(motorPin3,LOW);
 digitalWrite(motorPin4,LOW);  
}
 
void fillwater_off(void)
{
  	FillWater.write(0); 
 	ongoing_filling = false;
}
void watering_off(void)
{
  Watering.write(0);
  ongoing_watering = false;
}

void fillwater_on(void)
{
  	FillWater.write(90); 
  	ongoing_filling = true;
}
void watering_on(void)
{
    Watering.write(90);
  	ongoing_watering = true;
}

long microsecondsToCentimeters(long microseconds) {
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the object we
  // take half of the distance travelled.
  return microseconds / 29 / 2;
} 

long ultrasonic_measurement(void)
{
  long duration, cm;

  // The PING is triggered by a HIGH 
  // pulse of 2 or more microseconds.
  // Give a short LOW pulse beforehand 
  // to ensure a clean HIGH pulse:
  pinMode(US_PING, OUTPUT);
  digitalWrite(US_PING, LOW);
  delayMicroseconds(2);
  digitalWrite(US_PING, HIGH);
  delayMicroseconds(5);
  digitalWrite(US_PING, LOW);

  // The same pin is used to read the signal from the PING: 
  // a HIGH pulse
  // whose duration is the time (in microseconds) from the 
  // sending of the ping
  // to the reception of its echo off of an object.
  pinMode(US_PING, INPUT);
  duration = pulseIn(US_PING, HIGH);

  // convert the time into a distance
  cm = microsecondsToCentimeters(duration);
  return cm;
}

void setup()
{
  //Set pins
  pinMode(motorPin1, OUTPUT);
  pinMode(motorPin2, OUTPUT);
  pinMode(motorPin3, OUTPUT);
  pinMode(motorPin4, OUTPUT);
    
  Watering.attach(WATERING_SERVO_PIN);
  FillWater.attach(FILL_WATER_SERVO_PIN);
  //initially both servos off.
  fillwater_off();
  watering_off();
  Serial.begin(9600);
  //initially stopped both motors 
  motorA_stop();
  motorB_stop();
  //initialize lcd component
  lcd.init();
  lcd.backlight();
  // Create a custom character
  lcd.createChar(0, Degree);
  
  
}
float tempCels(int temp)
{
  float voltage = temp * (5.0 / 1024.0);
  return ((voltage - 0.5) * 100);
}
void printLCD(float tmp)
{
  // Print the temperature on the LCD;
  lcd.setCursor(0, 0);
  lcd.print("Tem/ure: ");
  lcd.print(tmp);
  lcd.write(0); // print the custom degree character
  lcd.print("C ");
  lcd.setCursor(0, 1);
  if(ongoing_filling){
  	lcd.print("water tank fill"); 
  } else if(ongoing_watering) {
  	lcd.print("Watering plants");  
  }else {
    lcd.print("                "); //clear 2nd row.
  }
}

void loop()
{
  int temp = analogRead(TEMP_PIN);
  tempC=tempCels(temp);
  long cm;
  int day = isDay();
  cm = ultrasonic_measurement();
  /* ---------------------- */
  Serial.print("DAY: ");
  Serial.println(day);
  Serial.print("Temp: ");
  Serial.println(tempC);
  Serial.print("Cm from top: ");
  Serial.println(cm);
  /* ---------------------- */
  printLCD(tempC);  
  
  // check for minimum water availability after
  // watering.
  if(watered_today == true)
  {
  	if(cm > 10)
      fillwater_on();
    else
      fillwater_off();
  }
  
	////temperature control  
  if(day) { //15-21 day temp
    if(tempC < 15.0) { //heat
      motorB_clockwise();
      motorA_stop();
    } else if(tempC > 21.0) { //cool
      motorA_clockwise();
      motorB_stop();      
    } else {
    	motorA_stop();
      	motorB_stop();
    }
  } else if(!day) { //10-12 night temp
    if(tempC < 10.0) { //heat
      motorB_clockwise();
      motorA_stop(); 
    } else if(tempC > 12.0) {//cool
      motorA_clockwise();
      motorB_stop(); 
    } else {
    	motorA_stop();
      	motorB_stop();
    } 
  }
  
  if( day && (last_round_photo == !day))
  	watered_today = false; //reset
  
  //ensure to avoid overfilling.
  if( (ongoing_filling == true) && (cm <= 10) )
    fillwater_off();
  //watering only if we have more than 0.73*1*0.55 = 400lt available
  if(day && (!watered_today) && (cm < 27) ) { //start watering
  	watering_on(); 
  }
  
  //after watering simulating the max range that ultrasonic can measure.
  //stop and treat as watered.
  if(day && ongoing_watering && (cm>309)) {
  	watering_off();
    watered_today = true;
  }
  
  last_round_photo = day; //store it to handle
  						  //transition for day<->night.
  
}
