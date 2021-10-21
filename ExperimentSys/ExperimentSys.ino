/*
    The following program will check soil moisture levels with interval specifed by the variable (long_measure_interval_ms) 
    and once it goes above a treshold specifed by the variable (critical_high_treshold) it will 
    open the Solenoid valve for duration of time specifed by the variable (valve_open_interval_ms). 
    It will then start to take measurements within intervals specified by the variable (freq_measure_interval_ms) 
    as long as average moisture level reads above (critical_low_treshold) 
    and open the valve again if average moisture reading is above (critical_low_treshold).
    Once the average moisure reading drops below (critical_low_treshold), everything starts all over again.

    Data is logged to mounted SD Card in the file "SOIL_LOG.TXT" in the following format.
    Reading outside valid bounds specified by the array (valid_moisture_bounds), will be marked as error with the sensor number. 
      Format eg. (Mon) - 2019/7/16 - 16:45:33 - 450,489,500,479,344,455,477,455,399,500,477
                 (Mon) - 2019/7/16 - 16:45:33 - 450,ERROR(#2),500,475,455,477,455,ERROR(#8),500,477,489

    Formula used for flowed volume of water:
        Volume = (pulse_count * 5.2) / 1000.00
        where pulse_count is the output from flow meter. 

    Flow data is logged to mounted SD card in the file "FLOW_LOG.TXT" with the following format.
    Format eg.  (Mon) - 2019/8/14 - 11:48:27 - 0.774578931245896 Liters

    * Varaible that end with *_ms or *_MS indicate unit of time in milliseconds.

    Library for the connected RTC module can be found at https://github.com/adafruit/RTClib
    Refer to Arduino documentation for how to add external libraries.

*/

#include <Wire.h>
#include "RTClib.h"
#include <SPI.h>
#include <SD.h>

#define debug

#define MOS_SEN_COUNT 10       //Number of moisture sensors connected

#pragma region 

// Pin Assignments

/*********************************************************************************************************/
/* Select the System by uncommenting the system you are uploading the sketch to, Comment the rest */

/* SYSTEM A */
//const int moisture_sensor_input[MOS_SEN_COUNT] = {A14,A12,A4,A3,A15,A13,A5,A2,A1,A0};    //System A

/* SYSTEM B */
const int moisture_sensor_input[MOS_SEN_COUNT] = {A4,A0,A2,A1,A12,A13,A5,A3,A14,A15};     //System B

/* SYSTEM C */
//const int moisture_sensor_input[MOS_SEN_COUNT] = {A2,A5,A12,A14,A3,A4,A13,A15,A1,A0};  //System C

/******************************************************************************************************/

const int chipSelector = 10;          // chip selector for SD card connected through ICSP
const int valve_relay_ctrl = 4;          //Solenoid Relay control pin
const int flow_meter_input = 2;         // make sure this pin has interrupt (2,3,18,19,20,21 on the Arduino Mega)


// I/O stream pointers and utility variables
File water_flow_file;                      // file pointers for SD card
File soil_moist_file;
RTC_PCF8523 rtc;                          // Instance of Real Time Clock module

const char daysOfTheWeek[7][12] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
int i;  // iterator

/*****************************************************************************************************/
/* Select the System by uncommenting the system you are uploading the sketch to, Comment the rest */

/* SYSTEM A */
//const int critical_low_treshold = 350;                   // Moisture level to triger frequent measurements:SYSTEM A
//const int critical_high_treshold = 350;                 // Moisture level treshold to open valve : SYSTEM A

/* SYSTEM B */
const int critical_low_treshold = 350;                   // Moisture level to triger frequent measurements: SYSTEM B
const int critical_high_treshold = 350;                 // Moisture level treshold to open valve:SYSTEM B

/* SYSTEM C */
//const int critical_low_treshold = 350;                   // Moisture level to triger frequent measurements : SYSTEM C
//const int critical_high_treshold = 350;                 // Moisture level treshold to open valve :SYSTEM C

/**************************************************************************************************/


