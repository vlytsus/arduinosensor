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

#define V_REF            5.0  //1100 arduino reference voltage for ADC
#define ADC_RESOLUTION   1024.0

#define PIN_LED          7
#define PIN_ANALOG_OUT   0

#define POWER_ON_LED_DELAY   280
#define POWER_ON_LED_SLEEP   40 // not used, digital read takes about 100 microseconds
#define POWER_OFF_LED_DELAY  9500
#define SENSOR_PIN           0

#define DISPLAY_REFRESH_INTERVAL 30
#define SAMPLES_PER_COMP     10 //perform N reads to filter noise and calculate mid value
#define STACK_SIZE           10
#define MAX_UNSIGNED_INT     65535

///////////////////////////////////////////////
// LCD data printing supported/not supported
#define LCD_PRINT       false
///////////////////////////////////////////////
// Serial/Usb port printing supported/not supported
#define SERIAL_PRINT    true
///////////////////////////////////////////////
// Raw data printing support. If true then raw analog data 0-1023 will be printed
#define DEBUG_MODE false

//by default this program is configured for Waveshare Dust Sensor board
//if you use GP2Y1010AU0F only then uncomment following line
//#define PURE_SHARP_GP2Y1010AU0F // uncomment it if you use GP2Y1010AU0F
#if defined (PURE_SHARP_GP2Y1010AU0F)
      #define IR_LED_ON   LOW
      #define IR_LED_OFF  HIGH
      #define DIV_CORRECTION  1.0
#else
      //in case if you use http://www.waveshare.com/wiki/Dust_Sensor
      #define IR_LED_ON   HIGH
      #define IR_LED_OFF  LOW
      #define DIV_CORRECTION  11.0 //Waveshare uses volt divider 1k/10k, so corection is needed
#endif

///////////////////////////////////////////
// Formulas to to calculate dust by minimum & maximum values.
// According to specification: min=600mv; max=3520mv
// using linear function: y = a*x + b;
// and Fig. 3 Output Voltage vs. Dust Density from specification 
// 0.6 = a*0 + b  =>  b = 0.6
// 3.52 = a*0.5 + 0.6 =>  a = (3.52 - 0.6)/0.5 = 5.84
// y = 5.84*x + 0.6
//
// Lets's inverse function, because y is unknown and x is our measured voltage
// 
// y - 0.6 = 5.84 * x
// x = (y-0.6)/5.84
// x = y*1/5.84-0.6/5.84
// x = 0.171*y - 0.1
#define A 0.171
#define B 0.1
// 
// Please note that ADC will return value in millivolts and dust is calculated in milligrams.
// x = 0.171 * y - 0.1
// finally let's rewrite the formula y is Vo (voltage) x is Dust value
// DUST = 0.171 * Vo - 0.1
//
// Vo for Arduino is calculated as:
// Vo = ADC_sample * V_REF / ADC_RESOLUTION 
/////////////////////////////////////////////////////
// Let's calculate volt correction  coefficient to use for Vo calculation later
// Vo = V_correction * ADC_sample
// We also need to use DIV_CORRECTION because Waveshare board uses voltage divider 1k/10k
float V_correction = DIV_CORRECTION * V_REF / ADC_RESOLUTION;

///////////////////////////////////////////
/// Start calibraion, because it is required to set minimum sensor output voltage 0.6V for 0 mg/m3 and 3.62V for max pollution (according to Fig3 of GP2Y1010AU0F specification)
// I was unable to calibrate by minimal pollution level (because I have no clean camera for calibration)
// But I was able to calibrate by upper limit, just put some stick inside sensor hole 
// I found that at max pollution Vo = ~ 3.1V So, I decided increase whole equation by 0.45V
#define V_CORRECTION_MAX 0.45

////////////////////////////////////////////////////
// Prepare stack to calculate rounded data
unsigned int stack[STACK_SIZE+1];// stack is used to calculate middle value for display output
unsigned int stackIter; // current stack iteration

///////////////////////////////////////////////////
// Calculation refresh interval used to not print data too frequently.
unsigned int refresh;
#define SLEEP 10000 //sleep after print

void setup() {
  initLCD();
    
  if(V_REF < 4)
    analogReference(INTERNAL);
 
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_ANALOG_OUT, INPUT);
  digitalWrite(PIN_LED, LOW);

  initStack();

  if(SERIAL_PRINT)
    Serial.begin(9600);

  printInitParams();
}

void printInitParams(){
  print("DIV_CORRECTION");
  if(LCD_PRINT)
    delay(1500);
  print(DIV_CORRECTION);
  delay(500);
}

int readRawSensorData(){
  int analogData; //ADC value 0-1023
  digitalWrite(PIN_LED, IR_LED_ON);
  
  delayMicroseconds(POWER_ON_LED_DELAY);
  analogData = analogRead(SENSOR_PIN);
  //print(analogData);
  
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
     //If refresh >= DISPLAY_REFRESH_INTERVAL then dust will be calculated and refresh updated to zero to start new cycle
     refresh = 0;
     if(DEBUG_MODE){
       //print voltage
       print(getAverageRawSamples());
     } else {
       print(calculateDust());
       if(!DEBUG_MODE)delay(SLEEP);
     }
   }
   stackIter++;
}

int calculateDust(){
  float Vo = (getAverageRawSamples() * V_correction + V_CORRECTION_MAX) * A - B;
  return Vo * 1000; // show result in micrograms
}
 
float getAverageRawSamples(){
   //calculate middle value from stack array
   float midVal = 0;
   for(int i = 0; i < STACK_SIZE ; i++){
     midVal += stack[i]; 
   }
   return midVal / STACK_SIZE;
}

int filterVoltageNoiseFromADC(){
  //perform several measurements and store to stack
  //for later midpoint calculations

   unsigned int maxDust = 0;
   unsigned int minDust = MAX_UNSIGNED_INT;
   unsigned long avgDust = 0;

   //perform several reads to filter noise and calculate mid value
   for(int i = 0; i< SAMPLES_PER_COMP;){
     int dustVal = readRawSensorData();
     if (dustVal >= 0){

      //find max dust per sample
      if(dustVal > maxDust)
        maxDust = dustVal;

      //find min dust per sample
      if(dustVal < minDust)
        minDust = dustVal;

      avgDust += dustVal;
      i++;
     }
   }

   //additional filter for input data
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
