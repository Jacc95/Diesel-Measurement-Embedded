#include <Wire.h>                                           // Library for I2C
#include <LiquidCrystal_I2C.h>                              // Library for the LCD
#include "RTClib.h"                                         // Library for RTC (Clock)
#include "AT24CX.h"                                         // Library for EEPROM (Memory)
#include "Adafruit_Thermal.h"                               // Library for Thermal Printer
#include <SPI.h>                                            // Library for the RFID
#include <MFRC522.h>                                        // Library for the RFID
#include <Keypad.h>                                         // Library for the Keypad

//Variables for keypad 4x4 *************************
#define ROW_NUM 4
#define COLUMN_NUM 4

//Assign PINs **************************************
#define PinSensor 3                                         // Sensor conectado en el pin 3
//#define CalButton 4                                         // Calibration button connected to pin 4
//#define PrintButton 5                                       // Printer button connected to pin 5
#define RST_PIN  9                                          // Constant to reference reset pin
#define SS_PIN  10                                          // Constant to reference pin of slave select 
byte pin_rows[ROW_NUM] = {A3, A2, A1, A0};                  // Connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {7, 6, 5, 4};                 // Connect to the column pinouts of the keypad

//Global variables *********************************
volatile int NumPulsos;                                     // Variable para la cantidad de pulsos recibidos
float factor_conversion = 1.400;                            // Para convertir de frecuencia a caudal
float factor_conversion_cal = 1.503;                        // Factor para calibracion
float volumen = 0;                                          // 
float volumen_ant = 0;                                      //
float carga;                                                // 
long dt = 0;                                                // Variación de tiempo por cada bucle
long t0 = 0;                                                // Millis() del bucle anterior
float total_freq = 0;                                       // 
int ticket = 1;                                             // Printing variables: Ticket number
float k_factor = 1.00;                                      // Calibration factor based on error %
bool calibration_flag = false;                              // Calibration condition flag
float vol_cal = 0;                                          // Calibration accumulated volume
float t0_cal = 0;                                           // Calibration time of measurement
//int jug_fill = 2000;
//String jug_fill_str;
String jug = "";                                            // Keypad variable to obtain jug
int jug_int = 2000;                                         // Integer version of jug variable
char jug_array[4] = {'2', '0', '0', '0'};                   // Jug array to show on display & change digit by digit
int jug_array_pos = 0;                                      // Jug array position variable

//Keypad mapping ***********************************
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}};

//Objects to be used *******************************
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);     // Change to (0x27,16,2) for 16x2 LCD
RTC_DS3231 rtc;                                             // Object RTC        
AT24C32 EepromRTC;                                          // Object EEpromRTC(Memory)
MFRC522 mfrc522(SS_PIN, RST_PIN);                           // Create mfrc522 object by sending slave select and reset pins 
Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

//Cards to be read by RFID *************************
byte LecturaUID[4];                                         // Create array to store the UID read
byte Usuario1[4]= {0x29, 0xB4, 0xA7, 0xB2} ;                // UID TO REST 
byte Usuario2[4]= {0x49, 0x9F, 0x11, 0xC2} ;                // UID TO RESET

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// FUNCTIONS //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---- Función que se ejecuta en interrupción ------------------------------------------------------------------------------------------------------------------------------
void ContarPulsos ()
{ 
  NumPulsos++;                                              // Incrementamos la variable de pulsos
} 

//---- Función para obtener frecuencia de los pulsos -----------------------------------------------------------------------------------------------------------------------
int ObtenerFrecuecia() 
{
  int frecuencia;
  interrupts();                                             // Habilitamos las interrupciones
  NumPulsos = 0;                                            // Ponemos a 0 el número de pulsos
  if(calibration_flag == true){
    delay(250);                                              // Muestra de 1 segundo
  } else {
    delay(1000);
  }
  frecuencia = NumPulsos;                                   // Hz(pulsos por segundo)
  noInterrupts();                                           // Deshabilitamos  las interrupciones
  return frecuencia;           
}

//---- LCD Display Function ------------------------------------------------------------------------------------------------------------------------------------------------
void lcd_display(float freq, float carga){
  lcd.setCursor(0, 0);                                      // Sets cursor at first row
  lcd.print(freq); 

 lcd.setCursor(0, 1);                                       // Sets cursor at second row
  lcd.print(carga);   
}

//---- Reset Totalizer function --------------------------------------------------------------------------------------------------------------------------------------------
void reset(){
    EepromRTC.writeFloat(1, 0);                             // Initializes total pulses memory to clean trash in memory
    EepromRTC.writeInt(5, 1);                               // Initializes ticket number to 1 and clean trash in memory
    EepromRTC.writeFloat(7, 0);                             // Initializes prev pulses memory to clean trash in memory 
    EepromRTC.write(11, 0);                                 // Tells the Arduino variables can only be initialized through INITIALIZE.INO
    EepromRTC.writeFloat(12, 1.00);                         // Initializes k_factor variable at 1.00
}

