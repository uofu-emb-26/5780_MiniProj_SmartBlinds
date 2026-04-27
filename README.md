# 5780_MiniProj_SmartBlinds
Automatic Blind Opener Depending On Daylight/Night

## Group Members
Matthew Lee and Sean Moon

## Overview
This project implements an automatic blind system using an STM32 microcontroller. The system can operate in both **manual mode** (user-controlled) and **automatic mode** (light-based), opening or closing blinds depending on ambient lighting conditions.

## Features
- Manual open/close control using external buttons  
- Automatic control based on daylight (light sensor)  
- Mode switching using the STM32 USER button  
- PWM-based motor control  
- LCD display via I2C (real-time status)  
- UART debugging output  
- Emergency stop functionality  

## System Architecture

### States (FSM)
- STOP  
- OPENING  
- CLOSING  

### Modes
- **Manual Mode**: User controls blinds  
- **Auto Mode**: Light sensor controls blinds

## Hardware Connections
- User Button (PA0): Switches between manual and automatic mode
- Light Sensor (PA1): Detects ambient light (bright/dark)
- Motion Button (PB2): Toggles opening and closing in manual mode
- Stop Button (PB11): Immediately stops the motor
- PWM (PA4): Controls motor speed
- IN1 (PA5), IN2 (PA9): Control motor direction (open/close)
- LCD (PB6, PB7): I2C communication (SCL, SDA) for displaying mode and state
- UART (PC4, PC5): Sends debugging information to a computer
- NOTE:
  - All grounds (GND) must be connected together
  - Motor driver powered separately if needed

## System Operation
- **Manual Mode**:
  - Press the motion button → toggles between opening and closing
  - Press the stop button → immediately stops the motor
- **Auto Mode**:
  - Bright environment → blinds open
  - Dark environment → blinds close

## Setup Instructions
- Connect all components according to the pin configuration
- Ensure all devices share a common ground
- Upload the code to the STM32 board
- Connect UART to a computer (115200 baud)
- Power the system

## Usage
- Press the USER button → switch between Manual and Auto mode
- Use the motion button to control blinds in Manual mode
- System operates automatically based on light in Auto mode
