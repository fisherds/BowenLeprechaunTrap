#include <LiquidCrystal.h>

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);  // 10 is the backlight

const int pingPin = 3;  // Trigger Pin of Ultrasonic Sensor
const int echoPin = 2;  // Echo Pin of Ultrasonic Sensor
long duration, inches;
#define DIST_THRESHOLD 6  // inches away threshold

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
#define PIN_RESET_DOOR 11

// Distance trackers
#define DISTANCE_MAX 800
int16_t distance = DISTANCE_MAX;  // Door fully open (hopefully)

// What is the motor doing? (we need to know transitions)
#define MOTOR_OFF 0
#define MOTOR_CLOSING 1
#define MOTOR_OPENING 2
uint8_t oldMotorState = MOTOR_OFF;
uint8_t newMotorState = MOTOR_OFF;

// Button overrides - manual open and close overrides
#define BTN_OVERRIDE_NONE 0
#define BTN_OVERRIDE_CLOSE 1
#define BTN_OVERRIDE_OPEN 1
uint8_t override = BTN_OVERRIDE_NONE;

// Auto door commands
#define AUTO_DISABLED 0
#define AUTO_ENABLED_ARMED 1
#define AUTO_CLOSING 2
#define AUTO_OPENING 3
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
  lcd.begin(16, 2);
  lcd.setCursor(0, 0);
  lcd.print("Bowen's Trap is off");

  // pinMode(0, OUTPUT);
  // pinMode(1, OUTPUT);
  pinMode(PIN_RESET_DOOR, INPUT_PULLUP);  // 11
  pinMode(PIN_MOTOR_OPEN, OUTPUT);        // 12 or 13
  pinMode(PIN_MOTOR_CLOSE, OUTPUT);       // 12 or 13

  pinMode(pingPin, OUTPUT);  // 2 or 3
  pinMode(echoPin, INPUT);   // 2 or 3
}

void loop() {
  lcd.setCursor(0, 1);
  lcd_key = read_LCD_buttons();
  switch (lcd_key) {
    case btnRIGHT:
      lcd.print("BTN OPEN ");
      override = BTN_OVERRIDE_OPEN;
      autoState = AUTO_DISABLED;
      lcd.setCursor(0, 0);
      lcd.print("Bowen's Trap off");
      break;
    case btnLEFT:
      lcd.print("BTN CLOSE");
      override = BTN_OVERRIDE_CLOSE;
      autoState = AUTO_DISABLED;
      lcd.setCursor(0, 0);
      lcd.print("Bowen's Trap off");
      break;
    case btnUP:
      lcd.print("DISABLE  ");
      autoState = AUTO_DISABLED;
      lcd.setCursor(0, 0);
      lcd.print("Bowen's Trap off");
      break;
    case btnDOWN:
      lcd.print("ARMED   ");
      distance = DISTANCE_MAX;
      autoState = AUTO_ENABLED_ARMED;
      lcd.setCursor(0, 0);
      lcd.print("Bowen's Trap ON");
      break;
    case btnSELECT:
      lcd.print("RESET");
      autoState = AUTO_OPENING;
      lcd.setCursor(0, 0);
      lcd.print("Reseting door");
      break;
    case btnNONE:
      lcd.print("NONE  ");
      override = BTN_OVERRIDE_NONE;
      break;
  }
  delay(100);
  // Ultrasonic ping
  digitalWrite(pingPin, LOW);
  delayMicroseconds(2);
  digitalWrite(pingPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(pingPin, LOW);
  // Get the distance
  duration = pulseIn(echoPin, HIGH);
  inches = microsecondsToInches(duration);
  lcd.print(inches);
  lcd.print("in, ");

  if (override == BTN_OVERRIDE_CLOSE) {
    newMotorState = MOTOR_CLOSING;
  } else if (override == BTN_OVERRIDE_OPEN) {
    newMotorState = MOTOR_OPENING;
  } else {
    if (autoState == AUTO_ENABLED_ARMED) {
      if (inches < DIST_THRESHOLD) {
        autoState = AUTO_CLOSING;
      }
    }
  }
}

long microsecondsToInches(long microseconds) {
  return microseconds / 74 / 2;
}
