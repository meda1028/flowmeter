# flowmeter
C/Arduino code for Projecting Hall-Sensor Data on a Nextion TFT Display

## Features

Page 1 SensorData

* Display Current ml/s from Waterflow Sensor

* Display Average ml/s from Waterflow Sensor

* Display total ml from Waterflow Sensor

Page 2 Graph

* Display Current ml/s from Waterflow Sensor

* Display Average ml/s from Waterflow Sensor

Page 3 Scoreboard

* Display The last 10 maximum Amount of ml/s

* R/W and Stored on the Arduino EPROM 

Page 4 CalibrationFactor Slider

* Set the CalibrationFactor to get the exact amount of ml for each Sensor


## Pin-Layout
Arduino - Display - YF-B6

RX        TX      

TX        RX

DIG2                Data

DIG3                GND

DIG4                5V (HIGH)

5V        5V

GND       GND


### Folder Structure

    .
    ├── flowmetertft.ino        # Arduino C - File for communicating between the Sensor and the TJC Display
    ├── 164_1_TJC.HMI           # TJC Display Configuration File for the UI
    ├── font_STATTRACK.zi       # large font file for the Display Configuration
    ├── font_STATTRACK_24.zi    # small font file for the Display Configuration
    ├── flowmeter.ino           # Arduino C - File for no Display Mode
    ├── LICENSE
    └── README.md



## Hardware
Board:        ELEGOO UNO R3

Display:      TJC3224T024_011

Hall_Sensor:  Water Flow Sensor YF-B6
