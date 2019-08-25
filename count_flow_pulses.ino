int flow_pin = 2; //must be an interrupt pin
volatile int pulseCount = 0;

// Adafruit 1/2" Liquid Flow Meter used
// This model outputs one pulse for approx. every 5.2 mL of water flowed 


void setup() {

  Serial.begin(9600);
  
  pinMode(flow_pin, INPUT); //set pin type
  digitalWrite(flow_pin, HIGH); //turn on pin

  attachInterrupt(digitalPinToInterrupt(2), pulseCounter, CHANGE); //detect change in pin
  
}

void loop() {

    detachInterrupt(digitalPinToInterrupt(2));

    int sec = millis() / 1000;

    //Print time since program started (seconds) and total number of pulses
    Serial.print("Seconds: " ); Serial.print(sec); Serial.print(" : ");
    Serial.println(pulseCount);
    delay(500);

    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(digitalPinToInterrupt(2), pulseCounter, CHANGE);
  
}

void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