//---- Print ticket format -------------------------------------------------------------------------------------------------------------------------------------------------
void data_ticket(DateTime date, float volumen, int ticket, float carga){
  
  Serial.println("           Lupeqsa");
  Serial.println("         S.A de C.V");
  Serial.println(" ");
  Serial.println("Medidor: ");
  Serial.println("L001");
  Serial.println("Ticket: ");
  Serial.println(ticket);
  Serial.println("Fecha:");
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
  Serial.println(volumen);
  Serial.println(" ");
  Serial.println("   -----------------------------");
  Serial.println(" ");

  lcd.clear();                                                            // Clears LCD screen
}

//---- RFID function -------------------------------------------------------------------------------------------------------------------------------------------------------
void rfid(){
  if ( ! mfrc522.PICC_ReadCardSerial())                                   // si no puede obtener datos de la tarjeta
      return;     
  for (byte i = 0; i < mfrc522.uid.size; i++) {                           // loop goes through the UID one byte at a time
      //Serial.print(mfrc522.uid.uidByte[i], HEX);                        // prints the byte of the UID read in hexadecimal
      LecturaUID[i]=mfrc522.uid.uidByte[i];                               // stores the byte of the UID read in an array   
  }
  if(comparaUID(LecturaUID, Usuario1)){   
      reset();
      volumen = 0;  
      volumen_ant = 0;
      lcd.clear();                                                        // Clears LCD screen
  } else if(comparaUID(LecturaUID, Usuario2)){
      reset(); 
      volumen = 0;
      volumen_ant = 0; 
      lcd.clear();
   }
   else                                                                   // si retorna falso
      Serial.println("Llave no valida");              
      mfrc522.PICC_HaltA();                                               // card communication stops              
}

boolean comparaUID(byte lectura[],byte usuario[])                         // comparaUID is used on RFID function
{
  for (byte i=0; i < mfrc522.uid.size; i++){                              // loop goes through the UID one byte at a time
    if(lectura[i] != usuario[i])                                          // if byte of UID read is different from user
      return false;                            
    }
  return true;                                                            // if the 4 bytes match it returns true
}

//---- Calibration --------------------------------------------------------------------------------------------------------------------------------------------------------
float calibration(float factor_conversion, float t0_cal, float vol_cal){
  float freq_cal = ObtenerFrecuecia();                                  // Obtenemos la frecuencia de los pulsos en Hz
  float caudal_cal = freq_cal/factor_conversion;                        // Calculamos el caudal en L/m
  float dt_cal = millis() - t0_cal;                                     // Calculamos la variación de tiempo

  vol_cal = vol_cal + (caudal_cal/60) * (dt_cal/250);                  // Volumen(L) = caudal(L/s)*tiempo(s)
  return vol_cal;
}

float k_fact(float vol_cal, int jug_int){
  float den = (jug_int - vol_cal*100) / jug_int;
  float k_factor = 1 / (1 - den);
  
  return k_factor;
}

