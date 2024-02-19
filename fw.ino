/**************************************************************************
 This is an example for our Monochrome OLEDs based on SSD1306 drivers

 Pick one up today in the adafruit shop!
 ------> http://www.adafruit.com/category/63_98

 This example is for a 128x32 pixel display using I2C to communicate
 3 pins are required to interface (two I2C and one reset).

 Adafruit invests time and resources providing this open
 source code, please support Adafruit and open-source
 hardware by purchasing products from Adafruit!

 Written by Limor Fried/Ladyada for Adafruit Industries,
 with contributions from the open source community.
 BSD license, check license.txt for more information
 All text above, and the splash screen below must be
 included in any redistribution.
 **************************************************************************/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// button pins
int sw0plus = 0;
int sw0minus = 2;
int sw1plus = 10;
int sw1minus = 12;

// status variables
int swSum = -1;
int gameStarted = 0;            // Contains 0 if game have not started; -1 if L player starts; +1 if R player starts

// time variables
long timeCurrent = 0;
long timeLastResetButtonsCheck = 0;
long timeoutResetButtons = 2000;

void setup() {
  pinMode(sw0plus, INPUT_PULLUP);
  pinMode(sw0minus, OUTPUT);
  digitalWrite(sw0minus, LOW);  // Set analog pin A0 to a low level

  pinMode(sw1plus, INPUT_PULLUP);
  pinMode(sw1minus, OUTPUT);
  digitalWrite(sw1minus, LOW);  // Set analog pin A0 to a low level

  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
  
}

void loop() {
  timeCurrent = millis();
  swSum = digitalRead(sw0plus) * 2 + digitalRead(sw1plus);

  if (swSum == 0) {
    reset();
  } else {
    timeLastResetButtonsCheck = timeCurrent;
  }

  if (gameStarted == 0) {
    // "New game" print
    newgame();

    if (swSum == 1) {
      gameStarted = -1;
      startL();
    } else if (swSum == 2) {
      gameStarted = 1;
      startR();
    }
  } else {

    display.clearDisplay();
  }
}

// Checks for 2xPressed buttons timeout and resets game
void reset(void) {
  if (timeCurrent > timeLastResetButtonsCheck + timeoutResetButtons) {
    gameStarted = 0;
  }
}

void newgame(void) {
  display.clearDisplay();

  display.setTextSize(2);                     // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);                     // Start at top-left corner
  display.println(F("START"));

  display.setTextSize(1);                     // Draw 2X-scale text
  display.println(F("Press L or R button"));
  display.println(F("Who starts?"));

  display.display();
}

void startL(void) {
    display.clearDisplay();

  display.setCursor(0,0);                             // Start at top-left corner
  display.setTextSize(1);                             // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);                // Draw white text
  display.println(F(""));
  display.setTextSize(2);                             // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE); // Draw 'inverse' text
  display.println(F(" L started"));

  display.display();
  delay(2000);
}

void startR(void) {
    display.clearDisplay();

  display.setCursor(0,0);                             // Start at top-left corner
  display.setTextSize(1);                             // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);                // Draw white text
  display.println(F(""));
  display.setTextSize(2);                             // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE); // Draw 'inverse' text
  display.println(F(" R started"));

  display.display();
  delay(2000);
}

// void testdrawstyles(void) {
//   display.clearDisplay();

//   display.setTextSize(1);             // Normal 1:1 pixel scale
//   display.setTextColor(SSD1306_WHITE);        // Draw white text
//   display.setCursor(0,0);             // Start at top-left corner
//   display.println(F("Hello, world!"));

//   display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
//   display.println(3.141592);

//   display.setTextSize(2);             // Draw 2X-scale text
//   display.setTextColor(SSD1306_WHITE);
//   display.print(F("0x")); display.println(0xDEADBEEF, HEX);

//   display.display();
//   delay(2000);
// }
