#include <Wire.h>                                           // Library for I2C
#include <LiquidCrystal_I2C.h>                              // Library for the LCD
#include "RTClib.h"                                         // Library for RTC (Clock)
#include "AT24CX.h"                                         // Library for EEPROM (Memory)
#include "Adafruit_Thermal.h"                               // Library for Thermal Printer

//Assign PINs *****************************************************************************************************************************************************************************
#define PinSensor 3    //Sensor conectado en el pin 3
#define PrintButton 5  //Printer button connected to pin 5

//Global variables ************************************************************************************************************************************************************************
volatile int NumPulsos;                                     // Variable para la cantidad de pulsos recibidos
float factor_conversion = 1.42;                             // Para convertir de frecuencia a caudal
float volumen = 0;                                          // 
float volumen_ant = 0;                                      //
float carga;                                                // 
long dt = 0;                                                // Variación de tiempo por cada bucle
long t0 = 0;                                                // Millis() del bucle anterior
float total_freq = 0;                                       // 
int ticket = 1;                                             // Printing variables: Ticket number


//Objects to be used
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);     // Change to (0x27,16,2) for 16x2 LCD
RTC_DS1307 rtc;                                             // Object RTC        
AT24C32 EepromRTC;                                          // Object EEpromRTC(Memory)

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---- Función que se ejecuta en interrupción ------------------------------------------------------------------------------------------------------------------------------
void ContarPulsos ()
{ 
  NumPulsos++;                 // Incrementamos la variable de pulsos
} 

//---- Función para obtener frecuencia de los pulsos -----------------------------------------------------------------------------------------------------------------------
int ObtenerFrecuecia() 
{
  int frecuencia;
  interrupts();                // Habilitamos las interrupciones
  NumPulsos = 0;               // Ponemos a 0 el número de pulsos
  delay(1000);                 // Muestra de 1 segundo
  frecuencia=NumPulsos;        // Hz(pulsos por segundo)
  noInterrupts();              // Deshabilitamos  las interrupciones
  return frecuencia;           
}

//---- LCD Display Function ------------------------------------------------------------------------------------------------------------------------------------------------
void lcd_display(float freq, float carga){
  lcd.setCursor(0, 0);         // Sets cursor at first row
  lcd.print(freq); 

 lcd.setCursor(0, 1);          // Sets cursor at second row
  lcd.print(carga);   
}

//---- Reset Totalizer function --------------------------------------------------------------------------------------------------------------------------------------------
void reset(){
    EepromRTC.writeFloat(1, 0);                     // Initializes total pulses memory to clean trash in memory
    EepromRTC.writeInt(5, 1);                       // Initializes ticket number to 1 and clean trash in memory
    EepromRTC.writeFloat(7, 0);                     // Initializes prev pulses memory to clean trash in memory 
    EepromRTC.write(11, 0);                         // Tells the Arduino variables can only be initialized through INITIALIZE.INO
    //EepromRTC.writeFloat(12, 100.0);              // Initializes pulses_per_litre variable at 100.0
}