const unsigned long long_measure_interval_ms = 259200000;    //3 days interval
const unsigned long freq_measure_interval_ms = 300000;   //5 min interval
const unsigned long one_hour_interval_ms = 3600000;   //5 min interval

const int valid_moisture_bounds[2] = {250,600};         // Upper and lower bounds for valid moisture readings

int moisture_readings[MOS_SEN_COUNT];              //Array to store moisture readings
bool sensor_err_flags[MOS_SEN_COUNT];             // To keep track of valid moisture readings inside specifed bounds
int sen_read_count = MOS_SEN_COUNT;              // Number of valid moisture readings.
double flowed_water_vol;                            // variable to store flowed volume of water

const unsigned long valve_open_interval_ms = 30000;             // specifies how long the valve should stay open, 30 sec
volatile unsigned long pulse_count = 0;                     // Used counting pulses form the output of flow meter

#pragma endregion

void setup() {

  pinMode(valve_relay_ctrl,OUTPUT);

  /*
    The flow meter in use outputs pulses according to the volume of water that has flowed
    through it. This program raises interrupts for every rising edge of said pulse.
    Refer to the funcion (pulse_increase_ISR) , for what takes place when interrupts are raised.

  */
  attachInterrupt(digitalPinToInterrupt(flow_meter_input),pulse_increase_ISR,RISING);
   

  #ifdef debug
  Serial.begin(9600);
  Serial.println("Program Init ...");   
  #endif

  /* 
    Check the presence and conditinon of SD card and RTC module and give warning on the 
    Serial Monitor in debug mode.
    Shoud errors occure here, inorder to make those error fatal uncomment call to (error_blink) function.
    This will blink the builtin LED on the Arduino board and everything will halt.
  */

  if (!rtc.begin()) {
    #ifdef debug
    Serial.println("Couldn't find RTC");
    #endif
    //error_blink(1000);       // Stop doing everything else and show error blinking
  }

  if(!rtc.initialized()){
    #ifdef debug
    Serial.println("RTC Not Initialized");
    #endif
    //error_blink(1000);       // Stop doing everything else and show error blinking
  }

  if(!SD.begin(chipSelector)){
    #ifdef debug
    Serial.println("SD card Init Failed");
    #endif
    //error_blink(1000);       // Stop doing everything else and show error blinking
  }
  
}

void loop() {
  do{

    /*
      This loop will commence as long as average moisture reading is below (critical_high_treshold),
      This indicated that the soil is moist enough, and all it does is log the readings to the mounted SD card 
      with the interval specifed by long_measure_interval_ms.(Default value is 2 hours).

    */

    #ifdef debug
    Serial.println("Checking Moisture ...");
    #endif
    int long_measure_interval_hrs = long_measure_interval_ms / (1000*60*60); //convert long interval ms to hrs
    
    sen_read_count = MOS_SEN_COUNT;

    for(i = 0;i<long_measure_interval_hrs;i++){
      /*
      This loop writes the sensor values to a memory each hour 
      */
      delay(one_hour_interval_ms);
      for(i = 0;i < MOS_SEN_COUNT;i++){
      if(read_moisture(i) < valid_moisture_bounds[0] || read_moisture(i) > valid_moisture_bounds[1]){
        moisture_readings[i] = 0;
        sensor_err_flags[i] = false;
        sen_read_count--;
      }
      else{
        moisture_readings[i] = read_moisture(i);
        sensor_err_flags[i] = true;
      } 
    }
      log_soil_str_SD("Hourly: ");
      log_soil_SD(rtc.now(),moisture_readings,sensor_err_flags);
    }

  }
  while(compute_average(moisture_readings,MOS_SEN_COUNT,sen_read_count) < critical_high_treshold);

  /*
      Exit from the above loop implies that,
      Average moisture reading has gone above (critical_high_treshold) i.e. Soil is getting dry 
      and valve needs to be opened.
      Valve will be opened for duration of (valve_open_interval_ms) and flowed water volume will be loged to 
      "flow_log.txt" in SD card.
  */

  open_solenoid_valve_timed(valve_open_interval_ms,flowed_water_vol);
  #ifdef debug
  Serial.print("Water supplied.");
  Serial.print("Flowed Water: ");Serial.println(flowed_water_vol);  
  #endif
  log_flow_SD(rtc.now(),flowed_water_vol);


  /*  Start taking frequent moisure measurements within intervals of (freq_measure_interval_ms)
      and open valve again if moisture level is above (critical_low_treshold). once value drops below 
      (critical_low_treshold) it implies that soil is moist enough.
      And log moisture reading into "SOIL_LOG.TXT" and flowed water in "FLOW_LOG.TXT" in mounted SD card.

  */
  
  #ifdef debug
  Serial.println("Frequent measurements started ...");
  #endif  

  while(1){
    delay(freq_measure_interval_ms);
    sen_read_count = MOS_SEN_COUNT;
    for(i = 0;i < MOS_SEN_COUNT;i++){
      if(read_moisture(i) < valid_moisture_bounds[0] || read_moisture(i) > valid_moisture_bounds[1]){
        moisture_readings[i] = 0;
        sensor_err_flags[i] = false;
        sen_read_count--;
      }
      else{
        moisture_readings[i] = read_moisture(i);
        sensor_err_flags[i] = true;
      } 
    }

    log_soil_str_SD("Freq: ");
    log_soil_SD(rtc.now(),moisture_readings,sensor_err_flags);

    if(compute_average(moisture_readings,MOS_SEN_COUNT,sen_read_count) > critical_low_treshold){
      open_solenoid_valve_timed(valve_open_interval_ms,flowed_water_vol);
      #ifdef debug
      Serial.print("Water supplied.");
      Serial.print("Flowed Water: ");Serial.println(flowed_water_vol);  
      #endif
      log_flow_SD(rtc.now(),flowed_water_vol);
    }
    else{
      break;
    }
  }   
}

 /*  Takes in an array of integers and computes the average of its elements */
