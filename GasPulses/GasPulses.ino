//Libraries
#include "Adafruit_Thermal.h"

//Macros
#define inpPin 2
#define sdPin 4
#define buttonPin 5

//Variables definition
volatile unsigned int pulse = 0;
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

//Runs only once
void setup() {
  Serial.begin(9600);                         //Opens Serial Monitor
  pinMode(inpPin, INPUT);                     //Input signal PIN2
  pinMode(buttonPin, INPUT);                  //Printer signal
  attachInterrupt(0, count_pulse, RISING);    //0 stands for PIN2 of the Arduino board
} 


//Runs continuously
void loop() 
{ 
  //Pulse measuring
  pulse=0;
  time_now = millis();                             //Delay setup
  interrupts();                                    //Starts interrupts
  while(millis() - time_now <= period){}           //Equivalent to delay(1000) instruction
  noInterrupts();                                  //Stops interrupts  

  //Printing
/*  Serial.print("Pulses per second: ");  //Debugging pulses
  Serial.println(pulse); */
  data_ticket();                          //Calls printing function    

  //Totalizer logic
  total_pulses += pulse;        
  totalizer = float(total_pulses)/pulses_per_litre; 

  //Button function
  buttonState = digitalRead(buttonPin);
  /*
  if(printer()){
    // Open SD card for writing
    tempsFile = SD.open("temps.txt", FILE_WRITE);

    if (tempsFile) {
      // write temps to SD card
      tempsFile.print("Total de litros: ");
      tempsFile.println(totalizer);
      // close the file
      tempsFile.close();
    }else{
      Serial.println("Error opening file");
    }
  }*/
  delay(10);
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
void data_ticket(){
  Serial.println("               Lupqsa");
  Serial.println("             S.A de C.V");
  Serial.println(" ");
  Serial.println("Medidor: ");
  Serial.println("L001");
  Serial.println("Ticket: ");
  Serial.println("01");
  Serial.println("fecha");
  Serial.println("06/08/2020");
  Serial.println("Hora: ");
  Serial.println("11:03 am");
  Serial.println("Litros: ");
  Serial.println(totalizer, 2);
}
