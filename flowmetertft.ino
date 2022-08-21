

/*
Vcc5V(RED) Gnd(BLACK)
signal: pin 2.
 */

/**
 * -----------------------------------------------------Libraries---------------------------------------------------------------
 */
#include "Nextion.h"
#include <EEPROM.h>
#include <String.h> 

/**
 * -----------------------------------------------------Globale Variablen-------------------------------------------------------
 */
int                 VCC2                  = 6;
int                 VCC3                  = 4;
int                 GND2                  = 7;
int                 GND3                  = 3;
int                 statusLed             = 8;
int                 sensorInterrupt       = 0;  // 0 = digital pin 2
int                 sensorPin             = 2;
int                 score[11];

const int           grenzwert             = 20;
unsigned int        flowMilliLitres;
uint32_t            slid                  = 0;

volatile byte       pulseCount;   

float               flowRate;
float               average;
float               maxFlow;
float               maximum;
uint32_t            calibrationFactor     = 43;

unsigned long       totalMilliLitres;
unsigned long       oldTime;
unsigned long       trichterStartZeit;
unsigned long       counter;

bool                trichter;

// Calibration Factor 1-30 L/min F=6.68Q +- 5% (Q=L/min) YF-B6
/**
 * -----------------------------------------------------------SETUP-----------------------------------------------------------
 */

//  Nextion - Arduino Communication
char buffer[100] = {0};
NexButton b0 = NexButton(1, 23, "b0"); // Reset Button added
NexSlider h0 = NexSlider(3, 2, "h0"); // Slider added

NexTouch *nex_listen_list[] =
{
  &b0,  // Reset Button added
  &h0,  // Slider added
  NULL  // String terminated
};

void setup()
{
  // Serielle Übertragung an Host
  Serial.begin(9600); //9600 BAUD
  Serial.print("baud=115200");
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.end();            
  delay(500);             
  Serial.begin(115200);

  // status LED line as an output
  pinMode(statusLed, OUTPUT);
  digitalWrite(statusLed, HIGH);  //active-low LED
 
  pinMode(sensorPin, INPUT);
  digitalWrite(sensorPin, HIGH); 
    //config Pins
    
  pinMode (VCC2,OUTPUT);
  digitalWrite(VCC2, HIGH);
  pinMode (VCC3,OUTPUT);
  digitalWrite(VCC3, HIGH);
  pinMode (GND2,OUTPUT);
  digitalWrite(GND2, LOW);
  pinMode (GND3,OUTPUT);
  digitalWrite(GND3, LOW);

  // Variablen

  pulseCount        = 0;
  flowRate          = 0.0;
  oldTime           = 0;
  average           = 0.0;
  trichter          = false;
  flowMilliLitres   = 0;
  totalMilliLitres  = 0;
  maximum           = 0.0;
  
  //reseteprom();
  readeprom();
  sendScoreboardtoNextion();
  attachInterrupt(sensorInterrupt, pulseCounter, FALLING);// pin 2 mit interrupt triggert bei fallender Flanke *
  b0.attachPush(b0PushCallback);
  h0.attachPop(h0PopCallback, &h0);
}


/**
 * -----------------------------------------------------------MAIN------------------------------------------------------------
 */
void loop()
{
  readSensor();
  tryTrichter();
  if(trichter == true) {
    trichterRecord();
  }
  sendFlowToNextion();
  sendTotalToNextion();
  sendmaximumToNextion();
  sendCFtoNextion();
  Bar();
  Graph();
  Graph2();
  nexLoop(nex_listen_list); //check for any touch event
  delay(50);
}

/**
 * -----------------------------------------------------------METHODEN--------------------------------------------------------
 */

void b0PushCallback(void *ptr) //Press event for button b0
{
  for (int i = 0 ; i < EEPROM.length() ; i++) {EEPROM.write(i, 0);}
}

int h0PopCallback(void *ptr) // Release event for slider
{
  h0.getValue(&slid);
  if(slid==0){h0.getValue(&slid);}
  String command2 = "page3.n4.txt=\""+String(slid)+"\"";
  Serial.print(command2);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  return slid;
}

int readSensor()
{  
   if((millis() - oldTime) > 100)    // Zählt einmal pro 100ms
  {
    // flow rate berechnen und an host übermittlen Interupt deaktivieren
    calibrationFactor = h0PopCallback(0);
    detachInterrupt(sensorInterrupt);
    flowRate = (((100.0 / (millis() - oldTime)) * pulseCount)*10) / calibrationFactor;  // L/4.3s = ((100ms/Verzögerung) * Anzahl Sensorpulse) / Kallibrierfaktor 
    flowMilliLitres = (flowRate / 6) * 1000; // ml/s
    totalMilliLitres += (flowMilliLitres/10); // ml total
    oldTime = millis();
    /* Durchschnittlicher DURCHFLUSS
    if (flowRate>0){
      for (int i=0; i < 10; i++){average = average + flowMilliLitres;}
      average = average / 10;
    }
    */
    pulseCount = 0;
    // Enable the interrupt again now that we've finished sending output
    attachInterrupt(sensorInterrupt, pulseCounter, FALLING);
    return flowMilliLitres; 
  }

}


void tryTrichter() {
  if((flowMilliLitres > grenzwert) && (trichter == false)) {
    trichterStartZeit = millis(); // ab hier 10s zählen
    maxFlow = 0;
    average = 0.0;
    counter = 0;
    trichter = true;
    String command = "g0.pco=5857";
    Serial.print(command);
    endNextionCommand();
  }
}