int compute_average(int *reading_arr,int len,int valid_count){         
  int sum = 0;        //Potential Memory Leak! - due for optimization later ;-)
  int avg_i;      // Loop Iterator

  for(avg_i = 0;avg_i < len; avg_i++){
    sum += reading_arr[avg_i];
  }
  return (int)(sum / valid_count);
}

/* Reads the moisture level form specified sensor, first sensor is 0 */
int read_moisture(int sensor_no){
  return analogRead(moisture_sensor_input[sensor_no]);
}


/*
  open the Solenoid valve,
  params: duration_ms - the time duration the valve statys open in milliseconds
          flowed_vol - Pass by refrence, holds the volume of water flowed.
*/
void open_solenoid_valve_timed(unsigned long duration_ms,double &flowed_vol){

  flowed_vol = 0.00;
  pulse_count = 0;
  digitalWrite(valve_relay_ctrl,HIGH);

  #ifdef debug
  Serial.println("Valve opened in time mode ...");
  #endif 

  delay(duration_ms);
  digitalWrite(valve_relay_ctrl,LOW);
  flowed_vol = (pulse_count * 5.2) / 1000.00;

  #ifdef debug
  Serial.println("Valve Closed.");
  Serial.print("Pulses: "); Serial.print(pulse_count);
  Serial.print("  , Liters: "); Serial.println(flowed_vol);
  #endif

}


/*
  logs data regarding water flow to SD card
  Format eg. (Mon) - 2019/7/16 - 16:45:33 - 450,0.77 Liters
*/
void log_flow_SD(DateTime time_input,double flowed_water){
  water_flow_file = SD.open("flow_log.txt",FILE_WRITE);
  if(water_flow_file){
    // Formating data that's to be logged to SD card
    #ifdef debug
    Serial.print("Logging to Flow file in SD card ...");
    #endif

    water_flow_file.print("(");
    water_flow_file.print(daysOfTheWeek[time_input.dayOfTheWeek()]);
    water_flow_file.print(") - ");
    water_flow_file.print(time_input.year(), DEC);
    water_flow_file.print('/');
    water_flow_file.print(time_input.month(), DEC);
    water_flow_file.print('/');
    water_flow_file.print(time_input.day(), DEC);
    water_flow_file.print('-');
    water_flow_file.print(time_input.hour(), DEC);
    water_flow_file.print(':');
    water_flow_file.print(time_input.minute(), DEC);
    water_flow_file.print(':');
    water_flow_file.print(time_input.second(), DEC);
    water_flow_file.print("  -  ");                // to be used as a delimiter
    water_flow_file.print(flowed_water,DEC);    // log flow meter reading
    water_flow_file.print(" Liters");
    water_flow_file.println();
  
    water_flow_file.close();                  

    #ifdef debug
    Serial.print("Done.");
    Serial.println();
    #endif
  }
  else{
    #ifdef debug
    Serial.println("SD Card Error : Flow file");
    #endif
    //error_blink(500);
  }
}

