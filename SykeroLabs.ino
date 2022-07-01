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

DateTime lastPumpTime;
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

  if (!clock.isrunning()) {
    clock.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
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

void adjustFanSpeed(uint8_t pulseWidth) {
  analogWrite(Pins::PWM_OUT, pulseWidth);
}

bool isNightTime(const DateTime& now) {
  if (!clock.isrunning()) {
    return false;
  }

  return now.hour() >= 21 || now.hour() <= 8;
}

void togglePumpRelay(const DateTime& now) {
  if (isNightTime(now)) {
    if (pumpRelayState != LOW) {
      Serial.println("Turning off pumps; night time.");
      pumpRelayState = LOW;
      digitalWrite(Pins::PUMP_RELAY, pumpRelayState);
      return;
    }

    Serial.println("ZzZzZz...");
    return;
  }

  const TimeSpan delta = now - lastPumpTime;

  if (pumpRelayState == LOW && delta.seconds() >= 3300)  {
      Serial.println("Turning on pumps!");
      pumpRelayState = HIGH;
      digitalWrite(Pins::PUMP_RELAY, pumpRelayState);
      lastPumpTime = now;
      return;
  }

  if (pumpRelayState == HIGH && delta.seconds() >= 60) {
    Serial.println("Turning off pumps.");
    pumpRelayState = LOW;
    digitalWrite(Pins::PUMP_RELAY, pumpRelayState);
    return;
  }
}

void loop() {
  dallas.requestTemperatures();
  
  const DateTime time = clock.now();
  const float temperature = dallas.getTempCByIndex(0);
  const uint8_t pulseWidth = pulseWidthFromTemperature(temperature);
  
  toggleFanRelay(pulseWidth);
  adjustFanSpeed(pulseWidth);
  togglePumpRelay(time);

  Serial.print(time.timestamp());
  Serial.print(';');
  Serial.print(temperature);
  Serial.print(';');
  Serial.print(pulseWidth);
  Serial.print(';');
  Serial.println(measureRpm());

  // TODO: this is fine for the fan, but it's unprecise for the pumps
  delay(60000); // 1 min
}
