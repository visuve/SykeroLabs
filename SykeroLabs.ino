#include <OneWire.h>
#include <DallasTemperature.h>

/*
  Parts used:
  - Elecrow Relay Shield v1.1
  - 12 volt PWM-controlled fan
  - Dallas DS18B20 temperature sensor
  - 4.7k resistor (for DS18B20)
  - 10k resistor (for RPM input)
  - 12v DC input
*/

namespace Pins {
  constexpr uint8_t RPM_IN = 2;
  constexpr uint8_t PWM_OUT = 3;
  constexpr uint8_t RELAY_CTRL = 7;
  constexpr uint8_t TEMPERATURE_IN = 9;
}

uint8_t relayState = LOW;
volatile uint32_t revolutions = 0;

OneWire oneWire(Pins::TEMPERATURE_IN);
DallasTemperature dallas(&oneWire);

void setup() { 
  pinMode(Pins::RPM_IN, INPUT);
  pinMode(Pins::PWM_OUT, OUTPUT);
  pinMode(Pins::RELAY_CTRL, OUTPUT);
  pinMode(Pins::TEMPERATURE_IN, INPUT);

  analogWrite(Pins::RELAY_CTRL, relayState);

  const int32_t interruptPin = digitalPinToInterrupt(Pins::RPM_IN);
  attachInterrupt(interruptPin, fanRevolutionInterrupt, RISING);

  Serial.begin(9600);
  dallas.begin();

  while (!Serial) {
    delay(10);
  }
}

void fanRevolutionInterrupt() {
  ++revolutions;
}

uint32_t measureRpm() {
  revolutions = 0;
  delay(1000);
  return revolutions * 30;
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

// NOTE: the relay shield might track the state by itself
void toggleRelay(uint8_t pulseWidth) {
  uint8_t newState = pulseWidth ? HIGH : LOW;

  if (newState != relayState) {
    digitalWrite(Pins::RELAY_CTRL, newState);
  }

  relayState = newState;
}

void loop() {
  delay(1000);

  dallas.requestTemperatures();
  float temperature = dallas.getTempCByIndex(0);
  uint8_t pulseWidth = pulseWidthFromTemperature(temperature);
  analogWrite(Pins::PWM_OUT, pulseWidth);

  toggleRelay(pulseWidth);

  uint32_t rpm = measureRpm();

  Serial.print("TMP=");
  Serial.print(temperature);
  Serial.print(" PWM=");
  Serial.print(pulseWidth);
  Serial.print(" RPM=");
  Serial.println(rpm);
}
