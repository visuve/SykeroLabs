#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>

/*
  SykeröLabs greenhouse automation
  Copyright (©) visuve 2022
*/

namespace Pins {
  enum : uint8_t {
    FAN1_RPM_IN = 2,
    FAN2_RPM_IN = 3,
    PUMP_RELAY = 4,
    FAN_RELAY = 7,
    TEMPERATURE_IN = 8,
    FAN_PWM_OUT = 9,  
    SD_CARD_CS_IN = 10
  };
}

OneWire TemperatureInterface(Pins::TEMPERATURE_IN);
DallasTemperature TemperatureSensors(&TemperatureInterface);
RTC_DS1307 Clock;
File LogFile;
Stream* Log = nullptr;

DateTime lastPumpTime;
uint8_t pumpRelayState = LOW;
uint8_t fanRelayState = LOW;
volatile uint32_t fan1Revolutions = 0;
volatile uint32_t fan2Revolutions = 0;
bool sdInitialized = false;
DateTime lastLogRotation;

constexpr char CSV_HEADER[] = "Time;Temperature;Pulse Width;Fan 1 RPM;Fan 2 RPM";

void setup() { 
  pinMode(Pins::FAN1_RPM_IN, INPUT);
  pinMode(Pins::FAN2_RPM_IN, INPUT);
  pinMode(Pins::PUMP_RELAY, OUTPUT);
  pinMode(Pins::FAN_RELAY, OUTPUT);

  pinMode(Pins::TEMPERATURE_IN, INPUT);
  pinMode(Pins::FAN_PWM_OUT, OUTPUT);
  pinMode(Pins::SD_CARD_CS_IN, INPUT);

  pinMode(LED_BUILTIN, OUTPUT);

  analogWrite(Pins::PUMP_RELAY, pumpRelayState);
  analogWrite(Pins::FAN_RELAY, fanRelayState);

  attachInterrupt(
    digitalPinToInterrupt(Pins::FAN1_RPM_IN), 
    fan1RevolutionInterrupt, 
    RISING);

  attachInterrupt(
    digitalPinToInterrupt(Pins::FAN2_RPM_IN),
    fan2RevolutionInterrupt,
    RISING);

  Serial.begin(9600);
  
  while (!Serial) {
    delay(10);
  }

  TemperatureSensors.begin();
  Clock.begin();

  sdInitialized = SD.begin(Pins::SD_CARD_CS_IN);

  if (!sdInitialized) {
    Serial.println("SD card not initialized!");
  }

  // NOTE: uncomment the first the a RTC module is taken into use
  // Clock.adjust(DateTime(F(__DATE__), F(__TIME__)));

  Serial.println("SykeroLabs started!");
}

void fan1RevolutionInterrupt() {
  ++fan1Revolutions;
}

void fan2RevolutionInterrupt() {
  ++fan2Revolutions;
}

// NOTE: this method is not 100% accurate as the
// revolutions are not read or written in an atomic way.
uint32_t measureRpm(volatile uint32_t& revolutions, uint32_t interval = 1000) {
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
  analogWrite(Pins::FAN_PWM_OUT, pulseWidth);
}

bool isNightTime(const DateTime& time) {
  if (!Clock.isrunning()) {
    return false;
  }

  return time.hour() >= 21 || time.hour() <= 8;
}

void togglePumpRelay(const DateTime& time) {
  if (isNightTime(time)) {
    if (pumpRelayState != LOW) {
      Serial.println("Turning off pumps; night time.");
      pumpRelayState = LOW;
      digitalWrite(Pins::PUMP_RELAY, pumpRelayState);
      return;
    }

    return;
  }

  const TimeSpan delta = time - lastPumpTime;

  if (pumpRelayState == LOW && delta.totalseconds() >= 3480)  {
      Serial.println("Turning on pumps!");
      pumpRelayState = HIGH;
      digitalWrite(Pins::PUMP_RELAY, pumpRelayState);
      lastPumpTime = time;
      return;
  }

  if (pumpRelayState == HIGH && delta.totalseconds() >= 120) {
    Serial.println("Turning off pumps.");
    pumpRelayState = LOW;
    digitalWrite(Pins::PUMP_RELAY, pumpRelayState);
    return;
  }
}

bool rotationNeeded(const DateTime& time) {
  if (!Log) {
    return true;
  }

  const TimeSpan delta = time - lastLogRotation;
  return time.hour() == 0 && delta.totalseconds() >= 3600;
}

void rotateLog(const DateTime& time) {
  if (!rotationNeeded(time)) {
    return;
  }

  // This enables to print the header only once when only serial is used
  bool printHeader = !Log;
  Log = &Serial;

  if (sdInitialized) {
    if (LogFile) {
      LogFile.close();
    }

    // The file name cannot be longer than 13 characters...
    char filename[] = "YYYYMMDD.csv";
    time.toString(filename);

    printHeader = !SD.exists(filename);

    LogFile = SD.open(filename, FILE_WRITE);

    if (LogFile) {
      Log = &LogFile;
      Serial.print("Log ");
      Serial.print(filename);
      Serial.println(" opened.");
    } else {
      Serial.print("Could not open ");
      Serial.print(filename);
      Serial.println(" for logging!");
    }
  }

  if (printHeader) {
    Log->println(CSV_HEADER);
  }

  lastLogRotation = time;
}

void loop() {
  const DateTime time = Clock.now();
  rotateLog(time);

  TemperatureSensors.requestTemperatures();
  const float temperature = TemperatureSensors.getTempCByIndex(0);
  const uint8_t pulseWidth = pulseWidthFromTemperature(temperature);
  
  toggleFanRelay(pulseWidth);
  adjustFanSpeed(pulseWidth);
  togglePumpRelay(time);

  const uint32_t rpm1 = measureRpm(fan1Revolutions);
  const uint32_t rpm2 = measureRpm(fan2Revolutions);

  Log->print(time.timestamp());
  Log->print(';');
  Log->print(temperature);
  Log->print(';');
  Log->print(pulseWidth);
  Log->print(';');
  Log->print(rpm1);
  Log->print(';');
  Log->print(rpm2);
  Log->println();
  Log->flush();

  digitalWrite(LED_BUILTIN, LOW);
  delay(60000); // TODO: this is fine for the fan, but it's unprecise for the pumps
  digitalWrite(LED_BUILTIN, HIGH);
}
