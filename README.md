
## Overview

This automated guitar tuner was designed to simplify the tuning process by using a piezoelectric sensor, signal processing, and a stepper motor controlled via an ESP32 microcontroller. The device captures the frequency produced by the vibration of guitar strings, filters the useful signal from noise, identifies the dominant frequency, and automatically adjusts the string tension to achieve the desired pitch.

### Project Details

-   **Author:** Tudor-Claudiu SCOARȚĂ
    
-   **Supervisor:** SL. dr. ing. Gabriel HARJA
    
-   **Institution:** Technical University of Cluj-Napoca, Faculty of Automation and Computers
    
-   **Year:** 2024
    

## Motivation

In modern musical performance, keeping a guitar correctly tuned is essential for harmonious sound quality. Traditional tuning methods often rely on manual adjustment, which can be time-consuming and prone to errors, especially for beginners. This project addresses the challenge by offering an automated tuning solution that ensures accuracy and convenience.

## Objectives

1.  **Signal Acquisition:** Use a piezoelectric sensor to read analog signals from vibrating guitar strings.
    
2.  **Signal Processing:** Filter the input signals to remove noise, amplify the useful frequency range, and center the signal for ADC compatibility.
    
3.  **Frequency Analysis:** Implement Fast Fourier Transform (FFT) for identifying the dominant frequency.
    
4.  **Control System:** Utilize a stepper motor controlled by the ESP32 microcontroller to adjust string tension based on the detected frequency.
    
5.  **Automation:** Automatically tune the guitar to the desired pitch with minimal iterations.
    

## System Architecture

The system is divided into two main components, each comprising several technical subunits that ensure the correct acquisition, processing, and control necessary for automatic guitar tuning.

### Frequency Detection Mechanism:

1.  **Sensor:**
    
    -   A piezoelectric sensor captures the vibrations from the guitar strings, producing an analog voltage proportional to the vibration amplitude.
