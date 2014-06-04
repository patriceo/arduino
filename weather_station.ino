#include <TFT.h>
#include <SPI.h>
#include <Wire.h>
#include <Bridge.h>

// pin definition for the Leonardo or Yun boards
#define CS   7
#define DC   0
#define RESET  1 

// Sensor address
#define ADDRESS_SENSOR 0x77                 

// Define chart config in px
#define CHART_WIDTH 120
#define CHART_HEIGHT 70

// Chart Offset
#define CHART_X_OFFSET 20
#define CHART_Y_OFFSET -15

// Chart temperature initial Range
float CHART_MAX_TEMP = 40;
float CHART_MIN_TEMP = 10;

// Chart pressure initial Range
float CHART_MAX_PRESS = 1100;
float CHART_MIN_PRESS = 900;

// Store current week and temperatures
// The whole arrays represent CHART_UPDATE_INTERVAL * CHART_WIDTH seconds
// So 5min * 120 = 10 hours on default settings
float temperatures[CHART_WIDTH];
float pressures[CHART_WIDTH];

int16_t  ac1, ac2, ac3, b1, b2, mb, mc, md; // Store sensor PROM values from BMP180
uint16_t ac4, ac5, ac6;                     // Store sensor PROM values from BMP180

const uint8_t oss = 3;                      // Set oversampling setting
const uint8_t osd = 26;                     // with corresponding oversampling delay 

float T, P;                                 // Set global variables for temperature and pressure 
float previousT = -100.0;
float previousP = -100.0;

// TFT screen
TFT myScreen = TFT(CS, DC, RESET);


// log chart data every 60s
int counter = 0;
const int CHART_UPDATE_INTERVAL = 60;

// char array buffer
char printout[8];

void setup() {
  Wire.begin();                             // Activate I2C
  Serial.begin(9600);                       // Set up serial port
  delay(4000);
  init_SENSOR();                            // Initialize baro sensor variables
  delay(100);
  
  // Static text
  initScreen(); 
}

void loop() {
  // First need to call temp, then pressure...
  // Read and calculate temperature (T) 
  int32_t b5 = temperature();                       
  P = pressure(b5); 

  // Draw values on top of the screen
  drawFloat(previousT, T, 80,0, 0,255,255);
  drawFloat(previousP, P, 70,15, 255,255,0);

  previousP = P;
  previousT = T;
  if(counter >= CHART_UPDATE_INTERVAL){ // Redraw chart every 60s 
    updateGraph();
    counter = 0;
  } else {
    counter++;
  }
  delay(1000);                               // Delay between each readout  
}

/**
* Draw on screen static things:
* - labels
* - Axis
*/
void initScreen() {
  myScreen.begin();  
  myScreen.background(0,0,0); // clear the screen
  myScreen.stroke(255,255,255);
  myScreen.text("Temperature:",0,0);
  myScreen.text("Pression: ",0,15);
  myScreen.stroke(0,255,255);
  myScreen.text("C", 120,0); 
  myScreen.stroke(255,255,0);
  myScreen.text("mbar", 120,15);
  
   // init tabs at -100 values
  for(int i=0;i<CHART_WIDTH;i++){
     temperatures[i] = -100.0;
     pressures[i]= -100.0;
  } 
  computeChartAutoRange();  
  drawAxis();
}

/**
* Draw the chart Rectangle axis
*/
void drawAxis() { 
  myScreen.noFill();
  myScreen.stroke(255,255,255);
  myScreen.rect(CHART_X_OFFSET-1, CHART_Y_OFFSET + myScreen.height() - CHART_HEIGHT-1, CHART_WIDTH + 1,  CHART_HEIGHT + 1);
}

/**
* Draw or update a float value on screen.
*/
void drawFloat(float previous, float current, int x, int y, int red, int green, int blue) {
  if(previous!=current && previous!=-100) {
    // First blank  
    myScreen.stroke(0,0,0); 
    dtostrf(previous, 6, 2, printout);
    myScreen.text(printout, x, y);
    myScreen.stroke(red, green, blue);
    dtostrf(current, 6, 2, printout);
    myScreen.text(printout, x, y);
  }
}

/**
* Draw axis ticks (min & max value only)
*/ 
void drawTicks(float minValue, float maxValue, float previousMin, float previousMax, int x, int red, int green, int blue) {
  drawFloat(previousMin, minValue, x, myScreen.height()+CHART_Y_OFFSET+5, red,green,blue);
  drawFloat(previousMax, maxValue, x, myScreen.height()-CHART_HEIGHT+CHART_Y_OFFSET - 10, red,green,blue);
}

