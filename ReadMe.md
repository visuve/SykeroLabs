# SykeröLabs

- Automating my NFT (Nutrient Film Technique) driven greenhouse

## Libraries used:

- DallasTemperature v3.9.0
  - Should include OneWire v2.3.6 automatically
  - https://github.com/milesburton/Arduino-Temperature-Control-Library
- RTCLib v2.0.2
  - Should include BusIO v1.11.1 automatically
  - https://github.com/adafruit/RTClib
- SD v1.2.4
  - Can be found in the library manager using search word "SD card"
  - https://github.com/arduino-libraries/SD

## Parts used:
  - Elecrow Relay Shield v1.1 
    - https://www.elecrow.com/wiki/index.php?title=Relay_Shield

  - Iduino ST1046
    - https://www.openhacks.com/uploadsproductos/st1046_iduino.pdf
    
  - 2x 200mm 12v PWM-controlled fan
    - https://noctua.at/en/nf-a20-pwm

  - Maxim Intergrated (former Dallas) DS18B20 temperature sensor
    - https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf
  
  - 4.7k resistor (for DS18B20)
  - 2x 10k resistors (for fan RPM inputs)
  - 2.54mm pitch male headers (for fans)
  - 16GB SDHC card (for Iduino ST ST1046 [logs])
  - Biltema 12v 2A submersible pumps (controlled by the relay shield)
  - SolarXon ~50W solar panel 
  - Biltema 12v solar panel controller

## TODO:

- A diagram or picture of the soldered parts on the Iduino ST1046
- PH and EC measurement probes (I might run out of program storage)
  - PH adjustment and nutrient pumps
