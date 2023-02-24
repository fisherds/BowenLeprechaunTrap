#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);  // 10 is the backlight

const int pingPin = 3;  // Trigger Pin of Ultrasonic Sensor
const int echoPin = 2;  // Echo Pin of Ultrasonic Sensor
long duration, inches;
#define DIST_THRESHOLD 6  // inches away threshold (less than this is a Leprechaun)

// define some values used by the panel and buttons
int lcd_key = 0;
int adc_key_in = 0;
#define btnRIGHT 0
#define btnUP 1
#define btnDOWN 2
#define btnLEFT 3
#define btnSELECT 4
#define btnNONE 5

// Motor pins
#define PIN_MOTOR_OPEN 12
#define PIN_MOTOR_CLOSE 13

// Reset button
#define PIN_DOOR_SENSOR 11

// Distance trackers
#define DISTANCE_MAX 55           // This is the limit when the door hits the box.
int16_t distance = DISTANCE_MAX;  // Default assumption that the door is in the worst spot.

// What is the motor doing? (we need to know transitions to avoid shoot through current)
#define MOTOR_OFF 0
#define MOTOR_CLOSING 1
#define MOTOR_OPENING 2
uint8_t oldMotorState = MOTOR_OFF;
uint8_t newMotorState = MOTOR_OFF;

// Button overrides - manual open, close, and stop overrides
// These all disable the auto mode.
#define BTN_OVERRIDE_NONE 0
#define BTN_OVERRIDE_CLOSE 1
#define BTN_OVERRIDE_OPEN 2
#define BTN_OVERRIDE_STOP 3
uint8_t override = BTN_OVERRIDE_NONE;

// Auto door commands
#define AUTO_DISABLED 0       // Disabled at the moment, probably due to an override
#define AUTO_ENABLED_ARMED 1  // Not moving, just waiting for a leprechaun
#define AUTO_CLOSING 2        // Get that Leprechaun!
#define AUTO_OPENING 3        // Looking for the switch
uint8_t autoState = AUTO_DISABLED;

int read_LCD_buttons() {
  adc_key_in = analogRead(0);
  if (adc_key_in > 1500) return btnNONE;
  if (adc_key_in < 50) return btnRIGHT;
  if (adc_key_in < 195) return btnUP;
  if (adc_key_in < 380) return btnDOWN;
  if (adc_key_in < 500) return btnLEFT;
  if (adc_key_in < 700) return btnSELECT;
  return btnNONE;
}

void setup() {
  Serial.begin(9600);
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Bowen's Trap");

  // pinMode(0, OUTPUT);
  // pinMode(1, OUTPUT);
  pinMode(PIN_DOOR_SENSOR, INPUT_PULLUP);  // 11 - door is fully open
  pinMode(PIN_MOTOR_OPEN, OUTPUT);         // 12
  pinMode(PIN_MOTOR_CLOSE, OUTPUT);        // 13

  pinMode(pingPin, OUTPUT);  // 3
  pinMode(echoPin, INPUT);   // 2
  setMotorOff();
}

