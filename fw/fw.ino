#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 20, 4);

// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...

// LED pins
const int ledLplus = 6;
const int ledLminus = 7;
const int ledRplus = 8;
const int ledRminus = 9;

// BUTTON pins
const byte sw0plus = 0;
const byte sw0minus = 2;
const byte sw1plus = 10;
const byte sw1minus = 12;
const byte sw2plus = A0;
const byte sw3plus = A1;

// LED blink timers
const long timeBlinkSlow = 300;
const long timeBlinkAverage = 150;
const long timeBlinkFast = 50;

// BUTTON status values
const int buttonPressedNothing = 0;
const int buttonPressedL = 1;
const int buttonPressedR = 2;
const int buttonPressedBoth = 3;
const int buttonPressedReset = 4;

// LCD timer
const long delayGameStart = 700;  // How long show L or R start screen

// BUTTON action timers
const long timeoutButtonReset = 2000;  // Hold 2x buttons time to reset game
const long timeoutButtonLR = 50;      // Timeout for another button activation
const long timeoutBetweenActions = 500;      // Timeout between single button actions
const long delayGameReset = 1500;  // Delay after game reset


// LED status: 0 == OFF; 1 = ON; 2 == BLINK low freq; 3 = VICTORY blink; 4 = BUTTON blink
int ledStateL = 0;
int ledStateR = 0;
bool ledLeftOn = false;
bool ledRightOn = false;

// BUTTON status variables
int swSum = -1;                 // Current buttons sum
int swPrevSum = -1;             // Previous button sum

// SHOW initial screen on boot
bool initialScreen = true;

// TIME variables
long timeCurrent = 0;

// BUTTON time variables
long timeLastPressL = 0;
long timeLastPressR = 0;
long timeLastPressNothing = 0;
long timeLastAction = 0;
long timeNotReset = 0;

// LED time variables
long blinkTimer = 0;
long timeLastSwitchLeftLED = 0;
long timeLastSwitchRightLED = 0;


// SCORE and TURN variables
const int numberOfTurnsToRemember = 20;  // How many steps back to remember
int scoreL[numberOfTurnsToRemember];
int scoreR[numberOfTurnsToRemember];
int turnLR[numberOfTurnsToRemember];     // Contains -1 if L player's turn; +1 if R player's turn; 0 if game have not started yet
int gameOver = 0;                        // Contains -1 if L player won; +1 if R player won; 0 if game goes on



void setup() {

  // Button pins init
  pinMode(sw0plus, INPUT_PULLUP);
  pinMode(sw0minus, OUTPUT);
  digitalWrite(sw0minus, LOW);
  pinMode(sw1plus, INPUT_PULLUP);
  pinMode(sw1minus, OUTPUT);
  digitalWrite(sw1minus, LOW);
  pinMode(sw2plus, INPUT_PULLUP);
  pinMode(sw3plus, INPUT_PULLUP);

  // LED pins init
  pinMode(ledLplus, OUTPUT);
  digitalWrite(ledLplus, LOW);
  pinMode(ledLminus, OUTPUT);
  digitalWrite(ledLminus, LOW);
  pinMode(ledRplus, OUTPUT);
  digitalWrite(ledRplus, LOW);
  pinMode(ledRminus, OUTPUT);
  digitalWrite(ledRminus, LOW);

  // Initialize serial communication at a baud rate of 9600
  Serial.begin(9600);

  // Initialize LED screen
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("  Hello, kood!  ");
  lcd.setCursor(0, 1);
  lcd.print("Dev.by A.Dubtsov");
  
  // init scores
  scoreL[0] = 0;
  scoreR[0] = 0;
  turnLR[0] = 0;

}


