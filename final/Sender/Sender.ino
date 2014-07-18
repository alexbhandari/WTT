/* added instructions for clarity -alex b
    This file is loaded on to the wire tracing sender unit.
      The external libraries Keypad.h (from the arduino site) must be added
      and a 3rd party library SD.h (from https://github.com/adafruit/SD)
      replaces the standard SD library in the stock version of arduino (this is needed for it to work on the mega version of the arduino board).
*/
/** Sender code. Sends the pin and block number over serial through the 20 pins attached through RS232 cable.
    The probe receives these signals and the sender displays the information from the gridmap corresponding to
    the attached pin from the gridmap. These serial signals are also read by the receiver.
*/

#include <SoftwareSerial.h>
#include <SD.h>
#include <Keypad.h>
#include <SPI.h>
#include <avr/pgmspace.h>
#include <MsgParser.h>

//program states
const int WAIT_FOR_BLOCK_SELECT = 0;
const int READ_PROBE = 1;
int state = WAIT_FOR_BLOCK_SELECT;
//dimensions of row[][][] and determines how to parse gridmap
const int BLOCKS = 5;
const int PINS_PER_BLOCK = 20;
const int DATA_PER_PIN = 2;
String row[BLOCKS][PINS_PER_BLOCK][DATA_PER_PIN] = { "" };
String s[]={"01","02","03","04","05","06","07","08","09","10","11","12","13","14","15","16","17","18","19","20"};
//keypad setup
const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};
byte rowPins[ROWS] = {5, 4, 3, 2}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {8, 7, 6}; //connect to the column pinouts of the keypad
Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );
int n=16;
String syster=NULL;

//Msgparser is a library that allows the Receiver and Sender to communicate with each other

//msgparser variables
//list of strings. You can have as many as you want.
//each string is saved in flash memory only and does not take up any ram.
char s0[] PROGMEM = "block";   //block command used to update the block number
char s1[] PROGMEM = "pin";     //pin command used to update the pin number
char s2[] PROGMEM = "printsd"; //allows the sd printout method to be run from the computer
//This is our look up table. It says which function to call when a particular string is received
FuncEntry_t functionTable[] PROGMEM = {
//   String     Function
    {s0,        setBlock    },
    {s1,        setPin  },
    {s2,        printSD     }
    };
//this is the compile time calculation of the length of our look up table.
int funcTableLength = (sizeof functionTable / sizeof functionTable[0]);     //number of elements in the function table
MsgParser myParser;     //this creates our parser

//other variables
char key;
File myFile;
boolean refresh = 1;
int selected_block;
int ReceivedChannelNumber;
int first = 1;

//This runs once when the arduino powers on. It is where everything is initialized
void setup()
{
  Serial.begin(9600);
  setSize(6, 4);
  backlightOn();

  //SD shield setup
  selectLineOne();
  Serial.print("Initializing...");
  //sets cs pin to 10
  pinMode(10, OUTPUT);   
   
  if (!SD.begin(10,11,12,13)) {
    selectLineTwo();
    Serial.print("initialization failed");
    return;
  }
  //selectLineTwo();
  Serial.print("initialization done.");
  //clearLCD();
 
  OpenSD();
  selectLineOne();
  clearLCD();
  
  // msgparser setup
  myParser.setTable(functionTable, funcTableLength);      //tell the parser to use our lookup table
  myParser.setHandlerForCmdNotFound(commandNotFound);     //Tell the parser which function to call when it gets a command it doesn't handle


}
//This part loops while the arduino is powered
void loop() {
  
  //State machine - the contents of the variable "state" determines the operation of the arduino
  switch (state) {
    
    case WAIT_FOR_BLOCK_SELECT: //waits for the user to select a terminal block
    {
      if(refresh) { //only prints messages once
        refresh = 0;
        selectLineOne();
        Serial.print("Choose block");
        selectLineTwo();
        Serial.print("number:");
      }
      key = keypad.getKey();
      int num = (int)(key) - 48; //converts ascii number to integer
      if(num > 0 && num < 10) { //key press from 1 to 9 is allowed for selecting a terminal block
        state = READ_PROBE;
        refresh = 1;
        selected_block = num;
      }
      
      break;
    } 
    case READ_PROBE: //monitors the probe (connected through the serial RX input) for a connected wire
    {
      if(!Serial.available() && refresh == 1) //only print once while the probe is floating (not connected to a wire)
      {
         refresh = 0;
         clearLCD();
         selectLineOne();
         Serial.print("-scanning probe-");
         selectLineTwo();
         Serial.print(" (* to go back)");
      }
      if(Serial.available()) { //when a wire is connected, print name and pin number to the screen and serial
         while ( Serial.available() )  myParser.processByte(Serial.read () ); //msgparser handles commands to update ReceivedChannelNumber
         clearLCD();
         selectLineOne();
         Serial.print(getName(ReceivedChannelNumber - 1)); //prints the selected pin
         selectLineTwo();
         Serial.print("Pin: ");
         Serial.print(ReceivedChannelNumber);
	       refresh = 1; //allows "-scanning probe-" message to refresh after the probe is disconnected
         delay(1000);
      }
      if(keypad.getKey() == '*') { //go back to block select
        clearLCD();
        state = WAIT_FOR_BLOCK_SELECT;
        refresh = 1;
      }
      sendChannelString(); //sends the pin numbers through the pins using serial
      break;
    }  
    default:  //should not occur
      return;
  }

}
String getName(int index) {
   String deviceName = row[selected_block][index][1]; 
   deviceName = deviceName.substring(0,deviceName.length()-1);
   return deviceName;
}
/** Reads the gridmap off the sd card using carrage returns ('\n') to delimit different pins
 ** and * or # to separate pin number, rfid_tag, and device name.
 ** Gridmap format:
 ** pn*rfid_tag*Device Name
 ** 01*15019521234*Tank In
*/
void OpenSD()
{
  if (SD.exists("MAPPING.txt")) {
    Serial.println("MAPPING.txt exists.");
  }
  else {
    Serial.println("MAPPING.txt doesn't exist.");
  }
  myFile = SD.open("MAPPING.txt");
  if (myFile) {
    selectLineOne();
    Serial.print("Reading from SD card...");
   
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
       }

       if(file_contents=='\n')
       {
         pin++;
         data_index=0;
       }

       else if(file_contents=='*' || file_contents=='#')
       {
         data_index++;
       }

       else
       {
         //need to exclude pin number which is currently listed in the grid map
         if(data_index-1 > -1) {
            row[block-1][pin-1][data_index-1].concat(file_contents);
         }
       }

    }
    // close the file:
    selectLineTwo();
    Serial.print("Finished reading");
    delay(500);
    clearLCD();
    
    myFile.close();
  }
  else {
    // if the file didn't open, print an error:
    selectLineTwo();
    Serial.print("error opening MAPPING.txt");
    delay(500);
    
  }
}