void loop() {
  checkForButtonOverrides();
  getUltrasonicDistance();
  delay(100);

  // Use the FSM to decide what to do with the motor.
  if (override == BTN_OVERRIDE_CLOSE) {
    newMotorState = MOTOR_CLOSING;
  } else if (override == BTN_OVERRIDE_OPEN) {
    newMotorState = MOTOR_OPENING;
  } else if (override == BTN_OVERRIDE_STOP) {
    newMotorState = MOTOR_OFF;
  } else {
    // No overrides are present.  Now do the normal stuff.
    if (autoState == AUTO_DISABLED) {
      newMotorState = MOTOR_OFF;  // Do nothing right now.  Probably the door is shut.
    } else if (autoState == AUTO_ENABLED_ARMED) {
      newMotorState = MOTOR_OFF;
      if (inches < DIST_THRESHOLD) {  // Look for Leprechauns!
        autoState = AUTO_CLOSING;
        newMotorState = MOTOR_CLOSING;
      }
    } else if (autoState == AUTO_CLOSING) {
      newMotorState = MOTOR_CLOSING;
      if (distance >= DISTANCE_MAX) {  // Go until the set distance.
        autoState = AUTO_DISABLED;     // Caught him!
        newMotorState = MOTOR_OFF;
        // TODO: Play a song or something.
        lcd.setCursor(0, 1);
        lcd.print("Caught!");
      }
    } else if (autoState == AUTO_OPENING) {
      newMotorState = MOTOR_OPENING;
      if (digitalRead(PIN_DOOR_SENSOR) == LOW) {  // Go until the switch is hit.
        smallMoveForward();
        distance = 0;
        newMotorState = MOTOR_OFF;
        autoState = AUTO_ENABLED_ARMED;
      }
    }
  }

  // Update the motors
  if (oldMotorState != newMotorState) {
    if (newMotorState == MOTOR_CLOSING) {
      setMotorToClose();
    } else if (newMotorState == MOTOR_OPENING) {
      setMotorToOpen();
    } else if (newMotorState == MOTOR_OFF) {
      setMotorOff();
    }
    oldMotorState = newMotorState;
  } else {
    // They are the same.  The door is maybe moving.
    if (newMotorState == MOTOR_CLOSING) {
      distance += 1;
      lcd.setCursor(0, 1);
      lcd.print("D=");
      lcd.print(distance);
      lcd.print("   ");
    } else if (newMotorState == MOTOR_OPENING) {
      distance -= 1;
    }
  }
}

void checkForButtonOverrides() {
  lcd.setCursor(0, 1);  // Write on line 2 (called 1 since 0 based)
  lcd_key = read_LCD_buttons();
  switch (lcd_key) {
    case btnRIGHT:
      lcd.print("OPEN   ");
      override = BTN_OVERRIDE_OPEN;
      autoState = AUTO_DISABLED;
      lcd.setCursor(0, 0);
      lcd.print("Bowen's Trap Off");
      break;
    case btnLEFT:
      lcd.print("CLOSE  ");
      override = BTN_OVERRIDE_CLOSE;
      autoState = AUTO_DISABLED;
      lcd.setCursor(0, 0);
      lcd.print("Bowen's Trap Off");
      break;
    case btnUP:
      lcd.print("STOP   ");
      override = BTN_OVERRIDE_STOP;
      autoState = AUTO_DISABLED;
      lcd.setCursor(0, 0);
      lcd.print("Bowen's Trap Off");
      break;
    case btnDOWN:
      lcd.print("ARMED  ");
      override = BTN_OVERRIDE_NONE;
      autoState = AUTO_OPENING;
      lcd.setCursor(0, 0);
      lcd.print("Bowen's Trap On ");
      break;
    case btnSELECT:
      lcd.print("TRIGGER");
      autoState = AUTO_CLOSING;
      newMotorState = MOTOR_CLOSING;
      break;
    case btnNONE:
      //lcd.print("        ");
      override = BTN_OVERRIDE_NONE;
      break;
  }
}

void getUltrasonicDistance() {
  // Ultrasonic ping
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);
  // Get the distance
  duration = pulseIn(echoPin, HIGH);
  inches = microsecondsToInches(duration);
  lcd.setCursor(8, 1);
  lcd.print(inches);
  lcd.print("in");
}

void smallMoveForward() {
  setMotorToClose();
  delay(400);
  setMotorOff();
}

void setMotorToClose() {
  digitalWrite(PIN_MOTOR_CLOSE, HIGH);
  digitalWrite(PIN_MOTOR_OPEN, HIGH);
  delay(50);
  digitalWrite(PIN_MOTOR_CLOSE, LOW);
  digitalWrite(PIN_MOTOR_OPEN, HIGH);
}

void setMotorToOpen() {
  digitalWrite(PIN_MOTOR_CLOSE, HIGH);
  digitalWrite(PIN_MOTOR_OPEN, HIGH);
  delay(50);
  digitalWrite(PIN_MOTOR_CLOSE, HIGH);
  digitalWrite(PIN_MOTOR_OPEN, LOW);
}

void setMotorOff() {
  digitalWrite(PIN_MOTOR_CLOSE, HIGH);
  digitalWrite(PIN_MOTOR_OPEN, HIGH);
}

long microsecondsToInches(long microseconds) {
  return microseconds / 74 / 2;
}
