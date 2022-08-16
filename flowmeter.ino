/*
Vcc5V(RED) Gnd(BLACK)
signal: pin 2.
 */
 
byte statusLed    = 13;
byte sensorInterrupt = 0;  // 0 = digital pin 2
byte sensorPin       = 2;
 
// 1-30 L/min F=6.68Q +- 5% (Q=L/min) YF-B6
float calibrationFactor = 5.3;
 
volatile byte pulseCount;  
 
float flowRate;
unsigned int flowMilliLitres;
unsigned long totalMilliLitres;
 
unsigned long oldTime;
 
void setup()
{
 
  // Serielle Übertragung an Host
  Serial.begin(9600); //38400 BAUD
   
  // status LED line as an output
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);  //active-low LED
 
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH);
 
  pulseCount        = 0;
  flowRate          = 0.0;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  oldTime           = 0;
 
  // pin 2 mit interrupt 0
  // Der Hurensohn triggert bei fallender Flanke
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
}
 
/**
 * Main
 */
void loop()
{
   
   if((millis() - oldTime) > 1000)    // Zählt einmal pro Sekunde
  {
    // flow rate berchnen und an host übermittlen Interupt deaktivieren
    detachInterrupt(sensorInterrupt);
       
    // Because this loop may not complete in exactly 1 second intervals we calculate
    // the number of milliseconds that have passed since the last execution and use
    // that to scale the output. We also apply the calibrationFactor to scale the output
    // based on the number of pulses per second per units of measure (litres/minute in
    // this case) coming from the sensor.
    flowRate = ((1000.0 / (millis() - oldTime)) * pulseCount) / calibrationFactor;
   
    // Note the time this processing pass was executed. Note that because we've
    // disabled interrupts the millis() function won't actually be incrementing right
    // at this point, but it will still return the value it was set to just before
    // interrupts went away.
    oldTime = millis();
   
    // Divide the flow rate in litres/minute by 60 to determine how many litres have
    // passed through the sensor in this 1 second interval, then multiply by 1000 to
    // convert to millilitres.
    flowMilliLitres = (flowRate / 60) * 1000;
   
    // Add the millilitres passed in this second to the cumulative total
    totalMilliLitres += flowMilliLitres;
     
    unsigned int frac;
   
    // Print the flow rate for this second in litres / minute
    Serial.print("Flow rate: ");
    Serial.print(int(flowRate));  // Print the integer part of the variable
    Serial.print(".");             // Print the decimal point
    String command = "output.txt=\""+String(flowRate)+"\"";
    Serial.print(command);
    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);
    
    // Determine the fractional part. The 10 multiplier gives us 1 decimal place.
    frac = (flowRate - int(flowRate)) * 10;
    Serial.print(frac, DEC) ;      // Print the fractional part of the variable
    Serial.print("L/min");
    // Print the number of litres flowed in this second
    Serial.print("  Current Liquid Flowing: ");             // Output separator
    Serial.print(flowMilliLitres);
    Serial.print("mL/Sec");
 
    // Print the cumulative total of litres flowed since starting
    Serial.print("  Output Liquid Quantity: ");             // Output separator
    Serial.print(totalMilliLitres);
    Serial.println("mL");
 
    // Reset the pulse counter so we can start incrementing again
    pulseCount = 0;
   
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
  }
}
 
/*
Insterrupt Service Routine
 */
void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}
