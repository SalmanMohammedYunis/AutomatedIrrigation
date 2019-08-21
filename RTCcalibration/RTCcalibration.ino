/*
  This program calibrates the RTC module and sets the clock to the time obtained form the computer used to interface
  the Arduino at the time of compilation.
  This program will display the time of the RTC module in the serial monitor.

  Uncomment the call to log_to_SD function if the time reading is to be logged to an SD card for calibration purposes or for the
  lack of a serial monitor.

  PLEASE RUN THIS PROGRAM IN YOUR ARDUINO ONLY ONCE.
  Time shifts will result if the program is run multiple times, because the clock resets to the initial compile time of the program. 

  Library for the connected RTC module can be found at https://github.com/adafruit/RTClib
  Refer to Arduino documentation for how to add external libraries.
*/


#include <Wire.h>
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>

#define CHIP_SELECT 10

RTC_PCF8523 rtc;
File file;

void setup(){
    Serial.begin(9600);
    Serial.println("Program init ...");

    if(!rtc.begin()){
        Serial.println("RTC begin error");
        //error_blink(500);
    }
    if(!rtc.initialized()){
        Serial.println("RTC init error");
        //error_blink(500);
    }
    if(!SD.begin(CHIP_SELECT)){
        Serial.println("SD card init error");
    }
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
}

void loop(){
    DateTime time = rtc.now();
    Serial.print(time.dayOfTheWeek());
    Serial.print(" , ");
    Serial.print(time.year());
    Serial.print('/');
    Serial.print(time.month());
    Serial.print('/');
    Serial.print(time.day());
    Serial.print(" - ");
    Serial.print(time.hour());
    Serial.print(':');
    Serial.print(time.minute());
    Serial.print(':');
    Serial.print(time.second());
    Serial.println();
    //log_to_SD(time);
}

void log_to_SD(DateTime time){
    file = SD.open("timelog.txt",FILE_WRITE);
    if(file){
        file.print(time.dayOfTheWeek());
        file.print(" , ");
        file.print(time.year());
        file.print('/');
        file.print(time.month());
        file.print('/');
        file.print(time.day());
        file.print(" - ");
        file.print(time.hour());
        file.print(':');
        file.print(time.minute());
        file.print(':');
        file.print(time.second());
        file.println();

        file.close();
    }
    else{
        Serial.println("SD Card Write Error");
        error_blink(500);
    }
}

void error_blink(unsigned long interval_ms){
    pinMode(LED_BUILTIN,OUTPUT);
    Serial.println("System Error");
    while(true){
        digitalWrite(LED_BUILTIN,HIGH);
        delay(interval_ms);
        digitalWrite(LED_BUILTIN,LOW);
        delay(interval_ms); 
    }
}
