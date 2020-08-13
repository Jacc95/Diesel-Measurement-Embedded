//Libraries
#include "Adafruit_Thermal.h"     // Library for Thermal Printer
#include <Wire.h>                 // Library for I2C
#include "RTClib.h"               // Library for RTC (Clock)
#include "AT24CX.h"               // Library for EEPROM (Memory)
#include <LiquidCrystal_I2C.h>    // Library for the LCD
#include <SPI.h>                  // Library for the RFID
#include <MFRC522.h>              // Library for the RFID

//Macros
#define inpPin 2                  // Pulse signal pin                   
#define buttonPin1 4              // Reset button
#define buttonPin2 5              // Print button

//Variables definition
volatile int pulse = 0;           // Variable modified inside the external interrupt
unsigned int total_pulses = 0;    // Total pulses saved inside the EEprom as well
unsigned int prev_pulses = 0;     // Prev pulses saved inside the EEprom as well
float totalizer = 0.00;           // Converted total pulses (Output in liters)
float prev_totalizer = 0.00;      // Converted prev pulses (Output in liters)
float carga = 0.0;                // Diesel litres per charge
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
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);    // Change to (0x27,16,2) for 16x2 LCD.

//RFID variables
#define RST_PIN  9                                         // Constant to reference reset pin
#define SS_PIN  10                                         // Constant to reference pin of slave select 

MFRC522 mfrc522(SS_PIN, RST_PIN);                          // Create mfrc522 object by sending slave select and reset pins 

byte LecturaUID[4];                                        // Create array to store the UID read
byte Usuario1[4]= {0xD9, 0x3A, 0x08, 0xBA} ;               // Card UID needed to reset
byte Usuario2[4]= {0x29, 0xB4, 0xA7, 0xB2} ;               // Card2 UID needed to reset

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
  lcd.clear();                                // Clears LCD screen

  //RFID Setup
  SPI.begin();                                // Initialize SPI bus
  mfrc522.PCD_Init();                         // Initialize reader module
  
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
    EepromRTC.writeFloat(1, 0.0);                   // Initializes total pulses memory to clean trash in memory
    EepromRTC.writeInt(5, 1);                       // Initializes ticket number to 1 and clean trash in memory
    EepromRTC.writeFloat(7, 0.0);                   // Initializes prev pulses memory to clean trash in memory 
    EepromRTC.write(11, 0);                         // Tells the Arduino variables can only be initialized through INITIALIZE.INO
  }
} 
  
///////////////////////////////////////////////////////////////////////////////////////////////
//MAIN FUNCTION. Runs continuously
///////////////////////////////////////////////////////////////////////////////////////////////
void loop() 
{ 
  //Read the EEPROM and get the latest totalizer value
  total_pulses = EepromRTC.readFloat(1);           //Read memory pulses from address 1 to 4
  
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

  //Resets totalizer when Reset Card is passed
  if (mfrc522.PICC_IsNewCardPresent()) {
    rfid();
  }
    
  //Total pulses to totalizer (litres) conversion
  totalizer = float(total_pulses)/pulses_per_litre; 

  //Write on the EEPROM the current totalizer value
  EepromRTC.writeFloat(1, total_pulses); 

  //Calculate the currente charge 
   prev_pulses = EepromRTC.readFloat(7);            //Read previous pulses from address 7 to 10          
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
  Serial.println("   -----------------------------");
  Serial.println(" ");

  lcd.clear();                                // Clears LCD screen
}


//LCD Display Function
void lcd_display(float carga){
  lcd.setCursor(0, 0);          //Sets cursor at first row
  lcd.print("Total litros:");
  
  lcd.setCursor(0, 1);          //Sets cursor at second row
  lcd.print(carga);
    
}


//Reset Totalizer function
void reset(){
  total_pulses = 0;             //Totalizer reset to zero
  EepromRTC.writeFloat(7, 0);
  EepromRTC.writeInt(5, 1);
}


//RFID function
void rfid(){
  // si no puede obtener datos de la tarjeta
  if ( ! mfrc522.PICC_ReadCardSerial())  
      return;   
      
   for (byte i = 0; i < mfrc522.uid.size; i++) {    // loop goes through the UID one byte at a time
      //Serial.print(mfrc522.uid.uidByte[i], HEX);    // prints the byte of the UID read in hexadecimal
      LecturaUID[i]=mfrc522.uid.uidByte[i];         // stores the byte of the UID read in an array   
   }
                             
                    
   if(comparaUID(LecturaUID, Usuario1)){   
      reset(); 
      lcd.clear();                                // Clears LCD screen
   } else if(comparaUID(LecturaUID, Usuario2)){
      reset(); 
      lcd.clear();
   }
   else           // si retorna falso
      Serial.println("Llave no valida");            
                  
   mfrc522.PICC_HaltA();     //card communication stops              
}


//comparaUID is used on RFID function
boolean comparaUID(byte lectura[],byte usuario[])
{
  for (byte i=0; i < mfrc522.uid.size; i++){      // loop goes through the UID one byte at a time
    if(lectura[i] != usuario[i])                 // if byte of UID read is different from user
      return(false);                            
    }
  return(true);                               //if the 4 bytes match it returns true
}
