#include "LedControl.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>

const int dinPin = 12;
const int clockPin = 11;
const int loadPin = 10;
const int xPin = A0;
const int yPin = A1;
const int buttonPin = 2;

LedControl lc = LedControl(dinPin, clockPin, loadPin, 1);

byte LCDBrightness = 150;
const int EEPROM_LCD_BRIGHTNESS_ADDR = 0;
const int EEPROM_MATRIX_BRIGHTNESS_ADDR = 5;
byte matrixBrightness = 2;
byte xPos = 0;
byte yPos = 0;
byte xLastPos = 0;
byte yLastPos = 0;

const int minThreshold = 200;
const int maxThreshold = 600;

const byte moveInterval = 100;
unsigned long lastMoved = 0;

const byte matrixSize = 8;

const byte playerBlinkInterval = 2000;
unsigned long lastPlayerBlinked = 0;
bool playerOn = true;

byte bombXPos = 0;
byte bombYPos = 0;
unsigned long bombPlantedTime = 0;
const byte bombBlinkInterval = 100;
unsigned long lastBombBlinked = 0;
bool bombPlanted = false;

byte specialBombXPos = 7;
byte specialBombYPos = 7;
const byte specialBombBlinkInterval = 100;
unsigned long lastSpecialBombBlinked = 0;
bool specialBombActive = true;

byte matrix[matrixSize][matrixSize] = {};

bool buttonState = HIGH;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
bool bombPlacementInitiated = false;
bool bombDefusalInitiated = false;

bool gameOver = false;

const byte rs = 9;
const byte en = 8;
const byte d4 = 7;
const byte d5 = 6;
const byte d6 = 5;
const byte d7 = 4;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

int centerJoystickX;
int centerJoystickY;
int joystickDeadZone = 200;

enum JoystickDirection { UP,
                         DOWN,
                         LEFT,
                         RIGHT,
                         CENTERED };

enum GameState {
  WELCOME,
  MENU,
  START_GAME,
  SETTINGS,
  ADJUST_LCD_BRIGHTNESS,
  ADJUST_MATRIX_BRIGHTNESS,
  ADJUST_PLAYER_NAME,
  HIGHSCORE,
  ABOUT,
  DEBUGG,
  END_MESSAGE
};

GameState gameState = WELCOME;

int selectedOption = 0;
int generated = 0;
int score = 0;

const int nameLength = 8;                   // Maximum length of the name
char playerName[nameLength + 1] = "     ";  // Initialize with spaces

struct Highscore {
  char name[nameLength + 1];
  unsigned long score;
  Highscore()
    : name("N/A"), score(999) {}
};


const int maxHighscores = 2;                  // Number of highscores to keep
const int EEPROM_HIGHSCORE_START_ADDR = 100;  // Starting address for highscores in EEPROM

Highscore highscores[maxHighscores];

unsigned long gameStartTime;

bool printed = false;


JoystickDirection lastJoystickDirection = CENTERED;

JoystickDirection determineJoystickMovement(int x, int y) {
  bool inDeadZone = abs(x - centerJoystickX) < joystickDeadZone && abs(y - centerJoystickY) < joystickDeadZone;

  JoystickDirection newDirection = CENTERED;

  if (!inDeadZone) {
    if (x < centerJoystickX - joystickDeadZone) newDirection = LEFT;
    else if (x > centerJoystickX + joystickDeadZone) newDirection = RIGHT;
    else if (y < centerJoystickY - joystickDeadZone) newDirection = UP;
    else if (y > centerJoystickY + joystickDeadZone) newDirection = DOWN;
  }

  if (newDirection != lastJoystickDirection) {
    lastJoystickDirection = newDirection;
    return newDirection;
  } else {
    return CENTERED;
  }
}

bool isJoystickButtonDebounced() {
  bool reading = digitalRead(buttonPin);
  bool buttonStateChanged = false;

  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // if the button state has changed and the amount of time since the last change is greater than the debounce delay, update the state
    if (reading != buttonState) {
      buttonState = reading;
      buttonStateChanged = true;
    }
  }

  lastButtonState = reading;
  return buttonStateChanged;
}

