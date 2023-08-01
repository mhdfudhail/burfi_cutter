#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <AccelStepper.h>

// Define the LCD screen
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Define the rotary encoder pins
const int encoderPinCLK = 2;
const int encoderPinDT = 3;
const int buttonPin = 4;
const int startStopPin = 6;

//stepper
#define dirPin 38
#define stepPin 40
#define motorInterfaceType 1

long initial_homing = -1; 
const int homeSwitch = 6;   
const int endSwitch = 36;
const int emergencyStop = 18;

const int pitch = 5; //one revolution estimated around 12mm length
const int oneRevolution = 800; // for one revolution 800 pulses are needed
const int sweetNos = 24; // number of sweets to be cut

AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

//start/stop buttons
const int backlightPin = 5; 
const int startButton = 8;
const int stopButton = 10;
int startState = 0;
bool startStatus = false;
int stopState = 0;
// long mills;

// Variables to track encoder state
volatile int lastCLKState;
volatile long lastValue = 0;
volatile bool buttonPressed = false;
volatile bool encoderRotated = false; // Flag to indicate encoder rotation

// Menu variables
int menuIndex = 0;
const int menuItems = 3;
const char* menuOptions[] = {"Size", "Speed", "Start/Stop"};
int sizeValue = 20;
int speedValue = 100;
bool startStopValue = false;

// State variables
enum MenuState { MAIN_MENU, SETTINGS_MENU, START_STOP_MENU };
MenuState currentMenuState = MAIN_MENU;

int rows[4] = {1, 0, 3, 2};

// characters in display
byte arrow_1[8] = { 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b00000, 0b00000, 0b00000 };
byte arrow_2[8] = { 0b10000, 0b11000, 0b11100, 0b11111, 0b11111, 0b11100, 0b11000, 0b10000 };

