/*
  Control fan speed using PWM and a DS18B20 temperature sensor.
*/


#include <OneWire.h>
#include <DallasTemperature.h>

constexpr uint8_t TEMPERATURE_PIN = 2;
constexpr uint8_t PWM_OUTPUT_PIN = 3;

OneWire oneWire(TEMPERATURE_PIN);
DallasTemperature sensors(&oneWire);

void setup() { 
  pinMode(PWM_OUTPUT_PIN, OUTPUT);

  Serial.begin(9600);
  sensors.begin();

  while (!Serial) {
    delay(10);
  }
}

uint8_t pulseWidthFromTemperature(float temperature) {
  static constexpr float TEMPERATURE_MIN = 20.0f;
  static constexpr float TEMPERATURE_MAX = 40.0f;
  static constexpr float TEMPERATURE_STEP = 
    0xFF / (TEMPERATURE_MAX - TEMPERATURE_MIN);

  if (temperature < TEMPERATURE_MIN) {
    return 0x00;
  }

  if (temperature >= TEMPERATURE_MAX) {
    return 0xFF;
  } 

  return (temperature - TEMPERATURE_MIN) * TEMPERATURE_STEP;
}

void loop() {
  delay(2000);

  sensors.requestTemperatures();
  float temperature = sensors.getTempCByIndex(0);
  uint8_t pulseWidth = pulseWidthFromTemperature(temperature);

  Serial.print("TMP=");
  Serial.print(temperature);
  Serial.print(" PWM=");
  Serial.println(pulseWidth);

  analogWrite(PWM_OUTPUT_PIN, pulseWidth);
}