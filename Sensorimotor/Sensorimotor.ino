/**
 * ALPIBot
 */

#include <Wire.h>
#include <Adafruit_MotorShield.h>
#include "utility/Adafruit_MS_PWMServoDriver.h"
#include <Servo.h>

// Create the motor shield object with the default I2C address
Adafruit_MotorShield AFMS = Adafruit_MotorShield();
// Or, create it with a different I2C address (say for stacking)
// Adafruit_MotorShield AFMS = Adafruit_MotorShield(0x61);

// Select which 'port' M1, M2, M3 or M4. In this case, M1
Adafruit_DCMotor *rightMotor = AFMS.getMotor(1);
Adafruit_DCMotor *leftMotor = AFMS.getMotor(2);
Adafruit_DCMotor *rightReel = AFMS.getMotor(3);
Adafruit_DCMotor *leftReel = AFMS.getMotor(4);

bool debug = false;
char* codeversion="1.0";

void dump(char *msg)
{
  if (true)
  {
    Serial.println(msg);
  }
}

struct sensortype
{
  float fps;        //    +4 = 4
  long code;        //    +4 = 8
  long rightEncoder; //   +4 = 12
  long leftEncoder; //    +4 = 16
  long righReelEncoder;// +4 = 20
  long leftReelEncoder;// +4 = 24
  float voltage;     //   +4 = 28
  float current;     //   +4 = 32
  int freq;          //   +2 = 34
  int counter;       //   +2 = 36
} sensor;



int StateMachine(int state, int controlvalue)
{
  static int previousState = 0;
  switch (state)
  {
    case 1:
      // Left
      rightMotor->setSpeed(controlvalue);
      rightMotor->run(FORWARD);
      leftMotor->setSpeed(controlvalue);
      leftMotor->run(BACKWARD); 
      break;
    case 2:
      // Right
      rightMotor->setSpeed(controlvalue);
      rightMotor->run(BACKWARD);
      leftMotor->setSpeed(controlvalue);
      leftMotor->run(FORWARD);
      break;
    case 3:
      rightMotor->setSpeed(controlvalue);
      rightMotor->run(FORWARD);
      leftMotor->setSpeed(controlvalue);
      leftMotor->run(FORWARD); 
      break; 
    case 4:
      rightMotor->setSpeed(controlvalue);
      rightMotor->run(BACKWARD);
      leftMotor->setSpeed(controlvalue);
      leftMotor->run(BACKWARD); 
      break;   
    case 5:
      rightMotor->setSpeed(controlvalue);
      rightMotor->run(BACKWARD);
      leftMotor->setSpeed(controlvalue);
      leftMotor->run(FORWARD); 
      break;  
    case 6:
      rightMotor->setSpeed(controlvalue);
      rightMotor->run(FORWARD);
      leftMotor->setSpeed(controlvalue);
      leftMotor->run(BACKWARD); 
      break;  
    case 7:
      rightReel->setSpeed(controlvalue);
      rightReel->run(FORWARD);
      //setTargetPos(controlvalue-150);
      break;   
    case 8:
      // Update desired position.
      //pan.tgtPos = controlvalue;
      leftReel->setSpeed(controlvalue);
      leftReel->run(FORWARD);
      break;
    case 9:
      //scanner.tgtPos = controlvalue;  
      break;
    case 0x0a:
      rightMotor->run(RELEASE);
      leftMotor->run(RELEASE);
      break;
    default:
      // Do Nothing
      resetEncoders();
      state = 0;
      break;
  }  
  previousState = state;
  return state;
}

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 9600 bits per second:
  Serial.begin(9600);
  Serial.print("AlpiBot v");Serial.println(codeversion);
  stopburst();
    
  AFMS.begin();  // create with the default frequency 1.6KHz
  //AFMS.begin(1000);  // OR with a different frequency, say 1KHz

  // Set the speed to start, from 0 (off) to 255 (max speed)
  rightMotor->setSpeed(1);
  rightMotor->run(FORWARD);
  // turn on motor
  rightMotor->run(RELEASE);

  leftMotor->setSpeed(1);
  leftMotor->run(FORWARD);
  // turn on motor
  leftMotor->run(RELEASE);

  leftReel->setSpeed(1);
  leftReel->run(FORWARD);
  // turn on motor
  leftReel->run(RELEASE);

  rightReel->setSpeed(1);
  rightReel->run(FORWARD);
  // turn on motor
  rightReel->run(RELEASE);

  memset(&sensor,0,sizeof(sensor));

  setupMotorEncoders();
  setupReelEncoders();
}

// the loop routine runs over and over again forever:
void loop() {
  unsigned long currentMillis = millis();
  
  sensor.freq = fps();
  sensor.fps = 0.0;

  int incomingByte;

  int action, state, controlvalue;
  
  if (checksensors())
  {
    // Put here all the sensor information that you want to do only when you are transmitting the information.
    //senseCurrentAndVoltage();
  }
  //senseCurrentAndVoltage();
  burstsensors();

  incomingByte = 0;//getcommand();
  bool doaction = false;

  if (incomingByte > 0)
  {
    doaction = true;
  }
  
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
    doaction = true;
  }

  if (doaction) 
  {
    switch (incomingByte) {
      case 'I':
        dump("SSMR");
        break;  
      case 'D':
        debug = (!debug);
        break;
      case 'A':
        readcommand(action, controlvalue);
        //Serial.println("Action:");Serial.println(action);
        switch (action) {
          case 0x0b:
            // Determines the amount of frames to send in a burst.
            setBurstSize(controlvalue);
            state = 0;
            break;
          case 0x0c:
            payloadsize();
            state = 0;
            break;
          case 0x0d:
            payloadstruct();
            state = 0;
            break;
          case 0x0e:
            // Determines the updating frequency in relation to current arduino frequency (which is variable)
            // For instance, 1 means the same frequency, 2 means half the frequency: 1/freq
            setUpdateFreq(controlvalue);
            state = 0;
            break;
          case 0x0f:
            setCode(controlvalue);
            state=0;
            break;
          default:
            state = action;
            break;
        }
        break;
      case 'S':
        startburst();
        break;
      case 'X':
        stopburst();
        break;
      case 'P':
        //senseCurrentAndVoltage();
        break;
      default:
        break;
    }
  }

  loopEncoders();
  loopReelEncoders();

  StateMachine(state,controlvalue);

  //rightMotor->setSpeed(30);
  //rightMotor->run(FORWARD);
  //leftMotor->setSpeed(30);
  //leftMotor->run(FORWARD); 

//  rightReel->setSpeed(250);
//  rightReel->run(FORWARD);
//  leftReel->setSpeed(255);
//  leftReel->run(FORWARD);
}



