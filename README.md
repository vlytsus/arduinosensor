![Arduino Sensor GP2Y1010AU0F arduinosensor.tumblr.com](https://upload.wikimedia.org/wikipedia/commons/thumb/8/87/Arduino_Logo.svg/320px-Arduino_Logo.svg.png)
# [Arduino Sensor Project](http://arduinosensor.tumblr.com)
## [arduinosensor.tumblr.com](http://arduinosensor.tumblr.com)

Arduino PM2.5 / PM10 Pollution Sensor based on Sharp Optical Dust Sensor GP2Y1010AU0F Check articles about air pollution and description of my arduino project at http://arduinosensor.tumblr.com/

***

### GP2Y1010AU0F Scharp Dust Sensor

Dust sensor calculates dust density based on reflected 
infrared light from IR diode. Light brightness coresponds 
to amount of dust in the air. Following program is responsible
to light-up IR diode, perform dust sampling and swith-off diode,
according to GP2Y1010AU0F specification. 
Arduino program performs sequence of several measurements, 
filters input data by mid point calulation based on several 
measurements, to avoid voltage spikes. Then it performs dust 
values transformation to ug/m2. Finaly calculated data could 
be printed to Arduino serial output or to LCD display.


AQI Index could be clculated as

    --------------------------------
    PM2.5 ug |   AQI   |  Pollution
    =========|=========|============
       0-35  |  0-50   |  Excelent
    ---------|---------|------------
      35-75  | 51-100  |  Average
    ---------|---------|-----------
      75-115 | 101-150 |  Light
    ---------|---------|-----------
     115-150 | 151-200 |  Modrate
    ---------|---------|-----------
     150-250 | 201-300 |  Heavy
    ---------|---------|-----------
     250-500 |  > 300  |  Serious
    ---------|---------|-----------
