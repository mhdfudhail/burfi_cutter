#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Define the LCD screen
LiquidCrystal_I2C lcd(0x27, 20, 4);

// Define the rotary encoder pins
const int encoderPinCLK = 2;
const int encoderPinDT = 3;
const int buttonPin = 4;
const int startStopPin = 6; // Additional switch for start/stop

const int backlightPin = 5; // Backlight control pin

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
byte arrow_1[8] =
{
0b00000,
0b00000,
0b00000,
0b11111,
0b11111,
0b00000,
0b00000,
0b00000
};

byte arrow_2[8] =
{
0b10000,
0b11000,
0b11100,
0b11111,
0b11111,
0b11100,
0b11000,
0b10000
};

void setup() {
  lcd.begin(20, 4);
  lcd.createChar(0, arrow_1);
  lcd.createChar(1, arrow_2);
  lcd.backlight();

  lcd.setCursor(0, rows[0]);
  lcd.print("  Candy -- Cutter  ");
  lcd.setCursor(0, rows[2]);
  lcd.print("  Rotate Knobe to  ");
  lcd.setCursor(0, rows[3]);
  lcd.print("    select Menu    ");
  delay(1000);

  pinMode(encoderPinCLK, INPUT_PULLUP);
  pinMode(encoderPinDT, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  pinMode(startStopPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(encoderPinCLK), encoderInterrupt, CHANGE);

  lastCLKState = digitalRead(encoderPinCLK);
}

void loop() {
  if (currentMenuState == MAIN_MENU && digitalRead(buttonPin) == LOW && !buttonPressed) {
    buttonPressed = true;
    handleMenuSelection();
    delay(250); // Debounce delay
  } else if (currentMenuState == SETTINGS_MENU && digitalRead(buttonPin) == LOW) {
    // Return to the main menu when in SETTINGS_MENU and the button is pressed
    currentMenuState = MAIN_MENU;
    encoderRotated = false; // Reset the encoder rotation flag
    updateMenu();
    delay(250); // Debounce delay
  } else if (encoderRotated) {
     // Reset the encoder rotation flag
    handleEncoderRotation();
    encoderRotated = false;
  } else if (currentMenuState == START_STOP_MENU && digitalRead(startStopPin) == LOW) {
    // Handle start/stop selection switch
    startStopValue = !startStopValue;
    lcd.setCursor(7, 2);
    lcd.print("      "); // Clear previous value
    if (startStopValue) {
      lcd.print("ON ");
    } else {
      lcd.print("OFF");
    }
    delay(250); // Debounce delay
  } else if (digitalRead(buttonPin) == HIGH) {
    buttonPressed = false;
  }
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
      speedValue = normalizeValue(speedValue + dir * 100, 100, 2000);
      lcd.setCursor(8, rows[3]);
      lcd.print("       "); // Clear previous value
      lcd.setCursor(8, rows[3]);
      lcd.print(speedValue);
      lastValue = 0;
    } else if (menuIndex == 2) {
      currentMenuState = START_STOP_MENU;
      lcd.clear();
      lcd.setCursor(0, rows[2]);
      lcd.print("Start: GREEN");
      lcd.setCursor(0, rows[3]);
      lcd.print("Start: RED  ");
      // if (startStopValue) {
      //   lcd.print("ON ");
      // } else {
      //   lcd.print("OFF");
      // }
    }
  }
}

void handleMenuSelection() {
  if (currentMenuState == MAIN_MENU) {
    currentMenuState = SETTINGS_MENU;
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
      lcd.setCursor(0, rows[2]);
      lcd.print("Start: GREEN");
      lcd.setCursor(0, rows[3]);
      lcd.print("Start: RED  ");
      // if (startStopValue) {
      //   lcd.print("ON ");
      // } else {
      //   lcd.print("OFF");
      // }
    }
  }
}
