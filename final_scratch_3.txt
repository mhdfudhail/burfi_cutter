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
long end_homing = -1 ;
const int homeSwitch = 6;   
const int endSwitch = 7;
const int pnumaticAknowledge = 9;
const int emergencyStop = 18;

const int pitch = 5; //one revolution estimated around 5mm length
const int oneRevolution = 800; // for one revolution 800 pulses are needed
const int sweetNos = 24; // number of sweets to be cut
int currentSweet = 0;

AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

//start/stop buttons
const int backlightPin = 5; 
const int startButton = 34;
const int stopButton = 32;
const int pnumaticValve = 33;
int startState = 0;
bool startStatus = false;
int stopState = 0;
long lastPulse;
bool secondCut=false;
bool pnumaticFlag=false; 
// long mills;

// Variables to track encoder state
volatile int lastCLKState;
volatile long lastValue = 0;
volatile bool buttonPressed = false;
volatile bool encoderRotated = false; // Flag to indicate encoder rotation

// Menu variables
int menuIndex = 0;
const int menuItems = 3;
const char* menuOptions[] = {"Size_1", "Size_2", "Start/Stop"};
int sizeValue_1 = 20;
int sizeValue_2 = 20;
int speedValue = 3000;
bool startStopValue = false;

// State variables
enum MenuState { MAIN_MENU, SETTINGS_MENU, START_STOP_MENU };
MenuState currentMenuState = MAIN_MENU;

int rows[4] = {1, 0, 3, 2};

// characters in display
byte arrow_1[8] = { 0b00000, 0b00000, 0b00000, 0b11111, 0b11111, 0b00000, 0b00000, 0b00000 };
byte arrow_2[8] = { 0b10000, 0b11000, 0b11100, 0b11111, 0b11111, 0b11100, 0b11000, 0b10000 };
byte pause[8] = {B00000, B11011, B11011, B11011, B11011, B11011, B11011, B00000};
byte warning[8] = {B01110,  B11011,  B11011,  B11011,  B11011,  B11111, B11011, B01110};