void loop() {

  timeCurrent = millis();

  // Fetching buttons status
  int activeButton = buttonState();

  // In case of any button is pressed -> blink both with high frequency
  if (swSum > 0) {

    ledStateL = 4;
    ledStateR = 4;

  }

  // LED processing
  leftLED();
  rightLED();


  // ### MAIN LOGIC STARTS HERE ###

  if (initialScreen) {

    // If any button pressed
    if (activeButton == buttonPressedL || activeButton == buttonPressedR || activeButton == buttonPressedBoth) {
      
      // "New game" print
      newGame();
      initialScreen = false;

    }

  } else {

    // If to hold 2x buttons pressed for some time => New Game starts
    if (activeButton == buttonPressedReset) {

      // Reset game state
      newGame();
      delay(delayGameReset);

    }

    // Check if one of players wins the game; 0 if game not over; -1 if L won; 1 if R won
    gameOver = checkForVictory();

    // If game goes on
    if (gameOver == 0) {

      // Different scenarios if different buttons are pressed
      action(activeButton);

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

}

void newGame(void) {

  ledStateL = 0;
  ledStateR = 0;

  // reset status of game
  gameOver = 0;

  // clear all scores and turns history
  for (int i = 0; i < numberOfTurnsToRemember; i++) {
    scoreL[i] = 0;
    scoreR[i] = 0;
    turnLR[i] = 0; 
  }

  // print initial menu
  lcd.setCursor(0, 0);
  lcd.print("START!  But who?");
  lcd.setCursor(0, 1);
  lcd.print("  Press L or R  ");

}

int buttonState(void) {

  int rusultButton = buttonPressedNothing;

  // Reading button pin status
  int buttonL = !digitalRead(sw0plus);
  int buttonR = !digitalRead(sw1plus);
  int buttonL2 = !digitalRead(sw2plus);
  int buttonR2 = !digitalRead(sw3plus);


  // Send variable to the serial output
  // Serial.println(buttonR2);

  swSum = 2*buttonL + buttonR + 2*buttonL2 + buttonR2;

  // If to hold 2x buttons pressed for some time => Reset
  if (swSum == 3 && (timeCurrent > timeNotReset + timeoutButtonReset)) {
    return buttonPressedReset;
  }

  // Different scenarios if different buttons are pressed
  switch (swSum) {

    case 0:       // Nothing is pressed

      // Decide what button is pressed this time
      rusultButton = buttonDecision();
      timeLastPressNothing = timeCurrent;
      timeNotReset = timeCurrent;
      swPrevSum = 0;
      
      break;
    case 1:       // R is pressed
      timeNotReset = timeCurrent;
      // Button is considered pressed only if previous state was "released"
      if (swPrevSum == 0 && timeCurrent > timeLastPressNothing + timeoutButtonLR) {
        swPrevSum = 1;
      }
      break;
    case 2:       // L is pressed
      timeNotReset = timeCurrent;
      // Button is considered pressed only if previous state was "released"
      if (swPrevSum == 0 && timeCurrent > timeLastPressNothing + timeoutButtonLR) {
        swPrevSum = 2;
      }
      break;
    case 3:       // Both are pressed
      // Button is considered pressed only if previous state was "released"
      if (swPrevSum == 0 && timeCurrent > timeLastPressNothing + timeoutButtonLR) {
        swPrevSum = 3;
      }
      break;
    default:
      break;
  }

  return rusultButton;

}

// Decide what button is pressed this time
int buttonDecision(void) {

  int result = -1;

  switch (swPrevSum) {

    case 0:
      // Nothing was pressed before
      result = buttonPressedNothing;
      break;
    case 1:
      // R was pressed
      result = buttonPressedR;
      timeLastAction = timeCurrent;
      break;
    case 2:
      // L was pressed
      result = buttonPressedL;
      timeLastAction = timeCurrent;
      break;
    case 3:
      // both were pressed
      result = buttonPressedBoth;
      timeLastAction = timeCurrent;
      break;
    default:
      break;
  }

  return result;

}

// Action on buttons release, depending on what button was pressed
void action(int activeButton) {

  switch (activeButton) {

    case buttonPressedNothing:

      // Nothing is pressed
      break;

    case buttonPressedR:
    
      // If game not started then start it. Else raise score up
      if (turnLR[0] == 0) {

        startR();
        
      } else {

        // scoreR++
        scoreRaiseR();

      }
      break;

    case buttonPressedL:
    
      // If game not started then start it. Else raise score up
      if (turnLR[0] == 0) {

        startL();

      } else {

        // scoreL++
        scoreRaiseL();

      }
      break;

    case buttonPressedBoth:

      // both were pressed
      // cancel one turn
      cancel();
      break;

    default:
      break;
  }

}

// Check if one of players won the game
int checkForVictory(void) {

  if (scoreL[0] > 10 && scoreR[0]  < 10) {        // L score 11 or more, R score 9 or less

    return -1;

  } else if (scoreL[0] < 10 && scoreR[0]  > 10) { // L score 9 or less, R score 11 or more

    return 1;

  } else if (scoreL[0] >= 10 && scoreR[0] >= 10 && scoreL[0]-scoreR[0] > 1) { // L and R score 11 or more, L wins 2 points

    return -1;

  } else if (scoreL[0] >= 10 && scoreR[0] >= 10 && scoreL[0]-scoreR[0] < -1) { // L and R score 11 or more, R wins 2 points

    return 1;

  } else {

    return 0;

  }

}

// Print out winner; winner == -1 means L won; winner == 1 means R won
void victory(int winner) {

  ledStateL = 3;
  ledStateR = 3;

  // print winner greetings
  // winner choice
  switch (winner) {
    case -1:

      // print Winner L
      lcd.setCursor(0, 1);
      lcd.print("   Winner:  L   ");
      break;

    case 1:

      // print Winner R
      lcd.setCursor(0, 1);
      lcd.print("   Winner:  R   ");
      break;

    default:

      // print sort of error
      lcd.setCursor(0, 1);
      lcd.print(" Winner?  What? ");
      break;
  }

}

// game started with L first turn
void startL(void) {

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
  lcd.setCursor(0, 0);
  lcd.print(" Game  started! ");
  lcd.setCursor(0, 1);
  lcd.print("  L's turn now  ");
  delay(delayGameStart);

}

// game started with R first turn
void startR(void) {

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
  lcd.setCursor(0, 0);
  lcd.print(" Game  started! ");
  lcd.setCursor(0, 1);
  lcd.print("  R's turn now  ");
  delay(delayGameStart);

}

void scoreRaiseL(void) {

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

  // LED blink or not on player's turn
  bool blinking = false;
  if (((scoreL[0] + scoreR[0]) % 2 == 0) && (scoreL[0] + scoreR[0]) < 20 ) {
    blinking = true;
  }

  long timerDiff = (timeCurrent - timeLastAction) / 1000;

  char bufferL[3];
  char bufferR[3];
  char turn[13];
  char result[17];

  char bufferTimer[17];
  char timer[17];
  
  // Convert newest score Int -> String
  sprintf(bufferL, "%d", scoreL[0]);
  sprintf(bufferR, "%d", scoreR[0]);

  sprintf(timer, "%ds ago", timerDiff);
  if (strlen(timer) < 7) {
    sprintf(timer, "Action    %ds ago", timerDiff);
  } else if (strlen(timer) < 8) {
    sprintf(timer, "Action   %ds ago", timerDiff);
  } else if (strlen(timer) < 9) {
    sprintf(timer, "Action  %ds ago", timerDiff);
  } else {
    sprintf(timer, "Action %ds ago", timerDiff);
  }

  switch (turnLR[0]) {

    case -1:

      ledStateL = 0;
      ledStateR = 0;
        
      if (!blinking) {

        ledStateL = 1;

      } else {

        ledStateL = 2;

      }

      for (int i = 0; i < 12; i++) {

        turn[i] = '<';

      }

      break;

    case 0:

      ledStateL = 0;
      ledStateR = 0;
      
      for (int i = 0; i < 12; i++) {

        turn[i] = '=';

      }
      break;

    case 1:

      ledStateL = 0;
      ledStateR = 0;
        
      if (!blinking) {

        ledStateR = 1;

      } else {

        ledStateR = 2;

      }

      for (int i = 0; i < 12; i++) {

        turn[i] = '>';

      }

      break;

    default:
      break;
  }

  // Add null terminator to make it a valid C-style string
  turn[12] = '\0';

  // Combine string for pring depending on length of score string
  if ((scoreL[0] >= 0 && scoreL[0] < 10) && (scoreR[0] >= 0 && scoreR[0] < 10)) {

    sprintf(result, "0%s%s0%s", bufferL, turn, bufferR);

  } else if ((scoreL[0] < 0 || scoreL[0] >= 10) && (scoreR[0] >= 0 && scoreR[0] < 10)) {

    sprintf(result, "%s%s0%s", bufferL, turn, bufferR);

  } else if ((scoreL[0] >= 0 && scoreL[0] < 10) && (scoreR[0] < 0 || scoreR[0] >= 10)) {

    sprintf(result, "0%s%s%s", bufferL, turn, bufferR);

  } else {

    sprintf(result, "%s%s%s", bufferL, turn, bufferR);

  }

  // print scores
  lcd.setCursor(0, 0);
  lcd.print(result);
  lcd.setCursor(0, 1);
  lcd.print(timer);
}

// Left LED 0 == OFF; 1 = ON; 2 == BLINK low freq; 3 = VICTORY blink; 4 = BUTTON blink
void leftLED() {

  switch (ledStateL) {

    case (0):

      digitalWrite(ledLplus, LOW);
      break;

    case (1):

      digitalWrite(ledLplus, HIGH);
      break;

    case (2):

      leftLedBlink(2);
      break;

    case (3):

      leftLedBlink(3);
      break;

    case (4):

      leftLedBlink(4);
      break;

    default:

      digitalWrite(ledLplus, LOW);
      break;
  }

}

// Right LED 0 == OFF; 1 = ON; 2 == BLINK low freq; 3 = VICTORY blink; 4 = BUTTON blink
void rightLED() {

  switch (ledStateR) {

    case (0):

      digitalWrite(ledRplus, LOW);
      break;

    case (1):

      digitalWrite(ledRplus, HIGH);
      break;

    case (2):

      rightLedBlink(2);
      break;

    case (3):

      rightLedBlink(3);
      break;

    case (4):

      rightLedBlink(4);
      break;

    default:

      digitalWrite(ledRplus, LOW);
      break;
  }

}

void leftLedBlink(int mode) {

  switch (mode) {

    case (2):
      blinkTimer = timeBlinkSlow;
      break;
    case (3):
      blinkTimer = timeBlinkAverage;
      break;
    case (4):
      blinkTimer = timeBlinkFast;
      break;
    default:
      break;
  }

  if (timeCurrent > timeLastSwitchLeftLED + blinkTimer) {

    timeLastSwitchLeftLED = timeCurrent;
    ledLeftOn = !ledLeftOn;

    if (ledLeftOn) {

      digitalWrite(ledLplus, LOW);

    } else {

      digitalWrite(ledLplus, HIGH);

    }
    
  }
}

void rightLedBlink(int mode) {

  long blinkTimer = 0;

  switch (mode) {

    case (2):
      blinkTimer = timeBlinkSlow;
      break;
    case (3):
      blinkTimer = timeBlinkAverage;
      break;
    case (4):
      blinkTimer = timeBlinkFast;
      break;
    default:
      break;
  }

  if (timeCurrent > timeLastSwitchRightLED + blinkTimer) {

    timeLastSwitchRightLED = timeCurrent;
    ledRightOn = !ledRightOn;

    if (ledRightOn) {

      digitalWrite(ledRplus, LOW);

    } else {

      digitalWrite(ledRplus, HIGH);

    }
    
  }
}