//---- Print ticket format -------------------------------------------------------------------------------------------------------------------------------------------------
void data_ticket(DateTime date, float volumen, int ticket, float carga){
  
  Serial.println("               Lupqsa");
  Serial.println("             S.A de C.V");
  Serial.println(" ");
  Serial.print("Medidor: ");
  Serial.println("L001");
  Serial.print("Ticket: ");
  Serial.println(ticket);
  Serial.print("Fecha");
  Serial.print(date.day(), DEC);
  Serial.print('/');
  Serial.print(date.month(), DEC);
  Serial.print('/');
  Serial.println(date.year(), DEC);
  Serial.print("Hora: ");
  Serial.print(date.hour(), DEC);
  Serial.print(':');
  
  //Adapts the minutes to the 2-digit format
  if(date.minute() < 10){     
    Serial.print("0");  
  }
  Serial.println(date.minute(), DEC);
  Serial.print("Carga: ");  
  //Serial.println(carga);
  Serial.print("Totalizer: ");  
  Serial.println(volumen);
  Serial.println(" ");
  Serial.println("   -----------------------------");
  Serial.println(" ");

  lcd.clear();                                // Clears LCD screen
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN FUNCTION. Runs once ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() 
{ 
  Serial.begin(9600);                                                     // 
  pinMode(PinSensor, INPUT);                                              //
  attachInterrupt(digitalPinToInterrupt(PinSensor),ContarPulsos,RISING);  //(Interrupción 0(Pin2),función,Flanco de subida)
  Serial.println ("Envie 'r' para restablecer el volumen a 0 Litros");    // 
  t0 = millis();                                                          //   
  Wire.begin();                                                           // 

  //LCD Setup
  lcd.init();                                                             // Initializes LCD object
  lcd.setBacklight(1);                                                    // Sets light to max
  lcd.clear();                                                            // Clears LCD screen

  //RTC Configuration
  if(!rtc.begin()){
    Serial.println("ERROR");
    return;
  }

  //Code that runs only when EEprom & RTC have not been set before
  //To INITIALIZE the variables RUN INITIALIZE.ino first
  if(EepromRTC.read(11)){
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));                       // Set time in the rtc when uploading code
    reset();                                                              //Initializes the variables to be used
  }

  //Read the EEPROM and get the latest totalizer value
  volumen = EepromRTC.readFloat(1);                                       // Read totalizer volume from address 1 to 4.
  volumen_ant = EepromRTC.readFloat(7);                                   // Read previous totalizer volume from address 7 to 10.
} 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN FUNCTION. Runs continuously ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop ()    
{
  if (Serial.available()) {
    if(Serial.read() == 'r') volumen = 0;                                 // Restablecemos el volumen si recibimos 'r'
  }
  float frecuencia = ObtenerFrecuecia();                                  // Obtenemos la frecuencia de los pulsos en Hz
  float caudal_L_m = frecuencia/factor_conversion;                        // Calculamos el caudal en L/m
  total_freq += frecuencia;
  dt = millis() - t0;                                                     // Calculamos la variación de tiempo
  t0 = millis();
  volumen = volumen + (caudal_L_m/60) * (dt/1000);                        // Volumen(L) = caudal(L/s)*tiempo(s)
  carga = volumen - volumen_ant;

  //----- Enviamos por el puerto serie -------------------------------------------------------------------------------------------------------------
  Serial.print ("Caudal: ");
  Serial.print (caudal_L_m,3);
  Serial.print ("L/min\tVolumen: ");
  Serial.print (volumen,3);
  Serial.print (" L\t Carga:");
  Serial.print (carga,3);
  Serial.println (" L");
  interrupts();
  lcd.clear();
  lcd_display(total_freq, carga);
  
  
  //---- Printing -----------------------------------------------------------------------------------------------------------------------------------
  DateTime now = rtc.now();                                                 // Gets current Date-Time
  if(digitalRead(PrintButton) == HIGH){
    //Gets the necessary variables for the ticket
    ticket = EepromRTC.readInt(5);                                          // Read memory pulses from address 5 to 6.
    
    data_ticket(now, volumen, ticket, carga);                               // Prints ticket
    volumen_ant = volumen;
    EepromRTC.writeInt(5, ++ticket);                                        // Writes next ticket number into EEPROM
    EepromRTC.writeFloat(7, volumen_ant);
    }
    
  //------ Updates EEPROM Values --------------------------------------------------------------------------------------------------------------------
  EepromRTC.writeFloat(1, volumen);                                         // Write accumulated volume from address 1 to 4.
  if((now.hour() == 23) && (now.minute() == 59) && (now.second() == 59)){   // If next day, restarts ticket number count at 23:59 as of right now. 
    EepromRTC.writeInt(5, 1);                                               // Write accumulated volume from address 5 to 6.
  }
  
}