void setup() {
  Serial.begin(9600);
  lcd.begin(20, 4);
  lcd.createChar(0, arrow_1);
  lcd.createChar(1, arrow_2);
  lcd.createChar(2, pause);
  lcd.createChar(3, warning);

  pinMode(encoderPinCLK, INPUT_PULLUP);
  pinMode(encoderPinDT, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(startStopPin, INPUT_PULLUP);
  pinMode(startButton, INPUT);
  pinMode(stopButton,INPUT);
  pinMode(homeSwitch, INPUT_PULLUP);
  pinMode(endSwitch, INPUT_PULLUP);
  pinMode(pnumaticValve, OUTPUT);
  digitalWrite(pnumaticValve, HIGH);
  pinMode(pnumaticAknowledge,INPUT);

  attachInterrupt(digitalPinToInterrupt(encoderPinCLK), encoderInterrupt, CHANGE);
  /////////////////Need to be configured///////////////////
  attachInterrupt(digitalPinToInterrupt(emergencyStop), emergencyStopRoutine, RISING);
  
  

  digitalWrite(pnumaticValve, HIGH);
  delay(1000);
  lcd.backlight();
  lcd.setCursor(2, rows[1]);
  lcd.print("Initializing....");
  lcd.setCursor(4, rows[2]);
  lcd.print("  Homing        ");
  initialHome();
  delay(100);
  home();
  // digitalWrite(pnumaticValve, HIGH);
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
  } else if (currentMenuState == START_STOP_MENU && digitalRead(startButton) == HIGH && !startStatus && !pnumaticFlag) {

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
    if (startStatus && !secondCut) {
      Serial.println("first cut Started!");
      lcd.setCursor(9, rows[1]);
      lcd.print("ON        ");
      cutFromStart(sizeValue_1); //argument sizeValue
    } else if(startStatus && secondCut) {
      Serial.println("second cut Started!");
      lcd.setCursor(9, rows[1]);
      lcd.print("ON       ");
      cutFromEnd(sizeValue_2);
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
    // pnumaticFlag
// 
  }
  if (digitalRead(pnumaticAknowledge)==HIGH){
    pnumaticFlag=false;
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
      sizeValue_1 = normalizeValue(sizeValue_1 + dir, 20, 30);
      lcd.setCursor(8, rows[3]);
      lcd.print("     "); // Clear previous value
      lcd.setCursor(8, rows[3]);
      lcd.print(sizeValue_1);
      lcd.print(" mm");
      lastValue = 0;
    } else if (menuIndex == 1) {
      sizeValue_2 = normalizeValue(sizeValue_2 + dir, 20, 30);
      lcd.setCursor(8, rows[3]);
      lcd.print("     "); // Clear previous value
      lcd.setCursor(8, rows[3]);
      lcd.print(sizeValue_2);
      lcd.print(" mm");
      // speedValue = normalizeValue(speedValue + dir * 500, 1000, 3500);
      // lcd.setCursor(8, rows[3]);
      // lcd.print("       "); // Clear previous value
      // lcd.setCursor(8, rows[3]);
      // lcd.print(speedValue);
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
        lcd.setCursor(19, rows[0]);
        lcd.print(" ");
      } else {
        lcd.setCursor(9, rows[1]);
        lcd.print("OFF       ");
        lcd.setCursor(19, rows[0]);
        lcd.write(2);
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
      lcd.print("Size_1: ");
      lcd.print(sizeValue_1);
      lcd.print(" mm");
    } else if (menuIndex == 1) {
      lcd.setCursor(3, rows[2]);
      lcd.print("Size_2: ");
      lcd.print(sizeValue_2);
      lcd.print(" mm");
      // lcd.setCursor(3, rows[2]);
      // lcd.print("Speed: ");
      // lcd.print(speedValue);
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
        lcd.setCursor(19, rows[0]);
        lcd.print(" ");
      } else {
        lcd.setCursor(9, rows[1]);
        lcd.print("OFF       ");
        lcd.setCursor(19, rows[0]);
        lcd.write(2);
      }
    }
  }
}
void cutFromStart(int length){
  float multiplier;
  long pulses;
  int constPulse;
  int sweetNos = (600/length);
  // int sweetNos=4;


  if (currentSweet==0){
    multiplier = length/pitch;
    pulses = multiplier*oneRevolution;
    constPulse = pulses;
    
  }else{
    pulses = lastPulse;
    currentSweet++;
  }

  Serial.print("initial pulse: ");
  Serial.println(pulses);

  for(currentSweet; currentSweet<sweetNos; currentSweet++){
    stepper.setMaxSpeed(speedValue);
    stepper.setAcceleration(3000);

    stepper.moveTo(pulses);
    stepper.runToPosition();
    
    delay(50);
    digitalWrite(pnumaticValve, LOW);
    delay(1000);
    digitalWrite(pnumaticValve, HIGH);
    delay(1000);

    // Do the aknowledge //
    if (digitalRead(pnumaticAknowledge)==LOW){
      pnumaticFlag=true;
      pulses =pulses+constPulse;
      lastPulse = pulses;
      startStatus=false;
      lcd.setCursor(18, rows[0]);
      lcd.write(3);
      Serial.println("Pnumatic Aknowledge:Triggered!!");
      break;
    }else{
      lcd.setCursor(18, rows[0]);
      lcd.print(" ");
      startStatus=true;
      Serial.println("Pnumatic Aknowledge:OK!!");

    }

    Serial.print("Current Pulse: ");
    Serial.println(pulses);
    Serial.print("Current Cutting: ");
    Serial.println(currentSweet);
    // Serial.print("Current Position of Stepper:");
    // Serial.println(stepper.Position());
    pulses =pulses+constPulse;
    lastPulse = pulses;
    if(!startStatus || digitalRead(stopButton)==HIGH){
      Serial.println("STOPPED");
      paused();
      break;
    }

  }
  if(currentSweet==sweetNos){
    Serial.println("first cutting Finished !");
    lcd.setCursor(9, rows[1]);
    lcd.print("Homing...");
    endHoming();
    startStatus=false;
    secondCut=true;
    currentSweet=0;

  }


  // stepper.moveTo(0);
  // delay(2000);
  
  // Serial.println("Returing to Home Position....");
  // stepper.runToPosition();
  //home();
  // handleSettingsMenu();
  // Serial.println("Home Set!");


}
void cutFromEnd(int length){
  float multiplier;
  long pulses;
  long constPulse;
  int sweetNos = (600/length);
  // int sweetNos=5;
  Serial.print("current sweet:");
  Serial.println(currentSweet);
  Serial.println(stepper.currentPosition());

  if (currentSweet==0){
    multiplier = length/pitch;
    pulses = multiplier*oneRevolution;
    constPulse = pulses;
    
  }else{
    pulses = lastPulse;
    currentSweet++;
  }
  Serial.print("current sweet:");
  Serial.println(currentSweet);

  for(currentSweet; currentSweet<sweetNos; currentSweet++){
    stepper.setMaxSpeed(speedValue);
    stepper.setAcceleration(3000);
    long setPulse=-(pulses);
    stepper.moveTo(setPulse);
    stepper.runToPosition();

    delay(50);
    digitalWrite(pnumaticValve, LOW);
    delay(1000);
    digitalWrite(pnumaticValve, HIGH);
    delay(1000);
   // Do the aknowledge //
    if (digitalRead(pnumaticAknowledge)==LOW){
      pnumaticFlag=true;
      pulses =pulses+constPulse;
      lastPulse = pulses;
      startStatus=false;
      Serial.println("Pnumatic Aknowledge:Triggered!!");
      lcd.setCursor(18, rows[0]);
      lcd.write(3);
      break;
    }else{
      lcd.setCursor(18, rows[0]);
      lcd.print(" ");
      startStatus=true;
      Serial.println("Pnumatic Aknowledge:OK!!");

    }

    Serial.print("Current Pulse: ");
    Serial.println(setPulse);
    Serial.print("Current Cutting: ");
    Serial.println(currentSweet);

    pulses =pulses+constPulse;
    lastPulse=pulses;
    if(!startStatus || digitalRead(stopButton)==HIGH){
      Serial.println("STOPPED");
      paused();
      break;
    }

  }
  if (currentSweet==sweetNos){
    Serial.println("cutting Finished !");
  lcd.setCursor(9, rows[1]);
  lcd.print("Homing...");
  delay(200);
  startStatus=false;
  secondCut=false;
  currentSweet=0;
  home();
  Serial.println("Returing to Home Position....");

  Serial.println("Home Set!");

  }
}
void initialHome(){
  stepper.setCurrentPosition(0);
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(1000);
  long step=1600;

  while(!digitalRead(homeSwitch)){
    stepper.moveTo(-step);
    stepper.runToPosition();
    step+=800;
    // initial_homing--;
    // Serial.print("while NO:2 ");
    // Serial.println(initial_homing);
    delay(50);
  }

}

