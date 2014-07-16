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

//program states
const int WAIT_FOR_BLOCK_SELECT = 0;
const int READ_PROBE = 1;
int state = WAIT_FOR_BLOCK_SELECT;
//dimensions of row[][][] and determines how to parse gridmap
const int BLOCKS = 1;
const int PINS_PER_BLOCK = 15;
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

//other variables
char key;
File myFile;
boolean refresh = 1;
int max_row_index=0;
int selected_block;
String ReceivedChannelNumber;

int first = 1;

void setup()
{
  Serial.begin(9600);
  setSize(6, 4);
  backlightOn();

  //SD setup
  selectLineOne();
  Serial.print("Initializing...");
  //comments below are probably not correct - alex b.
  // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
  // Note that even if it's not used as the CS pin, the hardware SS pin
  // (10 on most Arduino boards, 53 on the Mega) must be left as an output
  // or the SD library functions will not work.
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
  
  //need to figure out where row is populated
  // Serial.println(row[0]);
  // Serial.println(row[1]);
  // Serial.println(row[0].substring(3,14)); 
}

void loop() {
  
  //State machine
  switch (state) {
    
    case WAIT_FOR_BLOCK_SELECT:
    {
      if(refresh) {
        refresh = 0;
        selectLineOne();
        Serial.print("Choose block        number:");
      }
      key = keypad.getKey();
      int num = (int)(key) - 48; //convert ascii to integer
      if(num > 0 && num < 10) { //key press is 1-9
        state = READ_PROBE;
        refresh = 1;
        selected_block = num;
      }
      
      break;
    } 
    case READ_PROBE:
    {
      sendChannelString(); //sends the pin numbers through the pins using serial
      read_sender(); //reads the selected pin number into RecievedChannelNumber
      showChannelName(); //prints row[selected_block][RecievedChannelNumber][NAME]
      
      if(keypad.getKey() == '*') {
        clearLCD();
        state = WAIT_FOR_BLOCK_SELECT;
        refresh = 1;
      }
      
      break;
    }  
    default:  //should not occur
      return;
  }

}

void showChannelName()
{
  int index;
  //int errorFlag=0; //0 is correct(), 1 is wrong.
 
  for(index=0;index<=max_row_index;index++) 
  {
    if(ReceivedChannelNumber.toInt()==index+1) {
      clearLCD();
      String str = row[selected_block][index][1];
      //str = str.substring(0,str.length()-1);
      Serial.print(str); //prints the selected pin
      selectLineTwo();
      Serial.print("Pin: ");
      Serial.print(index+1);
      break;
     }   
  }
}

//Wire probe on the reciever is wired to the arduino's serial input.
//This method reads the communications over this line...
//    input:_123_456 is read into ReceivedChannelNumber as 123 and 456
void read_sender()
{
  String s;
  if(!Serial.available() && refresh == 1)
  {
    refresh = 0;
    clearLCD();
    Serial.print("-scanning probe-     (* to go back)");
  }
  
  while(Serial.available())
  {
    char ch=char(Serial.read());
    if(ch=='_')
    {
      char ch1=Serial.read();
      s.concat(ch1);
      char ch2=Serial.read();
      s.concat(ch2);
      refresh = 1;
      
      break;
    }
  }
  ReceivedChannelNumber=s;
}

/**Reads the gridmap off the sd card using carrage returns ('\n') to delimit different pins
 **and * or # to separate pin number, rfid_tag, and device name.
Gridmap format:
pn*rfid_tag*Device Name
01*15019521234*Tank In
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
    max_row_index = PINS_PER_BLOCK - 1; //sets the max row index to one less than the number of pins in the grid map
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
     wtt.print("_");
     wtt.print(s[n-22]);
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
