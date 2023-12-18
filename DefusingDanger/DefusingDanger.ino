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
byte xPos = 1;
byte yPos = 1;
byte xLastPos = 0;
byte yLastPos = 0;

const int minThreshold = 200;
const int maxThreshold = 600;

const byte moveInterval = 100;
unsigned long lastMoved = 0;

const byte matrixSize = 16;

const byte playerBlinkInterval = 2000;
unsigned long lastPlayerBlinked = 0;
bool playerOn = true;

byte bombXPos = 0;
byte bombYPos = 0;
unsigned long bombPlantedTime = 0;
const byte bombBlinkInterval = 100;
unsigned long lastBombBlinked = 0;
bool bombPlanted = false;

byte specialBombXPos = 6;
byte specialBombYPos = 6;
const byte specialBombBlinkInterval = 100;
unsigned long lastSpecialBombBlinked = 0;
bool specialBombActive = true;

byte matrix[matrixSize][matrixSize] = {
  { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 },
  { 4, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 4 },
  { 4, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 4 },
  { 4, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, 4 },
  { 4, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 4 },
  { 4, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 4 },
  { 4, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 4 },
  { 4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 5, 4, 4, 4, 4 },
  { 4, 4, 4, 5, 4, 4, 4, 4, 4, 4, 4, 5, 4, 4, 4, 4 },
  { 4, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 4 },
  { 4, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 4 },
  { 4, 0, 0, 0, 0, 0, 0, 5, 5, 0, 0, 0, 0, 0, 0, 4 },
  { 4, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 4 },
  { 4, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 4 },
  { 4, 0, 0, 0, 0, 0, 0, 4, 4, 0, 0, 0, 0, 0, 0, 4 },
  { 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4 }
};

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
  ADJUST_DIFFICULTY,
  HIGHSCORE,
  RESET_HIGHSCORES,
  ABOUT,
  END_MESSAGE
};

GameState gameState = WELCOME;

int selectedOption = 0;
int generated = 0;
int score = 0;
byte currentDifficulty = 0;

const int nameLength = 8;                    // Maximum length of the name
char playerName[nameLength + 1] = "N/A   ";  // Initialize with spaces

struct Highscore {
  char name[nameLength + 1];
  unsigned long score;
  Highscore()
    : name("N/A"), score(0) {}
};


const int maxHighscores = 3;                  // Number of highscores to keep
const int EEPROM_HIGHSCORE_START_ADDR = 100;  // Starting address for highscores in EEPROM
int currentHighscoreIndex = 0;

Highscore highscores[maxHighscores];

bool win;
unsigned long gameStartTime;
const unsigned long gameDuration = 60000;
unsigned long introMessageStartTime;

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

  lcd.begin(16, 2);

  introMessageStartTime = millis();
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
  Serial.print(playerName);
  EEPROM.get(EEPROM_HIGHSCORE_START_ADDR, highscores);

  int insertPosition = -1;  // Position where new highscore should be inserted


  if (playerName[0] == " ")
    strcpy(playerName, "N/A");
  Serial.print(playerName);
  // Find the position to insert the new highscore
  for (int i = 0; i < maxHighscores; i++) {
    if (playerScore > highscores[i].score) {
      insertPosition = i;
      break;
    }
  }

  // If a valid position was found, update highscores
  if (insertPosition != -1) {
    // Shift down highscores below the insertion position
    for (int i = maxHighscores - 1; i > insertPosition; i--) {
      highscores[i] = highscores[i - 1];
    }

    // Insert the new highscore
    strcpy(highscores[insertPosition].name, playerName);
    highscores[insertPosition].score = playerScore;

    // Save the updated highscores to EEPROM
    EEPROM.put(EEPROM_HIGHSCORE_START_ADDR, highscores);
  }
}

void displayHighscores() {
  EEPROM.get(EEPROM_HIGHSCORE_START_ADDR, highscores);

  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  currentHighscoreIndex = max(0, min(currentHighscoreIndex, maxHighscores - 1));

  JoystickDirection direction = determineJoystickMovement(xValue, yValue);

  switch (direction) {
    case RIGHT:
      currentHighscoreIndex = min(currentHighscoreIndex + 1, maxHighscores - 1);
      break;
    case LEFT:
      currentHighscoreIndex = max(currentHighscoreIndex - 1, 0);
      break;
    case DOWN:
      changeGameState(MENU);
  }

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Highscores");
  lcd.setCursor(0, 1);
  lcd.print(currentHighscoreIndex + 1);
  lcd.print(". ");
  lcd.print(highscores[currentHighscoreIndex].name);
  lcd.print(" - ");
  lcd.print(highscores[currentHighscoreIndex].score);
}

