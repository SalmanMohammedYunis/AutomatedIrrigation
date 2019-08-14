/* 
  The following program is used for a control setup.
  It will open the solenoid valve with the interval of time specfied by the variabel (VALVE_OPEN_INTERVAL_MS)
  once open the valve is open it will close when water volume specfied by (VALVE_OPEN_VOLUME) in liters has flowed.

  Formula used for flowed volume of water:
        Volume = (pulse_count * 5.2) / 1000.00
        where pulse_count is the output from flow meter.

  * Varaible that end with *_ms or *_MS indicate unit of time in milliseconds.
  
*/
#define debug

#pragma region 

//Pin Assignments
#define VALVE_RELAY_CTRL 4          // The pin connected to the control pin of the relay that drives the solenoid valve
#define FLOW_METER_INPUT 2         // make sure this pin has interrupt (pins 2,3,18,19,20,21 on the Arduino Mega and 2,3 on Arduino UNO)


//I/O and Utility Variables
double flowed_water_volume;                  //to store measured volume of water
volatile unsigned long pulse_count = 0;         // Used counting pulses form the output of flow meter

const unsigned long VALVE_OPEN_INTERVAL_MS = 14400000;    //The Interval between valve open commands,14400000 (4hrs)
const double VALVE_OPEN_VOLUME = 0.5;                   // volume of water in liters that should flow when valve opens 
const unsigned long WATER_SPREAD_TIME_MS = 10000;     //Time for water to percolate throughout soil

#pragma endregion

void setup(){

    pinMode(VALVE_RELAY_CTRL,OUTPUT);

    /*
    The flow meter in use outputs pulses according to the volume of water that has flowed
    through it. This program raises interrupts for every rising edge of said pulse.
    Refer to the funcion (pulse_increase_ISR) , for what takes place when interrupts are raised.

    */
    attachInterrupt(digitalPinToInterrupt(FLOW_METER_INPUT),pulse_increase_ISR,RISING);

    #ifdef debug
    Serial.begin(9600);
    Serial.println("Program Init ...");   
    #endif
    
}

/*
        wait for (VALVE_OPEN_INTERVAL_MS) -> Open Valve -> Wait for (WATER_SPREAD_TIME_MS)
 */
void loop(){
    delay(VALVE_OPEN_INTERVAL_MS);    
    open_valve_volume(VALVE_OPEN_VOLUME,flowed_water_volume);
    delay(WATER_SPREAD_TIME_MS);    
}

/*
  open the Solenoid valve,
  params: water_volume - the volume of water required to be supplied to the soil
          flowed_volume - Pass by refrence, holds the volume of water flowed.
*/
void open_valve_volume(double water_volume, double &flowed_volume){
  
  //Reset and clean values from previous call.(Since these are global Variables).
  pulse_count = 0;
  flowed_volume = 0.00;

  digitalWrite(VALVE_RELAY_CTRL,HIGH);      // Open valve

  #ifdef debug
  Serial.println("Valve Opened in volume mode ...");
  #endif

  while(((pulse_count * 5.2) / 1000.00) < water_volume);  // Do noting until water_volume has flowed

  digitalWrite(VALVE_RELAY_CTRL,LOW);       // Close Valve
  flowed_volume = (pulse_count * 5.2) / 1000.00;

  #ifdef debug
  Serial.println("Valve Closed.");
  Serial.print("Pulses: "); Serial.print(pulse_count);
  Serial.print(" , ");
  Serial.print("Volume: "); Serial.print(flowed_volume); Serial.println(" Liters");
  #endif

}


/* Increase (pulse_count) by 1, for every interrupt raised */
void pulse_increase_ISR(){    //ISR for flow meter interrupts
  pulse_count++;
}
