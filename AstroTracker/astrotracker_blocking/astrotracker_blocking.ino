/*
 * Barn door tracker
 * Based off of 
 * https://www.thingiverse.com/thing:1133193
 * 
 * 
*/

/* Calculated the following stepper delays for the motor
 *  Assumptions:
 *  R=20cm arm
 *  p=0.8mm pitch (M5 rod)
 *  big gear=43 teeth
 *  small gear=10 teeth
 *  stepper motor step=1.8 deg (200/rotation)
 *  resolution factor=16 (MS1=MS2=MS3=+3V)
 *  
 *  So for sidereal rate (Day=23h56m):
 *  big_gear_rate = 2*pi/Day/(pitch/radius) = 1.094 rotation/minute
 *  small_gear_rate = (big_teeth/small_teeth)*big_gear_rate = 4.70 rotation/minute
 *  stepper_rate=small_gear_rate * steps_per_rotation * resolution_factor = 250.8 Hz
 *  stepper_delay = 1/stepper_rate = 3986 us
 *  
 *  this can be extended for Solar day or Lunar day
 */

// all in microsecond
#define SIDEREAL_DELAY 3986
#define SOLAR_DELAY 3997
#define LUNAR_DELAY 3913

// blink for output
const int led_pin = 13;
// in msec
const long blink_interval = 500;
unsigned long previousMillis = 0;
const int step_pin=9;
const int direction_pin=8;
// two pins for the mode, since we will use a 3-position switch
const int modeA_pin=7;
const int modeB_pin=6;
const int fast_pin=5;
const int reverse_pin=4;
int delay_value=SIDEREAL_DELAY;

int ledState = LOW;

enum rate {SIDEREAL, SOLAR, LUNAR, FAST, REVERSE};
rate rate_state;
rate last_rate_state=SIDEREAL;

void setup() {
  // put your setup code here, to run once:
  pinMode(led_pin, OUTPUT);
  pinMode(step_pin, OUTPUT);
  pinMode(direction_pin, OUTPUT);

  pinMode(modeA_pin, INPUT_PULLUP);
  pinMode(modeB_pin, INPUT_PULLUP);
  pinMode(fast_pin, INPUT_PULLUP);
  pinMode(reverse_pin, INPUT_PULLUP);

}

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= blink_interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   

    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;

    // set the LED with the ledState of the variable:
    digitalWrite(led_pin, ledState);
  }
  // check the status of the switches
  // change the state if needed
  if (digitalRead(fast_pin) == LOW)
  {
    rate_state=FAST;
  }
  else if (digitalRead(reverse_pin) == LOW)
  {
    // REVERSE
    rate_state=REVERSE;
  }
  else 
  {
    if (digitalRead(modeA_pin) == LOW)
    {
      rate_state=SOLAR;
    }
    else if (digitalRead(modeB_pin) == LOW)
    {
      rate_state=LUNAR;
    }
    else
    {
      rate_state=SIDEREAL;
    }
  }
  if (rate_state != last_rate_state) {
    switch (rate_state) {
      case SIDEREAL:
        delay_value=SIDEREAL_DELAY;
        digitalWrite(direction_pin, HIGH);
        break;
      case SOLAR:
        delay_value=SOLAR_DELAY;
        digitalWrite(direction_pin, HIGH);
        break;
      case LUNAR:
        delay_value=LUNAR_DELAY;
        digitalWrite(direction_pin, HIGH);
        break;
      case FAST:
        delay_value=SIDEREAL_DELAY/10;
        digitalWrite(direction_pin, HIGH);
        break;
      case REVERSE:
        delay_value=SIDEREAL_DELAY/10;
        digitalWrite(direction_pin, LOW);
        break;
    }
    last_rate_state=rate_state;
  }
  digitalWrite(step_pin, LOW);
  digitalWrite(step_pin, HIGH);
  delayMicroseconds(delay_value);
}
