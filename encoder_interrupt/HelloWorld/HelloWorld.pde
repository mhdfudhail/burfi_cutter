#define CLK 2
#define DT 3
#define SW 4

int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir ="";
unsigned long lastButtonPress = 0;

int menuIndex =0;
const int menuItems = 3;



void setup() {

	pinMode(CLK,INPUT);
	pinMode(DT,INPUT);
	pinMode(SW, INPUT_PULLUP);

	Serial.begin(9600);
  attachInterrupt(digitalPinToInterrupt(CLK), encoderInterrupt, CHANGE);
	lastStateCLK = digitalRead(CLK);
}

void loop() {     
        
  Serial.println(counter);
  delay(500);
	
}

void encoderInterrupt(){
  currentStateCLK = digitalRead(CLK);
	if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){
		if (digitalRead(DT) != currentStateCLK) {
			counter ++;
			currentDir ="CCW";
		} else {
			counter --;
			currentDir ="CW";
		}
	}
	lastStateCLK = currentStateCLK;
}