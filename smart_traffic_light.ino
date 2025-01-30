
const int car_red = 13; //car traffic light
const int car_yellow = 11;
const int car_green = 12;
const int ped_red = 10;   //pedestrians traffic light
const int ped_green = 9;
const int buzzer = 8; //buzzer to arduino pin 8
const int button = 2; //button to arduino pin 2, to support ISR. (pins 2 and 3 support ISR on Uno)
const int ir_car = 3; //infared for car check
const int ir_ped = 4;  //infared for pedestrian check
volatile int button_pressed = 0;  
volatile int ped_walk = 0;
unsigned long previousMillis;
unsigned long start_cycle;
const unsigned long interval = 500; //on off buzzer when ped_walk is active ( value = 1).
volatile int car_exist, ped_exist; //for values read from ir-sensors.
volatile int buzz_yn = 0;

// Interrupt Service Routine (ISR)
void buttonISR() {
  Serial.println("isr");
  if(ped_walk == 1)
    return; //ignore if already green for peds.
  static unsigned long lastInterruptTime = 0;  // For debouncing
  unsigned long interruptTime = millis();  
  if (interruptTime - lastInterruptTime > 200) {  // 200ms of debounce time
    button_pressed = 1;  // indicate a button press
  }
  lastInterruptTime = interruptTime;  // Update the last interrupt time
}

void car_pass()
{

  digitalWrite(car_green, HIGH);
  digitalWrite(car_red, LOW);
  digitalWrite(car_yellow,LOW);
  digitalWrite(ped_red, HIGH); 
  digitalWrite(ped_green, LOW);
}

void car_orange()
{

  digitalWrite(car_green, LOW);
  digitalWrite(car_red, LOW);
  digitalWrite(car_yellow,HIGH);
  digitalWrite(ped_red, HIGH); 
  digitalWrite(ped_green, LOW);
}
void car_stop()
{
  digitalWrite(car_green, LOW);
  digitalWrite(car_red, HIGH);
  digitalWrite(car_yellow,LOW);
  digitalWrite(ped_red, LOW); 
  digitalWrite(ped_green, HIGH);
  ped_walk = 1;
  //treat it as pedestrian moving.
  start_cycle = millis();
  button_pressed = 0; //reset it.
}


void setup()
{
  Serial.begin(9600); // for some debuging.
  pinMode(ir_car, INPUT);
  pinMode(ir_ped, INPUT);
  pinMode(button, INPUT);
  for(int i = 8; i <14; i++)
    pinMode(i,OUTPUT);
  
  // Attach the interrupt to the button pin
  // Trigger on FALLING edge (button pressed, goes from HIGH to LOW)
  attachInterrupt(digitalPinToInterrupt(button), buttonISR, FALLING);
  //initial state
  car_pass();
}

void buzzer_sound(void)
{
  if(buzz_yn == 0)
  {
    tone(buzzer, 1000); // Send 1KHz sound signal...
   	buzz_yn = 1;
  } else if(buzz_yn == 1) {
  	noTone(buzzer);     // Stop sound...
	buzz_yn = 0;
  }
}
void loop()
{
  unsigned long currentMillis = millis();
  
  //sound buzz every 0.5seconds and check if cars should move again.
  if ( (ped_walk == 1) && (currentMillis - start_cycle >= 3000) ) {
   //allow cars go again.
  	car_pass();
    noTone(buzzer); // Ensure buzzer is turned off, handle the edge case.
    				// we don't need buzzer to remain on in any case when cars cross the road.
    ped_walk = 0;	  
  }
  if ((ped_walk == 1) && (currentMillis - previousMillis >= interval)) {
  	previousMillis = currentMillis; // Save the last time the action was performed
	buzzer_sound();
  }
  
  car_exist = digitalRead(ir_car);
  ped_exist = digitalRead(ir_ped);
  Serial.print("Car exist: ");
  Serial.println(car_exist);
  Serial.print("Ped exist: ");
  Serial.println(ped_exist);
  Serial.print("Button Pressed: ");
  Serial.println(button_pressed);
  
  //take latest time of movement of pedestrian 
  if(ped_exist)
    start_cycle = millis();
  //pedestrians should pass.
  if(button_pressed) {  
    if(car_exist == 1) { //cars detected
    	delay(5000);
      	car_orange();
      	delay(5000);
      	car_stop();
      	button_pressed = 0; //reset.
    } else if(car_exist == 0) { //no car movement, imediate switch (1s yellow only).
      	car_orange();
      	delay(1000);
      	car_stop();
    }
  }
      
}