void setup() {
  Serial.begin(9600);
  //I need this for generate maps that are different
  randomSeed(analogRead(0));

  lc.shutdown(0, false);
  EEPROM.get(EEPROM_MATRIX_BRIGHTNESS_ADDR, matrixBrightness);
  lc.setIntensity(0, matrixBrightness);
  lc.clearDisplay(0);

  pinMode(buttonPin, INPUT_PULLUP);

  initMatrix();

  highscoreInit();

  centerJoystickX = analogRead(xPin);
  centerJoystickY = analogRead(yPin);
}

void highscoreInit() {
  EEPROM.get(EEPROM_HIGHSCORE_START_ADDR, highscores);

  bool shouldUpdate = true;
  for (int i = 0; i < maxHighscores; ++i) {
    if (highscores[i].score < 0 || highscores[i].score > 100) {
      highscores[i].score = 0;
      strcpy(highscores[i].name, "N/A");
      shouldUpdate = true;
    }
  }

  if (shouldUpdate) {
    EEPROM.put(EEPROM_HIGHSCORE_START_ADDR, highscores);
  }
}

void swap(Highscore& a, Highscore& b) {
  Highscore temp = a;
  a = b;
  b = temp;
}

void updateHighscores(const char* playerName, unsigned long playerScore) {
  EEPROM.get(EEPROM_HIGHSCORE_START_ADDR, highscores);
  // Find the right position to insert the new score
  int position = 0;
  for (position = 0; position < maxHighscores; ++position) {
    if (playerScore > highscores[position].score && highscores[position].score != 0) {
      break;
    }
  }

  if (playerScore < highscores[0].score || highscores[0].score == 0) {
    //Serial.println(score);
    int length = sizeof(playerName);
    for (int i = 0; i < length; i++)
      Serial.print(playerName[i]);
    swap(highscores[0], highscores[1]);
    strcpy(highscores[0].name, playerName);
    highscores[0].score = playerScore;
  }

  EEPROM.put(EEPROM_HIGHSCORE_START_ADDR, highscores);
}

void displayHighscores() {
  EEPROM.get(EEPROM_HIGHSCORE_START_ADDR, highscores);

  lcd.clear();
  for (int i = 0; i < maxHighscores; ++i) {
    lcd.setCursor(0, i);
    lcd.print(i + 1);
    lcd.print(". ");
    lcd.print(highscores[i].name);
    lcd.print(" - ");
    lcd.print(highscores[i].score);
  }
}

void loop() {
  handleMenu();
}

void handleMenu() {
  switch (gameState) {
    case WELCOME:
      DisplayIntroMessage();
      break;
    case MENU:
      menuOption();
      break;
    case START_GAME:
      if (!generated) {
        generateMap();
        generated = 1;
      }
      startGame();
      break;
    case HIGHSCORE:
      displayHighscores();
      break;
    case SETTINGS:
      adjustSettings();
      break;
    case ADJUST_LCD_BRIGHTNESS:
      adjustLCDBrightness();
      break;
    case ADJUST_MATRIX_BRIGHTNESS:
      adjustMatrixBrightness();
      break;
    case ADJUST_PLAYER_NAME:
      adjustPlayerName();
      break;
    case DEBUGG:
      debugg();
      break;
    case ABOUT:
      displayAbout();
      break;
    case END_MESSAGE:
      displayEndMessage();
    default:
      break;
  }
}

void changeGameState(GameState newGameState) {
  lcd.setCursor(1, 0);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.clear();
  gameState = newGameState;
}

void DisplayIntroMessage() {
  lcd.begin(16, 2);
  lcd.print("Welcome to");

  lcd.setCursor(0, 1);
  lcd.print("Defusing Danger");

  unsigned long startTime = millis();
  unsigned long duration = 5000;

  while (millis() - startTime < duration) {}

  lcd.clear();
  changeGameState(MENU);

  gameStartTime = millis();
}

void displaySettingsMenu(int selectedOption) {
  //lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SETTINGS");
  lcd.setCursor(0, 1);
  lcd.print("                ");  // Clear the second line of the LCD
  lcd.setCursor(0, 1);
  if (selectedOption == 0) {
    lcd.print("LCD Brightness");
  } else if (selectedOption == 1) {
    lcd.print("Matrix Brightness");
  } else if (selectedOption == 2) {
    lcd.print("Set Name");
  }
}

