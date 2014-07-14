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

//msgparser vars

//list of strings. You can have as many as you want.
//each string is saved in flash memory only and does not take up any ram.
char s0[] PROGMEM = "start";
char s1[] PROGMEM = "end";
char s2[] PROGMEM = "rfid";
char s3[] PROGMEM = "digit";
//This is our look up table. It says which function to call when a particular string is received
FuncEntry_t functionTable[] PROGMEM = {
//   String     Function
    {s0,        sayHello        },
    {s1,        sayBye          },
    {s2,        getTag          },
    {s3,        add   }
    };
//this is the compile time calculation of the length of our look up table.
int funcTableLength = (sizeof functionTable / sizeof functionTable[0]);     //number of elements in the function table
MsgParser myParser;     //this creates our parser

//Init
void setup()
{
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
   
//  if (!SD.begin(6)) {
  if (!SD.begin(10,11,12,13)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");
 
  OpenSD();
  
  //testing area!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  Serial.println();
//  for(int i=0;i<BLOCKS;i++) {
//    for(int j=0;j<PINS_PER_BLOCK;j++) {
//      Serial.print(row[i][j][0]);
//      Serial.println(row[i][j][1]);
//    }
//  }

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
      break;
    }
  }
  ReceivedChannelNumber=s;
}

void Compare(int scannedTag)
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
    int SDtag=row[selected_block][index][0].toInt();
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

    if(scannedTag == SDtag)
    {
      Serial.print("TempVal equals tag#");
      if(ReceivedChannelNumber.toInt()==index+1)
      {
        Serial.print("Match"); //python cloud program looks for "Match"
        Serial.println(result);
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
        Serial.println(result);
        lcd.clear();
        lcd.print("X ");
        lcd.print(deviceName);
        lcd.print(" on ");
        lcd.print(expectingPinNumber);
        if (realReceivingPinNumber != "") {
          lcd.print(" wired to ");
          lcd.print(realReceivingPinNumber);
        }
        else {
          lcd.print(" not wired");
        }
        break;
      }  
    }
    else 
    {
      if(index==index_row)
      {
        Serial.print("index: " + index);
        Serial.println(" index_row: " + index_row);
        Serial.println(" scanned tag: " + scannedTag);
        Serial.println(" stored tag: " + SDtag);
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
void add() {}
void getTag()
{
    long rfidNumber;
    rfidNumber = myParser.getLong();

    Serial.print("found ");
    Serial.println (rfidNumber);
    Compare(rfidNumber);
}
//This function is called when the msgParser gets a command that it didnt handle.
void commandNotFound(uint8_t* pCmd, uint16_t length)
{
    Serial.print("Command not found: ");
    Serial.write(pCmd, length); //print out what command was not found
    Serial.println();           //print out a new line
}
