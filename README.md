# Automatic-PCB-developing-machine
Automatic PCB developing machine
## Contents

- [Background](#background)
- [Build Guide](#build-guide)

## Background
I wanted to create an automatic PCB development machine which can speed up PCB mass production with ultra sonic vibrator and heator. I could use Respbarry to monitor the temperature and water tubidity to control this system. This system also have web server and tcp server which could be controlled remotely. 

## Environment

## Build Guide

### Hardware
- **Parts**

Picture | Name | Purpose
--------|------|---------
|![Piboard](/imgs/pi_board.png)|Raspberry Pi 4 Model B|Host CPU Board|
|![Zero2](/imgs/zero_2.png)|Raspberry Pi Zero 2 W|System Control Board|
|![Adapter](/imgs/adapter.png)|Adapter|Power supply for Raspberry Pi 4 & Zero 2|
|![relay](/imgs/relay_s.png)|Power Relay Board|Switch Vibrator and Heater on or off|
|![ads](/imgs/ads.png)|ADC Convertor Board|Convert Analog Signal to Digital|
|![tubidity](/imgs/turbidity.png)|Turbidity Sensor|Turbidity Detect|
|![temp](/imgs/temp_sensor.png)|Temperature Sensor|Temperature Detect|
|![ulb](/imgs/ul_control.png)|Ultra Sonic Control Board|Ultra Sonic and Heator Control|
|![heat](/imgs/heator.png)|Heator|Heating|
|![vib](/imgs/vibrator.png)|Ultra Sonic Vibrator|Vibrating|
|![board](/imgs/Breadboard_s.png)|Bread Board|Creat Circuit|
|![extension](/imgs/extension_s.png)|GPIO Extension Board|GPIO Extension|



- **Hardware Architecture**

![Alt text](/imgs/Hardware_arch.png)

- **Pin Define**

![Alt text](/imgs/pin_define.png)

### Software

- **Software Architecture**
![Alt text](/imgs/Softeare_arch.png)