**Piezoelectric sensor:**
![Image](https://github.com/user-attachments/assets/49bb4ae1-f0d0-47dc-a09f-9d5ce51295b6)
        
    -   The signal is sensitive to environmental noise, necessitating precise filtering to obtain accurate frequency data.
        
2.  **Signal Conditioning Circuit:**
    
    -   Uses a 6th-order low-pass Sallen-Key filter to eliminate high-frequency noise.
        
    -   Employs cascading filters to enhance the filtering effect, resulting in a clean signal.
        
    -   Amplification is performed using an operational amplifier with a gain factor of G=25, which boosts the signal level suitable for ADC conversion.
        
    -   Signal recentering is achieved to maintain a stable mid-level voltage (1.65V), essential for ADC linearity.
        
    -   A precision rectifier circuit limits the signal between 0 and 3.3V to match the input range of the ESP32 ADC.
        
**LTSpice simulation of the Signal Conditioning Circuit:**
![Image](https://github.com/user-attachments/assets/8be7339c-e634-4404-9214-4d8bdce81902)

**Simulated 20Hz frequency over a 120 ms interval before filtering, as well as after passing through 2nd, 4th, and 6th order filters (LTSpice):**  
![Image](https://github.com/user-attachments/assets/880634fd-7edf-4f7c-8e0d-2e503e0d7356)

**Real 20 Hz frequency over a 120 ms interval before filtering, as well as after passing through 2nd, 4th, and 6th order filters (4 channels oscilloscope):**  
![Image](https://github.com/user-attachments/assets/338383f5-bcf6-483a-a32c-b9e11b27334a)

**Simulated 82.41 Hz frequency over a 24 ms interval before filtering, as well as after passing through 2nd, 4th, and 6th order filters (LTSpice):**  
![Image](https://github.com/user-attachments/assets/3a74229a-f175-45d3-938b-206b8098e8d8)

**Real 82.41 Hz frequency over a 24 ms interval before filtering, as well as after passing through 2nd, 4th, and 6th order filters (4 channels oscilloscope):**  
![Image](https://github.com/user-attachments/assets/80b3959d-0cf3-4b1a-86d1-8e1fb1380e37)

**Simulated 329.63 Hz frequency over a 6 ms interval before filtering, as well as after passing through 2nd, 4th, and 6th order filters (LTSpice):**  
![Image](https://github.com/user-attachments/assets/a0489d90-ecc9-467b-a072-a5b614f396de)

**Real 329.63 Hz frequency over a 6 ms interval before filtering, as well as after passing through 2nd, 4th, and 6th order filters (4 channels oscilloscope):**  
![Image](https://github.com/user-attachments/assets/b2eda27c-24f8-4b11-9879-ad846996d1e2)

**Simulated 2041 Hz frequency over a 1.2 ms interval before filtering, as well as after passing through 2nd, 4th, and 6th order filters (LTSpice):**  
![Image](https://github.com/user-attachments/assets/547616b3-7fd9-4804-823d-341cf184a65d)

**Real 2041 Hz frequency over a 1.2 ms interval before filtering, as well as after passing through 2nd, 4th, and 6th order filters (4 channels oscilloscope):**  
![Image](https://github.com/user-attachments/assets/a573d7da-cb76-492b-bafd-eeb77ecf8ded)

**Simulated 4000 Hz frequency over a 1.2 ms interval before filtering, as well as after passing through 2nd, 4th, and 6th order filters (LTSpice):**  
![Image](https://github.com/user-attachments/assets/9b134617-54d9-4a39-9ee0-a933cbd21225)

**Real 4000 Hz frequency over a 1.2 ms interval before filtering, as well as after passing through 2nd, 4th, and 6th order filters (4 channels oscilloscope):**  
![Image](https://github.com/user-attachments/assets/1c375f92-9e4a-4fba-9ca3-165f93994ea4)

**Implemented circuit board:**  
![Image](https://github.com/user-attachments/assets/b901de56-adfe-4d81-a8b5-a7b5befb9861)

3.  **Frequency Analysis:**
    
    -   The signal is processed using the Fast Fourier Transform (FFT) to identify the dominant frequency component from the conditioned analog signal.
        
    -   FFT was chosen over the Zero Crossing method for its robustness in noisy environments and its ability to provide detailed spectral analysis.
        

### Control Mechanism:

1.  **Motor Control:**
    
    -   Utilizes a NEMA 17 stepper motor driven by an A4988 motor driver.
        
    -   The ESP32 sends control signals to the driver, which then regulates the motor's movement to increase or decrease string tension.
        
2.  **Control Algorithm:**
    
    -   The proportional control algorithm (P-controller) minimizes the difference between the actual frequency and the target frequency.
        
    -   Two controllers (kp1=5.15 and kp2=11.52) were tested to optimize response time and accuracy.
        
    -   The system performs automatic tuning within three iterations, efficiently adapting even in noisy environments.

The system can be divided into two main components:

1.  **Frequency Detection Mechanism:**
    
    -   Uses a piezoelectric sensor to detect string vibrations.
        
    -   Analog signal is conditioned using a low-pass Sallen-Key filter to remove high-frequency noise.
        
    -   The signal is amplified and centered using operational amplifiers.
        
    -   The processed signal is digitized using the ADC of the ESP32 microcontroller.
        
2.  **Control Mechanism:**
    
    -   The detected frequency is compared to the reference frequency.
        
    -   The control algorithm adjusts the tension using a stepper motor, driven by an A4988 motor driver.
        
    -   Uses proportional control (P-controller) to minimize the difference between detected and desired frequencies.

## Process Identification
1.  **Identification Results:**
**(red is for lowering tension, blue is for increasing tension and the dotted black line is the ideal response)**


**E string:**  
![Image](https://github.com/user-attachments/assets/12e21f7b-40f6-4dea-86a8-afc050832350)

**A string:**  
![Image](https://github.com/user-attachments/assets/f7c3f002-f8df-44bb-8b75-4265d1c58b79)

**D string:**  
![Image](https://github.com/user-attachments/assets/11477b42-8077-41a3-8dd7-02345d7aacaf)

**G string:**  
![Image](https://github.com/user-attachments/assets/6aa34892-d47b-4e9a-9715-bc7fe4a53ef3)

**B string:**  
![Image](https://github.com/user-attachments/assets/89eec59d-a2df-46a2-bb89-ae2f5655fecf)

**e string:**   
![Image](https://github.com/user-attachments/assets/03d2ca15-0b9f-4de4-834b-64e9dd31b586)


2.  **Controller Results:**
**(red is for a very high kp value, blue is for a lower kp value and the dotted black line is the individual kp for increasing and the kp for decreasing each string's tension)**

**E string:**  
![Image](https://github.com/user-attachments/assets/431ffe6f-a1b6-419b-9a47-59c7bd450f77)

**A string:**  
![Image](https://github.com/user-attachments/assets/ce777b6a-6099-4ea7-ba37-e967fee4461e)

**D string:**  
![Image](https://github.com/user-attachments/assets/541ea8b5-75eb-484b-bfa9-eb4a96f72114)

**G string:**  
![Image](https://github.com/user-attachments/assets/2076e839-5fec-4667-a1a4-31416d2da61a)

**B string:**  
![Image](https://github.com/user-attachments/assets/a38a357e-ccf2-4f90-9b26-a0464c3a6041)

**e string:**   
![Image](https://github.com/user-attachments/assets/09f782db-3ebe-4216-9341-4a6bd3162211)


## 3D Printed Models
1.  **Gripper (Fusion360 drawing):**
![Image](https://github.com/user-attachments/assets/f606a982-8d4f-4bf7-aa8b-5c17109b1d7c)
1.  **Gripper (3D printed):**
![Image](https://github.com/user-attachments/assets/bd652e47-7b72-4a80-8a88-3f8f3d74c518)
1.  **Case (Fusion360 drawing):**
![Image](https://github.com/user-attachments/assets/6a9adc60-f700-4d9a-ad36-204f0c2fee83)
1.  **Case (3D printed):**
![Image](https://github.com/user-attachments/assets/4c3cd770-b0e4-4e61-ac67-73ba68c12b09)

## Features

-   Automatic detection and adjustment of guitar string tension.
    
-   Real-time frequency analysis using FFT.
    
-   Accurate tuning within three iterations.
    
-   Operates efficiently in noisy environments.
    
-   User-friendly interface with visual and audio feedback.
    

## Technologies Used

-   **Microcontroller:** ESP32
    
-   **Signal Processing:** Fast Fourier Transform (FFT)
    
-   **Motor Control:** NEMA 17 stepper motor, A4988 driver
    
-   **Signal Conditioning:** Sallen-Key low-pass filter
    
-   **Programming Language:** C 
    
-   **Development Environment:** Arduino IDE, Matlab and LTspice 

**Circuit Schematic**
![Image](https://github.com/user-attachments/assets/74e69162-70e2-4da4-8d50-c0ffcb50b035)
    

## Results

The final prototype achieved the following:

-   **Accuracy:** The device tunes the guitar within ±1 Hz of the desired frequency.
    
-   **Speed:** Automatic tuning is completed in a maximum of four iterations.
    
-   **Stability:** The system remains consistent even under noisy conditions due to effective signal filtering and robust frequency analysis.
    
-   **User Experience:** The automated tuning process is significantly faster and more accurate compared to manual methods, especially beneficial for beginners and professionals in live settings.

![Image](https://github.com/user-attachments/assets/e4056d48-c5f0-4bac-98f4-2cfc847252d5)
    


The system demonstrates precise tuning of guitar strings with minimal iterations, achieving accurate frequency detection and adjustment even in the presence of ambient noise. The filtering and amplification stages ensure a high-fidelity signal, allowing the motor control algorithm to adjust the string tension accurately.

## Future Enhancements

-   Integrate a user interface for customizing tuning settings.
    
-   Add support for multiple tuning presets.
    
-   Improve power efficiency and portability.
    
-   Implement wireless connectivity for remote control and monitoring.
    

## Acknowledgements

Special thanks to SL. dr. ing. Gabriel HARJA for the guidance and support throughout the project development.

## License

This project is released under the MIT License.