void adjustSettings() {
  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  JoystickDirection direction = determineJoystickMovement(xValue, yValue);

  if (direction == DOWN) {
    selectedOption = (selectedOption + 1) % 3;
  } else if (direction == UP) {
    selectedOption = (selectedOption - 1 + 3) % 3;
  } else if (direction == LEFT || direction == RIGHT) {
    changeGameState(MENU);
  }

  displaySettingsMenu(selectedOption);

  if (isJoystickButtonDebounced() && buttonState == LOW) {
    if (selectedOption == 0) {
      changeGameState(ADJUST_LCD_BRIGHTNESS);
    } else if (selectedOption == 1) {
      changeGameState(ADJUST_MATRIX_BRIGHTNESS);
    } else if (selectedOption == 2) {
      changeGameState(ADJUST_PLAYER_NAME);
    }
  }
}


void displayAbout() {
  const char* aboutTextDown[] = {
    "DefusingDanger",
    "Ispas Jany",
    "IaniIspas"
  };

  const char* aboutTextUp[] = {
    "Game Name",
    "Author",
    "Github"
  };

  lcd.clear();

  for (int i = 0; i < 3; i++) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(aboutTextUp[i]);
    lcd.setCursor(0, 1);
    lcd.print(aboutTextDown[i]);

    delay(2000);
  }

  changeGameState(MENU);
}


void initMatrix() {
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      matrix[row][col] = 1;
    }
  }
  updateMatrix();
}

void adjustMatrixBrightness() {
  lcd.setCursor(0, 0);
  lcd.print("Matrix Brightness");
  lcd.setCursor(0, 1);
  lcd.print(matrixBrightness);
  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  JoystickDirection direction = determineJoystickMovement(xValue, yValue);
  if (direction == LEFT) {
    matrixBrightness = max(matrixBrightness - 1, 0);
    EEPROM.put(EEPROM_MATRIX_BRIGHTNESS_ADDR, matrixBrightness);
  } else if (direction == RIGHT) {
    matrixBrightness = min(matrixBrightness + 1, 15);
    EEPROM.put(EEPROM_MATRIX_BRIGHTNESS_ADDR, matrixBrightness);
  }

  //EEPROM.get(EEPROM_MATRIX_BRIGHTNESS_ADDR, matrixBrightness);
  lc.setIntensity(0, matrixBrightness);

  if (direction == DOWN || direction == UP) {
    lcd.clear();
    changeGameState(SETTINGS);
  }
  lcd.clear();
}

void adjustLCDBrightness() {
  lcd.setCursor(0, 0);
  lcd.print("LCD Brightness");
  lcd.setCursor(0, 1);
  lcd.print(LCDBrightness);
  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  JoystickDirection direction = determineJoystickMovement(xValue, yValue);

  if (direction == LEFT) {
    LCDBrightness = max(LCDBrightness - 1, 150);
    EEPROM.put(EEPROM_LCD_BRIGHTNESS_ADDR, LCDBrightness);
  } else if (direction == RIGHT) {
    LCDBrightness = min(LCDBrightness + 1, 255);
    EEPROM.put(EEPROM_LCD_BRIGHTNESS_ADDR, LCDBrightness);
  }

  if (direction == DOWN || direction == UP) {
    changeGameState(SETTINGS);
  }
  lcd.clear();
}

void debugg() {
  if (!printed) {
    int length = sizeof(playerName);
    for (int i = 0; i < length; i++)
      Serial.print(playerName[i]);
  }
  printed = true;
}

void adjustPlayerName() {
  static int charPosition = 0;    // Current character position in the name
  static char currentChar = 'A';  // Current character being edited

  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  JoystickDirection direction = determineJoystickMovement(xValue, yValue);

  // Scroll through letters
  if (direction == RIGHT) {
    if (currentChar == 'Z') currentChar = 'A';
    else currentChar++;
  } else if (direction == LEFT) {
    if (currentChar == 'A') currentChar = 'Z';
    else currentChar--;
  }

  // Display the current name and character being edited
  //lcd.clear();
  lcd.print("                ");  // Clear the second line of the LCD
  lcd.setCursor(0, 0);
  //lcd.print("Name: ");
  for (int i = 0; i < charPosition; i++) {
    lcd.print(playerName[i]);
  }
  lcd.print(currentChar);

  // Save current character and move to the next
  if (direction == UP) {
    playerName[charPosition] = currentChar;
    charPosition++;
    if (charPosition >= nameLength) {
      charPosition = 0;
      changeGameState(MENU);  // Save the name and return to menu
    }
    currentChar = 'A';  // Reset for next character
  }

  // Save the name and return to the menu
  if (isJoystickButtonDebounced() && buttonState == LOW) {
    playerName[charPosition] = currentChar;
    playerName[charPosition + 1] = '\0';  // Null-terminate the string
    charPosition = 0;                 // Reset for next time
    changeGameState(MENU);
    //changeGameState(DEBUGG);
  }

  // Return to the menu without saving
  if (direction == DOWN) {
    charPosition = 0;  // Reset for next time
    changeGameState(MENU);
  }
}


