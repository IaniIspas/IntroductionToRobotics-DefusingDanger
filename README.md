# IntroductionToRobotics - Matrix Project

## Backstory

As a fan of tactical and strategy games like Counter-Strike and Bomberman, I wanted to create a unique game that combines elements of both. . It's a game that challenges players in a strategic environment with a mix of fun and problem-solving.

## Game Description
"Defusing Danger" is a grid-based game displayed on an LED matrix, powered by an Arduino. The player's objective is to navigate through a maze, locate a special bomb, and defuse it within a limited time. The game features a randomly generated map for each session, adding an element of unpredictability and replayability.

## How to Play
- **Starting the Game:** Power up the Arduino, and you'll be greeted with a welcome message, followed by the main menu.
- **Navigation:** Use the joystick to move around the grid displayed on the LED matrix.
- **Objective:** Your goal is to find and defuse the bomb before time runs out.
- **Game Display:** An LCD screen shows your name and the remaining time.
- **Scoring:** Gain points by time left until games ended.
- **Ending the Game:** Win by defusing the bomb within the time limit. The game ends if the timer runs out.

## Components
- LED Matrix 
- LCD Display 
- Buzzer
- Joystick
- Potentiometer
- Resistors and wires 

## Menu Requirements
  
- Create a menu for your game, emphasis on ‘the game. You should scroll on the LCD with the joystick. Remember you have quite a lot of flexibility here, but do not confuse that with a free ticket to slack off. The menu should include the following functionality:

- Intro Message - When powering up a game, a greeting message should be shown for a few moments.
- Should contain roughly the following categories:
    * Start game, starts the initial level of your game
    * Highscore :
      * Initially, we have 0.
      * Update it when the game is done. Highest possible score should be achieved by starting at a higher level.
      * Save the top 3+ values in EEPROM with name and score.
    * Settings:
      * Enter name. The name should be shown in highscore.
      * LCD brightness control (mandatory, must change LED wire that’s directly connected to 5v). Save it to eeprom.
      * Matrix brightness control. Save it to eeprom.
      * Sounds on or off. Save it to eeprom.
      * Extra stuff can include items specific to the game mechanics, or other settings such as chosen theme song etc. Again, save it to eeprom. You can even split the settings in 2: game
settings and system settings.
    * About: should include details about the creator(s) of the game. At least game name, author and github link or user
    * How to play: short and informative description
- While playing the game: display all relevant info:
    * Lives
    * Level
    * Score
    * Time?
    * Player name?
- Upon game ending:
    * Screen 1: a message such as ”Congratulations on reaching level/score X”. ”You did better than y people.” etc. Switches to screen 2 upon interaction (button press) or after a few moments.
    * Screen 2: display relevant game info: score, time, lives left etc. Must inform player if he/she beat the highscore. This menu should only be closed by the player, pressing a button.
 
## Game Requirements
- Minimal components: an LCD, a joystick, a buzzer and the led matrix.
- You must add basic sounds to the game (when ”eating” food, when dying, when finishing the level etc). Extra: add theme songs.
- Each level / instance should work on 16x16 matrix. You can apply the concept of visibility / fog of war (aka you only see 8x8 of the total 16x16 matrix, and you discover more as you move around) or you can use the concept of ”rooms”. Basically you will have 4 rooms that you need to go through on each level.
- It must be intuitive and fun to play.
- It must make sense in the current setup
- You should have a feeling of progression in difficulty. Depending on the dynamic of the game, this is done in the same level or with multiple levels. You can make them progress dynamically or have a number of fixed levels with an endgame. Try to introduce some randomness, though.

## Photo of the setup

## Video
