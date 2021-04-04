#include <Keypad.h>
#include <Wire.h>                                           // Library for I2C
#include <LiquidCrystal_I2C.h>                              // Library for the LCD

//Variables for keypad
const int ROW_NUM = 4; //four rows
const int COLUMN_NUM = 4; //four columns
String jug="";
int jugint;

char keys[ROW_NUM][COLUMN_NUM] = {
  {'1','2','3', 'A'},
  {'4','5','6', 'B'},
  {'7','8','9', 'C'},
  {'*','0','#', 'D'}
};

byte pin_rows[ROW_NUM] = {A3, A2, A1, A0}; //connect to the row pinouts of the keypad
byte pin_column[COLUMN_NUM] = {7, 6, 5, 4}; //connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM );

//Variables for LCD
LiquidCrystal_I2C lcd = LiquidCrystal_I2C(0x27, 16, 2);     // Change to (0x27,16,2) for 16x2 LCD
char Arreglo[4] = {'2', '0', '0', '0'};
int pos = 0;

//---- LCD Display Function ------------------------------------------------------------------------------------------------------------------------------------------------
void lcd_display(char Arreglo[]){
  lcd.setCursor(0, 0);                                       // Sets cursor at first row
  lcd.print(Arreglo[0]); 

  lcd.setCursor(1, 0);                                       // Sets cursor at first row
  lcd.print(Arreglo[1]); 

  lcd.setCursor(2, 0);                                       // Sets cursor at first row
  lcd.print('.'); 
  
  lcd.setCursor(3, 0);                                       // Sets cursor at first row
  lcd.print(Arreglo[2]); 

  lcd.setCursor(4, 0);                                       // Sets cursor at first row
  lcd.print(Arreglo[3]); 
  
  lcd.setCursor(0, 1);                                       // Sets cursor at second row
  lcd.print(20210403);   
}


void setup(){
  Serial.begin(9600);
  Wire.begin(); 
  
  //LCD Setup
  lcd.init();                                                             // Initializes LCD object
  lcd.setBacklight(255);                                                  // Sets light to max
  lcd.clear();                                                            // Clears LCD screen

}

void loop(){
  
  char key = keypad.getKey();
  
  if(key == '#' && pos < 3){
    pos++;
  } else if(key == '*' && pos > 0){
    pos--;
  } else if(key=='1' || key=='2' || key=='3' || key=='4' || key=='5' || key=='6' || key=='7' || key=='8' || key=='9' || key=='0'){
    Arreglo[pos] = key;
  }

  jug = "";
  for(int i=0; i<4; i++){
    jug.concat(Arreglo[i]);
  }
  jugint = jug.toInt();
  Serial.print(jugint);
  Serial.println();
  Serial.print(pos);
  Serial.print(" - ");
  Serial.println(key);
  lcd_display(Arreglo);
  delay(250);
}
