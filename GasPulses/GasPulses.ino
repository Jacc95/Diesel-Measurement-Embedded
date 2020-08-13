//Libraries
#include "Adafruit_Thermal.h"     // Library for Thermal Printer
#include <Wire.h>                 // Library for I2C
#include "RTClib.h"               // Library for RTC (Clock)
#include "AT24CX.h"               // Library for EEPROM (Memory)
#include <LiquidCrystal_I2C.h>    // Library for the LCD

//Macros
#define inpPin 2                  // Pulse signal pin                   
#define buttonPin1 4              // Reset button
#define buttonPin2 5              // Print button
#define buttonPin3 6              //

//Variables definition
volatile int pulse = 0;           // Variable modified inside the external interrupt
unsigned int total_pulses = 0;    // Total pulses saved inside the EEprom as well
unsigned int prev_pulses = 0;     //Prev pulses saved inside the EEprom as well
float totalizer = 0.00;           // Converted total pulses (Output in liters)
float prev_totalizer = 0.00;      // Converted prev pulses (Output in liters)
float carga = 0.0;
unsigned long time_now = 0;       // Variable used to compare
const int period = 1000;          // 1sec loop period
const int pulses_per_litre = 100; // Pulses required to count 1 litre of diesel

//Printing variables
int ticket;                       // Ticket number

//Debounce variables [UNUSED]
/*
bool buttonState = LOW;           
int prevState = LOW;        
long lastDebounce = 0;            // The last time the output pin was toggled
long debounceDelay = 50;          // The debounce time; increase if the output flickers*/

//Object rtc
RTC_DS1307 rtc;

//Object EEpromRTC (Memory)
AT24C32 EepromRTC;

//Object lcd
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2); // Change to (0x27,16,2) for 16x2 LCD.

///////////////////////////////////////////////////////////////////////////////////////////////
//MAIN FUNCTION. Runs once
///////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Wire.begin();
  Serial.begin(9600);                         // Opens Serial Monitor
  pinMode(inpPin, INPUT);                     // Input signal PIN2
  pinMode(buttonPin1, INPUT);                 // Reset signal
  pinMode(buttonPin2, INPUT);                 // Printer signal
  attachInterrupt(0, count_pulse, RISING);    // 0 stands for PIN2 of the Arduino board

  //LCD Setup
  lcd.init();                                 // Initializes LCD object
  lcd.setBacklight(1);                        // Sets light to max
  
  //RTC Configuration
  if(!rtc.begin()){
    Serial.println("ERROR");
    return;
  }  

  //Code that runs only when EEprom & RTC have not been set before. 
  //To INITIALIZE the variables RUN INITIALIZE.ino first
  if(EepromRTC.read(11)){
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set time in the rtc when uploading code
  
    //EEprom Initializing
    EepromRTC.writeFloat(1, 0.0);               // Initializes total pulses memory to clean trash in memory
    EepromRTC.writeInt(5, 1);                   // Initializes ticket number to 1 and clean trash in memory
    EepromRTC.writeFloat(7, 0.0);               // Initializes prev pulses memory to clean trash in memory 
    EepromRTC.write(11, 0);                     // Tells the Arduino variables can only be initialized through INITIALIZE.INO
  }
} 
  
///////////////////////////////////////////////////////////////////////////////////////////////
//MAIN FUNCTION. Runs continuously
///////////////////////////////////////////////////////////////////////////////////////////////
void loop() 
{ 
  //Read the EEPROM and get the latest totalizer value
  total_pulses = EepromRTC.readFloat(1);           //Read memory from address 1 to 4
  
  //Pulse measuring
  pulse=0;
  time_now = millis();                             //Delay setups
  while(millis() - time_now <= period){}           //Equivalent to delay(1000) instruction

  /*Serial.print("Pulses per second: ");             //Pulses debugger
  Serial.println(pulse); */

  //Catches the rtc value
  DateTime now = rtc.now();                        

  //If next day, restarts ticket number count
  if((now.hour() == 23) && (now.minute() == 59) && (now.second() == 59)){  //Resets at 23:59 as of right now
    EepromRTC.writeInt(5, 1);
  }

  // SENSIBLE CODE HERE ///////////////////////////////////////////////////////////////////////////////////////////////////////////
  noInterrupts();
  
  //Totalizer logic
  total_pulses += pulse;        
  
  interrupts();
  /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  //Resets totalizer when Reset button is pressed
  if(digitalRead(buttonPin1) == HIGH){
    lcd.clear();
    reset();
  }
    
  //Total pulses to totalizer (litres) conversion
  totalizer = float(total_pulses)/pulses_per_litre; 

  //Write on the EEPROM the current totalizer value
  EepromRTC.writeFloat(1, total_pulses); 

  //Calculate the currente charge
   prev_pulses = EepromRTC.readFloat(7);           
   prev_totalizer = float(prev_pulses)/pulses_per_litre;
   carga = totalizer - prev_totalizer;
    
  //Printing
  ticket = EepromRTC.readInt(5);                    // Continues from last ticket number
  if(digitalRead(buttonPin2) == HIGH){              // Calls printing function (Date, Totalizer,etc) when button is pressed
    
    data_ticket(now, totalizer, ticket,carga);
    ticket++;                                       // Increment ticket number
    
    prev_pulses = total_pulses;
    EepromRTC.writeFloat(7, prev_pulses);           // Writes prev pulses into EEPROM
    
  }
  
  EepromRTC.writeInt(5, ticket);                    // Writes next ticket number into EEPROM               
  
  //Displays the totalizer value on the LCD
  lcd_display(carga);
 
} 


///////////////////////////////////////////////////////////////////////////////////////////////
//FUNCTIONS. Called inside void loop
///////////////////////////////////////////////////////////////////////////////////////////////

//Interrupt function for pulses
void count_pulse() 
{
  pulse++; 
} 


//Debouncing function [UNUSED]
/*
bool debounce(){
  buttonState = digitalRead(buttonPin1);
  
  if(buttonState != prevState){
    lastDebounce = millis();
  
    if(millis() - lastDebounce > debounceDelay){
      if(buttonState == HIGH){
        return true;
      }
    }
  }
}*/


//Print ticket format
void data_ticket(DateTime date, float read_totalizer, int ticket, float carga){
  
  Serial.println("               Lupqsa");
  Serial.println("             S.A de C.V");
  Serial.println(" ");
  Serial.println("Medidor: ");
  Serial.println("L001");
  Serial.println("Ticket: ");
  Serial.println(ticket);
  Serial.println("Fecha");
  Serial.print(date.day(), DEC);
  Serial.print('/');
  Serial.print(date.month(), DEC);
  Serial.print('/');
  Serial.println(date.year(), DEC);
  Serial.println("Hora: ");
  Serial.print(date.hour(), DEC);
  Serial.print(':');
  
  //Adapts the minutes to the 2-digit format
  if(date.minute() < 10){     
    Serial.print("0");  
  }
  Serial.println(date.minute(), DEC);
  Serial.println("Carga: ");  
  Serial.println(carga);
  Serial.println("Totalizer: ");  
  Serial.println(read_totalizer);
  Serial.println(" ");
  Serial.println(" ");
}


//LCD Display Function
void lcd_display(float carga){
  lcd.setCursor(0, 0);          //Sets cursor at first row
  lcd.print("Total litros:");
  
  lcd.setCursor(0, 1);          //Sets cursor at second row
  lcd.print(carga);
    
}


//Totalizer reset function
void reset(){
  total_pulses = 0;             //Totalizer reset to zero
}
 
