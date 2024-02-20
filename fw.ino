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
int swSum = -1;                 // Current buttons state
int swPrevSum = -1;             // Previous button state

// time variables
long timeCurrent = 0;

long timeLastPressL = 0;
long timeLastPressR = 0;
long timeLastPressNothing = 0;
long timeNotReset = 0;

long timeoutButtonReset = 2000;  // Hold 2x buttons time to reset game
long timeoutButtonLR = 200;      // Timeout for another button activation

// score and turn variables
int scoreL = 0;
int scoreR = 0;
int turnLR = 0;                  // Contains -1 if L player's turn; +1 if R player's turn; 0 if game have not started yet

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
  swSum = 2*digitalRead(sw0plus) + digitalRead(sw1plus);

  switch (swSum) {
    case 3:       // Nothing is pressed
      action();
      timeLastPressNothing = timeCurrent;
      timeNotReset = timeCurrent;
      swPrevSum = 3;
      break;
    case 2:       // R is pressed
      timeNotReset = timeCurrent;
      if (swPrevSum == 3 && timeCurrent > timeLastPressNothing + timeoutButtonLR) {
        swPrevSum = 2;
      }
      break;
    case 1:       // L is pressed
      timeNotReset = timeCurrent;
      if (swPrevSum == 3 && timeCurrent > timeLastPressNothing + timeoutButtonLR) {
        swPrevSum = 1;
      }
      break;
    case 0:       // Both are pressed
      if (swPrevSum == 3 && timeCurrent > timeLastPressNothing + timeoutButtonLR) {
        swPrevSum = 0;
        // Action of reverting changes
      }
      if (timeCurrent > timeNotReset + timeoutButtonReset) {
        swPrevSum = 0;
        // Start new game
        turnLR = 0;
      }
      break;
    default:
      break;
  }

  // If New game
  if (turnLR == 0) {
    // "New game" print
    newgame();
  } else {
    printIt();
  }
}

// Action on buttons release, depending on what button was pressed
void action(void) {
  switch (swPrevSum) {
    case 3:
      // Nothing was pressed before
      break;
    case 2:
      // R was pressed
      if (turnLR == 0) {
        startR();
        turnLR = 1;
      } else {
        scoreR++;
        if ((scoreL + scoreR)%2 == 0) {
          turnLR*=-1;
        }
      }
      break;
    case 1:
      // L was pressed
      if (turnLR == 0) {
        startL();
        turnLR = -1;
      } else {
        scoreL++;
        if ((scoreL + scoreR)%2 == 0) {
          turnLR*=-1;
        }
      }
      break;
    case 0:
      // both were pressed
      // ADD HERE POSSIBILITY OF CANCEL PREVIOUS ACTION
      break;
    default:
      break;
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

void printIt(void) {
  char bufferL[3];
  char bufferR[3];
  char turn[2];
  char result[10];
  
  sprintf(bufferL, "%d", scoreL);
  sprintf(bufferR, "%d", scoreR);

  switch (turnLR) {
    case -1:
      turn[0] = '<';
      break;
    case 0:
      turn[0] = '=';
      break;
    case 1:
      turn[0] = '>';
      break;
    default:
      break;
  }
  // Add null terminator to make it a valid C-style string
  turn[1] = '\0';

  if (scoreL < 10 && scoreR < 10) {
    sprintf(result, "%s %s %s", bufferL, turn, bufferR);
  } else if (scoreL >= 10 && scoreR < 10) {
    sprintf(result, "%s%s %s", bufferL, turn, bufferR);
  } else if (scoreL < 10 && scoreR >= 10) {
    sprintf(result, "%s %s%s", bufferL, turn, bufferR);
  } else {
    sprintf(result, "%s%s%s", bufferL, turn, bufferR);
  }

  display.clearDisplay();

  display.setTextSize(4);                     // Draw 4X-scale text
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);                     // Start at top-left corner
  
  display.println(result);

  display.display();
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

