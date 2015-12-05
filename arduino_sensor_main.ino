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

#define SAMPLES_PER_COMP 5
#define STACK_SIZE 100
#define DISPLAY_REFRESH_INTERVAL 20

float stack[STACK_SIZE+1];
int stackIter;
int refresh; 

int analogData;
float voltage;
float dustVal;
float avgDust;
float maxDust;
float minDust;
float midVal;

//correction to convert analog input to ug/m3
float correction = DIV_CORRECTION * COMPARATOR_UPPER_VOLTAGE / ADC_RESOLUTION;

char str_temp[6];
 
void setup() {
  //Serial.begin(9600);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  // Print a message to the LCD.
  lcd.print("-=Dust+Sensor=-");
  lcd.setCursor(0, 1);
  
  if(COMPARATOR_UPPER_VOLTAGE < 4000)
    analogReference(INTERNAL);
 
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_ANALOG_OUT, INPUT);
  digitalWrite(PIN_LED, LOW);

  stackIter = 0;
  refresh = 0; 
  for(int i = 0; i < STACK_SIZE ; i++){
     stack[i] = 0; 
  }
}

void loop(void){
   avgDust = 0;
   maxDust = 0;
   minDust = 1000;
   
   if(stackIter >= STACK_SIZE)
     stackIter = 0;
 
   for(int i = 0; i< SAMPLES_PER_COMP; i++){
     dustVal = computeDust();
     if (dustVal > 0){

      //find max dust per sample
      if(dustVal > maxDust)
        maxDust = dustVal;

      //find min dust per sample
      if(dustVal < minDust)
        minDust = dustVal;      
        avgDust += dustVal;
     }
   }

   //don't take to consideration max & min values per sample
   //and save average to stack
   stack[stackIter] = (avgDust - maxDust - minDust) / (SAMPLES_PER_COMP - 2);
  
   if(refresh < DISPLAY_REFRESH_INTERVAL){
     refresh++;
   }else{
     refresh = 0;
     print(calculateMidVal());
   }

   stackIter++;
}

float computeDust(){  
  
  digitalWrite(PIN_LED, HIGH); // power on the LED
  delayMicroseconds(POWER_ON_LED_DELAY);
  analogData = analogRead(SENSOR_PIN);
  delayMicroseconds(POWER_ON_LED_SLEEP);
  digitalWrite(PIN_LED, LOW); // power off the LED
  delayMicroseconds(POWER_OFF_LED_DELAY);

  //return voltage;
  voltage = analogData * correction;
  if (voltage > MIN_VOLTAGE_IF_NO_DUST){
    return (voltage - MIN_VOLTAGE_IF_NO_DUST) * MGVOLT;
  }
 
  return 0;
}
 
float calculateMidVal(){
   float midVal = 0;
   for(int i = 0; i < STACK_SIZE ; i++){
     midVal += stack[i]; 
   }
   return midVal / STACK_SIZE;
}

void print(float val){
  dtostrf(val, 4, 1, str_temp);
  print(str_temp);
}

void print(String msg){
   //Serial.println(msg);
   lcd.setCursor(0, 1);
   lcd.print(msg);
}
