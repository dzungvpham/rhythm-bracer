/*
 * (c) 2017 Dzung Pham - Williams College
 * Code for Rhythm Bracer - Final project for Winter Study 2018 CS11: eTextile
 * Instructor: Professor Iris Howley 
 * Todo: Too much hard-coding (but okay for small project like this). Consider an event-driven approach for more complex projects
 */
 
//Constant for sew tabs
#define REC_PIN A5
#define TAP_PIN A4
#define PLAY_PIN A3
#define LIGHT_SENSOR A2
#define RGB_RED 11
#define RGB_BLUE 10
#define RGB_GREEN 9
#define BUZZER 6
#define VIBE 5

#define TIME_INTERVAL 50 //Delay between each loop
#define DEFAULT_NOTE 1046 //Default frequency to play on buzzer

//Current playing states
#define PLAYING_NONE 0
#define PLAYING_LIGHT 1
#define PLAYING_BUZZER 2
#define PLAYING_VIBE 3
#define PLAYING_ALL 4

/*
 * Represents the constituents of the pattern.
 * Example: Pressed for 0.5 seconds, Unpressed for 0.2 seconds, Pressed for 0.5 seconds
 * will be represented as: {0, 5}, {1, 2}, {0, 5}
 * 0 is Pressed, 1 is Unpressed (due to behavior of internal pull-up resistors)
 * Each 100 miliseconds will be 1
 */
struct PatternUnit {
  int value; //0 or 1
  int count; //Number of consecutive 0 or 1
};

//Growth factor by which to multiply max length of pattern array when dynamically allocating memory
const int GROWTH_FACTOR = 2;
//Actual size of pattern array
int patternMaxLength = 32;

struct PatternUnit *pattern; //"Array" of pointer
int patternLength = 0; //Number of pattern units currently in array
int currentPatternPos = 0; //Current pattern unit
int remainingDuration = 0; //Remaining duration for current pattern unit

bool isRecSwitchOn = false; //Whether the record pin is being pressed
bool isRecording = false; //Whether the pattern is being recorded (or the first tap has been made)

int playingState = PLAYING_NONE; //Current playing state. Initially none is playing
bool isPlayButtonPressed = false; //Whether the play button is pressed

int LIGHT_MAX = 170; //Maximum room light value
int LIGHT_MIN = 5; //Minimum room light value
//Frequencies for notes (C, C#, D, D#, E, F, F#, G, G#, A, A#, B)
int noteList[] = {1047, 1109, 1175, 1245, 1319, 1397, 1480, 1568, 1661, 1760, 1865, 1976};
int noteCount = sizeof(noteList)/sizeof(int);

//Represents color with R, G, B
//Note: LilyPad RGB color scheme can be calculated by substracting the true RGB values from 255
struct Color {
  int red;
  int green;
  int blue;
} colorList[] = {
  {0, 255, 255}, {0, 255, 128}, {0, 255, 0},    //Red, Rose, Magenta
  {128, 255, 0}, {255, 255, 0}, {255, 128, 0},  //Violet, Blue, Azure
  {255, 0, 0}, {255, 0, 128}, {255, 0, 255},    //Cyan, Spring Green, Green
  {128, 0, 255}, {0, 0, 255}, {0, 128, 255},    //Chartreuse, Yellow, Orange
  {0, 0, 0},                                    //White
  {255, 255, 255}                               //No color
};

int colorCount = sizeof(colorList)/sizeof(struct Color);
const int RED = 0, GREEN = 8, ORANGE = 11, NO_COLOR = colorCount - 1;
int curColorPos = colorCount - 1; //Current color of RGB in colorList. Initially no color

