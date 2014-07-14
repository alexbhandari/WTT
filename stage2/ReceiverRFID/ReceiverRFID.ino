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
String tag="";
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
    //print to serial port
    tag.concat(Str1[8]);
    tag.concat(Str1[7]);
    tag.concat(Str1[6]);
    tag.concat(Str1[5]);
    
    master.print("rfid ");
    master.print(tag);
    master.print('\r');
    tag="";
 
    delay(100);
    //check_for_notag();
  }
}

