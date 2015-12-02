#include <LiquidCrystal.h>
#include <stdlib.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

int prev = 0;
int cur = 0;

#define MIN_VOLTAGE_IF_NO_DUST     600
//#define COMPARATOR_UPPER_VOLTAGE 5000
#define COMPARATOR_UPPER_VOLTAGE   1100

#define ADC_RESOLUTION   1023.0
#define DIV_CORRECTION  11.0
#define MGVOLT  0.2

#define PIN_LED          7
#define PIN_ANALOG_OUT   0

#define POWER_ON_LED_DELAY 280
#define POWER_ON_LED_SLEEP 40
#define POWER_OFF_LED_DELAY 9680
#define SENSOR_PIN 0

#define SAMPLES_PER_COMP        5
#define STACK_SIZE 100
#define DISPLAY_REFRESH_INTERVAL 20

float stack[STACK_SIZE];
int iter = 0;
int refresh = 0; 

int iteration;
float voltage;
float dustVal;
float avgDust;
float maxDust;
float minDust;
float correction = DIV_CORRECTION * COMPARATOR_UPPER_VOLTAGE / ADC_RESOLUTION;
char str_temp[6];
 
void setup() {
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("Dust Sensor :) 4");
  
  if(COMPARATOR_UPPER_VOLTAGE < 4000)
    analogReference(INTERNAL);
 
  //Serial.begin(9600);
 
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_ANALOG_OUT, INPUT);
  digitalWrite(PIN_LED, LOW);

  for(int i = 0; i < STACK_SIZE ; i++){
     stack[i] = 0; 
  }
}

void loop(void){
   avgDust = 0;
   maxDust = 0;
   minDust = 1000;
   iteration = 0;
 
   while (iteration < SAMPLES_PER_COMP){
     dustVal = computeDust();
     if (dustVal > 0){
      if(dustVal > maxDust)
        maxDust = dustVal;

      if(dustVal < minDust)
        minDust = dustVal;
        
       
       avgDust += dustVal;
       iteration++;
     }     
   }

   avgDust = (avgDust - maxDust - minDust) / (SAMPLES_PER_COMP - 2);
   //avgDust = avgDust / SAMPLES_PER_COMP;
   //Serial.println(avgDust);

   stack[iter] = avgDust;
   if(iter < STACK_SIZE){
     iter++;
   }else{
     iter = 0;
     //calculate();
   }

   if(refresh < DISPLAY_REFRESH_INTERVAL){
     refresh++;
   }else{
     refresh = 0;
     calculateMidVal();
   }
}

float computeDust(){  
  
  digitalWrite(PIN_LED, HIGH); // power on the LED
  delayMicroseconds(POWER_ON_LED_DELAY);
  voltage = analogRead(SENSOR_PIN);
  delayMicroseconds(POWER_ON_LED_SLEEP);
  digitalWrite(PIN_LED, LOW); // power off the LED
  delayMicroseconds(POWER_OFF_LED_DELAY);
   
  voltage = voltage * correction;
  if (voltage > MIN_VOLTAGE_IF_NO_DUST){
    return (voltage - MIN_VOLTAGE_IF_NO_DUST) * MGVOLT;
  }
 
  return 0;
}
 
void calculateMidVal(){

float midVal = 0;
   for(int i = 0; i < STACK_SIZE ; i++){
     midVal+=stack[i]; 
   }
   midVal /= STACK_SIZE;
   
   float_str(midVal, str_temp);   
   print(str_temp);
}

void float_str(float val, char* str){
  char str_temp[6];
  dtostrf(val, 4, 1, str_temp);
  sprintf(str, "%s", str_temp);
}

void print(String msg){
   //Serial.print(msg);
   lcd.print(msg);
}
