#include <arduinoFFT.h>
#include <ESP_FlexyStepper.h>

#define SAMPLES 1024
#define SAMPLING_FREQUENCY 700
#define BUZZER_PIN 25

unsigned int samplingPeriod;
unsigned long microSeconds;

double vReal[SAMPLES];
double vImag[SAMPLES];
double averageSpectrum[SAMPLES / 2];

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

double expectedFrequencies[6] = {82.41, 110.00, 146.83, 196.00, 246.94, 329.63};
double frequencyThreshold = 60.0;
int stringToTune = -1;
bool tuneUp = true;  // Default tuning direction is up

// Stepper motor pins
const int MOTOR_STEP_PIN = 33;
const int MOTOR_DIRECTION_PIN = 32;
const int MOTOR_ENABLE_PIN = 35;

// Create stepper object
ESP_FlexyStepper stepper;

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  Serial.println("Select a string (1-6) and then tuning direction (u/d):");

  // Initialize stepper motor
  stepper.connectToPins(MOTOR_STEP_PIN, MOTOR_DIRECTION_PIN);
  pinMode(MOTOR_ENABLE_PIN, OUTPUT);
  digitalWrite(MOTOR_ENABLE_PIN, LOW); // Disable by default

  stepper.setSpeedInStepsPerSecond(20); // Speed for system identification
  // Do not set acceleration

  Serial.println("System ready. Please select a string to tune.");
}

void loop() {
  if (Serial.available() > 0) {
    char input = Serial.read();
    if (input >= '1' && input <= '6') {
      stringToTune = input - '1';
      Serial.print("Selected string ");
      Serial.println(stringToTune + 1);
      delay(3000);  // Delay 3 seconds for positioning

      // Reference tone
      tone(BUZZER_PIN, 262, 1000);  // Tone duration 1 second
      delay(1000);  // Wait for tone to finish and collect samples

      Serial.println("Select tuning direction (u for up, d for down):");
    } else if (input == 'u' || input == 'd') {
      tuneUp = (input == 'u');
      Serial.print("Selected tuning direction: ");
      Serial.println(tuneUp ? "up" : "down");

      performSystemIdentification();
    }
  }
}

void performSystemIdentification() {
  double currentFrequency = measureFrequency();
  Serial.print("Initial tuned frequency: ");
  Serial.println(currentFrequency);

  Serial.println("Frequency response:");

  if (tuneUp) {
    while (currentFrequency < expectedFrequencies[stringToTune]) {
      digitalWrite(MOTOR_ENABLE_PIN, HIGH);
      stepper.moveRelativeInSteps(-20);  // Apply constant step input of 20 steps up
      digitalWrite(MOTOR_ENABLE_PIN, LOW);
      delay(500); // Wait for the system to settle

      currentFrequency = measureFrequency();
      Serial.print("Current frequency: ");
      Serial.println(currentFrequency);
    }
  } else {
    while (currentFrequency > expectedFrequencies[stringToTune]) {
      digitalWrite(MOTOR_ENABLE_PIN, HIGH);
      stepper.moveRelativeInSteps(20);  // Apply constant step input of 20 steps down
      digitalWrite(MOTOR_ENABLE_PIN, LOW);
      delay(500); // Wait for the system to settle

      currentFrequency = measureFrequency();
      Serial.print("Current frequency: ");
      Serial.println(currentFrequency);
    }
  }

  Serial.println("Target frequency reached or exceeded.");
}

double measureFrequency() {
  computeAverageSpectrum(3);
  return findPeakFrequency();
}

void computeAverageSpectrum(int numAverages) {
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

double findPeakFrequency() {
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

void readAnalogValues() {
  for (int i = 0; i < SAMPLES; i++) {
    microSeconds = micros();
    vReal[i] = (analogRead(34) * 3.3 / 4096.0 - 1.65);
    vImag[i] = 0;
    while (micros() < (microSeconds + samplingPeriod)) {}
  }
}
