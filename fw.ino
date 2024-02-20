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


// button status variables
int swSum = -1;                 // Current buttons state
int swPrevSum = -1;             // Previous button state
bool serialChanged = false;


// time variables
long timeCurrent = 0;

long timeLastPressL = 0;
long timeLastPressR = 0;
long timeLastPressNothing = 0;
long timeNotReset = 0;

long timeoutButtonReset = 2000;  // Hold 2x buttons time to reset game
long timeoutButtonLR = 200;      // Timeout for another button activation


// score and turn variables
const int numberOfTurnsToRemember = 20;  // How many steps back to remember
int scoreL[numberOfTurnsToRemember];
int scoreR[numberOfTurnsToRemember];
int turnLR[numberOfTurnsToRemember];     // Contains -1 if L player's turn; +1 if R player's turn; 0 if game have not started yet
int gameOver = 0;                        // Contains -1 if L player won; +1 if R player won; 0 if game goes on

// final print variable
bool white = true;


void setup() {
  // Button pins init
  pinMode(sw0plus, INPUT_PULLUP);
  pinMode(sw0minus, OUTPUT);
  digitalWrite(sw0minus, LOW);
  pinMode(sw1plus, INPUT_PULLUP);
  pinMode(sw1minus, OUTPUT);
  digitalWrite(sw1minus, LOW);

  // Serial print for score table print-out
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Clear the buffer
  display.clearDisplay();
  
  // init scores
  scoreL[0] = 0;
  scoreR[0] = 0;
  turnLR[0] = 0;

  // Start new game
  // "New game" print
  newGame();
}


void loop() {
  // Check if one of players wins the game; 0 if game not over; -1 if L won; 1 if R won
  gameOver = checkForVictory();

  timeCurrent = millis();
  // Fetching buttons status
  swSum = 2*digitalRead(sw0plus) + digitalRead(sw1plus);


  // If to hold 2x buttons pressed for some time => New Game starts
  if (swSum == 0 && (timeCurrent > timeNotReset + timeoutButtonReset)) {
    // Start new game
    // "New game" print
    newGame();
  }

  // If game goes on
  if (gameOver == 0) {
    // Different scenarios if different buttons are pressed
    switch (swSum) {
      case 3:       // Nothing is pressed
        // All actions are done when button are not pressed
        action();
        timeLastPressNothing = timeCurrent;
        timeNotReset = timeCurrent;
        swPrevSum = 3;
        break;
      case 2:       // R is pressed
        timeNotReset = timeCurrent;
        // Button is considered pressed only if previous state was "released"
        if (swPrevSum == 3 && timeCurrent > timeLastPressNothing + timeoutButtonLR) {
          swPrevSum = 2;
        }
        break;
      case 1:       // L is pressed
        timeNotReset = timeCurrent;
        // Button is considered pressed only if previous state was "released"
        if (swPrevSum == 3 && timeCurrent > timeLastPressNothing + timeoutButtonLR) {
          swPrevSum = 1;
        }
        break;
      case 0:       // Both are pressed
        // Button is considered pressed only if previous state was "released"
        if (swPrevSum == 3 && timeCurrent > timeLastPressNothing + timeoutButtonLR) {
          swPrevSum = 0;
          // Action of reverting changes
        }
        break;
      default:
        break;
    }
  }

  // Game status
  if (turnLR[0] != 0) {   // If game started
    if (gameOver == 0) {  // Game goes on, scores print
      printIt();
    } else {              // Game over, print result
      victory(gameOver);
    }
  }
}

// Check if one of players won the game
int checkForVictory(void) {
  if (scoreL[0] > 10 && scoreR[0]  < 10) {        // L score 11 or more, R score 9 or less
    return -1;
  } else if (scoreL[0] < 10 && scoreR[0]  > 10) { // L score 9 or less, L score 11 or more
    return 1;
  } else if (scoreL[0] > 10 && scoreR[0] > 10 && scoreL[0]-scoreR[0] > 1) { // L and R score 11 or more, L wins 2 points
    return -1;
  } else if (scoreL[0] > 10 && scoreR[0] > 10 && scoreL[0]-scoreR[0] < -1) { // L and R score 11 or more, R wins 2 points
    return 1;
  } else {
    return 0;
  }
}

// Print out winner; winner == -1 means L won; winner == 1 means R won
void victory(int winner) {
  // print winner greetings
  display.clearDisplay();
  // text scale
  display.setTextSize(4);

  // text color change
  if (white) {
    display.setTextColor(SSD1306_WHITE);
    white = false;
  } else {
    display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
    white = true;
  }
  // Start at top-left corner
  display.setCursor(0,0);
  // winner choice
  switch (winner) {
    case -1:
      display.println(F("win L"));
      break;
    case 1:
      display.println(F("win R"));
      break;
    default:
      display.println(F("?win?"));
      break;
  }
  display.display();
  delay(700);
}

// Action on buttons release, depending on what button was pressed
void action(void) {
  switch (swPrevSum) {
    case 3:
      // Nothing was pressed before
      break;
    case 2:
      // R was pressed
      if (turnLR[0] == 0) {
        startR();
      } else {
        // scoreR++
        scoreRaiseR();
      }
      break;
    case 1:
      // L was pressed
      if (turnLR[0] == 0) {
        startL();
      } else {
        // scoreL++
        scoreRaiseL();
      }
      break;
    case 0:
      // both were pressed
      // cancel one turn
      cancel();
      // ADD HERE POSSIBILITY OF CANCEL PREVIOUS ACTION
      break;
    default:
      break;
  }
}

