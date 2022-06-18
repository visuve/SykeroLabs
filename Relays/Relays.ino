/*
  Toggle a relay on and off using an Elecrow Relay Shield
  
  https://www.elecrow.com/wiki/index.php?title=Relay_Shield
*/

void setup() {
  pinMode(7, OUTPUT);

  Serial.begin(9600);

  while (!Serial) {
    delay(10);
  }
}

void loop() {
  delay(10000);
  Serial.println("HIGH");
  digitalWrite(7, HIGH);
  
  delay(10000);
  Serial.println("LOW");
  digitalWrite(7, LOW);
}