/**
* Compute the most relevant MIN_CHART_TEMP / MAX_CHART_TEMP
* According to serie values
*/
void computeChartAutoRange() {
 float minTemp = 100;
 float maxTemp = -50;
 float maxPress = 0;
 float minPress = 2000;
  
 for( int i=0; i<CHART_WIDTH; i++) {
     minTemp = temperatures[i]!=-100 ? min(temperatures[i],minTemp) : minTemp;
     maxTemp = max(temperatures[i], maxTemp); 
     minPress = pressures[i]!=-100 ? min(pressures[i],minPress) : minPress;
     maxPress = max(pressures[i], maxPress);
 }
   // Display [Min - x , Max + x2] range
   minTemp -=1.0; // temp curve centered in chart
   maxTemp += 1.0;
   minPress -= 5; // Pressure curve
   maxPress += 6; // not centered to avoir overlaps
   
   drawTicks(minTemp, maxTemp, CHART_MIN_TEMP, CHART_MAX_TEMP, CHART_WIDTH+CHART_X_OFFSET-10, 0,255,255);
   drawTicks(minPress, maxPress, CHART_MIN_PRESS, CHART_MAX_PRESS, 0, 255,255,0);
   
   CHART_MIN_TEMP = minTemp;
   CHART_MAX_TEMP = maxTemp;
   CHART_MIN_PRESS = minPress;
   CHART_MAX_PRESS = maxPress;
}

/**
* This method is used to both 
* - draw temp & pressure curves
* - clear the curves when called with (0,0,0)
* Parameters are colors for temperature (t) curve and pressure (p) curve
*/
void drawCurves(int tRed, int tGreen, int tBlue, int pRed, int pGreen, int pBlue) {
    int i=CHART_WIDTH-2;
    int x1,y1,x2,y2;
    
    while (i>1 && temperatures[i-1]!=-100) {
      // First temperature curve
      drawPoints(i+CHART_X_OFFSET, getChartYPosition(temperatures[i],CHART_MIN_TEMP, CHART_MAX_TEMP),
                  i+CHART_X_OFFSET - 1, getChartYPosition(temperatures[i-1], CHART_MIN_TEMP, CHART_MAX_TEMP), tRed, tGreen, tBlue);
      // Then pressure curve
      drawPoints(i+CHART_X_OFFSET,getChartYPosition(pressures[i], CHART_MIN_PRESS, CHART_MAX_PRESS),
          i+CHART_X_OFFSET - 1, getChartYPosition(pressures[i-1], CHART_MIN_PRESS, CHART_MAX_PRESS), pRed, pGreen, pBlue);
      i--;
  }
}

/**
* Draw a line between 2 points with given color
*/
void drawPoints(int x1, int y1, int x2, int y2, int red, int green, int blue) {
  myScreen.stroke(red,green,blue);
  myScreen.line(x1, y1, x2, y2);
}

/**
* Routine method to update chart display on each loop iteration
*/
void updateGraph() {
  // Clear previous drawn curves
  drawCurves(0,0,0, 0,0,0);
  
  // Then shift values to left
  for(int i=0; i < CHART_WIDTH - 1 ; i++) {
     temperatures[i] = temperatures[i+1];
     pressures[i] = pressures[i+1]; 
  }
  
  // Update last value
  temperatures[CHART_WIDTH - 1] = T;
  pressures[CHART_WIDTH - 1] = P;
  
  // Compute autorange
  computeChartAutoRange();
  
  // Then redraw points
  drawCurves(0,255,255,255,255,0);
}

/**
* Compute y position on lcd from values & range
*/
int getChartYPosition(float value, int minRange, int maxRange) {
  return CHART_Y_OFFSET + myScreen.height() - ( (value-minRange) / (maxRange - minRange)) * CHART_HEIGHT;
}



/*********************************************************
  Bosch Pressure Sensor BMP085 / BMP180 readout routine
  for the Arduino platform.
   
  Compiled by Leo Nutz
  www.ALTDuino.de              
 **********************************************************/

/**********************************************
  Initialize sensor variables
 **********************************************/
void init_SENSOR()
{
  ac1 = read_2_bytes(0xAA);
  ac2 = read_2_bytes(0xAC);
  ac3 = read_2_bytes(0xAE);
  ac4 = read_2_bytes(0xB0);
  ac5 = read_2_bytes(0xB2);
  ac6 = read_2_bytes(0xB4);
  b1  = read_2_bytes(0xB6);
  b2  = read_2_bytes(0xB8);
  mb  = read_2_bytes(0xBA);
  mc  = read_2_bytes(0xBC);
  md  = read_2_bytes(0xBE);

/*  Serial.println("");
  Serial.println("Sensor calibration data:");
  Serial.print(F("AC1 = ")); Serial.println(ac1);
  Serial.print(F("AC2 = ")); Serial.println(ac2);
  Serial.print(F("AC3 = ")); Serial.println(ac3);
  Serial.print(F("AC4 = ")); Serial.println(ac4);
  Serial.print(F("AC5 = ")); Serial.println(ac5);
  Serial.print(F("AC6 = ")); Serial.println(ac6);
  Serial.print(F("B1 = "));  Serial.println(b1);
  Serial.print(F("B2 = "));  Serial.println(b2);
  Serial.print(F("MB = "));  Serial.println(mb);
  Serial.print(F("MC = "));  Serial.println(mc);
  Serial.print(F("MD = "));  Serial.println(md);
  Serial.println("");*/
}

