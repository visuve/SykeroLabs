# SykeröLabs

- Automating my NFT (Nutrient Film Technique) driven greenhouse

## Libraries used:

- DallasTemperature v3.9.0
  - Should include OneWire v2.3.6 automatically
  - https://github.com/milesburton/Arduino-Temperature-Control-Library
- RTCLib v2.0.2
  - Should include BusIO v1.11.1 automatically
  - https://github.com/adafruit/RTClib

## Parts used:
  - Elecrow Relay Shield v1.1 
    - https://www.elecrow.com/wiki/index.php?title=Relay_Shield

  - Elecrow Data Logger Shield v.1.1
    - https://www.elecrow.com/wiki/index.php?title=RTC_Data_Logger_Shield_v1.1
    - NOTE: the chip select pin interferes with the relay shield
    
  - SparkFun Qwiic Shield
    - https://www.sparkfun.com/products/14352

  - 200mm 12v PWM-controlled fan
    - https://noctua.at/en/nf-a20-pwm

  - Maxim Intergrated (former Dallas) DS18B20 temperature sensor
    - https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
  
  - 4.7k resistor (for DS18B20)
  - 10k resistor (for fan RPM input)

  - Biltema 12v 2A submersible pumps (controlled by the relay shield)
  - SolarXon ~50W solar panel 
  - Biltema 12v solar panel controller

## Diagram:

- TODO...