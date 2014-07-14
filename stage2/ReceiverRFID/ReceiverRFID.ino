/**
To be loaded on the the arduino uno rfid receiver board. It searches for rfid tags and sends them via serial to the receiver arduino mega board (the master for the receiver device).
RFID tags are 11 digits (too large for long or int types) and must be split. A more efficient solution would be to redesign the gridmap to translate rfid tags to numbers starting at 0 or 1 and counting up.
Tags are split into a high variable containing the top 6 digits and a low variable containing the low 5 digits.
*/

#include <serLCD.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <SPI.h>

SoftwareSerial rfid(7, 8);
SoftwareSerial master(4, 5);
serLCD lcd(3);

//Prototypes
void check_for_notag(void);
void halt(void);
void parse(void);
void print_serial(void);
void read_serial(void);
void seek(void);
void set_flag(void);

//Global var
File myFile;
int flag = 0;
int Str1[11];
int index_row=0;
String ReceivedChannelNumber;

//Init
void setup()
{
  Serial.begin(9600);
  master.begin(9600);
  //RFID setup
  Serial.println("RFID ready to read(Start)");
  rfid.begin(19200);
  delay(10);
  halt();
}

void loop()
{
  seek();
  delay(10);
  parse();
  set_flag();
  print_serial();
  delay(100);
}

void check_for_notag()
{
  seek();
  delay(10);
  parse();
  set_flag();
  
  if(flag = 1){
    seek();
    delay(10);
    parse();
  }
}
void halt()
{
 //Halt tag
  rfid.print((char)255);
  rfid.print((char)0);
  rfid.print((char)1);
  rfid.print((char)147);
  rfid.print((char)148);
}
void parse()
{
  while(rfid.available()){
    if(rfid.read() == 255){
      for(int i=1;i<11;i++){
        Str1[i]= rfid.read();
      }
    }
  }
}
void seek()
{
  //search for RFID tag
  rfid.print((char)255);
  rfid.print((char)0);
  rfid.print((char)1);
  rfid.print((char)130);
  rfid.print((char)131); 
  delay(10);
}
void set_flag()
{
  if(Str1[2] == 6){
    flag++;
  }
  if(Str1[2] == 2){
    flag = 0;
  }
}

void print_serial()
{
  if(flag == 1){
    
    String tag_high = "";
    String tag_low  = "";
    
    //print to serial port
    tag_high.concat(Str1[8]);
    tag_high.concat(Str1[7]);
    
    tag_low.concat(Str1[6]);
    tag_low.concat(Str1[5]);
    
    master.print("rhigh ");
    master.print(tag_high);
    master.print('\r');
    master.print("rlow ");
    master.print(tag_low);
    master.print('\r');

    delay(100);
    //check_for_notag();
  }
}

