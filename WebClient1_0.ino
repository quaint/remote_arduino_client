#include <SPI.h>
#include <Ethernet.h>
#include <PCF8574.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <Servo.h>

#define OUT_PIN_1 0
#define OUT_PIN_2 1
#define TEMP_ANALOG_PIN 0
#define LIGHT_ANALOG_PIN 1
#define EXPANDER_ADDR 0x20
#define LCD_TEMPLATE "X X X X X X X X"
#define SERVER_PORT 3000
//#define SERVER_PORT 80
#define SYNC_DEALY 10


String tempSensorSN = "12345";
String lightSensorSN = "11111";
String boardSerialNumber = "12345";
byte mac[] = { 0x90, 0xA2, 0xDA, 0x00, 0x41, 0xF0 };
//char serverName[] = "gentle-planet-1993.heroku.com";
char serverName[] = "10.0.1.101";
int tempSensorRatioCounter = 0;
int tempSensorRatio = 10;
int lightSensorRatioCounter = 0;
int lightSensorRatio = 10;

EthernetClient client;
PCF8574 expander;
LiquidCrystal lcd(8, 9, 7, 5, 3, 2);
Servo servo;

void setup() {
  Serial.begin(9600);
  
  servo.attach(6);
  
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
  handleLight();
  handleCountdown();
}

void handleCountdown() {
  lcd.setCursor(0, 0);
  lcd.print("t-      ");
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
      if(client.find("<kind>")) {
        int kind = client.parseInt();
        if(kind == 1) {
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
        } else if(kind == 2) {
          if(client.find("<setting>")) {
            int setting = client.parseInt();
            Serial.print("setting: ");
            Serial.println(setting);
            servo.write(setting);
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
  if(tempSensorRatioCounter >= tempSensorRatio) {
    tempSensorRatioCounter = 0;
    handleSensor(tempSensorSN, temp);
  } else {
    tempSensorRatioCounter++;
  }
}

void handleLight() {
  int light = readLight();
  lcd.setCursor(8, 0);
  lcd.print(light);
  Serial.print("light: ");
  Serial.println(light);
  if(lightSensorRatioCounter >= lightSensorRatio) {
    lightSensorRatioCounter = 0;
    handleSensor(lightSensorSN, (float)light);
  } else {
    lightSensorRatioCounter++;
  }
}

void handleSensor(String serial, float reading) {
  
  String parSerial = "serial=" + serial;
  String parValue = "&value=";
      
  Serial.println("sending data");
  lcd.setCursor(0, 0);
  lcd.print("sending   ");
  if (client.connect(serverName, SERVER_PORT) > 0) {
    client.println("POST /readings/upload HTTP/1.0");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Host: " + String(serverName));
    client.print("Content-Length: ");
    client.println(parValue.length() + parSerial.length() + Serial.println(reading));
    client.println();
    client.print(parSerial);
    client.print(parValue);
    client.println(reading);
    client.println();
    client.stop();
    client.flush();
    Serial.println("data sent");
    lcd.setCursor(0, 0);
    lcd.print("sent      ");
  } else {
    Serial.println("not connected when sending"); 
  }
}

float readTemp() {
  float temp = analogRead(TEMP_ANALOG_PIN) * 5/1024.0;
  return (temp - 0.5) / 0.01;
}

int readLight() {
  return analogRead(LIGHT_ANALOG_PIN) / 128;
}
