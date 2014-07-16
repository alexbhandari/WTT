/**The reciever reads the serial communications from the sender through the wires connected to the devices,
***and verifies that the pin matchs the rfid tag on the decive by comparing them with the gridmap.
*/

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
 ** WTT Serial RX  - pin 2 
 ** WTT Serial TX  - pin 3 --> LCD RX (WTT is outgoing only, thus TX is not used for WTT, and instead is used for LCD output)
 ---------------------------------
 ** Serial RX - pin 0 --> White clip(red probe)
 ---------------------------------
 ** Rfid Arduiono Uno serial comm RX - pin 4 -> pin 18, TX1
 ** Rfid Arduiono Userial comm TX - pin 5 -> pin 19, RX1
 ---------------------------------
 Red wire is 5V
 Green Wire are GNDs
 Black wire are Signals
*/

#include <serLCD.h>
#include <SD.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <avr/pgmspace.h>
#include <MsgParser.h>

SoftwareSerial rfid(4, 5);
serLCD lcd(3);

const int BLOCKS = 2;
const int PINS_PER_BLOCK = 5;
const int DATA_PER_PIN = 2;

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
static int selected_block;
String row[BLOCKS][PINS_PER_BLOCK][DATA_PER_PIN] = { "" };
long high = 0;
boolean received = false;

//msgparser vars

//list of strings. You can have as many as you want.
//each string is saved in flash memory only and does not take up any ram.
char s0[] PROGMEM = "start";
char s1[] PROGMEM = "rhigh";
char s2[] PROGMEM = "rlow";
char s3[] PROGMEM = "printsd";
//This is our look up table. It says which function to call when a particular string is received
FuncEntry_t functionTable[] PROGMEM = {
//   String     Function
    {s0,        sayHello    },
    {s1,        getHighTag  },
    {s2,        getLowTag   },
    {s3,        printSD     }
    };
//this is the compile time calculation of the length of our look up table.
int funcTableLength = (sizeof functionTable / sizeof functionTable[0]);     //number of elements in the function table
MsgParser myParser;     //this creates our parser

//Init
void setup()
{
   //Serial.begin(9600,SERIAL_8O2); //with parity bit
 Serial.begin(9600);

  //Serial LCD setup
  lcd.clear();
  //lcd.print("begin");
  //SD setup
  Serial.println("Initializing SD card...");
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output
  // or the SD library functions will not work.
  pinMode(10, OUTPUT);

  while (!SD.begin(6,11,12,13)) {
    Serial.println("initialization failed!");
    //return;
  }
  Serial.println("initialization done.");
 
  OpenSD();
  
  printSD();

//msg parser setup
    Serial1.begin(9600);
    Serial.println("Ready for action");
    myParser.setTable(functionTable, funcTableLength);      //tell the parser to use our lookup table
    myParser.setHandlerForCmdNotFound(commandNotFound);     //Tell the parser which function to call when it gets a command it doesn't handle

}

void loop()
{
   read_sender();
   //if we received any bytes from the serial port, pass them to the parser
   while ( Serial1.available() )  myParser.processByte(Serial1.read () );
   //while ( Serial.available() )  myParser.processByte(Serial.read () );
}

//Wire probe on the reciever is wired to the arduino's serial input.
//This method reads the communications over this line...
//    input:_123_456 is read into ReceivedChannelNumber as 123 and 456
void read_sender()
{
  String s;
  
  while(Serial.available())
  {
    char ch=char(Serial.read());
    if(ch=='_')
    {
      selected_block = (int)(Serial.read()-48);
      char ch1=Serial.read();
      s.concat(ch1);
      char ch2=Serial.read();
      s.concat(ch2);
      Serial.print("Block ");
      Serial.println(selected_block);
      Serial.print("Pin ");
      Serial.println(s);
      break;
    }
  }
  ReceivedChannelNumber=s;
}

