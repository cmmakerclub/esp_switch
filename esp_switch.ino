#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <stdio.h>
#define THRESHOLD 1000

const char* ssid = "OpenWrt_NAT_500GP.101";
const char* password = "activegateway";



char* server = "128.199.104.122";
String clientName;

char* clientNameCStr;
WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);


unsigned long prevMillis = 0;
unsigned long prevMillisPub = 0;

unsigned int buttonCounter = 0x00;
int buttonState = 0;
int toggler = 0;
unsigned long currentMillis = 0;
String topicAlive;

void dummyText() {
  int i = 0;
  for (i = 0; i < 100; i++) {
    Serial.println("GO GO GO");
  }
}

void callback(char* topic, byte* payload, unsigned int len) {
  
  if (len > 4) { 
    return ;
  }
  
  dummyText();
  
  Serial.print("C0 ");
  
  char buff[5];
  memcpy(buff, payload, len+1);
  buff[len] = '\0';
  Serial.print("C1 ");
  Serial.print(len); Serial.println(" < len");
  Serial.println(buff);
  if (strcmp(buff, "0") == 0) {
    Serial.println("OFF");
    digitalWrite(2, 0);
    buttonCounter = 0;
    toggler = 0;    
  }
  else if (strcmp(buff, "1") == 0) {
    Serial.println("ON"); 
    buttonCounter = 0;
    toggler = 1;
    digitalWrite(2, HIGH);
  }
  else {
    Serial.println("NON-SENSE");
  }

  Serial.println("END===========");
  
  dummyText();  
}


String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void genClientNameGlobal(){
  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
}

void setup() {
  int wifiWaiting = 0;
  Serial.begin(115200);
  pinMode(2, OUTPUT);
  pinMode(0, INPUT);
  delay(10);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  int waitSecs = 15;
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
    Serial.print(wifiWaiting);
    wifiWaiting++;
    
    if (wifiWaiting > 15 * waitSecs) {
      abort();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");  
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Generate client name based on MAC address and last 8 bits of microsecond counter

  genClientNameGlobal();

  topicAlive = clientName +"/data";
  
  while(!client.connect((char*) clientName.c_str())) {
    Serial.println("Connecting...");
    delay(100);
  }
  
  Serial.println("MQTT Connected");
  
  while (!client.subscribe((char*) clientName.c_str())) {
    Serial.println("WATING SUBSCRIBE...");
    delay(500);
  }

  Serial.println("SUBSCRIBED");


  prevMillisPub = -20000;  
}

void loop() {
  static unsigned long counter = 0;
  
  String payload = "{\"millis\":";
  payload += millis();
  payload += ",\"counter\":";
  payload += counter;
  payload += "}";

  client.loop();
    if (client.connected()) {
      buttonState = digitalRead(0);    
      currentMillis = millis();
      
      if (prevMillis != currentMillis && currentMillis - prevMillis >= THRESHOLD) {
        prevMillis = currentMillis;
        
        counter++;
        if (buttonState == 0) {
          buttonCounter++;
        }
        
        if (millis() - prevMillisPub > 1000) {
          prevMillisPub = millis();        
          if (client.publish((char*) topicAlive.c_str(), (char*) payload.c_str())) {
            Serial.println("Publish ok");
          }
          else {
            Serial.println("Publish failed");
          }
        }

      }
      
      if (buttonState == 1) {
        buttonCounter = 0;
      }
      
      if (buttonCounter >= 3) {
        String p = String(toggler);

        toggler = !toggler;
        buttonCounter = 0x00;
        Serial.print("IN IN IN STATE = ");
        Serial.println(toggler);      
        digitalWrite(2, toggler);
        int publish_counter = 0;
        while(!client.publish (clientNameCStr, (uint8_t*) p.c_str(), 1, true)){
          publish_counter++;
          // 15 seconds
          if (publish_counter > 5 * 100 * 10) {
            abort();
          }
          delay(100);
        } ;    
      }
      else {
        //nope
      }
      
    }
    else {
      // disconnected.
      abort();
    }
    
}


