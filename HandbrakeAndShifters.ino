//Driving Force Shifter

// SHIFTER PINS
//| DB9 | Description 
//|   1 | Button Clock            
//|   2 | Button Data             
//|   3 | Button !CS & !PL (Mode) 
//|   4 | Shifter X axis          
//|   5 | SPI input               
//|   6 | GND                    
//|   7 | +5V                    
//|   8 | Shifter Y axis        
//|   9 | +5V                    

//--Libraries--//
#include <Joystick.h>
#include <HX711.h>

//--Initialization--//
int useserial = 1;  // Enable/disable readout values of various analog and/or digital inputs. 0:Off, 1:On

//-Logitech H-Shifter Setup-//
#define SHIFTER_CLOCK_PIN  8
#define SHIFTER_DATA_PIN   7
#define SHIFTER_MODE_PIN   0
int xAxisPin = A6;
int yAxisPin = A7;
int gear = 0;                // Gear nr in use

//-Handbrake Setup-//
#define HANDBRAKE_DATA_PIN 19
#define HANDBRAKE_CLK_PIN  20
HX711 handBrake(HANDBRAKE_DATA_PIN, HANDBRAKE_CLK_PIN);
float handbrake_scale_factor = 1000;
float handbrake_raw;
float handbrake_raw_max = 320;
float max_output_value = 1023;

Joystick_ Joystick;
#define SIGNAL_SETTLE_DELAY 10

// SHIFTER CODE
#define OUTPUT_BLACK_TOP       7
#define OUTPUT_BLACK_LEFT      8
#define OUTPUT_BLACK_RIGHT     9
#define OUTPUT_BLACK_BOTTOM    10
#define OUTPUT_DPAD_TOP        11
#define OUTPUT_DPAD_LEFT       12
#define OUTPUT_DPAD_RIGHT      13
#define OUTPUT_DPAD_BOTTOM     14
#define OUTPUT_RED_LEFT        15
#define OUTPUT_RED_CENTERLEFT  16
#define OUTPUT_RED_CENTERRIGHT 17
#define OUTPUT_RED_RIGHT       18

#define BUTTON_REVERSE         1


////////////////////////////////////////
// HELPERS
////////////////////////////////////////

void waitForSignalToSettle() {
  delayMicroseconds(SIGNAL_SETTLE_DELAY);
}

void getButtonStates(int *ret) {
  digitalWrite(SHIFTER_MODE_PIN, LOW);    // Switch to parallel mode: digital inputs are read into shift register
  waitForSignalToSettle();
  digitalWrite(SHIFTER_MODE_PIN, HIGH);   // Switch to serial mode: one data bit is output on each clock falling edge

  for(int i = 0; i < 16; ++i) {           // Iteration over both 8 bit registers
    digitalWrite(SHIFTER_CLOCK_PIN, LOW);         // Generate clock falling edge
    waitForSignalToSettle();

    ret[i] = digitalRead(SHIFTER_DATA_PIN);

    digitalWrite(SHIFTER_CLOCK_PIN, HIGH);        // Generate clock rising edge
    waitForSignalToSettle();
  }
}

int16_t calculateHandbrakeAxisValue(float raw_reading, float max_reading) {
  Serial.print(raw_reading);
  Serial.print("\n");
  if(raw_reading > max_reading) {
    return max_output_value;
  }
  float result = (max_output_value * (raw_reading / max_reading));
  if(result < 0.0) {
    int16_t zero = 0;
    return zero;
  }
  int16_t ret_value = (int16_t)result;
  return ret_value;
}

/////////////////////////////////
// MAIN
/////////////////////////////////
void setup() {
 delay(5000);
 if (useserial == 1) {
   Serial.begin(9600);
 }

 handBrake.tare();
 handBrake.set_scale(handbrake_scale_factor);

  Joystick.begin();
  pinMode(SHIFTER_CLOCK_PIN, OUTPUT);
  pinMode(SHIFTER_MODE_PIN, OUTPUT);
  digitalWrite(SHIFTER_MODE_PIN, HIGH);
  digitalWrite(SHIFTER_CLOCK_PIN, HIGH);

}

void loop() {
  /////////////////////////////////
  /// Driving Force H-Shifter   ///
  /////////////////////////////////

  float shiftX = analogRead(xAxisPin);      // X axis position data input
  float shiftY = analogRead(yAxisPin);      // Y axis position data input

  //Query buttons for reverse pin
  int buttonStates[16];
  getButtonStates(buttonStates);
  int reverse = buttonStates[BUTTON_REVERSE];

  // Split X and Y axis into 6 positions, then have a switch activate the 7th position. To make up a 6+R shifter
   
  for (gear = 0; gear < 30; gear++) {
    if (shiftY > 800) { // Gear upper position - 1,3,5
      if (shiftX < 350) Joystick.pressButton(0);                       // Activate gear 1
      else if (shiftX > 400 && shiftX < 600) Joystick.pressButton(2);  // Activate gear 3
      else if (shiftX > 650) Joystick.pressButton(4);  // Activate gear 5
    }
    else if (shiftY < 200) {  // Gear lower position - 2,4,6,R
      if (shiftX < 350) Joystick.pressButton(1);                                            // Activate gear 2
      else if (shiftX > 400 && shiftX < 600) Joystick.pressButton(3);                       // Activate gear 4
      else if (reverse == LOW && shiftX > 650) Joystick.pressButton(5);  // Activate gear 6
      else if (reverse == HIGH && shiftX > 650) Joystick.pressButton(6);   // Activate Reverse
    }
    else Joystick.releaseButton(gear);  // Release any button that have been pressed
    //Joystick.setButton(31, 0); // For testing that it works only!
  }

  /////////////////////////////////
  /// Handbrake                 ///
  /////////////////////////////////
  Joystick.setXAxis(calculateHandbrakeAxisValue(handBrake.get_units(), handbrake_raw_max));

  delay(5);  // delay in between reads for stability
}

