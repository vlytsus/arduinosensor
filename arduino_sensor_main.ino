
/*  ***************************************************************
 *  ************* GP2Y1010AU0F Scharp Dust Sensor *****************
 *  ***************************************************************
 *  Dust sensor calculates dust density based on reflected 
 *  infrared light from IR diode. Light brightness coresponds 
 *  to amount of dust in the air. Following program is responsible
 *  to light-up IR diode, perform dust sampling and swith-off diode,
 *  according to GP2Y1010AU0F specification. 
 *  Arduino program performs sequence of several measurements, 
 *  filters input data by mid point calulation based on several 
 *  measurements, to avoid voltage spikes. Then it performs dust 
 *  values transformation to ug/m2. Finaly calculated data could 
 *  be printed to Arduino serial output or to LCD display.
 *  ***************************************************************
 */

#include <LiquidCrystal.h>
#include <stdlib.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

#define MIN_VOLTAGE_IF_NO_DUST    600     //mv   // 400 minimum voltage (close to clean air)
#define COMPARATOR_UPPER_VOLTAGE  1100.0    //5000 //mv  4500 - real USB voltage
#define MICROGR_2_MLVOLT_RATIO    0.2     //ug/m3 / mv
#define ADC_RESOLUTION            1023.0

//WaveShare board has voltage 1/10 divider
//ADC raw value should be corrected to compensate divider
//if you use GP2Y1010AU0F sensor without WaveShare board then should be DIV_CORRECTION = 1
#define DIV_CORRECTION            11   //11.0 //divider compensation

#define PIN_LED          7
#define PIN_ANALOG_OUT   0

#define POWER_ON_LED_DELAY   280
//#define POWER_ON_LED_SLEEP   40 // not used, digital read takes about 100 microseconds
#define POWER_OFF_LED_DELAY  9500
#define SENSOR_PIN           0

#define SAMPLES_PER_COMP     5
#define STACK_SIZE           65
#define DISPLAY_REFRESH_INTERVAL 30

#define LCD_PRINT       true
#define SERIAL_PRINT    true
#define RAW_OUTPUT_MODE false // if true then raw analog data 0-1023 will be printed

//if RAW_OUTPUT_MODE = false then correction will be performed to calculate dust as ug/m3

// Additional correction after calibration 
// by minimum & maximum values.
// According to specification: min=600mv; max=3600mv
// y = a*x + b;
#define ACORRECTION 0.74
#define BCORRECTION 450
float correction = DIV_CORRECTION * (COMPARATOR_UPPER_VOLTAGE / ADC_RESOLUTION) * ACORRECTION;

unsigned int stack[STACK_SIZE+1];// stack is used to calculate middle value for display output
unsigned int stackIter; // current stack iteration
unsigned int refresh; // current display counter, used to not print data too frequently

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

unsigned int computeSensorSequence(){
  //perform several measurements and store to stack
  //for later midpoint calculations

   unsigned int maxDust = 0;
   unsigned int minDust = 1000;
   unsigned int avgDust = 0; 
   unsigned int dustVal = 0;

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

unsigned int computeDust(){
 
  if(RAW_OUTPUT_MODE)
    return readRawSensorData();
  
  float voltage = readRawSensorData() * correction;
  voltage += BCORRECTION;
  
  if (voltage > MIN_VOLTAGE_IF_NO_DUST){
    return (voltage - MIN_VOLTAGE_IF_NO_DUST) * MICROGR_2_MLVOLT_RATIO;
  }
 
  return 0;
}
 
unsigned int calculateStackMidVal(){
   int midVal = 0;
   for(int i = 0; i < STACK_SIZE ; i++){
     midVal += stack[i]; 
   }
   return midVal / STACK_SIZE;
}

unsigned int readRawSensorData(){
  unsigned int analogData; //ADC value 0-1023
  digitalWrite(PIN_LED, HIGH); // power on the LED
  delayMicroseconds(POWER_ON_LED_DELAY);
  analogData = analogRead(SENSOR_PIN);
  //delayMicroseconds(POWER_ON_LED_SLEEP);//not used, digital read takes about 100 microseconds
  digitalWrite(PIN_LED, LOW); // power off the LED
  delayMicroseconds(POWER_OFF_LED_DELAY);//9500
  return analogData; 
}

void print(unsigned int val){
  print(String(val));
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
