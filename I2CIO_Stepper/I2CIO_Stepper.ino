
// I2CIO_Stepper
// Version 0.1 (NOV-2020)
// by Mark D. Lougheed - Project Struix

// Sets up the Project Struix I2CIO board (or Arduino Uno/Lilypad) 
// to drive an L298N as a stepper motor contoller
// < https://hackaday.io/project/176007-i2c-stepper-contoller >

// This code is in the public domain.



// Uncomment line below to include debug code
// #define DEBUG

#include <Wire.h>

#define I2CADDRESS 0x7d    // 0x7d --> X Stepper Address
                            // 0x7e --> Y Stepper Address
                            // 0x7f --> Z Stepper Address

#define MIN_STEP_DELAY 3
#define MAX_STEP_DELAY 1000

#define MOTOR_DIR_MASK 0x01
#define MOTOR_ON_MASK 0x02
#define MOTOR_BUSY_MASK 0x04

#define PRV_ON_MASK 0x01


struct registerFile {
  uint8_t  status;       // Busy, motor on, step direction
  uint8_t  steps;        // commanded number of steps to take
  uint16_t stepDelay;    // miliseconds delay between steps (MIN_STEP_DELAY >= stepDelay <= MAX_STEP_DELAY)                             
  uint8_t  prv;          // pressure relief valve on second control port
} regFile;


uint8_t steppersEnabled;
uint8_t stepDirection;      // 0 = negative direction; 1 = positive direction
uint8_t xPhaseIndex, yPhaseIndex;
uint8_t controlInputs;


uint8_t stepperPhase[]={
  0b1010,
  0b0110, 
  0b0101,
  0b1001
};

// Define stepper motor X pulse output pins. NOTE: All stepper output pins must be on the same port.
#define STEP_X_DDR        DDRA
#define STEP_X_PORT       PORTB
//
#define STEP_X_SHIFT  0
//
#define X0_STEP_BIT   0  //
#define X1_STEP_BIT   1  //
#define X2_STEP_BIT   2  //
#define X3_STEP_BIT   3  //
#define X_STEP_MASK       ((1<<X0_STEP_BIT)|(1<<X1_STEP_BIT)|(1<<X2_STEP_BIT)|(1<<X3_STEP_BIT)) // All Stepper Control Bits


void setup() {
  // put your setup code here, to run once:
  regFile.status=0x00;
  regFile.steps=0x00;
  regFile.stepDelay=MAX_STEP_DELAY;
  regFile.prv=0x00;

  
  pinMode( 8, OUTPUT);
  pinMode( 9, OUTPUT);
  pinMode(10, OUTPUT);  
  pinMode(11, OUTPUT);

  pinMode(13, OUTPUT);  // LED - motor on/off status

  pinMode(4, OUTPUT);  // PRV on/off controll
  

#ifdef DEBUG 
  Serial.begin(19200);
#endif
  
  Wire.begin(I2CADDRESS);                // join i2c bus with address #8
  Wire.onReceive(receiveEvent); // register event
  Wire.onRequest(requestEvent); // register event

  

// Set up state variables  
  xPhaseIndex=0; 

STEP_X_PORT=0b00000000;    
}

void loop() {
  // put your main code here, to run repeatedly:

  // Control Pressure Relief Valve
  if(regFile.prv & PRV_ON_MASK)
    digitalWrite(4, HIGH);
  else
    digitalWrite(4, LOW);


  // Control Stepper Motor
    do{
#ifdef DEBUG 
      Serial.println(regFile.steps);
#endif

      if(regFile.steps)
        regFile.status |= MOTOR_BUSY_MASK;

      if(regFile.status & MOTOR_ON_MASK)
      {
#ifdef DEBUG 
        Serial.println("Motor ON");
#endif
        digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
        STEP_X_PORT = STEP_X_PORT & (~X_STEP_MASK) | (stepperPhase[xPhaseIndex%4]) << STEP_X_SHIFT;
      }
      else
      {
#ifdef DEBUG 
        Serial.println("Motor OFF");
#endif
        digitalWrite(13, LOW);   // turn the LED off by making the voltage LOW
        STEP_X_PORT=0x00;        // disable STEP_X port pins - write zeroes to turn off stepper
      }

        
      if((regFile.steps != 0) && (regFile.status & MOTOR_ON_MASK))
      {
        if(regFile.status & MOTOR_DIR_MASK) // step direction is positive
          STEP_X_PORT = STEP_X_PORT & (~X_STEP_MASK) | (stepperPhase[++xPhaseIndex%4]) << STEP_X_SHIFT;
        else
          STEP_X_PORT = STEP_X_PORT & (~X_STEP_MASK) | (stepperPhase[--xPhaseIndex%4]) << STEP_X_SHIFT;

        --regFile.steps;
      }

#ifdef DEBUG 
      Serial.print("StepDelay:");
      Serial.println(regFile.stepDelay);
#endif
      {
//      unsigned long previousMillis=0; 
//      for(unsigned long currentMillis = millis(); currentMillis - previousMillis >= regFile.stepDelay; previousMillis = currentMillis);
      delay(regFile.stepDelay);
      }
      
  
    } while(regFile.steps);

  regFile.status &= (~MOTOR_BUSY_MASK);
}


// function that executes whenever data is received from I2C master
// this function is registered as an event, see setup()
void receiveEvent(int howMany)
{
uint8_t *regBase = (uint8_t*)&regFile;  // Pointer to base of register file

  if(Wire.available() > 1){ // Test if this is something more than just a "ping"
      uint8_t reg = Wire.read(); // first byte is the register address

    while(Wire.available()){
      uint8_t value = Wire.read();    // next byte(s) are the data values to store in the register file

      if(reg < sizeof(struct registerFile)){ // The data register index must be within the register file bounds
        if(reg==0){ // Status Register - user can only set/reset motor-on or step-direction bits.
          Serial.print("Status Reg:");
          value = (regBase[0] & MOTOR_BUSY_MASK) | (value & MOTOR_ON_MASK) | (value & MOTOR_DIR_MASK);
        }        

#ifdef DEBUG 
        Serial.print(reg);         // print the character
        Serial.print("-->");
        Serial.println(value);         // print the integer
#endif

        regBase[reg]=value;   // Assign the data value in the register file
      }
#ifdef DEBUG
      else
        Serial.println("Register out of bounds");
#endif
        
      ++reg;
    }

  // Premtively set busy if there is a step count set
  // this interrupt handler may return before the next main body loop and status not reflect busy
  if(regFile.steps)
    regFile.status |= MOTOR_BUSY_MASK;
  }
#ifdef DEBUG   
  else
    Serial.println("ping!");
#endif
}




// function that executes whenever data is requested by master
// this function is registered as an event, see setup()
void requestEvent()
{
  Wire.write(regFile.status);
}