void setup() {
  Serial.begin(9600);
  lcd.begin(20, 4);
  lcd.createChar(0, arrow_1);
  lcd.createChar(1, arrow_2);

  pinMode(encoderPinCLK, INPUT_PULLUP);
  pinMode(encoderPinDT, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(startStopPin, INPUT_PULLUP);
  pinMode(startButton, INPUT);
  pinMode(stopButton,INPUT);
  pinMode(homeSwitch, INPUT_PULLUP);
  pinMode(endSwitch, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(encoderPinCLK), encoderInterrupt, CHANGE);
  attachInterrupt(digitalPinToInterrupt(emergencyStop), emergencyStopRoutine, RISING);

  delay(5000);
  lcd.backlight();
  lcd.setCursor(2, rows[1]);
  lcd.print("Initializing....");
  lcd.setCursor(4, rows[2]);
  lcd.print("  Homing        ");
  home();

  lcd.clear();
  lcd.setCursor(0, rows[0]);
  lcd.print("  Candy -- Cutter  ");
  lcd.setCursor(0, rows[2]);
  lcd.print("  Rotate Knobe to  ");
  lcd.setCursor(0, rows[3]);
  lcd.print("    select Menu    ");
  delay(1000);


  lastCLKState = digitalRead(encoderPinCLK);
}

void loop() {
  if (currentMenuState == MAIN_MENU && digitalRead(buttonPin) == LOW && !buttonPressed) {
    buttonPressed = true;
    handleMenuSelection();
    delay(250); 
  } else if (currentMenuState == SETTINGS_MENU && digitalRead(buttonPin) == LOW) {
    
    currentMenuState = MAIN_MENU;
    encoderRotated = false; 
    updateMenu();
    delay(250); // 
  } else if (encoderRotated) {
    handleEncoderRotation();
    encoderRotated = false;
  } else if (currentMenuState == START_STOP_MENU && digitalRead(startButton) == HIGH && !startStatus) {

    // startStopValue = !startStopValue;
    Serial.println("start Button pressed !");
    startStatus = true;
    lcd.clear();
    lcd.setCursor(4, rows[0]);
    lcd.print("Start/Stop ");    
    lcd.setCursor(0, rows[1]);
    lcd.print("STATUS : ");
    lcd.setCursor(0, rows[2]);
    lcd.print("Start : GREEN ");
    lcd.setCursor(0, rows[3]);
    lcd.print("Stop  :  RED  ");
    if (startStatus) {
      Serial.println("Start Button Pressed !");
      lcd.setCursor(9, rows[1]);
      lcd.print("ON        ");
      startToCut(sizeValue); //argument sizeValue
    } else {
      lcd.setCursor(9, rows[1]);
      lcd.print("OFF       ");
    }
    delay(250); // Debounce delay
  } else if (digitalRead(buttonPin) == HIGH) {
    buttonPressed = false;
  }else if(currentMenuState == START_STOP_MENU && digitalRead(buttonPin) == LOW && !startStatus){
    // Return to the main menu when in SETTINGS_MENU and the button is pressed
    currentMenuState = MAIN_MENU;
    encoderRotated = false; // Reset the encoder rotation flag
    updateMenu();
    delay(250); // Debounce delay 
  }

  while(currentMenuState == START_STOP_MENU && digitalRead(stopButton)==HIGH && startStatus){
    handleSettingsMenu();
    Serial.println("Stop Button Pressed !");

  }
  // Serial.println(String(currentMenuState));
  // startState = digitalRead(startButton);
  // stopState = digitalRead(stopB)
  // if(startStatus){
  //   lcd.setCursor(14, rows[0]);
  //   lcd.print(millis()/1000);
  // }
}

void encoderInterrupt() {
  int CLKState = digitalRead(encoderPinCLK);
  int DTState = digitalRead(encoderPinDT);

  if (CLKState != lastCLKState) {
    if (DTState != CLKState) {
      lastValue++;
    } else {
      lastValue--;
    }
    lastCLKState = CLKState;
    encoderRotated = true; // Set the encoder rotation flag
  }
}

void handleEncoderRotation() {
  int encoded = lastValue;
  int dir = encoded / abs(encoded); // Extract direction from the value

  if (currentMenuState == MAIN_MENU) {
    menuIndex = (menuIndex + dir + menuItems) % menuItems;
    updateMenu();
  } else if (currentMenuState == SETTINGS_MENU) {
    if (menuIndex == 0) {
      sizeValue = normalizeValue(sizeValue + dir, 20, 30);
      lcd.setCursor(8, rows[3]);
      lcd.print("     "); // Clear previous value
      lcd.setCursor(8, rows[3]);
      lcd.print(sizeValue);
      lcd.print(" mm");
      lastValue = 0;
    } else if (menuIndex == 1) {
      speedValue = normalizeValue(speedValue + dir * 500, 1000, 3500);
      lcd.setCursor(8, rows[3]);
      lcd.print("       "); // Clear previous value
      lcd.setCursor(8, rows[3]);
      lcd.print(speedValue);
      lastValue = 0;
    } else if (menuIndex == 2) {
      currentMenuState = START_STOP_MENU;
      startState = digitalRead(startButton);
      lcd.clear();
      lcd.setCursor(0, rows[1]);
      lcd.print("STATUS : ");
      lcd.setCursor(0, rows[2]);
      lcd.print("Start :  GREEN");
      lcd.setCursor(0, rows[3]);
      lcd.print("Stop  :  RED  ");
      if (startStatus) {
        Serial.println("Start Button Pressed !");
        lcd.setCursor(9, rows[1]);
        lcd.print("ON        ");
      } else {
        lcd.setCursor(9, rows[1]);
        lcd.print("OFF       ");
      }
    }
  }
}

void handleMenuSelection() {
  if (currentMenuState == MAIN_MENU) {
    currentMenuState = SETTINGS_MENU;
    updateMenu();
  }
}
void handleSettingsMenu() {
  if (currentMenuState == START_STOP_MENU) {
    currentMenuState = SETTINGS_MENU;
    startStatus = !startStatus;
    updateMenu();
  }
}
int normalizeValue(long val, int minVal, int maxVal) {
  if (val < minVal) {
    return minVal;
  } else if (val > maxVal) {
    return maxVal;
  } else {
    return val;
  }
}

void updateMenu() {
  lcd.clear();
  if (currentMenuState == MAIN_MENU) {
    lcd.setCursor(4, rows[0]);
    lcd.print("Choose Menu:");
    for (int i = 0; i < menuItems; i++) {
      lcd.setCursor(0, rows[(i + 1)]);
      if (i == menuIndex) {
        lcd.setCursor(0, rows[(i + 1)]);
        lcd.write(0);
        lcd.setCursor(1, rows[(i + 1)]);
        lcd.write(1);
      } else {
        lcd.print("  ");
      }
      lcd.setCursor(2, rows[(i + 1)]);
      lcd.print(menuOptions[i]);
    }
  } else if (currentMenuState == SETTINGS_MENU) {
    lcd.setCursor(4, rows[0]);
    lcd.print(menuOptions[menuIndex]);
    if (menuIndex == 0) {
      lcd.setCursor(3, rows[2]);
      lcd.print("Size: ");
      lcd.print(sizeValue);
      lcd.print(" mm");
    } else if (menuIndex == 1) {
      lcd.setCursor(3, rows[2]);
      lcd.print("Speed: ");
      lcd.print(speedValue);
    } else if (menuIndex == 2) {
      currentMenuState = START_STOP_MENU;
      // startState = digitalRead(startButton);
      lcd.setCursor(0, rows[1]);
      lcd.print("STATUS : ");
      lcd.setCursor(0, rows[2]);
      lcd.print("Start :  GREEN");
      lcd.setCursor(0, rows[3]);
      lcd.print("Stop  :  RED  ");
      if (startStatus) {
        Serial.println("Start button Pressed !");     
        lcd.setCursor(9, rows[1]);
        lcd.print("ON        ");
      } else {
        lcd.setCursor(9, rows[1]);
        lcd.print("OFF       ");
      }
    }
  }
}
void startToCut(int length){
  float multiplier = length/pitch;
  long pulses = multiplier*oneRevolution;
  int constPulse = pulses;
  Serial.print("initial pulse: ");
  Serial.println(pulses);

  for(int i=0; i<sweetNos; i++){
    stepper.setMaxSpeed(speedValue);
    stepper.setAcceleration(2000);

    stepper.moveTo(pulses);
    stepper.runToPosition();
    delay(500);
    Serial.print("Current Pulse: ");
    Serial.println(pulses);
    Serial.print("Current Cutting: ");
    Serial.println(i);
    pulses =pulses+constPulse;
    if(!startStatus || digitalRead(stopButton)==HIGH){
      Serial.println("STOPPED");
      break;
    }

  }
  Serial.println("cutting Finished !");
  lcd.setCursor(9, rows[1]);
  lcd.print("Homing...");
  delay(2000);
  stepper.moveTo(0);// move to 2000
  Serial.println("Returing to Home Position....");
  stepper.runToPosition();
  //home();
  handleSettingsMenu();
  Serial.println("Home Set!");


}

void home(){
  Serial.println("Returing to Home Position....");
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(1000);

  while(digitalRead(homeSwitch)){
    stepper.moveTo(initial_homing);
    initial_homing--;
    stepper.run();
    delay(5);
  }
  stepper.setCurrentPosition(0);
  stepper.setMaxSpeed(100.0);
  stepper.setAcceleration(100.0);
  initial_homing = 1;
  while(!digitalRead(homeSwitch)){
    stepper.moveTo(initial_homing);
    stepper.run();
    initial_homing++;
    delay(5);
  }
  stepper.setCurrentPosition(0);
  Serial.println("Home Set!");
}
void emergencyStopRoutine(){
  stepper.disableOutputs();
  startStatus = false;
  encoderRotated = false;
  // handleMenuSelection();
  Serial.println("EMERGENCY BUTTON PRESSED!!");
  // Serial.println(i);
  // lcd.clear();
  // lcd.setCursor(0, rows[1]);
  // lcd.print(" EMERGENCY ");
  // lcd.setCursor(1, rows[2]);
  // lcd.print(" STOPPED ");

}
