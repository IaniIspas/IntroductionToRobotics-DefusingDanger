#include "LedControl.h"
#include <LiquidCrystal.h>
#include <EEPROM.h>

// Pin connections
const int dinPin = 12;
const int clockPin = 11;
const int loadPin = 10;
const int xPin = A0;
const int yPin = A1;
const int buttonPin = 2;

// LedControl initialization
LedControl lc = LedControl(dinPin, clockPin, loadPin, 1);

// EEPROM addresses
const int EEPROM_LCD_BRIGHTNESS_ADDR = 0;
const int EEPROM_MATRIX_BRIGHTNESS_ADDR = 5;
const int EEPROM_HIGHSCORE_START_ADDR = 100;

// LCD and matrix brightness settings
byte LCDBrightness;
byte matrixBrightness;

// Player position and movement
byte xPos = 1;
byte yPos = 1;
byte xLastPos = 0;
byte yLastPos = 0;
const byte moveInterval = 100;
unsigned long lastMoved = 0;

// Player blinking settings
const byte playerBlinkInterval = 2000;
unsigned long lastPlayerBlinked = 0;
bool playerOn = true;

// Bomb position, timing, and blinking
byte bombXPos = 0;
byte bombYPos = 0;
unsigned long bombPlantedTime = 0;
const byte bombBlinkInterval = 100;
unsigned long lastBombBlinked = 0;
bool bombPlanted = false;

// Special bomb position, timing, and blinking
byte specialBombXPos = 6;
byte specialBombYPos = 6;
const byte specialBombBlinkInterval = 100;
unsigned long lastSpecialBombBlinked = 0;
bool specialBombActive = true;
unsigned long specialBombMoveInterval = 15000;  // 25 seconds
unsigned long lastSpecialBombMoveTime = 0;

// Game map matrix
const byte matrixSize = 16;
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

// Button state and debouncing
bool buttonState = HIGH;
bool lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 50;
unsigned long buttonPressStartTime = 0;
bool buttonPressed = false;
bool bombPlacementInitiated = false;
bool bombDefusalInitiated = false;

// Enums
enum JoystickDirection { UP,
                         DOWN,
                         LEFT,
                         RIGHT,
                         CENTERED };
enum GameState { WELCOME,
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
                 HOW_TO_PLAY,
                 END_MESSAGE };

// Game state and settings
bool gameOver = false;
GameState gameState = WELCOME;
int selectedOption = 0;
int score = 0;
byte currentDifficulty = 0;
const int nameLength = 8;
char playerName[nameLength + 1] = "N/A   ";
unsigned long gameStartTime;
const unsigned long gameDuration = 60000;
unsigned long introMessageStartTime;
JoystickDirection lastJoystickDirection = CENTERED;

// Constants for Joystick
const int joystickDeadZone = 200;  // Deadzone threshold for joystick

// Variables for Joystick Center Position
int centerJoystickX;
int centerJoystickY;

// Highscore management

struct Highscore {
  char name[nameLength + 1];
  unsigned long score;
  Highscore()
    : name("N/A"), score(0) {}
};


const int maxHighscores = 3;
int currentHighscoreIndex = 0;
Highscore highscores[maxHighscores];

// LCD display setup
const byte rs = 9;
const byte en = 8;
const byte d4 = 7;
const byte d5 = 6;
const byte d6 = 5;
const byte d7 = 4;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// Player win state
bool win;

bool staticContentNeedsUpdate = true;


// Global variables for displayAbout
unsigned long aboutMessageStartTime;
int aboutMessageIndex = 0;
const int aboutMessageDuration = 5000;  // 5 seconds for each about message screen
const int aboutMessageCount = 3;        // total number of about messages

void initializeEEPROM() {
  // Read the current values of LCD and Matrix brightness from EEPROM
  //EEPROM.get(EEPROM_LCD_BRIGHTNESS_ADDR, LCDBrightness);
  EEPROM.get(EEPROM_MATRIX_BRIGHTNESS_ADDR, matrixBrightness);

  // Initialize highscores from EEPROM
  highscoreInit();
}

void setup() {
  //Initialize LCD
  lcd.begin(16, 2);

  //I need this for generate maps that are different
  randomSeed(analogRead(0));

  //Set intro message start time
  introMessageStartTime = millis();
  initializeEEPROM();
  lc.shutdown(0, false);
  lc.setIntensity(0, matrixBrightness);
  lc.clearDisplay(0);

  //Turn matrix ON
  turnOnMatrix();

  pinMode(buttonPin, INPUT_PULLUP);
  //Set Joystick Center Position
  centerJoystickX = analogRead(xPin);
  centerJoystickY = analogRead(yPin);
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
    case HOW_TO_PLAY:
      displayHowToPlay();
      break;
    case END_MESSAGE:
      displayEndMessage(win);
    default:
      break;
  }
}

void turnOnMatrix() {
  for (int row = 0; row < matrixSize; row++) {
    for (int col = 0; col < matrixSize; col++) {
      lc.setLed(0, row, col, true);
    }
  }
}


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

