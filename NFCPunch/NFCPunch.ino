/* 
 * TimeSerialDateStrings.pde
 * example code illustrating Time library date strings
 *
 * This sketch adds date string functionality to TimeSerial sketch
 * Also shows how to handle different messages
 *
 * A message starting with a time header sets the time
 * A Processing example sketch to automatically send the messages is inclided in the download
 * On Linux, you can use "date +T%s > /dev/ttyACM0" (UTC time zone)
 *
 * A message starting with a format header sets the date format
 *
 * send: Fs\n for short date format
 * send: Fl\n for long date format 
 */ 
 
#include <Time.h>  
#include <Wire.h>
#include <SD.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
#include <Adafruit_NFCShield_I2C.h>

// The shield uses the I2C SCL and SDA pins. On classic Arduinos
// this is Analog 4 and 5 so you can't use those for analogRead() anymore
// However, you can connect other I2C sensors to the I2C bus and share
// the I2C bus.
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7

#define IRQ   (2)
#define RESET (3)  // Not connected by default on the NFC Shield

Adafruit_NFCShield_I2C nfc(IRQ, RESET);

// single character message tags
#define TIME_HEADER   'T'   // Header tag for serial time sync message
#define TIME_REQUEST  7     // ASCII bell character requests a time sync message 

static boolean isLongFormat = false;
const int chipSelect = 4;

void setup()  {
  Serial.begin(115200);
  while (!Serial) {
    ; // Needed for Leonardo only
  }
 
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  
  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println("Waiting for sync message");
  lcd.print("HL Granite and");
  lcd.setCursor(0,1);
  lcd.print("Marble Sdn. Bhd.");
  lcd.setBacklight(WHITE);
  
  pinMode(9,OUTPUT);
  pinMode(4,OUTPUT);
  
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");
  
  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
  nfc.setPassiveActivationRetries(2);
  Serial.println("Waiting for an ISO14443A Card ...");
  
  
}


