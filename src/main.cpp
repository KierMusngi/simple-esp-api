#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>
#include <HTTPClient.h>

// Input
#define LM_SW_CLOSED 34
#define LM_SW_OPEN 14
#define IR_SENSOR 35

// Output
#define NETWORK 2
#define MOTOR_A 27
#define MOTOR_B 26
#define BUZZER 25
#define GREEN 33
#define RED 32

const char *SSID = "EAP-110";
const char *PWD = "4SuKfR%DHeFweqD";

const char *SERVER_NAME = "http://192.168.1.19/esp";


IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);
WebServer server(80);

// Task handlers

// States
int buttonStateFalling = 0;
int buttonFallingEdge = 1;
int lastButtonStateFalling = 1;
int alarmCount = 6;
int gateSecured = true;
bool gateIsClosing = false;
bool gateIsOpening = false;

// Functions
void func_greenOn() {
  digitalWrite(GREEN, LOW);
  Serial.println("Green LED is on");
}

void func_greenOff() {
  digitalWrite(GREEN, HIGH);
  Serial.println("Green LED is off");
}

void func_redOn() {
  digitalWrite(RED, LOW);
  Serial.println("Red LED is on");
}

void func_redOff() {
  digitalWrite(RED, HIGH);
  Serial.println("Red LED is off");
}
void func_stopGate() {
  digitalWrite(MOTOR_A, HIGH);
  digitalWrite(MOTOR_B, HIGH);
  gateIsClosing = false;
  gateIsOpening = false;
  Serial.println("Gate stopped");
}

void func_openGate() {
  digitalWrite(MOTOR_A, HIGH);
  digitalWrite(MOTOR_B, LOW);
  gateIsClosing = false;
  gateIsOpening = true;
  func_greenOn();
  gateSecured = false;
  Serial.println("Gate is opening");
}

void func_closeGate() {
  digitalWrite(MOTOR_A, LOW);
  digitalWrite(MOTOR_B, HIGH);
  gateIsClosing = true;
  gateIsOpening = false;
  Serial.println("Gate is closing");
}

void func_buzzerOn() {
  digitalWrite(BUZZER, LOW);
  Serial.println("Buzzer is on");
}

void func_buzzerOff() {
  digitalWrite(BUZZER, HIGH);
  Serial.println("Buzzer is off");
}

void func_triggerAlarm() {
  for (int i=0; i<alarmCount; i++) {
    func_buzzerOn();
    delay(200);
    func_buzzerOff();
    delay(200);
  }
}

void func_systemReady() {
  for (int i=0; i<2; i++) {
    func_buzzerOn();
    delay(200);
    func_buzzerOff();
    delay(200);
  }
}

void func_alarm() {
  Serial.println("Alarm triggered");
  func_redOn();
  for (int i=0; i<3; i++) {
    func_buzzerOn();
    delay(1000);
    func_buzzerOff();
    delay(1000);
  }
  delay(200);
  func_redOff();
  Serial.println("Alarm done");
}

// API endpoints
void openGate() {
  func_openGate();
  server.send(200, "application/json", "Gate is opening");
}

void closeGate() {
  func_closeGate();
  server.send(200, "application/json", "Gate is closing");
}

void alarmSystem() {
  func_alarm();
  server.send(200, "application/json", "Alarm triggered");
}

// Setup server API endpoints
void setup_routing() {
  server.on("/open-gate", openGate);
  server.on("/close-gate", closeGate);
  server.on("/alarm", alarmSystem);

  server.begin();
}

// Tasks
void task_detectPassBy(void * parameter) {
  delay(1000);
  for (;;) {
    buttonStateFalling = digitalRead(IR_SENSOR);
    if (buttonStateFalling != lastButtonStateFalling) {
      if (buttonStateFalling == HIGH) {
        buttonFallingEdge = 1;
        // Logic here
        if (gateSecured) {
          func_triggerAlarm();
          Serial.println("Intruder entered");
        }
        else {
          Serial.println("Guest entered");
          func_closeGate();
        }
      }
    }
    else{
      buttonFallingEdge = 0;
    }
    lastButtonStateFalling = buttonStateFalling;
  }
}

void task_manageGate(void * parameter) {
  for(;;) {
    delay(200);
    if ((gateIsClosing && !digitalRead(LM_SW_CLOSED)) || (gateIsOpening && !digitalRead(LM_SW_OPEN))) {
      func_stopGate();
      if (!digitalRead(LM_SW_CLOSED)) {
        gateSecured = true;
        func_greenOff();
      }
    }
  }
}

// Network setup
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
}

void setup() {
  Serial.begin(115200);

  // Input pin modes
  pinMode(LM_SW_CLOSED, INPUT);
  pinMode(LM_SW_OPEN, INPUT);
  pinMode(IR_SENSOR, INPUT);

  //Output pin modes
  pinMode(NETWORK, OUTPUT);
  pinMode(MOTOR_A, OUTPUT);
  pinMode(MOTOR_B, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(GREEN, OUTPUT);
  pinMode(RED, OUTPUT);

  // Initial output states
  digitalWrite(NETWORK, LOW);
  func_stopGate();
  func_buzzerOff();
  func_greenOff();
  func_redOff();

  // Connect to network and setup APIs
  connect_to_wifi();
  setup_routing();
  digitalWrite(NETWORK, HIGH); // ESP32 is connected to network

  // Initialize tasks
  xTaskCreate(
    task_manageGate,
    "Manage stop for opening and closing gate",
    10000,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    task_detectPassBy,
    "Detect if a human passed by",
    10000,
    NULL,
    1,
    NULL
  );
  
  // Startup
  func_systemReady();
  delay(2000);
  func_closeGate();

  Serial.println("System Ready!");
}

void loop() {
  server.handleClient();
}
