//RUN INITIALIZE.INO FIRST TO RUN FROM UPLOADING TIME
#include <Wire.h>
#include "RTClib.h"

RTC_DS1307 rtc;

char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};


void setup () 
{
  Serial.begin(9600);
  delay(3000); // wait for console opening

  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    return;
  }

  if(EepromRTC.read(11)){
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__))); // Set time in the rtc when uploading code
  
    //EEprom Initializing
    EepromRTC.writeFloat(1, 0.0);                   // Initializes total pulses memory to clean trash in memory
    EepromRTC.writeInt(5, 1);                       // Initializes ticket number to 1 and clean trash in memory
    EepromRTC.writeFloat(7, 0.0);                   // Initializes prev pulses memory to clean trash in memory 
    EepromRTC.write(11, 0);                         // Tells the Arduino variables can only be initialized through INITIALIZE.INO
  }
}

void loop () 
{   
    DateTime now = rtc.now();
    
    Serial.println("Current Date & Time: ");
    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(" (");
    Serial.print(daysOfTheWeek[now.dayOfTheWeek()]);
    Serial.print(") ");
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();
        
    delay(1000);
}
