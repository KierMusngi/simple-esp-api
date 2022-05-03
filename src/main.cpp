#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#define LED 2

const char *SSID = "";
const char *PWD = "";

// Set your Static IP address
IPAddress local_IP(192, 168, 1, 100); // Should be in a config
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1); // Should be in a config

IPAddress subnet(255, 255, 0, 0); // Should be in a config
IPAddress primaryDNS(8, 8, 8, 8);   //optional // Should be in a config
IPAddress secondaryDNS(8, 8, 4, 4); //optional // Should be in a config

WebServer server(80);

// Alarm
// Light indicator
// Infrared proximity sensor
// Reset

void toggleLed() {
  bool state = digitalRead(LED);
  digitalWrite(LED, !state);
  server.send(200, "application/json", "Hell yea!");
}

// Setup API resources
void setup_routing() {
  // Endpoints
  server.on("/switch", toggleLed);
 
  // Start server
  server.begin();
}

void connect_to_wifi() {
  Serial.print("Connecting to ");
  Serial.println(SSID);

  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("Failed to configure network");
  }
  
  WiFi.begin(SSID, PWD);
  
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
 
  Serial.print("Connected. IP: ");
  Serial.println(WiFi.localIP());
  digitalWrite(LED, HIGH);
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  // Pin modes and initial states
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  // Other setup
  connect_to_wifi();
  setup_routing();
}

void loop() {
  // put your main code here, to run repeatedly: 
  server.handleClient();
}
