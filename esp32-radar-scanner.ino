#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const int TRIG_PIN = 13;
const int ECHO_PIN = 34;
const int SERVO_PIN = 27;
const int BUZZER_PIN = 26;

const float CM_PER_US = 0.0343 / 2.0;
const int STEP_DEG = 2;          // bump to 3 for an even faster search sweep
const int SETTLE_MS = 12;        // was 25 — SG90 clears a 2° step in ~3ms

// CHANGED: echo timeout now matches max useful range (~300cm) instead of 30000us
const unsigned long ECHO_TIMEOUT_US = 18000;

const float ALERT_THRESHOLD_CM = 70.0;
const float MONITOR_THRESHOLD_CM = 300.0;

const char* WIFI_SSID = "Projects";
const char* WIFI_PASSWORD = "Projects";

const char* SERVER_URL = "http://192.168.1.70:3000/scan-data";

Servo scanServo;
int angle = 0;
int direction = 1;

// --- tracking/dwell state ---
bool tracking = false;
unsigned long trackStartTime = 0;
const unsigned long TRACK_DURATION_MS = 3500;
int trackedAngle = 0;

// --- escalating buzzer state ---
unsigned long lastBeepToggle = 0;
bool buzzerState = false;

// CHANGED: persistent client + HTTPClient so the TCP connection is reused
WiFiClient wifiClient;
HTTPClient http;
bool httpConfigured = false;

void setup() {
  Serial.begin(115200);

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(TRIG_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  scanServo.attach(SERVO_PIN);
  scanServo.write(angle);
  delay(300);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println();
    Serial.print("Connected! IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println();
    Serial.println("Failed to connect — check SSID/password and 2.4GHz band");
  }

  Serial.println("Stage 11 (fast sweep + track/dwell) starting...");
}

float readDistanceCm() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
  if (duration == 0) return -1.0;

  return duration * CM_PER_US;
}

char getZone(int angleDeg) {
  if (angleDeg <= 45) return 'A';
  if (angleDeg <= 90) return 'B';
  if (angleDeg <= 135) return 'C';
  return 'D';
}

// CHANGED: keeps the socket alive between POSTs, short timeouts so a dead
// server can't stall the sweep
void sendReading(const String& jsonPayload) {
  if (WiFi.status() != WL_CONNECTED) return;

  if (!httpConfigured) {
    http.setReuse(true);
    http.setConnectTimeout(600);  // ms
    http.setTimeout(600);         // ms
    httpConfigured = true;
  }

  http.begin(wifiClient, SERVER_URL);
  http.addHeader("Content-Type", "application/json");
  http.POST(jsonPayload);
  http.end();   // with setReuse(true) this keeps the TCP connection open
}

bool reportReading(int angleDeg, float distanceCm) {
  char zone = getZone(angleDeg);
  String status;
  bool alert = false;

  if (distanceCm < 0) {
    status = "NO_ECHO";
  } else if (distanceCm < ALERT_THRESHOLD_CM) {
    status = "ALERT";
    alert = true;
  } else if (distanceCm < MONITOR_THRESHOLD_CM) {
    status = "MONITOR";
  } else {
    status = "CLEAR";
  }

  JsonDocument doc;   // CHANGED: StaticJsonDocument is deprecated in ArduinoJson 7
  doc["angle"] = angleDeg;
  doc["distance"] = distanceCm < 0 ? -1 : distanceCm;
  doc["status"] = status;
  doc["zone"] = String(zone);
  doc["timestamp"] = millis();

  String jsonOutput;
  serializeJson(doc, jsonOutput);
  Serial.println(jsonOutput);

  sendReading(jsonOutput);

  return alert;
}

void updateBuzzer(float distanceCm) {
  if (distanceCm < 0 || distanceCm >= ALERT_THRESHOLD_CM) {
    digitalWrite(BUZZER_PIN, LOW);
    buzzerState = false;
    return;
  }

  int beepInterval = map(constrain((int)distanceCm, 0, (int)ALERT_THRESHOLD_CM),
                         0, (int)ALERT_THRESHOLD_CM,
                         60, 400);

  if (millis() - lastBeepToggle >= (unsigned long)beepInterval) {
    buzzerState = !buzzerState;
    digitalWrite(BUZZER_PIN, buzzerState);
    lastBeepToggle = millis();
  }
}

void loop() {
  if (tracking) {
    // CHANGED: no settle delay — the servo is already parked at trackedAngle
    scanServo.write(trackedAngle);

    float distanceCm = readDistanceCm();
    updateBuzzer(distanceCm);
    reportReading(trackedAngle, distanceCm);

    bool stillInRange = (distanceCm >= 0 && distanceCm < ALERT_THRESHOLD_CM);
    bool dwellExpired = (millis() - trackStartTime >= TRACK_DURATION_MS);

    if (!stillInRange || dwellExpired) {
  tracking = false;
  digitalWrite(BUZZER_PIN, LOW);

  // NEW: step the sweep past this bearing instead of leaving `angle`
  // sitting where the object was — otherwise a persistent object at
  // the same spot re-triggers tracking on the very next loop pass
  angle = trackedAngle + direction * STEP_DEG;
  if (angle >= 180) { angle = 180; direction = -1; }
  else if (angle <= 0) { angle = 0; direction = 1; }
}
return;
  }

  // --- normal sweep ---
  scanServo.write(angle);
  delay(SETTLE_MS);

  float distanceCm = readDistanceCm();
  updateBuzzer(distanceCm);
  bool alert = reportReading(angle, distanceCm);

  if (alert) {
    tracking = true;
    trackedAngle = angle;
    trackStartTime = millis();
    return;
  }

  angle += direction * STEP_DEG;
  if (angle >= 180) {
    angle = 180;
    direction = -1;
  } else if (angle <= 0) {
    angle = 0;
    direction = 1;
  }
}