/*
* SD card attached to SPI bus as follows:
 ** MOSI    - pin 11
 ** MISO    - pin 12
 ** CLK     - pin 13
 ** CS      - pin 6
 ** ss      - pin 10
 ---------------------------------
 ** RFID RX - pin 7
 ** RFID TX - pin 8
 ---------------------------------
 ** WTT RX  - pin 2 
 ** WTT TX  - pin 3 --> LCD RX
 ---------------------------------
 ** Serial RX - pin 0 --> White clip(red probe)
 ---------------------------------
 Red wire is 5V
 Green Wire are GNDs
 Black wire are Signals
*/

#include <serLCD.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <SPI.h>

SoftwareSerial rfid(7, 8);
SoftwareSerial wtt(2, 3);
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
String TemptValue="";
String row[20]="";
int index_row=0;
String ReceivedChannelNumber;


//Init
void setup()
{
  Serial.begin(9600);
  //Serial LCD setup
  
  lcd.clear();
  //lcd.print("begin");
  wtt.begin(9600);
  //RFID setup
  Serial.println("RFID ready to read(Start)");
  rfid.begin(19200);
  delay(10);
  halt();
  //SD setup
  Serial.println("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output
  // or the SD library functions will not work.
  pinMode(10, OUTPUT);
   
  if (!SD.begin(6)) { //should work... need to test with new lib
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
 
  OpenSD();
  
  //testing area!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  Serial.println(row[0]);
  Serial.println(row[1]);
  Serial.println(row[2]);
  Serial.println(row[3]);
}

void loop()
{
   read_sender();
//   Serial.println(ReceivedChannelNumber);
   read_serial();
}


void read_sender()
{
  String s;
  
  while(Serial.available())
  {
    char ch=char(Serial.read());
     if(ch=='_')
    {
      char ch1=Serial.read();
      s.concat(ch1);
      char ch2=Serial.read();
      s.concat(ch2);
      Serial.println("yes");
      lcd.print("yes");
      break;
    }
    else {
      Serial.println("no");
    }
  }
  lcd.clear();
  ReceivedChannelNumber=s;
}



void Compare(String row[], String TemptValue)
{
  int index;
  
  for(index=0;index<=index_row;index++)
  {
    int m= row[index].indexOf('*');
  int n= row[index].indexOf('#');
   // Serial.println(index);
  // Serial.println(row[index].substring((m+1),n));
    //Serial.println(TemptValue);
    //Serial.println("///////////////////////////");
    String deviceName=row[index].substring(n+1);
    String expectingPinNumber= row[index].substring(m-2,m);
    String rfidTagnumber=row[index].substring(m+1,n);
    String realReceivingPinNumber=ReceivedChannelNumber;
    String result=" ... ";
    result+=deviceName;
    result+="; ";
    result+=expectingPinNumber;
    result+="; ";
    result+=rfidTagnumber;
    result+="; ";
    result+=realReceivingPinNumber;
    result+="; ";
    
    if(TemptValue.equals(row[index].substring((m+1),n)))
    {
      if(ReceivedChannelNumber.toInt()==index+1)
      {
        Serial.println("Match"); //python cloud program looks for "Match"
        Serial.print(result);
        lcd.clear();
        //lcd.selectLine(1);
        lcd.print("Match!       ");
        //lcd.selectLine(2);
        lcd.print(row[index].substring((n+1)));
        break;
      }
      else
      {
        Serial.print("Not Match"); //python looks for "No Match" **NEED TO FIX
        Serial.println(result);
//        Serial.println("Received Channel Number is:"+ReceivedChannelNumber);
//        Serial.println(ReceivedChannelNumber);
//        Serial.println("Not Matching!!!!!!!!!");
//        Serial.println(row[index].substring((n+1)));
//        Serial.println(row[index]);
        lcd.clear();
        lcd.print("Not Match!");
        break;
      }
    }
    else 
    {
      if(index==index_row)
      {
        Serial.println("Not valid RFID tag");
        lcd.clear();
        lcd.print("Not valid RFID tag!");
      }
    }
    
  }
}


void OpenSD()
{
  myFile = SD.open("MAPPING.txt");
  if (myFile) {
    Serial.println("MAPPING.txt:");
   
    // read from the file until there's nothing else in it:
    char file_contents[256];
    int index=0;
    while (myFile.available()) {
       file_contents[index]=myFile.read();
       if(file_contents[index]=='\n')
       {
         index_row++;
       }
       if(file_contents[index]!='\n')
       {
         row[index_row].concat(file_contents[index]);
       }
       //Serial.print(file_contents[index]);
    }
    
    
    // close the file:
    myFile.close();
  } else {
     // if the file didn't open, print an error:
    Serial.println("error opening MAPPING.txt");
  }
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



void read_serial()
{
  seek();
  delay(10);
  parse();
  set_flag();
  print_serial();
  delay(100);
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
    
    TemptValue.concat(Str1[8]);
    TemptValue.concat(Str1[7]);
    TemptValue.concat(Str1[6]);
    TemptValue.concat(Str1[5]);
    /*
    Serial.print(Str1[8]);
    Serial.print(Str1[7]);
    Serial.print(Str1[6]);
    Serial.print(Str1[5]);
    */
//    Serial.print("TempValue is");
//    Serial.println(TemptValue);
    
    
    Compare(row,TemptValue);
    TemptValue="";
//    Serial.println();
 
    delay(100);
    //check_for_notag();
  }
}
 
 
 
 
 
 
 
 

