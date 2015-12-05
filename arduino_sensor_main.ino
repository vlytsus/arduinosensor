#include <LiquidCrystal.h>
#include <stdlib.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

#define MIN_VOLTAGE_IF_NO_DUST     600 //mv
//#define COMPARATOR_UPPER_VOLTAGE 5000
#define COMPARATOR_UPPER_VOLTAGE   1100 //mv

#define ADC_RESOLUTION   1023.0
#define DIV_CORRECTION  11.0    //WaveShare board has voltage 1/10 divider
#define UG2MV_RATIO  0.2        //ug/m3 / mv

#define PIN_LED          7
#define PIN_ANALOG_OUT   0

#define POWER_ON_LED_DELAY 280
#define POWER_ON_LED_SLEEP 40
#define POWER_OFF_LED_DELAY 9680
#define SENSOR_PIN 0

#define SAMPLES_PER_COMP 5
#define STACK_SIZE 100
#define DISPLAY_REFRESH_INTERVAL 20

#define LCD_PRINT true
#define SERIAL_PRINT true
#define SERIAL_PRINT true

#define RAW_OUTPUT_MODE true //if true then raw analog data 0-1023 will be printed

//if RAW_OUTPUT_MODE = false then correction will be performed to calculate dust as ug/m3
float correction = DIV_CORRECTION * COMPARATOR_UPPER_VOLTAGE / ADC_RESOLUTION;

float stack[STACK_SIZE+1];//stack is used to calculate middle value for display output
int stackIter; //current stack iteration
int refresh; //current display counter, used to not print data too frequently

char str_temp[6];
 
void setup() {

  if(LCD_PRINT){
    // set up the LCD's number of columns and rows: 
    lcd.begin(16, 2);
    // Print a message to the LCD.
    lcd.print("-=Dust+Sensor=-");
  }
  
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

  if(SERIAL_PRINT)
    Serial.begin(9600);
}

void loop(void){

   if(stackIter >= STACK_SIZE)
     stackIter = 0;
     
   stack[stackIter] = computeSensorSequence();
  
   if(refresh < DISPLAY_REFRESH_INTERVAL){
     refresh++;
   }else{
     refresh = 0;
     //calculate midpoint value and print
     print(calculateStackMidVal());
   }

   stackIter++;
}

float computeSensorSequence(){
  //perform several measurements and store to stack
  //for later midpoint calculations

   float maxDust = 0;
   float minDust = 1000;
   float avgDust = 0; 
   float dustVal = 0;

   //perform several sensor reads and calculate midpoint
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

   //filter input data
   //don't take to consideration max & min values per sample
   //and save average to stack
   return (avgDust - maxDust - minDust) / (SAMPLES_PER_COMP - 2);
}

float computeDust(){
 
  if(RAW_OUTPUT_MODE)
    return readRawSensorData();
  
  float voltage = readRawSensorData() * correction;
  if (voltage > MIN_VOLTAGE_IF_NO_DUST){
    return (voltage - MIN_VOLTAGE_IF_NO_DUST) * UG2MV_RATIO;
  }
 
  return 0;
}
 
float calculateStackMidVal(){
   float midVal = 0;
   for(int i = 0; i < STACK_SIZE ; i++){
     midVal += stack[i]; 
   }
   return midVal / STACK_SIZE;
}

int readRawSensorData(){
  int analogData; //ADC value 0-1023
  digitalWrite(PIN_LED, HIGH); // power on the LED
  delayMicroseconds(POWER_ON_LED_DELAY);
  analogData = analogRead(SENSOR_PIN);
  delayMicroseconds(POWER_ON_LED_SLEEP);
  digitalWrite(PIN_LED, LOW); // power off the LED
  delayMicroseconds(POWER_OFF_LED_DELAY);
  return analogData; 
}

void print(float val){
  dtostrf(val, 4, 1, str_temp);
  print(str_temp);
}

void print(String msg){
  if(SERIAL_PRINT)
    Serial.println(msg);
  if(LCD_PRINT){
   lcd.setCursor(0, 1);
   lcd.print(msg);
  }
}
