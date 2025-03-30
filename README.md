# iot-rat-repeller

This project presents an innovative solution for rodent deterrence by developing an **IoT-based rat repeller prototype**. The device is designed as a **proof of concept** for future advancements in rodent control technology.  

## Overview  
The system utilizes **ESP32-CAM, a PIR sensor, and a buzzer** to detect rodent activity and respond with preventive actions.  

- The **PIR sensor** detects movement when a rat enters the monitored area.  
- Once detected, the **buzzer activates**, emitting ultrasonic sounds to repel the rat.  
- Simultaneously, the **ESP32-CAM's OV2640 camera** begins live streaming, accessible via a web interface.  
- A **flashlight** is triggered during nighttime, determined using the **Network Time Protocol (NTP)** for real-time day-night detection.  

This system aims to **effectively deter rodents** while serving as a foundation for future enhancements in pest control technology.  

## Tech Stack  
- **Hardware:** ESP32-CAM, PIR sensor, Buzzer  
- **Camera:** OV2640 (ESP32-CAM)  
- **Live Streaming:** Web-based access  
- **Time-Based Control:** Network Time Protocol (NTP)  
- **Programming:** C++ (Arduino framework)  

## Features  
- **Automatic Rodent Detection**: Detects movement using the PIR sensor.  
- **Ultrasonic Repelling**: Activates a buzzer to emit high-frequency sounds.  
- **Live Camera Feed**: Streams real-time video via the ESP32-CAM module.  
- **Nighttime Illumination**: Enables the flashlight only during dark hours.

## Results?
Check this journal for final tests and results: https://pepadun.fmipa.unila.ac.id/index.php/jurnal/article/view/239