void display_cal(float vol_cal, char Arreglo[]){
  interrupts();
  lcd.clear();
  
  lcd.setCursor(0, 0);                                                  // Sets cursor at first row & shows 1st digit
  lcd.print(Arreglo[0]); 
  lcd.setCursor(1, 0);                                                  // Sets cursor at first row & shows 2nd digit
  lcd.print(Arreglo[1]); 
  lcd.setCursor(2, 0);                                                  // Sets cursor at first row & displays decimal point
  lcd.print('.'); 
  lcd.setCursor(3, 0);                                                  // Sets cursor at first row & shows 3rd digit
  lcd.print(Arreglo[2]); 
  lcd.setCursor(4, 0);                                                  // Sets cursor at first row & shows 4th digit
  lcd.print(Arreglo[3]); 
  
  lcd.setCursor(0, 1);                                                  // Sets cursor at second row & show current liters passed
  lcd.print(vol_cal);
  lcd.print(" L");
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN FUNCTION. Runs once ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void setup() 
{ 
  Serial.begin(9600);                                                     // Starts Serial comm at 9600 baud rate
  pinMode(PinSensor, INPUT);                                              // Defines pin 3 as input
  //pinMode(CalButton, INPUT_PULLUP);                                       // Defines pin 4 as input with default state of TRUE
  //pinMode(PrintButton, INPUT_PULLUP);                                     // Defines pin 5 as input with default state of TRUE
  attachInterrupt(digitalPinToInterrupt(PinSensor),ContarPulsos,RISING);  // (Interrupción 0(Pin2),función,Flanco de subida)
  Serial.println ("Envie 'r' para restablecer el volumen a 0 Litros");    // 
  t0 = millis();                                                          //   
  Wire.begin();                                                           // 

  //LCD Setup
  lcd.init();                                                             // Initializes LCD object
  lcd.setBacklight(125);                                                  // Sets light to max
  lcd.clear();                                                            // Clears LCD screen

  //RFID Setup
  SPI.begin();                                                            // Initialize SPI bus
  mfrc522.PCD_Init();                                                     // Initialize reader module
  
  //RTC Configuration
  if(!rtc.begin()){
    Serial.println("ERROR");
    return;
  }

  //Code that runs only when EEprom & RTC have not been set before
  //To INITIALIZE the variables RUN INITIALIZE.ino first
  if(EepromRTC.read(11)){
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));                       // Set time in the rtc when uploading code
    reset();                                                              // Initializes the variables to be used
  }

  //Read the EEPROM and get the latest values
  volumen = EepromRTC.readFloat(1);                                       // Read totalizer volume from address 1 to 4.
  volumen_ant = EepromRTC.readFloat(7);                                   // Read previous totalizer volume from address 7 to 10.
  //k_factor = EepromRTC.readFloat(12);                                   // Read the latest calibration factor
  k_factor = 1;
} 

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MAIN FUNCTION. Runs continuously ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void loop ()    
{
  float frecuencia = ObtenerFrecuecia();                                  // Obtenemos la frecuencia de los pulsos en Hz
  float caudal_L_m = frecuencia/factor_conversion;                        // Calculamos el caudal en L/m
  total_freq += frecuencia;
  dt = millis() - t0;                                                     // Calculamos la variación de tiempo
  t0 = millis();
  volumen = volumen + k_factor * (caudal_L_m/60) * (dt/1000);             // Volumen(L) = caudal(L/s)*tiempo(s)
  carga = volumen - volumen_ant;

  //----- Enviamos por el puerto serie -------------------------------------------------------------------------------------------------------------
  /*Serial.print ("Caudal: ");
  Serial.print (caudal_L_m,3);
  Serial.print ("L/min\tVolumen: ");
  Serial.print (volumen,3);
  Serial.print (" L\t Carga:");
  Serial.print (carga,3);
  Serial.println (" L");*/
  interrupts();
  lcd.clear();
  if(calibration_flag == false){
    lcd_display(k_factor, carga);
  }
  //---- Keypad Selection ---------------------------------------------------------------------------------------------------------------------------
  char key = keypad.getKey();                                                // Cleans the mode variable if no input.
  
  //---- Calibration --------------------------------------------------------------------------------------------------------------------------------
  if(key == 'C'){                                                            // Press the button to enter calibration mode
      if(calibration_flag == false){
        calibration_flag = true;
        vol_cal = 0;
        //jug_fill_str = "2000";
      }
  }
  
  while(calibration_flag == true){                                           // Calibration procedure                                                                                
    vol_cal = calibration(factor_conversion_cal, t0_cal,vol_cal);            // 1 sec delay is included here
    t0_cal = millis();
    display_cal(vol_cal, jug_array);
    
    /*if (Serial.available()) {
      jug_fill_str = Serial.readString();                                    // Restablecemos el volumen si recibimos 'r'
      jug_fill = jug_fill_str.toInt();
    }*/
    
    key = keypad.getKey();                                                   // Gets new values for jug or exits calibration mode
    if(key == '#' && jug_array_pos < 3){
      jug_array_pos++;
    } else if(key == '*' && jug_array_pos > 0){
      jug_array_pos--;
    } else if(key=='1' || key=='2' || key=='3' || key=='4' || key=='5' || key=='6' || key=='7' || key=='8' || key=='9' || key=='0'){
      jug_array[jug_array_pos] = key;
    }

    jug = "";
    for(int i=0; i<4; i++){
      jug.concat(jug_array[i]);
    }
    jug_int = jug.toInt();
    
    if(key == 'C'){                                                          // Press the button again to finish calib mode
      calibration_flag = false;
      if(vol_cal > 6.00){                                                    // Measured volume should be higher than 6 to make sure there was no mistake
        k_factor = k_fact(vol_cal, jug_int);                                 // Get new k_factor
        EepromRTC.writeFloat(12, k_factor);                                  // Write new k_factor constant in memory position 12
      }
    }
  }
  
  //---- Printing -----------------------------------------------------------------------------------------------------------------------------------
  DateTime now = rtc.now();                                                 // Gets current Date-Time
  if(key == 'B'){
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

  //------ RFID Reset -------------------------------------------------------------------------------------------------------------------------------
  if (mfrc522.PICC_IsNewCardPresent()) {
    rfid();
  }
}