void menuOption() {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print("MENU");

  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  JoystickDirection direction = determineJoystickMovement(xValue, yValue);


  if (direction == DOWN) {
    selectedOption = (selectedOption + 1) % 4;
  } else if (direction == UP) {
    selectedOption = (selectedOption - 1 + 4) % 4;
  }

  lcd.setCursor(0, 1);
  lcd.print("                ");  // Clear the second line of the LCD
  lcd.setCursor(0, 1);

  if (selectedOption == 0) {
    lcd.print("Start Game");
  } else if (selectedOption == 1) {
    lcd.print("Settings");
  } else if (selectedOption == 2) {
    lcd.print("About");
  } else if (selectedOption == 3) {
    lcd.print("Highscore");
  }

  if (isJoystickButtonDebounced()) {
    if (buttonState == LOW) {
      if (selectedOption == 0) {
        selectedOption = 0;
        lcd.clear();
        changeGameState(START_GAME);
      } else if (selectedOption == 1) {
        lcd.clear();
        selectedOption = 0;
        changeGameState(SETTINGS);
      } else if (selectedOption == 2) {
        lcd.clear();
        selectedOption = 0;
        changeGameState(ABOUT);
      } else if (selectedOption == 3) {
        lcd.clear();
        selectedOption = 0;
        changeGameState(HIGHSCORE);
      }
    }
  }
  gameStartTime = millis();
}


void displayEndMessage() {
  // Serial.println(playerName[0]);
  // Serial.println(playerName[1]);
  // Serial.println(playerName[2]);
  // Serial.println(score);
  lcd.print("Congratulations");
  lcd.setCursor(0, 1);
  lcd.print("YOU WIN!");
  unsigned long startTime = millis();
  unsigned long duration = 5000;

  while (millis() - startTime < duration) {}

  changeGameState(MENU);
}

void startGame() {
  if (gameOver) {
    unsigned long endTime = millis();
    score = (endTime - gameStartTime) / 1000;
    updateHighscores(playerName, score);
    changeGameState(END_MESSAGE);
    return;
  }

  updateGameDisplay();
  handlePlayerBlinking();
  handlePlayerMovement();
  handleSpecialBombBlinking();
  handleButtonInteractions();
  handleBombBlinking();
  handleBombExplosion();
}

void updateGameDisplay() {
  if (!bombDefusalInitiated) {
    lcd.clear();
    lcd.print("Time : ");
    lcd.setCursor(7, 0);
    lcd.print((millis() - gameStartTime) / 1000);
    lcd.setCursor(0, 0);
  } else {
    showDefusingCountdown();
  }
}

void handlePlayerBlinking() {
  if (millis() - lastPlayerBlinked > playerBlinkInterval) {
    playerOn = !playerOn;
    lastPlayerBlinked = millis();
    matrix[xPos][yPos] = playerOn;
    lc.setLed(0, xPos, yPos, playerOn);
  }
}

void handlePlayerMovement() {
  if (millis() - lastMoved > moveInterval) {
    updatePositions();
    lastMoved = millis();
  }
}

void handleSpecialBombBlinking() {
  if (specialBombActive && millis() - lastSpecialBombBlinked > specialBombBlinkInterval) {
    matrix[specialBombXPos][specialBombYPos] = !matrix[specialBombXPos][specialBombYPos];
    lastSpecialBombBlinked = millis();
    updateMatrix();
  }
}

void handleButtonInteractions() {
  if (isJoystickButtonDebounced()) {
    if (buttonState == LOW) {
      buttonPressStartTime = millis();
      bombPlacementInitiated = true;

      if ((xPos == bombXPos && yPos == bombYPos) || (xPos == specialBombXPos && yPos == specialBombYPos)) {
        bombDefusalInitiated = true;
      }
    } else {
      if (!bombDefusalInitiated && bombPlacementInitiated) {
        placeBomb();
      }
      bombPlacementInitiated = false;
    }
  }
}