/*
  Logs data regarding soil moisture level into SD card
  Format eg. (Mon) - 2019/7/16 - 16:45:33 - 450,489,500,479
             (Mon) - 2019/7/16 - 16:45:33 - 450,ERROR(#2),500,475
*/
void log_soil_SD(DateTime time_input,int *moist_reads,bool *err_flags){
  soil_moist_file = SD.open("soil_log.txt",FILE_WRITE);
  if(soil_moist_file){
    #ifdef debug
    Serial.print("Logging to Soil file into SD card ...");
    #endif
    soil_moist_file.print("(");
    soil_moist_file.print(daysOfTheWeek[time_input.dayOfTheWeek()]);
    soil_moist_file.print(") - ");
    soil_moist_file.print(time_input.year(), DEC);
    soil_moist_file.print('/');
    soil_moist_file.print(time_input.month(), DEC);
    soil_moist_file.print('/');
    soil_moist_file.print(time_input.day(), DEC);
    soil_moist_file.print('-');
    soil_moist_file.print(time_input.hour(), DEC);
    soil_moist_file.print(':');
    soil_moist_file.print(time_input.minute(), DEC);
    soil_moist_file.print(':');
    soil_moist_file.print(time_input.second(), DEC);
    soil_moist_file.print("  -  ");                // to be used as a delimiter

    int valid_sen_reads = 0;
    int log_i;
    for(log_i = 0;log_i < MOS_SEN_COUNT;log_i++){
      if(err_flags[log_i]){
        soil_moist_file.print(moist_reads[log_i],DEC);
        soil_moist_file.print(',');                // to be used as a delimiter
        valid_sen_reads++;
      }
      else{
        soil_moist_file.print(" ERROR(#");
        soil_moist_file.print(log_i + 1,DEC);
        soil_moist_file.print(") ");
        soil_moist_file.print(',');                // to be used as a delimiter
      }
    }
    soil_moist_file.print(compute_average(moist_reads,MOS_SEN_COUNT,valid_sen_reads),DEC);
    soil_moist_file.println();
    soil_moist_file.close();
    
    #ifdef debug
    Serial.print("Done.");
    Serial.println();
    #endif
  }
  else{
    #ifdef debug
    Serial.println("SD Card Error : Soil file");
    #endif
    //error_blink(500);
  }
}

/*
    Logs a simple string to the SD card.
*/
void log_soil_str_SD(char *str){
  soil_moist_file = SD.open("soil_log.txt",FILE_WRITE);
  if(soil_moist_file){
    soil_moist_file.print(str);
    soil_moist_file.close();
  }
  else{
    #ifdef debug
    Serial.println("SD Card Error");
    #endif
    //error_blink(500);
  }
}

/*
    At the event of an error, this funcion makes the builtin LED 
    of the Arduino, blink within (interval_ms) milliseconds and all other actions come to halt
*/
void error_blink(int interval_ms){
  #ifdef debug
  Serial.println("System Error");
  #endif                 
  pinMode(LED_BUILTIN, OUTPUT);
  while(1){
    digitalWrite(LED_BUILTIN,HIGH);
    delay(interval_ms);
    digitalWrite(LED_BUILTIN,LOW);
    delay(interval_ms);   
  } 
}

 /* ISR(Interrupt Service Routine) for flow meter interrupts
    Increase (pulse_count) by 1, for every interrupt raised */
void pulse_increase_ISR(){   
  pulse_count++;
}