void resetHighscores() {
  lcd.clear();
  lcd.print("Reset Highscores?");
  lcd.setCursor(0, 1);
  lcd.print("Press btn to conf");

  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  JoystickDirection direction = determineJoystickMovement(xValue, yValue);
  // Check joystick button state
  if (isJoystickButtonDebounced() && buttonState == LOW) {
    for (int i = 0; i < maxHighscores; ++i) {
      strcpy(highscores[i].name, "N/A");
      highscores[i].score = 0;
    }
    EEPROM.put(EEPROM_HIGHSCORE_START_ADDR, highscores);
    changeGameState(SETTINGS);
  }

  if (direction == DOWN) {
    changeGameState(SETTINGS);
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
    case RESET_HIGHSCORES:
      resetHighscores();
      break;
    case SETTINGS:
      adjustSettings();
      break;
    case ADJUST_DIFFICULTY:
      adjustDifficulty();
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
    case ABOUT:
      displayAbout();
      break;
    case END_MESSAGE:
      displayEndMessage(win);
    default:
      break;
  }
}

void changeGameState(GameState newGameState) {
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.clear();
  gameState = newGameState;
}

void DisplayIntroMessage() {
  int currentTime = millis();

  if (currentTime - introMessageStartTime < 5000) {
    lcd.setCursor(0, 0);
    lcd.print("Welcome to");
    lcd.setCursor(0, 1);
    lcd.print("Defusing Danger");
  } else {
    changeGameState(MENU);
  }
}

void displaySettingsMenu(int selectedOption) {
  lcd.setCursor(0, 0);
  lcd.print("SETTINGS");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  if (selectedOption == 0) {
    lcd.print("LCD Brightness");
  } else if (selectedOption == 1) {
    lcd.print("Matrix Brightness");
  } else if (selectedOption == 2) {
    lcd.print("Set Name");
  } else if (selectedOption == 3) {
    lcd.print("Reset Highscores");
  }
}

void adjustSettings() {
  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  JoystickDirection direction = determineJoystickMovement(xValue, yValue);

  if (direction == DOWN) {
    selectedOption = (selectedOption + 1) % 4;
  } else if (direction == UP) {
    selectedOption = (selectedOption - 1 + 4) % 4;
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
    } else if (selectedOption == 3) {
      changeGameState(RESET_HIGHSCORES);
    }
  }
}

void adjustDifficulty() {
  lcd.setCursor(0, 0);
  lcd.print("Difficulty:    ");
  lcd.setCursor(0, 1);

  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  JoystickDirection direction = determineJoystickMovement(xValue, yValue);

  if (direction == DOWN) {
    currentDifficulty = (currentDifficulty + 1) % 3;
  } else if (direction == UP) {
    currentDifficulty = (currentDifficulty + 3 - 1) % 3;
  }

  switch (currentDifficulty) {
    case 0:
      lcd.print("Easy   ");
      break;
    case 1:
      lcd.print("Medium ");
      break;
    case 2:
      lcd.print("Hard   ");
      break;
  }

  if (isJoystickButtonDebounced() && buttonState == LOW) {
    changeGameState(MENU);
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
      lc.setLed(0, row, col, true);
    }
  }
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

  EEPROM.get(EEPROM_MATRIX_BRIGHTNESS_ADDR, matrixBrightness);
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
    charPosition = 0;                     // Reset for next time
    changeGameState(MENU);
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
    selectedOption = (selectedOption + 1) % 5;
  } else if (direction == UP) {
    selectedOption = (selectedOption - 1 + 5) % 5;
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
  } else if (selectedOption == 4) {
    lcd.print("Difficulty:    ");
  }

  if (isJoystickButtonDebounced()) {
    if (buttonState == LOW) {
      if (selectedOption == 0) {
        resetGame();
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
      } else if (selectedOption == 4) {
        lcd.clear();
        selectedOption = 0;
        changeGameState(ADJUST_DIFFICULTY);
        selectedOption = 0;
      }
    }
  }
  gameStartTime = millis();
}

void displayEndMessage(bool win) {
  initMatrix();
  lcd.clear();
  lcd.setCursor(0, 0);
  if (win) {
    lcd.print("Congratulations");
    lcd.setCursor(0, 1);
    lcd.print("YOU WIN!");
  } else {
    lcd.print("Game Over");
    lcd.setCursor(0, 1);
    lcd.print("FAIL");
  }

  delay(5000);            // Display the message for 5 seconds
  changeGameState(MENU);  // Return to the main menu
}


