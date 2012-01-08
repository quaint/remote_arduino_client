#include <SPI.h>
#include <Ethernet.h>

#define OUT_PIN_1 2
#define OUT_PIN_2 3
#define TEMP_ANALOG_PIN 0
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

void setup() {
  Serial.begin(9600);
  
  pinMode(OUT_PIN_1, OUTPUT);
  pinMode(OUT_PIN_2, OUTPUT);
  
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
  delay(1000*SYNC_DEALY);
}

void handleSync() {
  if (client.connect(serverName, SERVER_PORT) > 0) {
    Serial.println("connected");
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
          if(state == 1) {
            digitalWrite(OUT_PIN_1, HIGH);
          } else if(state == 0) {
            digitalWrite(OUT_PIN_1, LOW);
          }
        } else if(port == 2) {
          if(state == 1) {
            digitalWrite(OUT_PIN_2, HIGH);
          } else if(state == 0) {
            digitalWrite(OUT_PIN_2, LOW);
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
  Serial.print("temp: ");
  Serial.println(temp);
  
  if(tempRatioCounter >= TEMP_RATIO) {
    tempRatioCounter = 0;
    String parValue = "value=";
    String parSerial = "&serial=" + sensorSerialNumber;
      
    Serial.println("sending temp");
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