void loop(){ 
  
  
 if (Serial.available()) {
    
    char c = Serial.read();
    if( c == TIME_HEADER) {
      processSyncMessage();
    }
    lcd.clear();
  }
  
  if (timeStatus()!= timeNotSet) {
    digitalClockDisplay();  
  } 
  
  uint8_t i=0;
  uint8_t success;
  uint8_t success1;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
   
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  
// set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 0);
  // print the number of seconds since reset:
  
  
  
  if (success) {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    
    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ... 
      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
	  
      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
      Serial.println("Trying to authenticate block 4 with default KEYA value");
      uint8_t keya[6] = { 0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7 };
	  
	  // Start with block 4 (the first block of sector 1) since sector 0
	  // contains the manufacturer data and it's probably better just
	  // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);
	  
      if (success)
      {
        Serial.println("Sector 1 (Blocks 4..7) has been authenticated");
        uint8_t data[16];
        uint8_t data1[16];
        uint8_t data2[16];
        uint8_t inout[16];
        int w,e,r;
	String dataString = "";
	
        // If you want to write something to block 4 to test with, uncomment
		// the following line and this text should be read back in a minute
//         data = { 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0};
//         success = nfc.mifareclassic_WriteDataBlock (4, data);

        // Try to read the contents of block 4
        success = nfc.mifareclassic_ReadDataBlock(4, data);
        success = nfc.mifareclassic_ReadDataBlock(5, data1);
        success = nfc.mifareclassic_ReadDataBlock(6, data2);
        w=hour();
        e=minute();
        r=second();
        
        if(data2[0] == 'I')
        {
          uint8_t inout[16]={'O','U','T',0,0,0,0,0,0,0,0,0,0,0,0,0};
          success = nfc.mifareclassic_WriteDataBlock (6, inout);
        }
        else
        {
          uint8_t inout[16]={'I','N',0,0,0,0,0,0,0,0,0,0,0,0,0,0};
          success = nfc.mifareclassic_WriteDataBlock (6, inout);
        }
        	
	
        if (success)
        {
          lcd.clear();
          // Data seems to have been read ... spit it out
          Serial.println("Reading Block 4:");
          nfc.PrintHexChar(data, 16);
          Serial.println("");
          for(int y=0; y<5; y++)
          {
          lcd.print((char)data[11+y]);
          dataString += String((char)data[11+y]);
          }
          nfc.PrintHexChar(data1, 16);
          Serial.println("");
          
                  
          
          for(int y=0; y<16; y++)
          {
                       
            if(data1[y] == 0xFE  )
            {;}
            else if(data1[y] == 0x00)
            {;}
            else
            {
              lcd.print((char)data1[y]);
              dataString += String((char)data1[y]);
            }
            
        }
        lcd.setCursor(0,1);
        
        lcd.print(w);
        printDigits(e);
        printDigits(r);
        lcd.print(' ');
        lcd.print(' ');
        lcd.print(' ');
        
        for(int y=0; y<16; y++)
          {
                       
            if(data2[y] == 0xFE  )
            {;}
            else if(data2[y] == 0x00)
            {;}
            else
            {
              lcd.print((char)data2[y]);
            }
            
        }
        nfc.PrintHexChar(data2, 16);
          Serial.println("");
          
          File dataFile = SD.open("datalog.txt", FILE_WRITE);
          
          if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }  
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
          
          // Wait a bit before reading the card again
         delay(1000);
         lcd.clear();
        }
        else
        {
          Serial.println("Ooops ... unable to read the requested block.  Try another key?");
        }
      }
      else
      {
        Serial.println("Ooops ... authentication failed: Try another key?");
      }
    }
    
    if (uidLength == 7)
    {
      // We probably have a Mifare Ultralight card ...
      Serial.println("Seems to be a Mifare Ultralight tag (7 byte UID)");
	  
      // Try to read the first general-purpose user page (#4)
      Serial.println("Reading page 4");
      uint8_t data[32];
      success = nfc.mifareultralight_ReadPage (4, data);
      if (success)
      {
        // Data seems to have been read ... spit it out
        nfc.PrintHexChar(data, 4);
        Serial.println("");
		
        // Wait a bit before reading the card again
        delay(1000);
      }
      else
      {
        Serial.println("Ooops ... unable to read the requested page!?");
      }
    }
  }
  
  
  
    
  
    
  

  uint8_t buttons = lcd.readButtons();
  
  
  
  //button function
  if (buttons) {
    lcd.clear();
    lcd.setCursor(0,0);
    if (buttons & BUTTON_UP) {
      lcd.print("UP ");
      lcd.setBacklight(RED);
    }
    if (buttons & BUTTON_DOWN) {
      lcd.print("DOWN ");
      lcd.setBacklight(YELLOW);
    }
    if (buttons & BUTTON_LEFT) {
      lcd.print("LEFT ");
      lcd.setBacklight(GREEN);
    }
    if (buttons & BUTTON_RIGHT) {
      lcd.print("RIGHT ");
      lcd.setBacklight(TEAL);
    }
    if (buttons & BUTTON_SELECT) {
      lcd.print("SELECT ");
      lcd.setBacklight(VIOLET);
    }
  }
 
  
  
  
}

void digitalClockDisplay(){
  // digital clock display of the time
  lcd.print(hour());
  printDigits(minute());
  printDigits(second());
  lcd.print(" ");
  
   lcd.print(dayShortStr(weekday()));
  lcd.print(" ");
  lcd.setCursor(0, 1);
  lcd.print(day());
  lcd.print(" ");
  
     lcd.print(monthShortStr(month()));
  lcd.print(" ");
  lcd.print(year()); 
  
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  lcd.print(":");
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}



void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013 - paul, perhaps we define in time.h?

   pctime = Serial.parseInt();
   if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
     setTime(pctime); // Sync Arduino clock to the time received on the serial port
   } 
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

