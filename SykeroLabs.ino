#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <RTClib.h>

/*
  SykeröLabs greenhouse automation
  Copyright (©) visuve 2022
*/

namespace Pins {
  constexpr uint8_t RPM_IN = 2;
  constexpr uint8_t PUMP_RELAY = 4;
  constexpr uint8_t FAN_RELAY = 7;
  constexpr uint8_t TEMPERATURE_IN = 8;
  constexpr uint8_t PWM_OUT = 9;
}

OneWire oneWire(Pins::TEMPERATURE_IN);
DallasTemperature dallas(&oneWire);
RTC_DS1307 clock;

uint32_t lastPumpTime = 0;
uint8_t pumpRelayState = LOW;
uint8_t fanRelayState = LOW;
volatile uint32_t revolutions = 0;

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
  clock.begin();

  while (!Serial) {
    delay(10);
  }

  // Adjust to compile time
  clock.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void fanRevolutionInterrupt() {
  ++revolutions;
}

// NOTE: this method is not 100% accurate as the
// revolutions are not read or written in an atomic way.
uint32_t measureRpm(const uint32_t interval = 1000) {
  revolutions = 0;
  // Revolutions are incremented in the interrupt above
  delay(interval);
  return revolutions * (interval * 0.03f);
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
  const uint8_t newState = pulseWidth ? HIGH : LOW;

  if (newState != fanRelayState) {
    Serial.println(newState ? "Switching fan on!" : "Switching fan off.");
    digitalWrite(Pins::FAN_RELAY, newState);
  }

  fanRelayState = newState;
}

bool isNightTime() {
  if (clock.isrunning()) {
    return false;
  }

  const DateTime now = clock.now();
  return now.hour() < 8 && now.hour() > 21;
}

void togglePumpRelay() {
  if (isNightTime() && pumpRelayState != LOW) {
    Serial.println("Turning off pumps; night time.");
    pumpRelayState = LOW;
    digitalWrite(Pins::PUMP_RELAY, pumpRelayState);
    return;
  }

  const uint32_t now = millis();
  const uint32_t delta = now - lastPumpTime;

  // Five minutes of pumping for every hour
  static constexpr uint32_t SLEEP_TIME_MS = 3600000;
  static constexpr uint32_t PUMP_TIME_MS = 300000; 

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

  // TODO: this is fine for the fan, but it's far too unprecise for the pumps
  // Use RTC alarms or some other method
  delay(120000); // 2 min

  dallas.requestTemperatures();
  
  const float temperature = dallas.getTempCByIndex(0);
  const uint8_t pulseWidth = pulseWidthFromTemperature(temperature);
  
  analogWrite(Pins::PWM_OUT, pulseWidth);

  toggleFanRelay(pulseWidth);
  togglePumpRelay();

  const uint32_t rpm = measureRpm();

  Serial.print("TMP=");
  Serial.print(temperature);
  Serial.print(" PWM=");
  Serial.print(pulseWidth);
  Serial.print(" RPM=");
  Serial.println(rpm);
}
