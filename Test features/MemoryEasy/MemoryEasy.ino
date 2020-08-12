#include <Wire.h>
#include <AT24CX.h>

AT24C32 EepromRTC;

void setup() {
  Serial.begin(9600);
}


void loop() {
  // read and write byte
  byte variable_byte=235;
  int variable_int=3000;
  long variable_long=48300011;
  float variable_float=3.14;
  char cadena[30] = "Naylamp Mechatronics";
  
  Serial.println("Guardando datos en la EEPROM...");
  
  EepromRTC.write(1,variable_byte); // posiscion 1:ocupa 1 byte de tamaño
  EepromRTC.writeInt(2,variable_int); // posiscion 2:ocupa 2 bytes de tamaño 
  EepromRTC.writeLong(4,variable_long); // posiscion 4: ocupa 4 bytes de tamaño 
  EepromRTC.writeFloat(8,variable_float); // posiscion 8:ocupa 4 bytes de tamaño 
  EepromRTC.writeChars(12, cadena, 30);// posiscion 16 y  20 bytes de tamaño 

  
  Serial.println("Leyendo datos guardados...");

  byte a = EepromRTC.read(1);
  int b = EepromRTC.readInt(2);
  long c = EepromRTC.readLong(4);
  float d = EepromRTC.readFloat(8);
  char cadena2[30];
  EepromRTC.readChars(12,cadena2,30);

  Serial.print("Dato byte: ");Serial.println(a);
  Serial.print("Dato int: "); Serial.println(b);
  Serial.print("Dato long: "); Serial.println(c);
  Serial.print("Dato float: "); Serial.println(d);
  Serial.print("Dato Cadena : "); Serial.println(cadena2);
  Serial.println();

  delay(5000);
}