/**********************************************
  Calcualte pressure readings
 **********************************************/
float pressure(int32_t b5)
{
  int32_t x1, x2, x3, b3, b6, p, UP;
  uint32_t b4, b7; 

  UP = read_pressure();                         // Read raw pressure

  b6 = b5 - 4000;
  x1 = (b2 * (b6 * b6 >> 12)) >> 11; 
  x2 = ac2 * b6 >> 11;
  x3 = x1 + x2;
  b3 = (((ac1 * 4 + x3) << oss) + 2) >> 2;
  x1 = ac3 * b6 >> 13;
  x2 = (b1 * (b6 * b6 >> 12)) >> 16;
  x3 = ((x1 + x2) + 2) >> 2;
  b4 = (ac4 * (uint32_t)(x3 + 32768)) >> 15;
  b7 = ((uint32_t)UP - b3) * (50000 >> oss);
  if(b7 < 0x80000000) { p = (b7 << 1) / b4; } else { p = (b7 / b4) << 1; } // or p = b7 < 0x80000000 ? (b7 * 2) / b4 : (b7 / b4) * 2;
  x1 = (p >> 8) * (p >> 8);
  x1 = (x1 * 3038) >> 16;
  x2 = (-7357 * p) >> 16;
  return (p + ((x1 + x2 + 3791) >> 4)) / 100.0f; // Return pressure in mbar
}

/**********************************************
  Read uncompensated temperature
 **********************************************/
int32_t temperature()
{
  int32_t x1, x2, b5, UT;

  Wire.beginTransmission(ADDRESS_SENSOR); // Start transmission to device 
  Wire.write(0xf4);                       // Sends register address
  Wire.write(0x2e);                       // Write data
  Wire.endTransmission();                 // End transmission
  delay(5);                               // Datasheet suggests 4.5 ms
  
  UT = read_2_bytes(0xf6);                // Read uncompensated TEMPERATURE value

  // Calculate true temperature
  x1 = (UT - (int32_t)ac6) * (int32_t)ac5 >> 15;
  x2 = ((int32_t)mc << 11) / (x1 + (int32_t)md);
  b5 = x1 + x2;
  T  = (b5 + 8) >> 4;
  T = T / 10.0;                           // Temperature in celsius 
  return b5;  
}

/**********************************************
  Read uncompensated pressure value
 **********************************************/
int32_t read_pressure()
{
  int32_t value; 
  Wire.beginTransmission(ADDRESS_SENSOR);   // Start transmission to device 
  Wire.write(0xf4);                         // Sends register address to read from
  Wire.write(0x34 + (oss << 6));            // Write data
  Wire.endTransmission();                   // SEd transmission
  delay(osd);                               // Oversampling setting delay
  Wire.beginTransmission(ADDRESS_SENSOR);
  Wire.write(0xf6);                         // Register to read
  Wire.endTransmission();
  Wire.requestFrom(ADDRESS_SENSOR, 3);      // Request three bytes
  if(Wire.available() >= 3)
  {
    value = (((int32_t)Wire.read() << 16) | ((int32_t)Wire.read() << 8) | ((int32_t)Wire.read())) >> (8 - oss);
  }
  return value;                             // Return value
}

/**********************************************
  Read 1 byte from the BMP sensor
 **********************************************/
uint8_t read_1_byte(uint8_t code)
{
  uint8_t value;
  Wire.beginTransmission(ADDRESS_SENSOR);         // Start transmission to device 
  Wire.write(code);                               // Sends register address to read from
  Wire.endTransmission();                         // End transmission
  Wire.requestFrom(ADDRESS_SENSOR, 1);            // Request data for 1 byte to be read
  if(Wire.available() >= 1)
  {
    value = Wire.read();                          // Get 1 byte of data
  }
  return value;                                   // Return value
}

/**********************************************
  Read 2 bytes from the BMP sensor
 **********************************************/
uint16_t read_2_bytes(uint8_t code)
{
  uint16_t value;
  Wire.beginTransmission(ADDRESS_SENSOR);         // Start transmission to device 
  Wire.write(code);                               // Sends register address to read from
  Wire.endTransmission();                         // End transmission
  Wire.requestFrom(ADDRESS_SENSOR, 2);            // Request 2 bytes from device
  if(Wire.available() >= 2)
  {
    value = (Wire.read() << 8) | Wire.read();     // Get 2 bytes of data
  }
  return value;                                   // Return value
}

