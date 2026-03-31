#include <arduinoFFT.h>
#include <ESP_FlexyStepper.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define SAMPLES 1024
#define SAMPLING_FREQUENCY 700
#define BUZZER_PIN 25

// WiFi and MQTT configuration
const char* WIFI_SSID     = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
const char* MQTT_BROKER   = "YOUR_MQTT_BROKER_IP";
const int   MQTT_PORT     = 1883;
const char* MQTT_TOPIC_LOG    = "guitar_tuner/log";
const char* MQTT_TOPIC_STATUS = "guitar_tuner/status";

WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

unsigned int samplingPeriod;
unsigned long microSeconds;

double vReal[SAMPLES];
double vImag[SAMPLES];
double averageSpectrum[SAMPLES / 2];

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

double expectedFrequencies[6] = {82.41, 110.00, 146.83, 196.00, 246.94, 329.63};
double frequencyThreshold = 40.0;
int stringToTune = -1;

// Stepper motor pin assignments
const int MOTOR_STEP_PIN = 33;
const int MOTOR_DIRECTION_PIN = 32;
const int MOTOR_ENABLE_PIN = 35;

// Button pin assignments
const int buttonPins[6] = {12, 13, 14, 15, 16, 17};

// Create the stepper motor object 
ESP_FlexyStepper stepper;

// P-controller gains for raising and lowering the pitch for each string
double kp_increase[6] = {9.05, 10.56, 8.27, 6.56, 5.15, 6.12};
double kp_decrease[6] = {10.56, 11.52, 9.49, 6.63, 5.40, 8.15};

// PID integral and derivative terms (per string, reset on each new tuning session)
double pid_integral[6]      = {0, 0, 0, 0, 0, 0};
double pid_prev_error[6]    = {0, 0, 0, 0, 0, 0};
unsigned long pid_last_time[6] = {0, 0, 0, 0, 0, 0};

// PID Ki and Kd gains (tuned conservatively to supplement the existing Kp)
const double KI = 0.02;
const double KD = 1.50;

// Maximum allowable integral windup (in Hz·s)
const double INTEGRAL_WINDUP_LIMIT = 50.0;

void setupWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
    delay(500);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("\nWiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi connection failed. Continuing offline.");
  }
}

void mqttReconnect() {
  if (WiFi.status() != WL_CONNECTED) return;
  if (mqttClient.connected()) return;
  mqttClient.setServer(MQTT_BROKER, MQTT_PORT);
  if (mqttClient.connect("GuitarTunerESP32")) {
    Serial.println("MQTT connected.");
    mqttClient.publish(MQTT_TOPIC_STATUS, "online");
  }
}

