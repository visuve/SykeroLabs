#include <OneWire.h>
#include <DallasTemperature.h>

/*
  Parts used:
  - Elecrow Relay Shield v1.1
  - 200mm 12v PWM-controlled fan
  - Dallas DS18B20 temperature sensor
  - 4.7k resistor (for DS18B20)
  - 10k resistor (for RPM input)
  - 12v submersible pumps
  - 12v DC input
*/

namespace Pins {
  constexpr uint8_t RPM_IN = 2;
  constexpr uint8_t PWM_OUT = 3;
  constexpr uint8_t PUMP_RELAY = 4;
  constexpr uint8_t FAN_RELAY = 7;
  constexpr uint8_t TEMPERATURE_IN = 9;
}

uint32_t lastPumpTime = 0;
uint8_t pumpRelayState = LOW;
uint8_t fanRelayState = LOW;
volatile uint32_t revolutions = 0;

OneWire oneWire(Pins::TEMPERATURE_IN);
DallasTemperature dallas(&oneWire);

void setup() { 
  pinMode(Pins::RPM_IN, INPUT);
  pinMode(Pins::PWM_OUT, OUTPUT);
  pinMode(Pins::PUMP_RELAY, OUTPUT);
  pinMode(Pins::FAN_RELAY, OUTPUT);
  pinMode(Pins::TEMPERATURE_IN, INPUT);

  analogWrite(Pins::PUMP_RELAY, pumpRelayState);
  analogWrite(Pins::FAN_RELAY, fanRelayState);

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
void toggleFanRelay(uint8_t pulseWidth) {
  uint8_t newState = pulseWidth ? HIGH : LOW;

  if (newState != fanRelayState) {
    Serial.println(newState ? "Switching fan on!" : "Switching fan off.");
    digitalWrite(Pins::FAN_RELAY, newState);
  }

  fanRelayState = newState;
}

void togglePumpRelay() {
  uint32_t now = millis();
  uint32_t delta = now - lastPumpTime;

  // One minute of pumping for every 10min
  constexpr uint32_t SLEEP_TIME_MS = 600000;
  constexpr uint32_t PUMP_TIME_MS = 60000; 

  if (pumpRelayState == LOW && delta >= SLEEP_TIME_MS)  {
      Serial.println("Turning on pumps!");
      pumpRelayState = HIGH;
      digitalWrite(Pins::PUMP_RELAY, pumpRelayState);
      lastPumpTime = now;
      return;
  }

  if (pumpRelayState == HIGH && delta >= PUMP_TIME_MS) {
    Serial.println("Turning off pumps.");
    pumpRelayState = LOW;
    digitalWrite(Pins::PUMP_RELAY, pumpRelayState);
    return;
  }
}

void loop() {
  delay(1000);

  dallas.requestTemperatures();
  float temperature = dallas.getTempCByIndex(0);
  uint8_t pulseWidth = pulseWidthFromTemperature(temperature);
  analogWrite(Pins::PWM_OUT, pulseWidth);

  toggleFanRelay(pulseWidth);
  togglePumpRelay();

  uint32_t rpm = measureRpm();

  Serial.print("TMP=");
  Serial.print(temperature);
  Serial.print(" PWM=");
  Serial.print(pulseWidth);
  Serial.print(" RPM=");
  Serial.println(rpm);
}