void changeGameState(GameState newGameState) {
  lcd.setCursor(0, 0);
  //lcd.clear();
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  //lcd.clear();
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

void menuOption() {
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 0);
  lcd.print("MENU");

  int xValue = analogRead(xPin);
  int yValue = analogRead(yPin);
  JoystickDirection direction = determineJoystickMovement(xValue, yValue);


  if (direction == DOWN) {
    selectedOption = (selectedOption + 1) % 6;
  } else if (direction == UP) {
    selectedOption = (selectedOption - 1 + 5) % 6;
  }

  lcd.setCursor(0, 1);
  lcd.print("                ");
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
    lcd.print("Difficulty");
  } else if (selectedOption == 5) {
    lcd.print("How to play");
  }

  if (isJoystickButtonDebounced()) {
    if (buttonState == LOW) {
      if (selectedOption == 0) {
        resetGame();
        //lcd.clear();
        changeGameState(START_GAME);
      } else if (selectedOption == 1) {
        //lcd.clear();
        changeGameState(SETTINGS);
      } else if (selectedOption == 2) {
        startAboutSection();
        //lcd.clear();
        changeGameState(ABOUT);
      } else if (selectedOption == 3) {
        //lcd.clear();
        changeGameState(HIGHSCORE);
      } else if (selectedOption == 4) {
        //lcd.clear();
        changeGameState(ADJUST_DIFFICULTY);
      } else if (selectedOption == 5) {
        gameState = HOW_TO_PLAY;
      }
    }
  }
  // gameStartTime = millis();
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

  // Position where new highscore should be inserted
  int insertPosition = -1;

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

  lcd.setCursor(0, 0);
  lcd.print("Highscores");
  lcd.setCursor(0, 1);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print(currentHighscoreIndex + 1);
  lcd.print(".");
  lcd.print(highscores[currentHighscoreIndex].name);
  lcd.print("-");
  lcd.print(highscores[currentHighscoreIndex].score);
}

void resetHighscores() {
  lcd.print("                ");
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
  lcd.print("Difficulty : ");
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
  unsigned long currentTime = millis();

  // Check if it's time to switch to the next about message
  if (currentTime - aboutMessageStartTime > aboutMessageDuration) {
    aboutMessageStartTime = currentTime;  // Reset the start time for the next message
    aboutMessageIndex++;

    if (aboutMessageIndex >= aboutMessageCount) {
      changeGameState(MENU);  // Return to the menu after the last message
      aboutMessageIndex = 0;  // Reset the index for the next time
      return;
    }
  }

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

  // Display the current about message
  //lcd.clear();
  lcd.print("              ");
  lcd.setCursor(0, 0);
  lcd.print(aboutTextUp[aboutMessageIndex]);
  lcd.setCursor(0, 1);
  lcd.print(aboutTextDown[aboutMessageIndex]);
}

// Call this function to start displaying the about section
void startAboutSection() {
  aboutMessageStartTime = millis();  // Set the start time for the about section
  aboutMessageIndex = 0;             // Start with the first message
}