void handleBombBlinking() {
  if (bombPlanted && millis() - lastBombBlinked > bombBlinkInterval) {
    matrix[bombXPos][bombYPos] = !matrix[bombXPos][bombYPos];
    lastBombBlinked = millis();
    updateMatrix();
  }
}

void handleBombExplosion() {
  if (bombPlanted && millis() - bombPlantedTime > 3000) {
    explodeBomb();
  }
}

void showDefusingCountdown() {
  unsigned long buttonPressDuration = millis() - buttonPressStartTime;
  unsigned long timeRemaining = 5000 - buttonPressDuration;

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Defusing: ");
  lcd.print(timeRemaining / 1000);

  if (buttonPressDuration >= 5000) {
    defuseBomb();
    bombDefusalInitiated = false;  // Reset defusal state
  }
}


void generateMap() {
  for (int i = 0; i < matrixSize; i++) {
    for (int j = 0; j < matrixSize; j++) {
      matrix[i][j] = 0;
    }
  }
  matrix[xPos][yPos] = 1;
  for (int i = 0; i < matrixSize; i++) {
    for (int j = 0; j < matrixSize; j++) {
      // Check if the current position is not around the player
      if ((abs(i - xPos) > 1 || abs(j - yPos) > 1) && (i != specialBombXPos || j != specialBombYPos) && random(2)) {
        // Place a wall
        matrix[i][j] = 3;
      }
    }
  }
}

void updateMatrix() {
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      lc.setLed(0, row, col, matrix[row][col] > 0);
    }
  }
}

void updatePositions() {
  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);

  xLastPos = xPos;
  yLastPos = yPos;

  // Calculate the new position
  byte newXPos = xPos;
  byte newYPos = yPos;

  JoystickDirection direction = determineJoystickMovement(xValue, yValue);

  switch (direction) {
    case UP:
      newYPos = (yPos > 0) ? yPos - 1 : yPos;  // Move up only if not at the top edge
      break;
    case DOWN:
      newYPos = (yPos < matrixSize - 1) ? yPos + 1 : yPos;  // Move down only if not at the bottom edge
      break;
    case LEFT:
      newXPos = (xPos > 0) ? xPos - 1 : xPos;  // Move left only if not at the left edge
      break;
    case RIGHT:
      newXPos = (xPos < matrixSize - 1) ? xPos + 1 : xPos;  // Move right only if not at the right edge
      break;
    case CENTERED:
      // Do nothing if the joystick is centered
      break;
  }

  // Only update the position if the new position is not a wall
  if (matrix[newXPos][newYPos] != 3) {
    xPos = newXPos;
    yPos = newYPos;
  }

  // Update matrix
  if (xPos != xLastPos || yPos != yLastPos) {
    updateMatrix();
    matrix[xLastPos][yLastPos] = 0;
    matrix[xPos][yPos] = 1;
  }
}


void placeBomb() {
  bombXPos = xPos;
  bombYPos = yPos;
  bombPlantedTime = millis();
  bombPlanted = true;
  matrix[bombXPos][bombYPos] = 2;
  updateMatrix();
}

void explodeBomb() {
  matrix[bombXPos][bombYPos] = 0;
  if (bombXPos > 0) matrix[bombXPos - 1][bombYPos] = 0;
  if (bombXPos < matrixSize - 1) matrix[bombXPos + 1][bombYPos] = 0;
  if (bombYPos > 0) matrix[bombXPos][bombYPos - 1] = 0;
  if (bombYPos < matrixSize - 1) matrix[bombXPos][bombYPos + 1] = 0;
  bombPlanted = false;
  updateMatrix();
}

void defuseBomb() {
  specialBombActive = false;
  matrix[bombXPos][bombYPos] = 0;
  gameOver = true;
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      matrix[row][col] = 1;
    }
  }
  updateMatrix();
}


void printMatrix() {
  for (int i = 0; i < matrixSize; i++) {
    for (int j = 0; j < matrixSize; j++) {
      Serial.print(matrix[i][j]);
      Serial.print(" ");
    }
    Serial.println();
  }
}
