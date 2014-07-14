#include <SoftwareSerial.h>
#include <SD.h>
#include <Keypad.h>
#include <SPI.h>

/* added comments for clarity -alex b
    This file is loaded on to the wire tracing sender unit.
      The external libraries Keypad.h (from the arduino site) must be added
      and a 3rd party library SD.h (from https://github.com/adafruit/SD)
      replaces the standard SD library in the stock version of arduino (this is needed for it to work on the mega version of the arduino board).
*/

const int WAIT_FOR_POUND = 0;
const int WAIT_FOR_BLOCK_SELECT = 1;
const int READ_PROBE = 2;

int state = WAIT_FOR_POUND;
char key;

///////////////////////////keypad//////////////////////
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
////////////////////////////////////////////////////////

String s[]={"01","02","03","04","05","06","07","08","09","10","11","12","13","14","15","16","17","18","19","20"};
File myFile;
String row[30]="";
int index_row=0;
String ReceivedChannelNumber;

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
    
    case WAIT_FOR_POUND:
    {
      selectLineOne();
      Serial.print("Press # to continue:");
      key = keypad.getKey();
      if (key != NO_KEY){
        goTo(n);
        syster.concat(key);
        Serial.print(key);
        n++;
      }
      if(key == '#') {
        state = WAIT_FOR_BLOCK_SELECT;
      }
      
      break;
    }
    case WAIT_FOR_BLOCK_SELECT:
    {
      selectLineOne();
      Serial.print("Choose block number:");
      key = keypad.getKey();
      if(key == '5') {
        state = READ_PROBE;
      }
      
      break;
    } 
    case READ_PROBE:
    {
      sendChannelString(); //just a command for LCD
      read_sender(); //sets the variable: RecievedChannelNumber
      showChannelName(row); //prints row[RecievedChannelNumber]
      
      if(keypad.getKey() == '*') {
        clearLCD();
        state = WAIT_FOR_POUND;
      }
      
      break;
    }  
    default:  //should not occur
      return;
  }

}

void showChannelName(String row[])
{
  int index;
  //int errorFlag=0; //0 is correct(), 1 is wrong.
 
  for(index=0;index<=index_row;index++)
  {
    if(ReceivedChannelNumber.toInt()==index+1)
    {
      
      //Serial.println(row[index].substring(3,14));
      clearLCD();
      Serial.print(row[index].substring(15)); //prints the selected pin, the index corresponds to the pin number, which corresponds to the substring of row where the information for the pin is located
      //
     // Serial.print("Please connect to Channel" + ReceivedChannelNumber );
      /*
      lcd.println(row[index].substring(3,14));
      lcd.print(row[index].substring(15));
      lcd.clear();
        //lcd.selectLine(1);
      lcd.print("Matching       ");
        //lcd.selectLine(2);
      lcd.print(row[index].substring(15));
      */
      break;
     }
     
     
  }
}

void read_sender()
{
  String s;
  if(!Serial.available())
  {
    clearLCD();
    Serial.print("-scanning probe- (* to go back)");
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
      Serial.println("connected");
      
      
      break;
    }
  }
  ReceivedChannelNumber=s;
 
  //Serial.println(s);--------------------------------------------test
}



void OpenSD()
{
  if (SD.exists("MAPPING.txt")) {
    Serial.println("MAPPING.txt exists.");
  }
  else {
    Serial.println("MAPPING.txt doesn't exist.");
  }
  myFile = SD.open("MAPPING.txt");////////////////////////////////////////////////////////////66666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666666
  if (myFile) {
   // Serial.println("MAPPING.txt:");------------------------------------------test
    selectLineOne();
    Serial.print("Reading from SD card...");
   
    // read from the file until there's nothing else in it:
    char file_contents;
    int index=0;
    while (myFile.available()) { //reads the file byte by byte and places each byte in the string row at the corresponding index
       file_contents=myFile.read();
       if(file_contents=='\n')
       {
         index_row++;
       }
       if(file_contents!='\n')
       {
         row[index_row].concat(file_contents);
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

void sendChannelString() //prints the 22 to 41 bytes of s to the display, this is likely why different information is printed to the screen and serial
{
  for(int n=22;n<42;n++)
  {
    SoftwareSerial wtt(2,n); //initialize the LCD serial backpack on pins 2, 22-41
    wtt.begin(9600);
    wtt.print("_" + s[n-22]);
    /*
    if(Serial.available())
    {
     Serial.print(char(Serial.read()));
    }
    */
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
