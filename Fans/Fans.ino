/*
  Control fan speed using PWM and a DS18B20 temperature sensor.
*/


#include <OneWire.h>
#include <DallasTemperature.h>

// https://docs.arduino.cc/tutorials/generic/secrets-of-arduino-pwm

constexpr uint16_t PWM_FREQUENCY_MIN = 0;
constexpr uint16_t PWM_FREQUENCY_MAX = 320; // ~25khz

constexpr float TMP_MIN_THRESHOLD = 20.0f;
constexpr float TMP_MAX_THRESHOLD = 40.0f;
constexpr float TMP_THRESHOLD_MULTIPLIER = 16.0f;

OneWire oneWire(2);
DallasTemperature sensors(&oneWire);

void setup() { 
  pinMode(9, OUTPUT);

  // COM1A(1:0) = 0b10   (Output A clear rising/set falling)
  // COM1B(1:0) = 0b00   (Output B normal operation)
  // WGM(13:10) = 0b1010 (Phase correct PWM)
  // ICNC1      = 0b0    (Input capture noise canceler disabled)
  // ICES1      = 0b0    (Input capture edge select disabled)
  // CS(12:10)  = 0b001  (Input clock select = clock/1)
  
  TCCR1A = (1 << COM1A1) | (1 << WGM11);
  ICR1 = PWM_FREQUENCY_MAX;
  OCR1A = 0;

  Serial.begin(9600);

  while (!Serial) {
    delay(10);
  }

  sensors.begin();
}

void loop() {
  delay(2000);

  sensors.requestTemperatures();
  float tmp = sensors.getTempCByIndex(0);
  uint16_t pwm;

  Serial.print("TMP=");
  Serial.print(tmp);

  if (tmp < TMP_MIN_THRESHOLD) {
    pwm = PWM_FREQUENCY_MIN;
    Serial.println(" PWM=0");
  } else if (tmp >= TMP_MAX_THRESHOLD) {
    pwm = PWM_FREQUENCY_MAX;
    Serial.println(" PWM=320");
  } else {
    pwm = (tmp - TMP_MIN_THRESHOLD) * TMP_THRESHOLD_MULTIPLIER;
    Serial.print(" PWM=");
    Serial.println(pwm);
  }

  OCR1A = pwm;
}