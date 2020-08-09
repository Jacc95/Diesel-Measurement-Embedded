//Libraries
#include "Adafruit_Thermal.h"     //Library for Thermal Printer
#include <Wire.h>                 //Library for I2C
#include "RTClib.h"               //Library for RTC (Clock)
#include "AT24CX.h"               //Library for EEPROM (Memory)
#include "LiquidCrystal_I2C.h"    //Library for the LCD

//Macros
#define inpPin 2                  //Pulse signal pin                   
#define buttonPin1 3              //
#define buttonPin2 4              //
#define buttonPin3 5              //

//Variables definition
volatile int pulse = 0;
unsigned int total_pulses = 0;
float totalizer;
unsigned long time_now = 0;       //
const int period = 1000;          //1sec loop period
const int pulses_per_litre = 90;

//Debounce variables
int buttonState = LOW;      
int prevState = LOW;        
long lastDebounce = 0;      // The last time the output pin was toggled
long debounceDelay = 50;    // The debounce time; increase if the output flickers

//Object rtc
RTC_DS1307 rtc;

//Object EepromRTC (Memory)
AT24C32 EepromRTC;

//Object lcd
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2); // Change to (0x27,16,2) for 16x2 LCD.

///////////////////////////////////////////////////////////////////////////////////////////////
//MAIN FUNCTION. Runs once
///////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
  Wire.begin();
  Serial.begin(9600);                         //Opens Serial Monitor
  pinMode(inpPin, INPUT);                     //Input signal PIN2
  pinMode(buttonPin1, INPUT);                 //Printer signal
  attachInterrupt(0, count_pulse, RISING);    //0 stands for PIN2 of the Arduino board
  
  //RTC Configuration
    if(!rtc.begin()){
      Serial.println("ERROR");
      return;
    }  
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // set time in the rtc when uploading code
  
  //Initiate the LCD:
  lcd.init();
  lcd.backlight();
} 

///////////////////////////////////////////////////////////////////////////////////////////////
//MAIN FUNCTION. Runs continuously
///////////////////////////////////////////////////////////////////////////////////////////////
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
  data_ticket(now);                          //Calls printing function (Date, Totalizer,etc)   

  //Totalizer logic
  total_pulses += pulse;        
  totalizer = float(total_pulses)/pulses_per_litre; 
  
  interrupts();
  
  //Button function
  buttonState = digitalRead(buttonPin1);

  //Write on the EEPROM the last totalizer value
  EepromRTC.writeFloat(1, totalizer); 

  //Displays the totalizer value on the LCD
  lcd_display(totalizer);
  
} 

///////////////////////////////////////////////////////////////////////////////////////////////
//FUNCTIONS. Called inside void loop
///////////////////////////////////////////////////////////////////////////////////////////////

//Interrupt function
void count_pulse() 
{ 
  pulse++; 
} 


//Debouncing function
bool printer(){
  buttonState = digitalRead(buttonPin1);
  
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
  
  //Adapts the minutes to the 2-digit format
  if(date.minute() < 10){     
    Serial.print("0");  
  }
  Serial.println(date.minute(), DEC);
  Serial.println("Litros: ");
    
  float read_totalizer = EepromRTC.readFloat(1); //leer desde memoria en la posicion 1
  Serial.println(read_totalizer);
}

void lcd_display(float totalizer){
  lcd.setCursor(0, 0);          //Sets cursor at first row
  lcd.print("Total litros:");
  lcd.setCursor(0, 1);          //Sets cursor at second row
  lcd.print(totalizer);
    
}
