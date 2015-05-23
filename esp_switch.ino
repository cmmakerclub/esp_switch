#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <stdio.h>
#define THRESHOLD 1000
#define DEBUG 1
const char* ssid = "OpenWrt_NAT_500GP.101";
const char* password = "activegateway";

//char* topic = "esp8266-18:fe:34:fe:c3:20/data";
char* server = "128.199.191.223";
String clientName;
char* clientNameCStr;
WiFiClient wifiClient;
PubSubClient client(server, 1883, callback, wifiClient);


unsigned long prevMillis = 0;

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
  clientName;
  clientName += "esp8266-";
  uint8_t mac[6];
  WiFi.macAddress(mac);
  clientName += macToStr(mac);
  
  topicAlive = clientName +"/data";
  
  if (client.connect((char*) clientName.c_str(), (char*) topicAlive.c_str(), 1, 1, "willMessage")) {
    Serial.println("Connected to MQTT broker");
    Serial.print("Topic is: ");
    Serial.println((char*) topicAlive.c_str());
    clientNameCStr = const_cast<char*>(clientName.c_str());
    unsigned int isSubscribed = client.subscribe(clientNameCStr);
    if (isSubscribed) {
        dummyText();
        Serial.print("subscribed to >>> ");
        Serial.print(clientNameCStr);
        Serial.println("<< ");       
    }
    else {
     Serial.println("subscribed error");
     abort();
    }
  }
  else {
    Serial.println("MQTT connect failed");
    Serial.println("Will reset and try again...");
    abort();
  }
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
        
        if (client.publish((char*) topicAlive.c_str(), (char*) payload.c_str())) {
          Serial.println("Publish ok");
        }
        else {
          Serial.println("Publish failed");
        }      

      }
      
      if (buttonState == 1) {
        buttonCounter = 0;
      }
      
      if (buttonCounter >= 3) {
        String p = String(toggler);

        toggler = !toggler;
        buttonCounter = 0x00;
        #ifdef DEBUG
          Serial.println(toggler);      
          digitalWrite(2, toggler);
        #endif
        int publish_counter = 0;
        while(!client.publish (clientNameCStr, (uint8_t*) p.c_str(), 1, true)){
          publish_counter++;
          // 10 seconds
          if (publish_counter > 15 * 100 * 10) {
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


