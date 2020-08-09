//Libraries
#include "Adafruit_Thermal.h"
#include <Wire.h>
#include "RTClib.h"
#include "AT24CX.h"

//Macros
#define inpPin 2
#define sdPin 4
#define buttonPin 5

//Variables definition
volatile int pulse = 0;
unsigned int total_pulses = 0;
float totalizer;
unsigned long time_now = 0;
const int period = 1000;          //1sec
const int pulses_per_litre = 90;

//Debounce variables
int buttonState = LOW;
int prevState = LOW;
long lastDebounce = 0;      // The last time the output pin was toggled
long debounceDelay = 50;    // The debounce time; increase if the output flickers

//variables for RTC
RTC_DS1307 rtc;

//variables for memory
AT24C32 EepromRTC;

//Runs only once
void setup() {
  Wire.begin();
  Serial.begin(9600);                         //Opens Serial Monitor
  pinMode(inpPin, INPUT);                     //Input signal PIN2
  pinMode(buttonPin, INPUT);                  //Printer signal
  attachInterrupt(0, count_pulse, RISING);    //0 stands for PIN2 of the Arduino board
  
  //RTC Configuration
    if(!rtc.begin()){
      Serial.println("ERROR");
      return;
    }
    

    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // set time in the rtc first time
} 


//Runs continuously
void loop() 
{ 
  //Pulse measuring
  pulse=0;
  time_now = millis();                             //Delay setups
  while(millis() - time_now <= period){}           //Equivalent to delay(1000) instruction
  
 
/*  Serial.print("Pulses per second: ");  //Debugging pulses
  Serial.println(pulse); */
  DateTime now = rtc.now();
  noInterrupts();
  data_ticket(now);                          //Calls printing function    

  //Totalizer logic
  total_pulses += pulse;        
  totalizer = float(total_pulses)/pulses_per_litre; 
  
  interrupts();
  //Button function
  buttonState = digitalRead(buttonPin);
  
  EepromRTC.writeFloat(1, totalizer); //escribir en memoria en la posicion 1
  float read_totalizer = EepromRTC.readFloat(1); //leer desde memoria en la posicion 1

  Serial.print("Dato float: "); Serial.println(read_totalizer);
   
  
} 


//Interrupt function
void count_pulse() 
{ 
  pulse++; 
} 


//Debouncing function
bool printera(){
  buttonState = digitalRead(buttonPin);
  
  if(buttonState != prevState){
    lastDebounce = millis();
  
    if(millis() - lastDebounce > debounceDelay){
      if(buttonState == HIGH){
        return true;
      }
    }
  }
}


//Print ticket format
void data_ticket(DateTime date){
     
  Serial.println("               Lupqsa");
  Serial.println("             S.A de C.V");
  Serial.println(" ");
  Serial.println("Medidor: ");
  Serial.println("L001");
  Serial.println("Ticket: ");
  Serial.println("01");
  Serial.println("fecha");
  Serial.print(date.day(), DEC);
  Serial.print('/');
  Serial.print(date.month(), DEC);
  Serial.print('/');
  Serial.println(date.year(), DEC);
  Serial.println("Hora: ");
  Serial.print(date.hour(), DEC);
  Serial.print(':');
  Serial.println(date.minute(), DEC);
  //Serial.println("Litros: ");
  //Serial.println(totalizer, 2);
}
