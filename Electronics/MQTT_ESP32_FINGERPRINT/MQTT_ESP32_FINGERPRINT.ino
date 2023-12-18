#include "WiFi.h"
#include "PubSubClient.h"
#include "HCSR04.h"


WiFiClient wifi;
PubSubClient client(wifi);
HCSR04 hc(2, 3);


const char* ssid = "Home";
const char* password =  "95036113";

const char* mqtt_server = "192.168.0.197";

void callback(char* topic, byte* payload, unsigned int length) {
  byte* p = (byte*)malloc(length);
  memcpy(p,payload,length);
  client.publish("outTopic", p, length);
  free(p);
}

void setup() {
 
  Serial.begin(115200);
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println("Connected to the WiFi network");
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Connecting to MQTT");
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  if(!client.connected()){
    client.connect("ESP32Client");
    Serial.println("MQTT connected successfully");
  }
}




 
void loop() {
  client.loop();
  Serial.println( hc.dist() );
  client.publish("/first/hello", "hi");  
  
}
