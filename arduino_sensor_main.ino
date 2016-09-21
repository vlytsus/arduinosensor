
/*  ***************************************************************
 *  **** GP2Y1010AU0F Scharp Dust Sensor with waveshare board *****
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
 *  http://www.waveshare.com/wiki/Dust_Sensor
 *  ***************************************************************
 *  AQI Index could be calculated as
 *  --------------------------------
 *  PM2.5 ug |   AQI   |  Pollution
 *  =========|=========|============
 *     0-35  |  0-50   |  Excelent
 *  ---------|---------|------------
 *    35-75  | 51-100  |  Average
 *  ---------|---------|------------
 *    75-115 | 101-150 |  Light
 *  ---------|---------|------------
 *   115-150 | 151-200 |  Modrate
 *  ---------|---------|------------
 *   150-250 | 201-300 |  Heavy
 *  ---------|---------|------------
 *   250-500 |  > 300  |  Serious
 *  ---------|---------|------------
 */

#include <LiquidCrystal.h>
#include <stdlib.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

#define ADC_BASE_V  5000.0    //1100
#define ADC_RESOLUTION        1024.0

#define PIN_LED          7
#define PIN_ANALOG_OUT   0

#define POWER_ON_LED_DELAY   280
#define POWER_ON_LED_SLEEP   40 // not used, digital read takes about 100 microseconds
#define POWER_OFF_LED_DELAY  9500
#define SENSOR_PIN           0

#define DISPLAY_REFRESH_INTERVAL 30
#define SAMPLES_PER_COMP     5 //perform N reads to filter noise and calculate mid value
#define STACK_SIZE           100
#define MAX_UNSIGNED_INT     65535

#define LCD_PRINT       false
#define SERIAL_PRINT    true
#define RAW_OUTPUT_MODE false // if true then raw analog data 0-1023 will be printed

#define CALIBRATION 0.6

//by default this program is configured for Waveshare Dust Sensor board
//if you use GP2Y1010AU0F only then uncomment following line
//#define PURE_SHARP_GP2Y1010AU0F // uncomment it if you use GP2Y1010AU0F
#if defined (PURE_SHARP_GP2Y1010AU0F)
      #define IR_LED_ON   LOW
      #define IR_LED_OFF  HIGH
      #define VOLT_CORRECTION  1
#else
      //in case if you use http://www.waveshare.com/wiki/Dust_Sensor
      #define IR_LED_ON   HIGH
      #define IR_LED_OFF  LOW
      #define VOLT_CORRECTION  11.0 //Waveshare uses volt divider 1/10, so corection is needed
#endif

// Additional correction after calibration 
// to calculate dust as ug/m3
// by minimum & maximum values.
// According to specification: min=600mv; max=3600mv
// using linear function: y = a*x + b;
// where y is Vo (voltage)
//       x is Dust value
//
//       Vo = a*DUST + b;
//       DUST = (Vo - b)/a;
//
//    Vo = ADC_sample * V_adc_base / ADC_resolution

float a_correction = (ADC_BASE_V / ADC_RESOLUTION);

unsigned int stack[STACK_SIZE+1];// stack is used to calculate middle value for display output
unsigned int stackIter; // current stack iteration
unsigned int refresh; // current display counter, used to not print data too frequently

void setup() {
  initLCD();
    
  if(ADC_BASE_V < 4000)
    analogReference(INTERNAL);
 
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_ANALOG_OUT, INPUT);
  digitalWrite(PIN_LED, LOW);

  initStack();

  if(SERIAL_PRINT)
    Serial.begin(9600);
}

unsigned int readRawSensorData(){
  unsigned int analogData; //ADC value 0-1023
  digitalWrite(PIN_LED, IR_LED_ON);
  
  delayMicroseconds(POWER_ON_LED_DELAY);
  analogData = analogRead(SENSOR_PIN);
  
  delayMicroseconds(POWER_ON_LED_SLEEP);//not used, digital read takes about 100 microseconds
  digitalWrite(PIN_LED, IR_LED_OFF);
  
  delayMicroseconds(POWER_OFF_LED_DELAY);//9500
  return analogData; 
}

void loop(void){
   if(stackIter >= STACK_SIZE)
     stackIter = 0;
     
   stack[stackIter] = filterVoltageNoiseFromADC();
  
   if(refresh < DISPLAY_REFRESH_INTERVAL){
     refresh++;
   }else{
     refresh = 0;
     print(calculateDust());
   }
   stackIter++;
}

float calculateDust(){
  float voltage = getAverageRawSamples() * VOLT_CORRECTION * a_correction + CALIBRATION;
  return 0.17 * voltage - 0.0999;
}
 
float getAverageRawSamples(){
   int midVal = 0;
   for(int i = 0; i < STACK_SIZE ; i++){
     midVal += stack[i]; 
   }
   return midVal / STACK_SIZE;
}

unsigned int filterVoltageNoiseFromADC(){
  //perform several measurements and store to stack
  //for later midpoint calculations

   unsigned int maxDust = 0;
   unsigned int minDust = MAX_UNSIGNED_INT;
   unsigned long avgDust = 0;

   //perform several reads to filter noise and calculate mid value
   for(int i = 0; i< SAMPLES_PER_COMP; i++){
     unsigned int dustVal = readRawSensorData();
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

void initLCD(){
  if(LCD_PRINT){
    // set up the LCD's number of columns and rows: 
    lcd.begin(16, 2);
    // Print a message to the LCD.
    lcd.print("-=Dust+Sensor=-");
  }
}

void initStack(){
  stackIter = 0;
  refresh = 0; 
  for(int i = 0; i < STACK_SIZE ; i++){
     stack[i] = 0; 
  }
}

void print(double val){
  print(String(val));
}

void print(int val){
  print(String(val));
}

void print(String msg){
  if(SERIAL_PRINT)
    Serial.println(msg);
  if(LCD_PRINT){
   lcd.setCursor(0, 1);
   lcd.print(msg);
  }
}