void newGame(void) {
  // flag to refresh serial output
  serialChanged = true;

  // reset status of game
  gameOver = 0;

  // clear all scores and turns history
  for (int i = 0; i < numberOfTurnsToRemember; i++) {
    scoreL[i] = 0;
    scoreR[i] = 0;
    turnLR[i] = 0; 
  }

  // print initial menu
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

// game started with L first turn
void startL(void) {
  // flag to refresh serial output
  serialChanged = true;

  // Move score arrays one element front
  for (int i = numberOfTurnsToRemember-1; i > 0; i--) {
    scoreL[i] = scoreL[i-1];
    scoreR[i] = scoreR[i-1];
    turnLR[i] = turnLR[i-1]; 
  }
  // Add value to turnLR[0]
  scoreL[0] = 0;
  scoreR[0] = 0;
  turnLR[0] = -1;

  // print "L started" for few seconds
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

// game started with R first turn
void startR(void) {
  // flag to refresh serial output
  serialChanged = true;

  // Move score arrays one element front
  for (int i = numberOfTurnsToRemember-1; i > 0; i--) {
    scoreL[i] = scoreL[i-1];
    scoreR[i] = scoreR[i-1];
    turnLR[i] = turnLR[i-1]; 
  }
  // Add value to turnLR[0]
  scoreL[0] = 0;
  scoreR[0] = 0;
  turnLR[0] = 1;

  // print "R started" for few seconds
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

void scoreRaiseL(void) {
  // flag to refresh serial output
  serialChanged = true;

  // Move score arrays one element front
  for (int i = numberOfTurnsToRemember-1; i > 0; i--) {
    scoreL[i] = scoreL[i-1];
    scoreR[i] = scoreR[i-1];
    turnLR[i] = turnLR[i-1]; 
  }

  // Add 1 to scoreL[0]
  scoreL[0]++;

  // switch turn to another player
  if ((scoreL[0] + scoreR[0]) >= 20) {
    turnLR[0]*=-1;
  } else if ((scoreL[0] + scoreR[0])%2 == 0) {
    turnLR[0]*=-1;
  }
}

void scoreRaiseR(void) {
  // flag to refresh serial output
  serialChanged = true;
  // Move score arrays one element front
  for (int i = numberOfTurnsToRemember-1; i > 0; i--) {
    scoreL[i] = scoreL[i-1];
    scoreR[i] = scoreR[i-1];
    turnLR[i] = turnLR[i-1]; 
  }
  // Add 1 to scoreR[0]
  scoreR[0]++;

  // switch turn to another player
  if ((scoreL[0] + scoreR[0]) >= 20) {
    turnLR[0]*=-1;
  } else if ((scoreL[0] + scoreR[0])%2 == 0) {
    turnLR[0]*=-1;
  }
}

void cancel(void) {
  // flag to refresh serial output
  serialChanged = true;
  // Move score arrays one element back
  for (int i = 0; i < numberOfTurnsToRemember-1; i++) {
    scoreL[i] = scoreL[i+1];
    scoreR[i] = scoreR[i+1];
    turnLR[i] = turnLR[i+1]; 
  }
  scoreL[numberOfTurnsToRemember-1] = -1;
  scoreR[numberOfTurnsToRemember-1] = -1;
  turnLR[numberOfTurnsToRemember-1] = 0;
}

void printIt(void) {
  if (serialChanged) {
    Serial.println("scoreL");
    for (int i = 0; i < numberOfTurnsToRemember; i++) {
      Serial.print(scoreL[i]);
      Serial.print(" ");
    }
    Serial.println(); // Print a new line at the end

    Serial.println("scoreR");
    for (int i = 0; i < numberOfTurnsToRemember; i++) {
      Serial.print(scoreR[i]);
      Serial.print(" ");
    }
    Serial.println(); // Print a new line at the end

    Serial.println("turnLR");
    for (int i = 0; i < numberOfTurnsToRemember; i++) {
      Serial.print(turnLR[i]);
      Serial.print(" ");
    }
    Serial.println(); // Print a new line at the end
    serialChanged = false;
  }

  char bufferL[3];
  char bufferR[3];
  char turn[2];
  char result[10];
  
  sprintf(bufferL, "%d", scoreL[0]);
  sprintf(bufferR, "%d", scoreR[0]);

  switch (turnLR[0]) {
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

  if ((scoreL[0] >= 0 && scoreL[0] < 10) && (scoreR[0] >= 0 && scoreR[0] < 10)) {
    sprintf(result, "%s %s %s", bufferL, turn, bufferR);
  } else if ((scoreL[0] < 0 || scoreL[0] >= 10) && (scoreR[0] >= 0 && scoreR[0] < 10)) {
    sprintf(result, "%s%s %s", bufferL, turn, bufferR);
  } else if ((scoreL[0] >= 0 && scoreL[0] < 10) && (scoreR[0] < 0 || scoreR[0] >= 10)) {
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

