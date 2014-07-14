#include <serLCD.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <SPI.h>

SoftwareSerial wtt(2, 3);
//SoftwareSerial rfid(4, 5);
serLCD lcd(3);

//Init
void setup()
{
  Serial.begin(9600);
  Serial1.begin(9600);
  //rfid.begin(9600);
  //Serial LCD setup
  lcd.clear();
  //lcd.print("begin");
  wtt.begin(9600);
}

void loop()
{
   Serial.print(getTag());
}

String getTag()
{
  String tag;
  
  while(Serial1.available())
  {
     char ch=char(Serial1.read());
     if(ch=='_')
      {
      ch = Serial1.read();
      for(int i=0; ch != '*'; i++) {
      	tag.concat(ch);
      	ch=Serial1.read();
      }
      return tag;
    }
  }
  return "";
}
