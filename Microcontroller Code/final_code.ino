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

// kp values for raising and lowering the pitch for each string
double kp_crestere[6] = {9.05, 10.56, 8.27, 6.56, 5.15, 6.12}; 
double kp_scadere[6] = {10.56, 11.52, 9.49, 6.63, 5.40, 8.15}; 

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  samplingPeriod = round(1000000 * (1.0 / SAMPLING_FREQUENCY));
  Serial.println("Selecteaza o coarda (apasati butonul corespunzator):");

  // Initialize stepper motor
  stepper.connectToPins(MOTOR_STEP_PIN, MOTOR_DIRECTION_PIN);
  pinMode(MOTOR_ENABLE_PIN, OUTPUT);
  digitalWrite(MOTOR_ENABLE_PIN, LOW);  // Motor driver disabled by default (used only when moving the motor)

  stepper.setSpeedInStepsPerSecond(400); // Set initial speed
  //stepper.setAccelerationInStepsPerSecondPerSecond(5000); // Setare acceleration (if needed)

  // Initialize button pins
  for (int i = 0; i < 6; i++) {
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
}

void loop() {
  // Check button press
  for (int i = 0; i < 6; i++) {
    if (digitalRead(buttonPins[i]) == LOW) {
      stringToTune = i;
      Serial.print("S-a selectat coarda ");
      Serial.println(stringToTune + 1);
      delay(3000);  // Delay 3 seconds for tuner positioning

      // Play C4 note to start sample collection (261.63 Hz)
      tone(BUZZER_PIN, 262, 1000);  // Lasts 1 second
      delay(1000);  // Wait for sound to finish and collect samples
    }
  }

  if (stringToTune != -1) {
    tuneString();
  }
}

void tuneString() {
  while (true) {
    computeAverageSpectrum(3);
    double dominantFrequency = findPeakFrequency();
    double targetFrequency = expectedFrequencies[stringToTune];
    double Difference = targetFrequency - dominantFrequency;

    Serial.print("Current frequency for string ");
    Serial.print(stringToTune + 1);
    Serial.print(": ");
    Serial.println(dominantFrequency);

    if (abs(Difference) <= 1) {  // Check if string is within ±1Hz of target frequency
      Serial.println("Coarda selectata este acordata.");
      tone(BUZZER_PIN, 349, 500);  // Play F4 note (string tuned)
      delay(500);
      tone(BUZZER_PIN, 262, 500);  // Play C4 note (string tuned)
      delay(500);

      stringToTune = -1;  // Resetare coarda pentru acordat
      Serial.println("Choose a string to tune (1-6):");
      break;
    }

    int StepsRequired;

    if (dominantFrequency < targetFrequency - 1) {
      Serial.println("Increasing string tension.");
      StepsRequired = Difference * kp_increase[stringToTune]; // Larger step size for increasing tension
      StepsRequired = -abs(StepsRequired); // Ensure steps are negative for raising tension
    } else if (dominantFrequency > targetFrequency + 1) {
      Serial.println("Decreasing string tension.");
      StepsRequired = Difference * kp_decrease[stringToTune]; // Smaller step size for decreasing tension
      StepsRequired = abs(StepsRequired); // Ensure steps are positive for lowering tension
    }

     // Debugging statements to check motor control values
    Serial.print("Pasi necesari: ");
    Serial.println(StepsRequired);

    // Activate motor driver, move to position, and disable driver
    digitalWrite(MOTOR_ENABLE_PIN, HIGH); // ENABLE driver
    stepper.setSpeedInStepsPerSecond(200); // Constant speed
    stepper.setAccelerationInStepsPerSecondPerSecond(5000); // Constant acceleration
    stepper.moveToPositionInSteps(stepper.getCurrentPositionInSteps() + StepsRequired);
    digitalWrite(MOTOR_ENABLE_PIN, LOW); // DISABLE driver
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
