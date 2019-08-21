/*
  This program displays the reading obtained from the moisture sensors in the serial monitor.
  This program can be used to check and calibrate if the sensors are working correctly.
  Display format: 
     eg. 1) 587    2) 570    3) 539    4) 555    5) 579    6) 582    7) 584    8) 585    9) 587    10) 586
     preceding number identifies the sensor number. 
*/

#define SEN_COUNT 10


/**********************************************************************************************************/
/* Uncomment the system that is being tested, comment the rest */

const int sensor_input[SEN_COUNT] = {A14,A12,A4,A3,A15,A13,A5,A2,A1,A0};  //System A

//const int sensor_input[SEN_COUNT] = {A4,A0,A2,A1,A12,A13,A5,A3,A14,A15}; //System B

//const int sensor_input[SEN_COUNT] = {A2,A5,A12,A14,A3,A4,A13,A15,A1,A0};  //System C

//const int sensor_input[SEN_COUNT] = {A0};     //Single Sensor Test, set SEN_COUNT to 1

/********************************************************************************************************/

int i;
int reading_value;

void setup(){
    Serial.begin(9600);
}

void loop(){
    for(i = 0; i < SEN_COUNT; i++){
        reading_value = analogRead(sensor_input[i]);
        Serial.print(i + 1); Serial.print(") "); Serial.print(reading_value); Serial.print("    ");
        delay(50);
    }
    Serial.println();
}