void trichterRecord() {
  if ((millis()-trichterStartZeit) < 7000) {
    if (flowMilliLitres > maxFlow) {
      maxFlow = flowMilliLitres;
    }
    average += flowMilliLitres;
    counter++;
  } else {
    int maxFlowTemp = (int)(maxFlow); 
    int temp;
    for(int i = 1; i < 11; i++) {
      if (maxFlowTemp > score[i]) {
        temp = score[i];
        score[i] = maxFlowTemp;
        maxFlowTemp = temp;
      }
    }
    average = (average/counter);
    sendaverageToNextion();
    sendScoreboardtoNextion();//score[10] ausgeben
    savetoeprom(); 
    trichter = false;
    String command2 = "g0.pco=65535";
    Serial.print(command2);
    endNextionCommand();
  }
}

void readeprom()
{
  EEPROM.get(0, score);

      /*for(int i=0, k=0; i<10 && k<19; i++,k+2)
       {
        EEPROM.get(k, (byte)(score[i] & 0xFF));
        EEPROM.get(k+1, (byte)((score[i]>>8) & 0xFF));
       }
       for(int i=0; i<10; i++){EEPROM.get(i, score[i]);} 
       for(int i=0; i<10; i++){score[i]=EEPROM.read(i);}    */    
}

void savetoeprom()
{ 
  EEPROM.put(0, score);
      /*
      for(int i=0; i<20; i=i+2)
       {
        EEPROM.put(i, (byte)(score[i] & 0xFF));
        EEPROM.put(i-1, (byte)((score[i]>>8) & 0xFF));
       }
       */
}

void  reseteprom()
{
 for (int i = 0 ; i < EEPROM.length() ; i++) {EEPROM.write(i, 0);}
}

/*
Interrupt Service Routine
 */

void pulseCounter()
{
  // Increment the pulse counter
  pulseCount++;
}

/**
 * ----------------------------------------------------------Display/Nextion Commands--------------------------------------------
 */
 
void sendFlowToNextion()
{
  String command = "page0.flow.txt=\""+String(flowMilliLitres)+"\"";
  Serial.print(command);
  endNextionCommand();
}

void sendmaximumToNextion()
{
  String command = "page0.maximum.txt=\""+String((int)maxFlow)+"\"";
  Serial.print(command);
  endNextionCommand();
}

void sendTotalToNextion()
{
  String command = "page0.total.txt=\""+String(totalMilliLitres)+"\"";
  Serial.print(command);
  endNextionCommand();
}

void sendaverageToNextion()
{
  String command = "page0.average.txt=\""+String(average)+"\"";
  Serial.print(command);
  endNextionCommand();
}

void sendCFtoNextion()
{
  calibrationFactor = h0PopCallback(0);
  String command = "page3.cF.txt=\""+String(calibrationFactor)+"\"";
  Serial.print(command);
  endNextionCommand();
}


void Graph()
{
  int Value = map(average,0,1000,0,50);
  String command = "add ";
  command += 2;
  command += ",";
  command += 0;
  command += ",";
  command += Value;
  Serial.print(command);
  endNextionCommand();
}

void Graph2()
{
  int Value = map(maxFlow,0,1000,0,50);
  String command = "add ";
  command += 2;
  command += ",";
  command += 1;
  command += ",";
  command += Value;
  Serial.print(command);
  endNextionCommand();
}

void Bar()
{
  int Value = map(flowMilliLitres,0,maxFlow,0,100);
  String command = "j0.val=";
  command += Value ;
  Serial.print(command);
  endNextionCommand();
}


void sendScoreboardtoNextion()
{ 
  String command = "page1.t1.txt=\""+String(score[1])+"\"";
  Serial.print(command);
  endNextionCommand();
  String command1 = "page1.t2.txt=\""+String(score[2])+"\"";
  Serial.print(command1);
  endNextionCommand();
  String command2 = "page1.t3.txt=\""+String(score[3])+"\"";
  Serial.print(command2);
  endNextionCommand();
  String command3 = "page1.t4.txt=\""+String(score[4])+"\"";
  Serial.print(command3);
  endNextionCommand();
  String command4 = "page1.t5.txt=\""+String(score[5])+"\"";
  Serial.print(command4);
  endNextionCommand();
  String command5 = "page1.t6.txt=\""+String(score[6])+"\"";
  Serial.print(command5);
  endNextionCommand();
  String command6 = "page1.t7.txt=\""+String(score[7])+"\"";
  Serial.print(command6);
  endNextionCommand();
  String command7 = "page1.t8.txt=\""+String(score[8])+"\"";
  Serial.print(command7);
  endNextionCommand();
  String command8 = "page1.t9.txt=\""+String(score[9])+"\"";
  Serial.print(command8);
  endNextionCommand();
  String command9 = "page1.t10.txt=\""+String(score[10])+"\"";
  Serial.print(command9);
  endNextionCommand();
}


void endNextionCommand()
{
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}

void test()
{  
  int x = EEPROM.read(1);
  String command = "tex.txt=\""+String(x)+"\"";
  endNextionCommand();
}

/*void Record()
{
  String command = "g1.en=";
  command += 1 ;
  Serial.print(command);
  endNextionCommand();
  String command2 = "g1.bco=5857";
  Serial.print(command2);
  endNextionCommand();
  /*delay(5000);
  String command3 = "g1.en=";
  command3 += 0 ;
  Serial.print(command3);
  endNextionCommand();
  String command4 = "g1.bco=0";
  Serial.print(command4);
}*/ 
