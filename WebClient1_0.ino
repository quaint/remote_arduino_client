#include <SPI.h>
#include <Ethernet.h>
#include <PCF8574.h>
#include <Wire.h>
#include <LiquidCrystal.h>

#define OUT_PIN_1 0
#define OUT_PIN_2 1
#define TEMP_ANALOG_PIN 0
#define EXPANDER_ADDR 0x20
#define LCD_TEMPLATE "X X X X X X X X"
//#define SERVER_PORT 3000
#define SERVER_PORT 80
#define SYNC_DEALY 10
#define TEMP_RATIO 360

String sensorSerialNumber = "12345";
String boardSerialNumber = "12345";
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x41, 0xF0 };
char serverName[] = "gentle-planet-1993.heroku.com";
//char serverName[] = "10.0.1.101";
int tempRatioCounter = 0;

EthernetClient client;
PCF8574 expander;
LiquidCrystal lcd(8, 9, 6, 5, 3, 2);

void setup() {
  Serial.begin(9600);
  
  expander.begin(EXPANDER_ADDR);
  expander.pinMode(OUT_PIN_1, OUTPUT);
  expander.pinMode(OUT_PIN_2, OUTPUT);
  expander.digitalWrite(OUT_PIN_1, LOW);
  expander.digitalWrite(OUT_PIN_2, LOW);
  
  lcd.begin(16, 2);
  lcd.setCursor(0, 1);
  lcd.print(LCD_TEMPLATE);
  
  if (Ethernet.begin(mac) == 0) {
    Serial.println("failed to configure Ethernet using DHCP");
    while(true);
  }

  Serial.print("IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();

  delay(1000);
  
  Serial.println("starting...");
}

void loop()
{
  handleSync();
  handleTemp();
  handleCountdown();
}

void handleCountdown() {
  lcd.setCursor(0, 0);
  lcd.print("t-        ");
  for(int i=SYNC_DEALY; i>0; i--) {
    lcd.setCursor(2, 0);
    lcd.print(i);
    if(i<10) {
      lcd.setCursor(3, 0);
      lcd.print(" ");
    }
    delay(1000);
  }
}

void handleSync() {
  if (client.connect(serverName, SERVER_PORT) > 0) {
    Serial.println("connected");
    lcd.setCursor(0, 0);
    lcd.print("syncing ");
    client.println("GET /boards/sync/" + boardSerialNumber + ".xml HTTP/1.0");
    client.println("Host: " + String(serverName));
    client.println();

    while( client.find("<port>") ) {
      int port = client.parseInt();
      Serial.print("port: ");
      Serial.println(port);
//       if(port > 2) {
//         break;
//       }
      if(client.find("<state>")) {
        int state = client.parseInt();
        Serial.print("state: ");
        Serial.println(state);
        if(port == 1) {
          lcd.setCursor(0, 1);
          if(state == 1) {
            expander.digitalWrite(OUT_PIN_1, HIGH);
            lcd.print("1");
          } else if(state == 0) {
            expander.digitalWrite(OUT_PIN_1, LOW);
            lcd.print("0");
          }
        } else if(port == 2) {
          lcd.setCursor(2, 1);
          if(state == 1) {
            expander.digitalWrite(OUT_PIN_2, HIGH);
            lcd.print("1");
          } else if(state == 0) {
            expander.digitalWrite(OUT_PIN_2, LOW);
            lcd.print("0");
          }
        }
      }
    }
    client.stop();
    client.flush();
  } else {
    Serial.println("not connected when syncing"); 
  }
}

void handleTemp() {
  float temp = readTemp();
  lcd.setCursor(10, 0);
  lcd.print(temp);
  Serial.print("temp: ");
  Serial.println(temp);
  
  if(tempRatioCounter >= TEMP_RATIO) {
    tempRatioCounter = 0;
    String parValue = "value=";
    String parSerial = "&serial=" + sensorSerialNumber;
      
    Serial.println("sending temp");
    lcd.setCursor(0, 0);
    lcd.print("sending   ");
    if (client.connect(serverName, SERVER_PORT) > 0) {
      client.println("POST /readings/upload HTTP/1.0");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.println("Host: " + String(serverName));
      client.print("Content-Length: ");
      client.println(parValue.length() + parSerial.length() + sizeof(temp) + 1);
      client.println();
      client.print(parValue);
      client.print(temp);
      client.println(parSerial);
      client.println();
      client.stop();
      client.flush();
      Serial.println("temp sent");
      lcd.setCursor(0, 0);
      lcd.print("sent      ");
    } else {
      Serial.println("not connected when sending"); 
    }
  } else {
    tempRatioCounter++;
  }
}

float readTemp() {
  float temp = analogRead(TEMP_ANALOG_PIN) * 5/1024.0;
  return (temp - 0.5) / 0.01;
}
