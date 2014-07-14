#include <serLCD.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <SPI.h>

String tag="";
SoftwareSerial master(4, 5);

//Init
void setup()
{
//  master.begin(9600,SERIAL_8O2);
  master.begin(9600);
}

void loop()
{
  master.print("rfid 9823732346\r");
  delay(100);
  master.print("rfid 1823798345\r");
  delay(100);
}