void setup() {
  //Set up the pins
  pinMode(TAP_PIN, INPUT_PULLUP);
  pinMode(REC_PIN, INPUT_PULLUP);
  pinMode(PLAY_PIN, INPUT_PULLUP);
  pinMode(RGB_RED, OUTPUT);
  pinMode(RGB_GREEN, OUTPUT);
  pinMode(RGB_BLUE, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(VIBE, OUTPUT);
  pinMode(LIGHT_SENSOR, INPUT);

  //Set RGB light to none
  digitalWrite(RGB_RED, HIGH);
  digitalWrite(RGB_GREEN, HIGH);
  digitalWrite(RGB_BLUE, HIGH);

  //Allocate space for pattern array
  pattern = (PatternUnit *) malloc(patternMaxLength * sizeof(PatternUnit));
  //Seed the random generator
  randomSeed(analogRead(0));
  //Serial.begin(9600);
}

void loop() {
  //Read values from record, tap, and play button
  int recInput = digitalRead(REC_PIN);
  int tapInput = digitalRead(TAP_PIN);
  int playInput = digitalRead(PLAY_PIN);

  //Check if there is weird hardware error (tap button turning on record switch)
  bool hardwareErr = false;
  if (recInput == 0 && tapInput == 0 && !isRecSwitchOn) {
    hardwareErr = true;
  }

  //Record and tap behavior
  if (!hardwareErr) { //If there is no hardware error
    if (recInput == 0) { //If record switch is turned on
      if (!isRecSwitchOn) { //If record switch was not on before being turned on
        //Serial.println("Record switch ON");
        stopPlayBehavior(); //Stop play behavior if was playing
        playingState = PLAYING_NONE; //Nothing should be playing
        setColorRGB(RED); //Set RGB red
        isRecSwitchOn = true; //Switch is now on
        patternLength = 0; //Reset pattern length to 0. No need to free up array
        
      } else {
        if (playInput == 0) { //If play button is pressed when record switch is on
          blinkRGB(3); //Blink RGB red 3 times
        }
        
        if (!isRecording) { //Else if record switch is on but not currently recording
          if (tapInput == 0) { //If there is a tap
            //Serial.println("Recording START");
            isRecording = true; //Start recording
            addToPattern(0); //Add 0x1 as the first pattern unit
            tone(BUZZER, DEFAULT_NOTE); //Makes sound to signify tap
          }
          
        } else { //Else if record switch is on and we are recording
          addToPattern(tapInput); //Add the input value from the tap button to pattern array
  
          if (tapInput == 0) { //If tap button was pressed, make a sound
            tone(BUZZER, DEFAULT_NOTE);
          } else { //Else, no sound
            noTone(BUZZER);
          }
        }
      }
      
    } else if (isRecSwitchOn) { //Else if record switch is turned off and was on before
      if (patternLength > 0) { //If a pattern is recorded, RGB is green
        setColorRGB(GREEN);
      } else { //Else if no pattern is recorded, RGB is orange
        setColorRGB(ORANGE);
      }
      isRecSwitchOn = false; //Record switch is turned off
      isRecording = false; //Stops recording
      noTone(BUZZER); //Stops any sound from the tap button
      //printPattern();
      //Serial.println("Record switch OFF");
      
    } else if (tapInput == 0 && playingState == PLAYING_NONE) {
      blinkRGB(3); //Else if tap button is pressed while record switch is OFF and nothing is playing
    }
  }

  //Play behavior
  if (playingState != PLAYING_NONE) { //If something is being played
    if (playInput == 0) { //If play button is pressed
      if (!isPlayButtonPressed) { //If it was unpressed before being pressed now
        isPlayButtonPressed = true; //Play button is now being pressed
        stopPlayBehavior(); //Stop current playing behavior
        playingState = getNextPlayingState(); //Switch to next playing state
        //Serial.print("Playing state switched: ");
        //Serial.println(playingState);

        //If the next state is not stopping everything, reset
        if (playingState != PLAYING_NONE) {
          currentPatternPos = 0;
          remainingDuration = (*pattern).count + 1; //Increase by 1 because we are deducting immediately
          executePlayBehavior(isLightValueLow());
        }
      }
    } else {
      isPlayButtonPressed = false; //Else play button is not being pressed
    }

    remainingDuration--; //Reduce remaining duration of current pattern unit
    if (remainingDuration == 0) { //If current pattern unit is over
      currentPatternPos++; //Move to next pattern
      
      if (currentPatternPos == patternLength) { //If we have executed all pattern units, repeat
        currentPatternPos = 0;
        remainingDuration = (*pattern).count;
        executePlayBehavior(isLightValueLow());
        
      } else { //Else execute the next pattern unit
        remainingDuration = (*(pattern + currentPatternPos)).count;
        if ((*(pattern + currentPatternPos)).value == 0) {
          executePlayBehavior(isLightValueLow());
        } else {
          stopPlayBehavior();
        }
      }
    }
      
  } else if (!isRecSwitchOn && playInput == 0 && !isPlayButtonPressed) {
    if (patternLength > 0) { //If there is a pattern
      isPlayButtonPressed = true;
      playingState = getNextPlayingState();
      currentPatternPos = 0;
      remainingDuration = (*pattern).count;
      executePlayBehavior(isLightValueLow());
      //Serial.println("Playing ON");
    } else { //If there is no pattern, RGB blinks 3 times fast
      blinkRGB(3);
    }
    
  } else if (playInput != 0) {
    isPlayButtonPressed = false;
  }

  //Delay the loop by time interval to prevent overflow
  delay(TIME_INTERVAL);
}

/*
 * Add to pattern array. If input value is the same as the last pattern unit, increase count
 * Else create a new pattern unit and add to array.
 */
void addToPattern(int value) {
  struct PatternUnit unit;
  
  if (patternLength == patternMaxLength) { //Reallocate more space for pattern array if necessary
    patternMaxLength *= GROWTH_FACTOR;
    pattern = (PatternUnit *) realloc(pattern, patternMaxLength * sizeof(PatternUnit));
  }

  //If pattern array is not empty and input value is the same as the previous pattern unit, increase count
  if (patternLength > 0 && (*(pattern + patternLength - 1)).value == value) {
    (*(pattern + patternLength - 1)).count++;
    
  } else { //Else, add as a new unit
    unit.value = value;
    unit.count = 1;
    *(pattern + patternLength++) = unit;
  }
}

/*
 * Prints acquired pattern in serial monitor
 */
void printPattern() {
  int n = 0;
  struct PatternUnit unit;
  Serial.print("Pattern: ");
  while (n < patternLength) {
    unit = *(pattern + n++);
    Serial.print(unit.value);
    Serial.print("x");
    Serial.print(unit.count);
    Serial.print(" ");
  }
  Serial.println("");
}

/*
 * Return the next playing state
 */
int getNextPlayingState() {
  return (playingState + 1) % 5;
}

/*
 * Execute behavior for current playing state
 * If isRandomBehavior == true, then random behavior will be generated
 * - Note: 
 * + PLAYING_ALL: VIBE might not be played because of lack of power
 * + PLAYING_VIBE: analogWrite to VIBE will also cause BUZZER to emit sound. digitalSound doesn't.
  */
void executePlayBehavior(bool isRandomBehavior) {
  if (isRandomBehavior) {
    struct Color color;
    switch(playingState) {
      case PLAYING_LIGHT:
        setColorRGB(random(0, colorCount - 1));
        break;
        
      case PLAYING_BUZZER:
        tone(BUZZER, noteList[random(0, noteCount)]);
        break;
  
      case PLAYING_VIBE:
        //analogWrite(VIBE, random(150, 255));
        digitalWrite(VIBE, HIGH);
        break;
  
      case PLAYING_ALL:
        setColorRGB(random(0, colorCount - 1));
        tone(BUZZER, noteList[random(0, noteCount)]);
        //analogWrite(VIBE, random(150, 255));
        digitalWrite(VIBE, HIGH);
        break;
    }
    
  } else {
    switch(playingState) {
      case PLAYING_LIGHT:
        setColorRGB(RED);
        break;
        
      case PLAYING_BUZZER:
        tone(BUZZER, DEFAULT_NOTE);
        break;
  
      case PLAYING_VIBE:
        digitalWrite(VIBE, HIGH);
        break;
  
      case PLAYING_ALL:
        setColorRGB(RED);
        tone(BUZZER, DEFAULT_NOTE);
        digitalWrite(VIBE, HIGH);
        break;
    }
  }
}

/*
 * Stops playing behavior for current playing state
 */
void stopPlayBehavior() {
  switch(playingState) {
    case PLAYING_LIGHT:
      setColorRGB(NO_COLOR);
      break;
      
    case PLAYING_BUZZER:
      noTone(BUZZER);
      break;

    case PLAYING_VIBE:
      digitalWrite(VIBE, LOW);
      break;

    case PLAYING_ALL:
      setColorRGB(NO_COLOR);
      noTone(BUZZER);
      digitalWrite(VIBE, LOW);
      break;

    default:
      break;
  }
}

/*
 * Check if light value is low (less than 1/3 maximum room light)
 */
bool isLightValueLow() {
  return (analogRead(LIGHT_SENSOR) - LIGHT_MIN <= (LIGHT_MAX - LIGHT_MIN)/3);
}

/*
 * Set color of RGB
 */
void setColorRGB(int index) {
  struct Color color = colorList[index];
  curColorPos = index;
  analogWrite(RGB_RED, color.red);
  analogWrite(RGB_GREEN, color.green);
  analogWrite(RGB_BLUE, color.blue);
}

/*
 * Blink RGB n times
 */
void blinkRGB(int n) {
  int curColor;
  bool isNoColor = curColorPos == NO_COLOR;
  
  if (isNoColor) {
    curColor = RED;
  } else {
    curColor = curColorPos;
  }
  
  while (n-- > 1) { //Blink n - 1 times
    setColorRGB(NO_COLOR);
    delay(100);
    setColorRGB(curColor);
    delay(100);
  }

  if (n == 0) { //The final blink doesn't delay after RGB is turned on
    setColorRGB(NO_COLOR);
    delay(100);
    setColorRGB(curColor);

    if (isNoColor) {
      delay(100);
      setColorRGB(NO_COLOR);
    }
  }
}