void Compare(String scannedTag)
{
  int index;
  
  for(index=0;index<=index_row;index++)
  {
    String deviceName=row[selected_block][index][1];
    deviceName = deviceName.substring(0,deviceName.length()-1);
    String expectingPinNumber;
    char str[10];
    sprintf(str,"%d",index + 1);
    expectingPinNumber = str;
    String SDtag=row[selected_block][index][0];
    String realReceivingPinNumber=ReceivedChannelNumber;
    String result=" ... ";
    result.concat(deviceName);
    result.concat(" ");
    result.concat(expectingPinNumber);
    result.concat(" ");
    result.concat(scannedTag);
    result.concat(" ");
    result.concat(realReceivingPinNumber);
    result.concat(" ");

//    Serial.print("block: ");
//    Serial.println(selected_block);
//    Serial.print("result: ");
//    Serial.println(result);
//    Serial.print("rfid: ");
//    Serial.println(scannedTag);

    if(scannedTag.equals(SDtag)) //the current index matches the index of the scanned tag (this should happen unless the tag is invalid)
    {
      Serial.print("Scanned tag matched to gridmap on pin ");
      Serial.println(index+1);
      if(ReceivedChannelNumber.toInt()==index+1) //if the pin selected with the probe matches the scanned rfid's pin number
      {
        Serial.print("Match");  //python cloud program looks for "Match"
        Serial.println(result); //data corresponding to the match
        lcd.clear();
        //lcd.selectLine(1);
        lcd.print("Match! ");
        //lcd.selectLine(2);
        lcd.print(deviceName);
        lcd.print(" on ");
        lcd.print(expectingPinNumber);
        break;
      }
      else
      {
        Serial.print("Not Matching"); //python looks for "Not Matching"
        Serial.print(result);	      //data corresponding to the mismatch
        Serial.print("| ");

	//example: prints to the lcd "X Tank on 3 wired to 4" if the probe is connected or ..."not wired" if the probe is disconnected
	//and to the display: "Not Matching Tank 3 2345464352 | wired to 4" or ..." Probe disconnected"
        lcd.clear();
        lcd.print("X "); 
        lcd.print(deviceName);
        lcd.print(" on ");
        lcd.print(expectingPinNumber);
        if (realReceivingPinNumber != "") {
          lcd.print(" wired to ");
          lcd.print(realReceivingPinNumber);
          Serial.print(" wired to ");
          Serial.println(realReceivingPinNumber);
        }
        else {
          lcd.print(" not wired");
          Serial.println("Probe disconnected");
        }
        break;
      }  
    }
    //if this is the last checked index, then the rfid tag is not recognized from the gridmap
    else 
    {
      if(index==index_row)
      {
        //Serial.print("index: " + index);
        //Serial.println(" index_row: " + index_row);
        //Serial.print(" scanned tag: ");
        //Serial.println(scannedTag);
        //Serial.print(" stored tag: ");
        //Serial.println(SDtag);
        Serial.println("Not valid RFID tag");
        lcd.clear();
        lcd.print("Not valid RFID tag!");
      }
    }
    
  }
}


void OpenSD()
{
//  if (SD.exists("MAPPING.txt")) {
//    Serial.println("MAPPING.txt exists.");
//  }
//  else {
//    Serial.println("MAPPING.txt doesn't exist.");
//  }
  myFile = SD.open("MAPPING.txt");
  if (myFile) {
    Serial.print("Reading from SD card...");
//    lcd.clear();
//    lcd.print("Reading from SD card...");

    // read from the file until there's nothing else in it:
    char file_contents;
    int pin=1;
    int block=1;
    int data_index=0;
    //reads the file byte by byte, concatinates the bytes in each line into a string, and places them in row[line_index]
    while (myFile.available()) { 

       file_contents=myFile.read();

       if(pin > PINS_PER_BLOCK) {
         block++;
         pin=1;
         Serial.print("block");
         Serial.println(block);
       }

       if(file_contents=='\n')
       {
         pin++;
         data_index=0;
         Serial.print("pin");
         Serial.println(pin);
       }

       else if(file_contents=='*' || file_contents=='#')
       {
         data_index++;
         Serial.print("data");
         Serial.println(data_index);
       }

       else
       {
         //need to exclude pin number which is currently listed in the grid map
         if(data_index-1 > -1) {
            row[block-1][pin-1][data_index-1].concat(file_contents);
         }
       }
    }
    Serial.print("hi");    
    index_row = PINS_PER_BLOCK - 1;
    
    // close the file:
    Serial.print("Finished reading");
//    lcd.clear();
//    lcd.print("Finished reading");
    delay(500);
  
    myFile.close();
//    lcd.clear();
  } 
  else {
    // if the file didn't open, print an error:
    Serial.println("error opening MAPPING.txt");
    lcd.clear();
    lcd.println("error opening MAPPING.txt");
    delay(500);
  }
}

/**msgparser methods
*/
void sayHello(){}
void sayBye() {}
void getHighTag() {
  high = myParser.getLong();
  received = true;
}
void getLowTag()
{
    if(received) { //only valid if both halves of the tag are recieved
      
      String rfidNumber = String(high) + String( myParser.getLong() );
      Serial.print("found ");
      Serial.println(rfidNumber);
      Compare(rfidNumber);
      received = false;
    }
}
void printSD() {
    Serial.println("---------------------------");
    Serial.println("SD printout:");
    for(int i=0;i<BLOCKS;i++) {
      for(int j=0;j<PINS_PER_BLOCK;j++) {
        Serial.print("rfid tag: ");
        Serial.print(row[i][j][0]);
        Serial.print(" | device name: ");
        Serial.println(row[i][j][1]);
      }
    }
}
//This function is called when the msgParser gets a command that it didnt handle.
void commandNotFound(uint8_t* pCmd, uint16_t length)
{
    Serial.print("Command not found: ");
    Serial.write(pCmd, length); //print out what command was not found
    Serial.println();           //print out a new line
}