void resetGame() {
  gameOver = false;
  win = false;
  generated = 0;
  score = 0;
  xPos = 1;
  yPos = 1;
  xLastPos = 0;
  yLastPos = 0;
  bombPlanted = false;
  specialBombActive = true;
  gameStartTime = millis();  // Reset the game start time

  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      if (((row == 4 || row == 11) && (col == 7 || col == 8)) || ((row == 7 || row == 8) && (col == 3 || col == 11))) {
        matrix[row][col] = 5;
      } else if (row == 0 || row == 7 || row == 8 || row == matrixSize - 1 || col == 0 || col == 7 || col == 8 || col == matrixSize - 1) {
        matrix[row][col] = 4;
      } else {
        matrix[row][col] = 0;
      }
    }
  }
}

void startGame() {
  if (gameOver) {
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
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - gameStartTime;
  unsigned long remainingTime = (gameDuration > elapsedTime) ? (gameDuration - elapsedTime) : 0;
  if (!bombDefusalInitiated) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Name: ");
    lcd.print(playerName);
    lcd.setCursor(0, 1);
    lcd.print("Time: ");
    lcd.print(remainingTime / 1000);

    if (remainingTime == 0) {
      gameOver = true;
      changeGameState(END_MESSAGE);
    }
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
    bombDefusalInitiated = false;
  }
}


void generateMap() {
  matrix[xPos][yPos] = 1;
  int wallProbability = 0;
  switch (currentDifficulty) {
    case 0:
      wallProbability = 25;
      break;
    case 1:
      wallProbability = 50;
      break;
    case 2:
      wallProbability = 75;
      break;
  }
  for (int i = 1; i < matrixSize - 1; i++) {
    for (int j = 1; j < matrixSize - 1; j++) {
      // Check if the current position is not around the player
      if ((abs(i - xPos) > 1 || abs(j - yPos) > 1) && (i != specialBombXPos || j != specialBombYPos) && matrix[i][j] != 4 && random(100) < wallProbability && matrix[i][j] != 5) {
        // Place a wall
        matrix[i][j] = 3;
      }
    }
  }
}

void updateMatrix() {
  byte startRow = (xPos / 8) * 8;
  byte startCol = (yPos / 8) * 8;

  for (int row = 0; row < 8; row++) {
    for (int col = 0; col < 8; col++) {
      byte actualRow = startRow + row;
      byte actualCol = startCol + col;
      lc.setLed(0, row, col, (matrix[actualRow][actualCol] > 0 && matrix[actualRow][actualCol] != 5));
    }
  }
}


void updatePositions() {
  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);

  xLastPos = xPos;
  yLastPos = yPos;

  byte newXPos = xPos;
  byte newYPos = yPos;

  JoystickDirection direction = determineJoystickMovement(xValue, yValue);

  switch (direction) {
    case UP:
      newXPos = max(xPos - 1, 0);
      break;
    case DOWN:
      newXPos = min(xPos + 1, matrixSize - 1);
      break;
    case LEFT:
      newYPos = max(yPos - 1, 0);
      break;
    case RIGHT:
      newYPos = min(yPos + 1, matrixSize - 1);
      break;
    case CENTERED:
      break;
  }

  // Check if the new position is a wall or door
  if (matrix[newXPos][newYPos] != 3 && matrix[newXPos][newYPos] != 4) {
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
  if (bombXPos > 0 && matrix[bombXPos - 1][bombYPos] != 4) matrix[bombXPos - 1][bombYPos] = 0;
  if (bombXPos < matrixSize - 1 && matrix[bombXPos + 1][bombYPos] != 4) matrix[bombXPos + 1][bombYPos] = 0;
  if (bombYPos > 0 && matrix[bombXPos][bombYPos - 1] != 4) matrix[bombXPos][bombYPos - 1] = 0;
  if (bombYPos < matrixSize - 1 && matrix[bombXPos][bombYPos + 1] != 4) matrix[bombXPos][bombYPos + 1] = 0;
  bombPlanted = false;
  updateMatrix();
}

void defuseBomb() {
  specialBombActive = false;
  matrix[bombXPos][bombYPos] = 0;
  gameOver = true;
  win = true;
  unsigned long endTime = millis();
  score = (gameDuration - (endTime - gameStartTime)) / 1000;
  updateHighscores(playerName, score);
  changeGameState(END_MESSAGE);
}


void printMatrix() {
  for (int i = 0; i < matrixSize; i++) {
    for (int j = 0; j < matrixSize; j++) {
      (matrix[i][j]);
      Serial.print(" ");
    }
    Serial.println();
  }
}