void mqttPublish(const char* topic, const String& payload) {
  if (mqttClient.connected()) {
    mqttClient.publish(topic, payload.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  Serial.println("Select a string (press the corresponding button):");

  // Initialize stepper motor
  stepper.connectToPins(MOTOR_STEP_PIN, MOTOR_DIRECTION_PIN);
  pinMode(MOTOR_ENABLE_PIN, OUTPUT);
  digitalWrite(MOTOR_ENABLE_PIN, LOW);  // Motor driver disabled by default

  stepper.setSpeedInStepsPerSecond(400);

  // Initialize button pins
  for (int i = 0; i < 6; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }

  setupWiFi();
  mqttReconnect();
}

void loop() {
  mqttClient.loop();
  mqttReconnect();

  // Check button press
  for (int i = 0; i < 6; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {
      stringToTune = i;
      // Reset PID state for the newly selected string
      pid_integral[i]   = 0;
      pid_prev_error[i] = 0;
      pid_last_time[i]  = millis();

      Serial.print("Selected string ");
      Serial.println(stringToTune + 1);

      String msg = "{\"event\":\"string_selected\",\"string\":" + String(stringToTune + 1) + "}";
      mqttPublish(MQTT_TOPIC_LOG, msg);

      delay(3000);  // Delay 3 seconds for tuner positioning

      // Play C4 note to signal start of sample collection (261.63 Hz)
      tone(BUZZER_PIN, 262, 1000);
      delay(1000);
    }
  }

  if (stringToTune != -1) {
    tuneString();
  }
}

void tuneString() {
  while (true) {
    mqttClient.loop();

    computeAverageSpectrum(3);
    double dominantFrequency = findPeakFrequency();
    double targetFrequency   = expectedFrequencies[stringToTune];
    double error             = targetFrequency - dominantFrequency;

    Serial.print("Current frequency for string ");
    Serial.print(stringToTune + 1);
    Serial.print(": ");
    Serial.println(dominantFrequency);

    // Publish telemetry
    String telemetry = "{\"string\":" + String(stringToTune + 1) +
                       ",\"measured\":" + String(dominantFrequency, 2) +
                       ",\"target\":"   + String(targetFrequency, 2) +
                       ",\"error\":"    + String(error, 2) + "}";
    mqttPublish(MQTT_TOPIC_LOG, telemetry);

    if (abs(error) <= 1.0) {  // String is within ±1 Hz of target
      Serial.println("String is in tune.");
      tone(BUZZER_PIN, 349, 500);
      delay(500);
      tone(BUZZER_PIN, 262, 500);
      delay(500);

      mqttPublish(MQTT_TOPIC_STATUS, ("{\"event\":\"tuned\",\"string\":" + String(stringToTune + 1) + "}").c_str());

      // Reset PID state
      pid_integral[stringToTune]   = 0;
      pid_prev_error[stringToTune] = 0;
      stringToTune = -1;
      Serial.println("Choose a string to tune (1-6):");
      break;
    }

    // ── PID controller ───────────────────────────────────────────────────
    unsigned long now     = millis();
    double dt             = (now - pid_last_time[stringToTune]) / 1000.0;
    if (dt <= 0) dt = 0.1;  // Guard against zero dt on first iteration
    pid_last_time[stringToTune] = now;

    // Integral with anti-windup clamping
    pid_integral[stringToTune] += error * dt;
    pid_integral[stringToTune] = constrain(pid_integral[stringToTune],
                                            -INTEGRAL_WINDUP_LIMIT,
                                             INTEGRAL_WINDUP_LIMIT);

    // Derivative (filtered: no derivative kick on setpoint change)
    double derivative = (error - pid_prev_error[stringToTune]) / dt;
    pid_prev_error[stringToTune] = error;

    // Select direction-specific Kp
    double kp = (error > 0) ? kp_increase[stringToTune] : kp_decrease[stringToTune];

    // PID output (motor steps)
    double pidOutput = kp * error
                     + KI * pid_integral[stringToTune]
                     + KD * derivative;

    int stepsRequired = (int)round(pidOutput);
    // ─────────────────────────────────────────────────────────────────────

    if (error > 0) {
      Serial.println("Increasing string tension.");
      stepsRequired = -abs(stepsRequired);  // Negative steps raise tension
    } else {
      Serial.println("Decreasing string tension.");
      stepsRequired = abs(stepsRequired);   // Positive steps lower tension
    }

    Serial.print("Steps required: ");
    Serial.println(stepsRequired);

    // Activate motor driver, move, then disable driver
    digitalWrite(MOTOR_ENABLE_PIN, HIGH);
    stepper.setSpeedInStepsPerSecond(200);
    stepper.setAccelerationInStepsPerSecondPerSecond(5000);
    stepper.moveToPositionInSteps(stepper.getCurrentPositionInSteps() + stepsRequired);
    digitalWrite(MOTOR_ENABLE_PIN, LOW);
  }
}

void computeAverageSpectrum(int numAverages) { //FFT 
  for (int i = 0; i < SAMPLES / 2; i++) {
    averageSpectrum[i] = 0;
  }

  for (int n = 0; n < numAverages; n++) {
    readAnalogValues();

    FFT.dcRemoval(vReal, SAMPLES);
    FFT.windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.compute(vReal, vImag, SAMPLES, FFT_FORWARD);
    FFT.complexToMagnitude(vReal, vImag, SAMPLES);

    for (int i = 0; i < SAMPLES / 2; i++) {
      averageSpectrum[i] += vReal[i];
    }
  }

  for (int i = 0; i < SAMPLES / 2; i++) {
    averageSpectrum[i] /= numAverages;
  }
}

double findPeakFrequency() { //Function to reduce tuning range to ±threshold
  double maxMagnitude = 0;
  int maxIndex = 0;

  for (int i = 1; i < SAMPLES / 2; ++i) {
    double currentFrequency = (i * 1.0 * SAMPLING_FREQUENCY) / SAMPLES;
    if (abs(currentFrequency - expectedFrequencies[stringToTune]) <= frequencyThreshold && averageSpectrum[i] > maxMagnitude) {
      maxMagnitude = averageSpectrum[i];
      maxIndex = i;
    }
  }

  return (maxIndex * 1.0 * SAMPLING_FREQUENCY) / SAMPLES;
}

void readAnalogValues() { //Read analog pin values
  for (int i = 0; i < SAMPLES; i++) {
    microSeconds = micros();
    vReal[i] = (analogRead(34) * 3.3 / 4096.0 - 1.65);
    vImag[i] = 0;
    while (micros() < (microSeconds + samplingPeriod)) {} //Loop for sample stabilization
  }
}
