#include <AccelStepper.h>

#define dirPin 2
#define stepPin 3
#define motorInterfaceType 1

long initial_homing = -1; 
const int homeSwitch = 6;   


const int pitch = 12; //one revolution estimated around 12mm length
const int oneRevolution = 800; // for one revolution 800 pulses are needed
const int sweetNos = 3; // number of sweets to be cut


AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);


void setup() {
  //////////--Inintializations--///////////
  Serial.begin(9600);

  pinMode(6, INPUT_PULLUP);

  ////////////////////////////////////////
  delay(5000);
  home();

  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
  delay(2000);
  startToCut(25);

  

}

void loop() {
  

}
void startToCut(int length){
  int multiplier = length/pitch;
  int pulses = multiplier*oneRevolution;
  int constPulse = pulses;
  Serial.print("initial pulse: ");
  Serial.println(pulses);

  for(int i=0; i<sweetNos; i++){
    stepper.setMaxSpeed(1000);
    stepper.setAcceleration(500);

    stepper.moveTo(pulses);
    stepper.runToPosition();
    delay(500);
    Serial.print("Current Pulse: ");
    Serial.println(pulses);
    pulses =pulses+constPulse;

  
  }
  Serial.println("cutting Finished !");
  delay(2000);
  stepper.moveTo(0);
  Serial.println("Returing to Home Position....");
  stepper.runToPosition();
  Serial.println("Home Set!");


}

void home(){
  Serial.println("Returing to Home Position....");
  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);

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
