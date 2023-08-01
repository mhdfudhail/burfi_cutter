#include <LCD_I2C.h>

#define CLK 2
#define DT 3
#define SW 4

LCD_I2C lcd(0x27, 20, 4);
int rows[4] = {1, 0, 3, 2};

int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir ="";
unsigned long lastButtonPress = 0;
int pageNo = 1;


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
        
	pinMode(CLK,INPUT);
	pinMode(DT,INPUT);
	pinMode(SW, INPUT_PULLUP);

	// Setup Serial Monitor
	Serial.begin(9600);
  lcd.begin();
  lcd.backlight();

	lastStateCLK = digitalRead(CLK);

  lcd.setCursor(0, rows[1]);
  lcd.print("       Machine      ");
  lcd.setCursor(0, rows[2]);
  lcd.print("   Initializing...  ");
  delay(4000);
  lcd.clear();
  lcd.createChar(0, arrow_1);
  lcd.createChar(1, arrow_2);

}

void loop() {

	currentStateCLK = digitalRead(CLK);
	if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){
		if (digitalRead(DT) != currentStateCLK) {
			counter --;
			currentDir ="CCW";
		} else {
			counter ++;
			currentDir ="CW";
		}
  }

  if(pageNo == 1){
    lcd.setCursor(2, rows[0]);
    lcd.print("Candy -- Cutter");
    lcd.setCursor(2, rows[1]);
    lcd.print(" Set-Size ");
    lcd.setCursor(2, rows[2]);
    lcd.print(" Speed ");
    lcd.setCursor(2, rows[3]);
    lcd.print(" Reset ");
    lcd.setCursor(0, rows[1]);
    lcd.write(0);
    lcd.setCursor(1, rows[1]);
    lcd.write(1        );

    
  }





  lastStateCLK = currentStateCLK;
}