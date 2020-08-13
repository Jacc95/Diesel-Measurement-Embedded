#include <Wire.h>
#include "AT24CX.h"

AT24C32 EepromRTC;

void setup() {
  Serial.begin(9600);
}

void loop() {
  // read and write byte
  byte variable_byte=1;
  
  Serial.println("Guardando datos en la EEPROM...");
  
  EepromRTC.write(11, variable_byte); // Escribe 1 en la posicion 11 de memoria, 
                                      // para activar la cond de Inicializacion en el codigo principal
  
  Serial.println("Leyendo datos guardados...");

  byte a = EepromRTC.read(11);

  Serial.print("Listo para inicializar variables: ");Serial.println(a);

  delay(5000);
}
