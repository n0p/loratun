#define BLINK_LED 13
#define BLINK_INTERVAL 500
boolean blink_state=false;

// volcar dato hexadecimal por puerto USB
void SerialPrintHex(unsigned char c) {
  if (c<=0x0F) Serial.print(0);
  Serial.print(c, HEX);
}

// setup
void setup() {
  
  // USB serial speed
  Serial.begin(9600);

  // pin de parpadeo
  pinMode(BLINK_LED, OUTPUT);

        Serial.print("ELM327 v1.5b startup\r");

}

// main loop
void loop() {
  
  // millis directo
  extern volatile unsigned long timer0_millis;

  // parpadear para indicar funcionamiento OK (OJO: Si no es ATmega2560, comentar, interfiere con SPI)
  static unsigned long millis_blink;
  if (millis()-millis_blink > BLINK_INTERVAL) {
    millis_blink=millis();
    //blink_state=!blink_state;
    blink_state^=true;
    digitalWrite(BLINK_LED, (blink_state?LOW:HIGH));
  }

/*
  while (true) {
    Serial.print("\r\r\r\rOMGWTFBBQ\rYES\r\r\r\r"); delay(1000);
    Serial.print("\r"); delay(1000);
    Serial.print("OK\rA"); delay(1000);
    Serial.print("B\rCD"); delay(1000);
    Serial.print("\rHOLA\rHOLA2"); delay(1000);
  }
*/

  if (Serial.available() >= 4) {
    if (Serial.read()=='A' && Serial.read()=='T') {
      Serial.print("yes\r");
      char c=Serial.read();
      if (c=='Z' && Serial.read()=='\r') {
        delay(200);
        Serial.print("ELM327 v1.5b\r"); delay(650);
      } else if (c=='D' && Serial.read()=='\r') {
        Serial.print("\rOK\r"); delay(850);
        Serial.print("\rE0\r"); delay(150);
        Serial.print("\rOK\r"); delay(450);
        Serial.print("\rOK\r"); delay(250);
        Serial.print("\rOK\r"); delay(150);
        Serial.print("\rBUS INIT: OK\r"); delay(150);
        //Serial.print("41 00 FF FF FF FF\r"); delay(1000);

        long p=0;
        unsigned int v=0;
        while (true) {
          while (Serial.available()) Serial.read(); // free read buffer
          p+=1+p/2; if (p>100) p=0;
          v+=5; if (v>255) v=0;
          Serial.print("41 04 "); SerialPrintHex(p); Serial.print("\r"); delay(10);
          Serial.print("41 0C 2D "); SerialPrintHex(v); Serial.print("\r"); delay(10);
          Serial.print("41 0D "); SerialPrintHex(v); Serial.print("\r"); delay(300);
        }
      }
    }
  }

}

