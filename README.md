# Automatic-PCB-developing-machine
Automatic PCB developing machine
## Contents

- [Background](#background)
- [Build Guide](#build-guide)

## Background
I wanted to create a auto control viarium system that I did not have to monitor 24/7 and tend to all change from day to night for my Mom. It started out as a project to monitor the humidity. I want to maintain the right temperature and humidity for the plants. I detect temperature and humidity by sensors to control the humidifier and plant light.

## Environment

## Build Guide

### Hardware
- **Parts**

Picture | Name | Purpose
--------|------|---------
|![Piboard](/imgs/pi_board.png)|Raspberry Pi 4 Model B|Main CPU Board|
|![Adapter](/imgs/adapter.png)|Adapter|Power supply for Raspberry Pi 4 & Zero 2|
|![relay](/imgs/relay_s.png)|Power Relay Board|Switch Vibrator and Heater on or off|

- **Hardware Architecture**

![Alt text](/imgs/Hardware_arch.png)

- **Pin Define**

![Alt text](/imgs/pin_define.png)

### Software

- **Software Architecture**
![Alt text](/imgs/Softeare_arch.png)