void displayHowToPlay() {
  static int page = 0;  // Keeps track of the instruction page

  lcd.print("                ");
  lcd.setCursor(0, 0);

  switch (page) {
    case 0:
      lcd.print("Goal:");
      lcd.setCursor(0, 1);
      lcd.print("Defuse bomb");
      break;
    case 1:
      lcd.print("Find blinking");
      lcd.setCursor(0, 1);
      lcd.print("special bomb");
      break;
    case 2:
      lcd.print("Go next to it");
      lcd.setCursor(0, 1);
      lcd.print("& press button");
      break;
      // Add more cases if you have additional instructions
  }

  // Logic to change page on button press
  if (isJoystickButtonDebounced() && buttonState == LOW) {
    page++;
    if (page > 2) {  // Adjust this number based on the total number of pages
      page = 0;
      changeGameState(MENU);  // Return to menu after the last page
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
    //lcd.clear();
    lcd.print("              ");
    changeGameState(SETTINGS);
  }
  //lcd.clear();
  lcd.print("                ");
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
  //lcd.clear();
  lcd.print("                ");
}

void adjustPlayerName() {
  static int charPosition = 0;
  static char currentChar = 'A';

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

  lcd.print("                ");  // Clear the second line of the LCD
  lcd.setCursor(0, 0);
  lcd.print("Name: ");
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

void displayEndMessage(bool win) {
  turnOnMatrix();
  //lcd.clear();
  lcd.print("              ");
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
  delay(5000);  // Keep the message for 5 seconds

  changeGameState(MENU);  // Return to the main menu
}


void resetGame() {
  gameOver = false;
  win = false;
  score = 0;
  xPos = 1;
  yPos = 1;
  xLastPos = 0;
  yLastPos = 0;
  bombPlanted = false;
  specialBombActive = true;
  specialBombXPos = 6;
  specialBombYPos = 6;
  gameStartTime = millis();
  lastSpecialBombMoveTime = 0;
  lastSpecialBombMoveTime = millis();

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
  generateMap();
}

void startGame() {
  updateGameDisplay();
  handlePlayerBlinking();
  handlePlayerMovement();
  handleSpecialBombBlinking();
  handleButtonInteractions();
  handleBombBlinking();
  handleSpecialBombMovement();
  handleBombExplosion();
}

void updateGameDisplay() {
  unsigned long currentTime = millis();
  unsigned long elapsedTime = currentTime - gameStartTime;
  unsigned long remainingTime = (gameDuration > elapsedTime) ? (gameDuration - elapsedTime) : 0;
  if (!bombDefusalInitiated) {
    //lcd.clear();
    lcd.print("        ");
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


      // Check if player is adjacent to the special bomb's position to initiate defusing
      if ((abs(xPos - specialBombXPos) <= 1 && yPos == specialBombYPos) || (abs(yPos - specialBombYPos) <= 1 && xPos == specialBombXPos)) {
        unsigned long remainingTime = (gameDuration - (millis() - gameStartTime)) / 1000;
        if (remainingTime >= 5) {
          bombDefusalInitiated = true;
        }
      } else {
        // Else, initiate bomb placement if not already defusing a bomb
        bombPlacementInitiated = !bombDefusalInitiated;
      }
    } else {
      if (bombPlacementInitiated && !bombDefusalInitiated) {
        placeBomb();
      }
      bombPlacementInitiated = false;
      bombDefusalInitiated = false;
    }
  }
}

// void handleButtonInteractions() {
//   if (isJoystickButtonDebounced()) {
//     if (buttonState == LOW) {
//       buttonPressStartTime = millis();

//       // Check if there's enough time left to defuse the bomb
// unsigned long remainingTime = (gameDuration - (millis() - gameStartTime)) / 1000;
//       if (remainingTime >= 5) {
//         if ((abs(xPos - specialBombXPos) <= 1 && yPos == specialBombYPos) || (abs(yPos - specialBombYPos) <= 1 && xPos == specialBombXPos)) {
//           bombDefusalInitiated = true;
//         } else {
//           bombPlacementInitiated = !bombDefusalInitiated;
//         }
//       }
//     } else {
//       bombPlacementInitiated = false;
//       bombDefusalInitiated = false;
//     }
//   }
// }


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

  lcd.setCursor(0, 1);
  lcd.print("        ");
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
  if (matrix[newXPos][newYPos] != 3 && matrix[newXPos][newYPos] != 4 && !(newXPos == specialBombXPos && newYPos == specialBombYPos)) {
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

// void explodeBomb() {
//   matrix[bombXPos][bombYPos] = 0;
//   if (bombXPos > 0 && matrix[bombXPos - 1][bombYPos] != 4) matrix[bombXPos - 1][bombYPos] = 0;
//   if (bombXPos < matrixSize - 1 && matrix[bombXPos + 1][bombYPos] != 4) matrix[bombXPos + 1][bombYPos] = 0;
//   if (bombYPos > 0 && matrix[bombXPos][bombYPos - 1] != 4) matrix[bombXPos][bombYPos - 1] = 0;
//   if (bombYPos < matrixSize - 1 && matrix[bombXPos][bombYPos + 1] != 4) matrix[bombXPos][bombYPos + 1] = 0;
//   bombPlanted = false;
//   updateMatrix();
// }

void explodeBomb() {
  // Check if player is next to the bomb
  bool playerIsAdjacent = (xPos == bombXPos && abs(yPos - bombYPos) <= 1) || 
                          (yPos == bombYPos && abs(xPos - bombXPos) <= 1);

  // Clear around areas (except for indestructible walls)
  matrix[bombXPos][bombYPos] = 0;
  if (bombXPos > 0 && matrix[bombXPos - 1][bombYPos] != 4) matrix[bombXPos - 1][bombYPos] = 0;
  if (bombXPos < matrixSize - 1 && matrix[bombXPos + 1][bombYPos] != 4) matrix[bombXPos + 1][bombYPos] = 0;
  if (bombYPos > 0 && matrix[bombXPos][bombYPos - 1] != 4) matrix[bombXPos][bombYPos - 1] = 0;
  if (bombYPos < matrixSize - 1 && matrix[bombXPos][bombYPos + 1] != 4) matrix[bombXPos][bombYPos + 1] = 0;
  
  bombPlanted = false;
  updateMatrix();

  // If player is next to the bomb, they lose
  if (playerIsAdjacent) {
    gameOver = true;
    win = false; 
    changeGameState(END_MESSAGE);
  }
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

void moveSpecialBomb() {
  byte newSpecialBombX, newSpecialBombY;
  do {
    newSpecialBombX = random(1, matrixSize - 1);
    newSpecialBombY = random(1, matrixSize - 1);
  } while (matrix[newSpecialBombX][newSpecialBombY] != 0);  // Ensure the new position is empty

  // Update the position of the special bomb
  specialBombXPos = newSpecialBombX;
  specialBombYPos = newSpecialBombY;

  lastSpecialBombMoveTime = millis();
}

void handleSpecialBombMovement() {
  if (millis() - lastSpecialBombMoveTime > specialBombMoveInterval) {
    moveSpecialBomb();
  }
}
