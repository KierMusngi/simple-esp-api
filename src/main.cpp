#include <Arduino.h>
#include <WebServer.h>
#include <WiFi.h>

#define LED_NETWORK 2
#define IR_SENSOR 25
#define RELAY_LED_GREEN 12
#define RELAY_LED_RED 14
#define RELAY_MOTOR_A 27
#define RELAY_MOTOR_B 26
#define RELAY_BUZZER 33
#define RELAY 33

#define LIMIT_SWITCH_CLOSED 5
#define LIMIT_SWITCH_OPEN 15

const char *SSID = "mguest";
const char *PWD = "itsnotworkinG12345!@#$%";

IPAddress local_IP(192, 168, 1, 100);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 0, 0);
IPAddress primaryDNS(8, 8, 8, 8);
IPAddress secondaryDNS(8, 8, 4, 4);
WebServer server(80);

int buttonStateFalling = 0;
int buttonFallingEdge = 1;
int lastButtonStateFalling = 1;
int poepleCount = 0;

// Task handlers
TaskHandle_t task_triggerAlarm_handler = NULL;

// States
int alarmCount = 0;
bool isFlapClosed = false;

void closeGate() {
  digitalWrite(RELAY_MOTOR_A, LOW);
  digitalWrite(RELAY_MOTOR_B, HIGH);
  isFlapClosed = true;
  Serial.println("Gate is closing");
  server.send(200, "application/json", "Flap opening");
}

void openGate() {
  digitalWrite(RELAY_MOTOR_A, HIGH);
  digitalWrite(RELAY_MOTOR_B, LOW);
  isFlapClosed = false;
  Serial.println("Gate is opening");
  server.send(200, "application/json", "Flap closing");
}

void stopMotor() {
  digitalWrite(RELAY_MOTOR_A, HIGH);
  digitalWrite(RELAY_MOTOR_B, HIGH);
  Serial.println("Gate motor is stopped");
  server.send(200, "application/json", "Flap motor stopped");
}

void openGreen() {
  digitalWrite(RELAY_LED_GREEN, LOW);
  Serial.println("Green light open");
  server.send(200, "application/json", "Green LED open");
}

void closeGreen() {
  digitalWrite(RELAY_LED_GREEN, HIGH);
  Serial.println("Green light closed");
  server.send(200, "application/json", "Green LED closed");
}

void openRed() {
  digitalWrite(RELAY_LED_RED, LOW);
  Serial.println("Red light open");
  server.send(200, "application/json", "Red LED open");
}

void closeRed() {
  digitalWrite(RELAY_LED_RED, HIGH);
  Serial.println("Red light closed");
  server.send(200, "application/json", "Red LED closed");
}

void openBuzzer() {
  digitalWrite(RELAY_BUZZER, LOW);
  server.send(200, "application/json", "Buzzer open");
}

void closeBuzzer() {
  digitalWrite(RELAY_BUZZER, HIGH);
  server.send(200, "application/json", "Buzzer closed");
}

void generalTestSetup() {
  openGreen();
  delay(1000);
  closeGreen();
  delay(1000);
  openRed();
  delay(1000);
  closeRed();
  delay(1000);
  openBuzzer();
  delay(1000);
  closeBuzzer();
  delay(1000);
  openBuzzer();
  delay(1000);
  closeBuzzer();

  Serial.println("System ready!");
}

// Tasks
void task_detectPassBy(void * parameter) {
  for (;;) {
    buttonStateFalling = digitalRead(IR_SENSOR);
    if (buttonStateFalling != lastButtonStateFalling) {
      if (buttonStateFalling == HIGH) {
        buttonFallingEdge = 1;
        if (isFlapClosed && alarmCount == 0 && task_triggerAlarm_handler != NULL) {
          vTaskResume(task_triggerAlarm_handler);
          Serial.println("Intruder alert!");
        }
        else {
          Serial.println("Human passed");
          closeGate();
        }
      }
    }
    else{
      buttonFallingEdge = 0;
    }
    lastButtonStateFalling = buttonStateFalling;
  }
}

void task_triggerAlarm(void * parameter) {
  for (;;) {
    digitalWrite(RELAY_BUZZER, LOW);
    delay(500);
    digitalWrite(RELAY_BUZZER, HIGH);
    delay(500);
    alarmCount++;
    Serial.println("Buzz! Intruder Alert!");
    if (alarmCount >= 3){
      alarmCount = 0;
      vTaskSuspend(NULL);
    }
  }
}

// Setup API resources
void setup_routing() {
  // Endpoints
  server.on("/openGate", openGate);
  server.on("/closeGate", closeGate);
  server.on("/stopMotor", stopMotor);
  server.on("/openGreen", openGreen);
  server.on("/closeGreen", closeGreen);
  server.on("/openRed", openRed);
  server.on("/closeRed", closeRed);
  server.on("/openBuzzer", openBuzzer);
  server.on("/closeBuzzer", closeBuzzer);
  server.on("/test", generalTestSetup);
 
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
  digitalWrite(LED_NETWORK, HIGH);
}



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(IR_SENSOR, INPUT);
  pinMode(LIMIT_SWITCH_CLOSED, INPUT_PULLDOWN);
  pinMode(LIMIT_SWITCH_OPEN, INPUT_PULLDOWN);

  pinMode(LED_NETWORK, OUTPUT);
  pinMode(RELAY_LED_GREEN, OUTPUT);
  pinMode(RELAY_LED_RED, OUTPUT);
  pinMode(RELAY_MOTOR_A, OUTPUT);
  pinMode(RELAY_MOTOR_B, OUTPUT);
  pinMode(RELAY_BUZZER, OUTPUT);

  digitalWrite(LED_NETWORK, LOW);
  digitalWrite(RELAY_LED_GREEN, HIGH);
  digitalWrite(RELAY_LED_RED, HIGH);
  digitalWrite(RELAY_MOTOR_A, HIGH);
  digitalWrite(RELAY_MOTOR_B, HIGH);
  digitalWrite(RELAY_BUZZER, HIGH);

  connect_to_wifi();
  setup_routing();

  xTaskCreate(
    task_detectPassBy,
    "Detect if a human passed by",
    1000,
    NULL,
    1,
    NULL
  );

  xTaskCreate(
    task_triggerAlarm,
    "Alarms when an intruder passed by",
    1000,
    NULL,
    2,
    &task_triggerAlarm_handler
  );

  if (task_triggerAlarm_handler != NULL) vTaskSuspend(task_triggerAlarm_handler);

  if (!digitalRead(LIMIT_SWITCH_CLOSED)) {
    closeGate();
  }
}

void loop() {
  // put your main code here, to run repeatedly: 
  if (isFlapClosed && digitalRead(LIMIT_SWITCH_CLOSED) == HIGH) {
    stopMotor();
    Serial.println("Flap is closed");
  }

  if (!isFlapClosed && digitalRead(LIMIT_SWITCH_OPEN) == HIGH) {
    stopMotor();
    Serial.println("Flap is opened");
  }

  server.handleClient();
}