void home(){
  Serial.println("Returing to Home Position....");
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(1000);


  stepper.setCurrentPosition(0);

  // stepper.setMaxSpeed(100.0);
  // stepper.setAcceleration(100.0);
  initial_homing = -1;
  while(!digitalRead(homeSwitch)){
    stepper.moveTo(initial_homing);
    stepper.run();
    initial_homing--;
    Serial.print("while NO:2 ");
    Serial.println(initial_homing);
    delay(1);
  }
  initial_homing = 1;
  // stepper.setMaxSpeed(100.0);
  // stepper.setAcceleration(100.0);
  while(digitalRead(homeSwitch)){ //while No:1
    stepper.moveTo(initial_homing);
    initial_homing++;
    stepper.run();
    Serial.print("while NO:1 ");
    Serial.println(initial_homing);
    delay(5);
  }

  stepper.setCurrentPosition(0);
    lcd.setCursor(9, rows[1]);
  lcd.print("OFF      ");
  Serial.println("Home Set!");
}


void endHoming(){
  Serial.println("Returing to End Position....");
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(1000);
  Serial.println(digitalRead(endSwitch));

  // while(digitalRead(endSwitch)){ //while NO :1
  //   stepper.moveTo(end_homing);
  //   end_homing--;
  //   stepper.run();
  //   Serial.print("while NO:1");
  //   Serial.println(end_homing);
  //   delay(5);
  // }
  stepper.setCurrentPosition(0);
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(1000);
  end_homing = 1;
  while(!digitalRead(endSwitch)){  //while No : 2
    stepper.moveTo(end_homing);
    end_homing++;
    stepper.run();
    Serial.print("while NO:2");
    Serial.println(end_homing);
    delay(5);
  }
  end_homing = -1;
  while(digitalRead(endSwitch)){ //while NO :1
    stepper.moveTo(end_homing);
    end_homing--;
    stepper.run();
    Serial.print("while NO:1");
    Serial.println(end_homing);
    delay(5);
  }
  stepper.setCurrentPosition(0);
  lcd.setCursor(9, rows[1]);
  lcd.print("OFF      ");
  lastPulse = stepper.currentPosition();

  Serial.println("Home Set!");
  // end_homing = -1;
}

void paused(){
  lcd.setCursor(9, rows[1]);
  lcd.print("PAUSED ! ");
  lcd.setCursor(19, rows[0]);
  lcd.write(2);
  
  // lcd.setCursor(9, rows[1]);
  // lcd.print("Homing...");

}

void emergencyStopRoutine(){
  // stepper.disableOutputs();
  startStatus = false;
  encoderRotated = false;
  // handleMenuSelection();
  // Serial.println("EMERGENCY BUTTON PRESSED!!");
  // Serial.println(i);
  // lcd.clear();
  // lcd.setCursor(0, rows[1]);
  // lcd.print(" EMERGENCY ");
  // lcd.setCursor(1, rows[2]);
  // lcd.print(" STOPPED ");

}