//Sends the pin number preceeded by "_" on each pin (numbers 1-20) in serial format
//arduino output pins 22-41 correspond to pins 1-20
void sendChannelString()
{
  for(int n=22;n<42;n++)
  {
     SoftwareSerial wtt(2,n);
     wtt.begin(9600);
     wtt.print("block ");
     wtt.print(selected_block);
     wtt.print("\r");
     wtt.print("pin ");
     wtt.print(s[n-22]);
     wtt.print("\r");
  }
}


void selectLineOne(){  //puts the cursor at line 0 char 0.
   Serial.write(0xFE);   //command flag
   Serial.write(128);    //position
   delay(10);
}
void selectLineTwo(){  //puts the cursor at line 0 char 0.
   Serial.write(0xFE);   //command flag
   Serial.write(192);    //position
   delay(10);
}
//not used
void goTo(int position) { 

   //position = line 1: 0-15, line 2: 16-31, 31+ defaults back to 0
   if (position<16){ Serial.write(0xFE);   //command flag
      Serial.write((position+128));    //position
   }
   else if (position<32){Serial.write(0xFE);   //command flag
      Serial.write((position+48+128));    //position 
   } 
   else { 
      goTo(0); 
   }
   delay(10);
}

void clearLCD(){
   Serial.write(0xFE);   //command flag
   Serial.write(0x01);   //clear command.
   delay(10);
}
void backlightOn(){  //turns on the backlight
    Serial.write(0x7C);   //command flag for backlight stuff
    Serial.write(157);    //light level.
   delay(10);
}
void backlightOff(){  //turns off the backlight
    Serial.write(0x7C);   //command flag for backlight stuff
    Serial.write(128);     //light level for off.
   delay(10);
}
void serCommand(){   //a general function to call the command flag for issuing all other commands   
  Serial.write(0xFE);
}
void setSize(int lines, int wide)
{
  Serial.write(0x7C);
  Serial.write(lines); //lines:4 lines is 5; 2 lines is 6;
  Serial.write(wide); //wide: 20 characters wide is 3;  16 characters wide is 4;
  delay(10);
}

//msgparser methods
//Wire probe on the reciever is wired to the arduino's serial input.
//setPin and setBlock reads the communications over this line.
void setPin() //updates the connected pin number, run through the command "pin"
{
  ReceivedChannelNumber = myParser.getInt();
}
void setBlock() {}
void printSD() {
    Serial.println("---------------------------");
    Serial.println("SD printout:");
    for(int i=0;i<BLOCKS;i++) {
      Serial.print("Block ");
      Serial.println(i+1);
      for(int j=0;j<PINS_PER_BLOCK;j++) {
        Serial.print("rfid tag: ");
        Serial.print(row[i][j][0]);
        Serial.print(" | device name: ");
        Serial.println(row[i][j][1]);
      }
    }
}
//This function is called when the msgParser gets a command that it didnt handle.
void commandNotFound(uint8_t* pCmd, uint16_t length) //updates the connected block number, run through the command "block"
{
    Serial.print("Command not found: ");
    Serial.write(pCmd, length); //print out what command was not found
    Serial.println();           //print out a new line
}
