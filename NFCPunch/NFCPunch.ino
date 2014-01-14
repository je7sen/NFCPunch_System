#ifndef INTERNET
//#define INTERNET
#endif

//----------------------------------------------------------------------------------- 
/*Include Library*/
//-----------------------------------------------------------------------------------  
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
#include <Adafruit_NFCShield_I2C.h>
#include <TimeDS1302.h>
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <string.h>
#include "utility/debug.h"
#include "utility/socket.h"
//-----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------- 
/*Setting Up LCD*/
//-----------------------------------------------------------------------------------
Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// These #defines make it easy to set the backlight color
#define RED 0x1
#define YELLOW 0x3
#define GREEN 0x2
#define TEAL 0x6
#define BLUE 0x4
#define VIOLET 0x5
#define WHITE 0x7
//-----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------- 
/*Setting Up NFC*/
//-----------------------------------------------------------------------------------
#define IRQ   (2)
#define RESET (3)  // Not connected by default on the NFC Shield

Adafruit_NFCShield_I2C nfc(IRQ, RESET);
//-----------------------------------------------------------------------------------
 
//----------------------------------------------------------------------------------- 
 /*Setting Up TimeDS1302*/
 //-----------------------------------------------------------------------------------
#define PIN_SCLK 46
#define PIN_IO 49
#define PIN_CE 47

TimeDS1302 clock(PIN_SCLK, PIN_IO, PIN_CE);
//-----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------- 
/*Setting Up WIFI*/
//-----------------------------------------------------------------------------------
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIV2);
                                         
#define WLAN_SSID       "yourSSID"           // cannot be longer than 32 characters!
#define WLAN_PASS       "yourPASS"

#define WLAN_SECURITY   WLAN_SEC_WPA2
#define IDLE_TIMEOUT_MS  3000

#define LISTEN_PORT           80
Adafruit_CC3000_Server ftpServer(LISTEN_PORT);

String ssid;
String pass;
uint8_t mode=2;
status_t newNetworkState = STATUS_DISCONNECTED;
//-----------------------------------------------------------------------------------


//----------------------------------------------------------------------------------- 
/*Setting Up Internet*/
//-----------------------------------------------------------------------------------
char server[] = "api.pushingbox.com";//213.186.33.19
String NFC_Id("vB9504FE1A766082");
char timeServer[] = "time.is";
Adafruit_CC3000_Client client;
//-----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------- 
/*Setting Up SD card*/
//-----------------------------------------------------------------------------------
File dataFile;
File ROOT;
Sd2Card card;
SdVolume volume;
SdFile root;
SdFile file;

#define BUFSIZ 100
//-----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------- 
/*store error strings in flash to save RAM*/
//-----------------------------------------------------------------------------------
#define error(s) error_P(PSTR(s))
//-----------------------------------------------------------------------------------

//----------------------------------------------------------------------------------- 
/*Global Variable*/
//-----------------------------------------------------------------------------------
uint8_t buttons;
uint32_t ip;
boolean SET = false;
boolean stopclock = false;
int setting=0;
int _sec=0,_min=0,_hour=0,_week=0,_date=0,_month=0,_year=2014;
String timeString,First,Second,Third,Forth;
int dateInt1,dateInt2,dateInt3,dateInt4;
String folder[20];
String folder2[20];
int folderCount =0;
int folderIndex=0;
bool firstLayer = true;
bool secondLayer = false;
//-----------------------------------------------------------------------------------

void setup() {
  
  Serial.begin(115200);
  while (!Serial) {
    ; // Needed for Leonardo only
  }
  
  pinMode(48,OUTPUT);
  digitalWrite(48,HIGH);
  clock.begin();
  
  pinMode(4,OUTPUT);  //sd card
  pinMode(8,OUTPUT);  //buzzer
  pinMode(6,OUTPUT);  //green LED  
  pinMode(7,OUTPUT);  //red LED
  
  delay(400);
   
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
       

  lcd.print("HL Granite and");
  lcd.setCursor(0,1);
  lcd.print("Marble Sdn. Bhd.");
  lcd.setBacklight(WHITE);
    
  if (!SD.begin(4)) {
    Serial.println("No SD Card");
   lcd.clear();
   lcd.setCursor(0,0);
   lcd.print("No Sd card.");
   delay(1500);
   // don't do anything more:
   return;
   }
   else{
     if (!SD.exists("secure")) 
     {
       Serial.println("No File Found");
       lcd.clear();
       lcd.print("No File Found.");
       delay(1500);
       return;
     }
     else{
      nfc.begin();
      // configure board to read RFID tags
      nfc.SAMConfig();
      nfc.setPassiveActivationRetries(3);
     }
       
   }
    
    //initialize buzzer & led
    beep(200);
    digitalWrite(6,HIGH);
    digitalWrite(7,HIGH);
    delay(500);
    digitalWrite(6,LOW);
    digitalWrite(7,LOW);
    
    if (!card.init(SPI_HALF_SPEED, 4)) error("card.init failed!");
  
  // initialize a FAT volume
  if (!volume.init(&card)) error("vol.init failed!");

  PgmPrint("Volume is FAT");
  Serial.println(volume.fatType(),DEC);
  Serial.println();
  
  if (!root.openRoot(&volume)) error("openRoot failed");

  // list file in root with date and size
  PgmPrintln("Files found in root:");
  root.ls(LS_DATE | LS_SIZE);
  Serial.println();
  
  // Recursive list of all directories
  PgmPrintln("Files found in all dirs:");
  root.ls(LS_R | LS_SIZE);
  
  
  Serial.println();
  PgmPrintln("Done");
  

  //Get all wifi setting from SD Card
  if(getWlanSetting(&ssid,&pass,&mode)){

    char ssid_c[ssid.length()+1];
    char pass_c[pass.length()+1];
    
    ssid.toCharArray(ssid_c,ssid.length()+1);
    pass.toCharArray(pass_c,pass.length()+1);
     
    const char* SSID_C = ssid_c;
    const char* PASS_C = pass_c;
    
    Serial.println(SSID_C);
    Serial.println(PASS_C);
    Serial.println(mode);
    
    //Setup Internet//
    if(!cc3000.begin())
    {
      Serial.println("Could't Start Wifi");
      lcd.clear();
      lcd.print("Could't Start");
      while(1);
    }
    
    if (!cc3000.connectToAP(SSID_C,PASS_C, mode))
    {
      Serial.println("Could't Connect to network");
      lcd.clear();
      lcd.print("Could't Connect");
      while(1);
    }
    
    while(!cc3000.checkDHCP())
    {
      delay(100);
    }
    
    //get the server ready
    ftpServer.begin();
    
    //display connected info
    while (! displayConnectionDetails()) {
    delay(1000);
    }
    
  Serial.println("Connected to Internet");
    
  }
    

#if defined INTERNET
  if(getOnlineTime(&timeString))
  {
    setToOnlineTime(timeString);
  }
#endif

   
  delay(1000);
  lcd.clear();    
 
  
}


void loop(){ 
  
  status_t networkState=cc3000.getStatus();
  
  
  
  if(networkState != newNetworkState){
   Serial.println(networkState); 
   newNetworkState = networkState;
  }
  char clientline[BUFSIZ];
  int index = 0;
  Adafruit_CC3000_ClientRef clientRef = ftpServer.available();

  if (clientRef) {
    // an http request ends with a blank line
    boolean current_line_is_blank = true;
 
    // reset the input buffer
    index = 0;
 
    while (clientRef.connected()) {
      if (clientRef.available()) {
        char c = clientRef.read();
 
        // If it isn't a new line, add the character to the buffer
        if (c != '\n' && c != '\r') {
          clientline[index] = c;
          index++;
          // are we too big for the buffer? start tossing out data
          if (index >= BUFSIZ) 
            index = BUFSIZ -1;
 
          // continue to read more data!
          continue;
        }
 
        // got a \n or \r new line, which means the string is done
        clientline[index] = 0;
 
        // Print it out for debugging
        Serial.println(clientline);
 
        // Look for substring such as a request to get the root file
        if (strstr(clientline, "GET / ") != 0) {
          // send a standard http response header
          clientRef.println("HTTP/1.1 200 OK");
          clientRef.println("Content-Type: text/html");
          clientRef.println();
 
          // print all the files, use a helper to keep it clean
          clientRef.println("<h2>Files:</h2>");
          firstLayer = true;
          secondLayer = false;
          root.seekSet(416);
          Serial.println(root.curPosition());
          ListFiles(root,clientRef, LS_R,0);
          Serial.println(root.curPosition());
          
        }else if (strstr(clientline, "GET /") != 0) {
          
          
           // this time no space after the /, so a sub-file!
          char *direc;
 
          direc = clientline + 5; // look after the "GET /" (5 chars)
          // a little trick, look for the " HTTP/1.1" string and 
          // turn the first character of the substring into a 0 to clear it out.
          (strstr(clientline, " HTTP"))[0] = 0;
          
          
          
          bool isFolder = false;
          for(int k=0; k<20; k++)
          {
            if(firstLayer){
              if(folder[k].equals((String)direc))
              {
                
                Serial.println("1st Layer");
                Serial.println(folder[k]);
                
                char di[folder[k].length()+1];
                folder[k].toCharArray(di,folder[k].length()+1);
                
                folderIndex = k;
                
                firstLayer = false;
                secondLayer = true;   
   
                // send a standard http response header
                clientRef.println("HTTP/1.1 200 OK");
                clientRef.println("Content-Type: text/html");
                clientRef.println();
                SdFile s;
                clientRef.println("<h2>Files:</h2>");
                Serial.println(root.curPosition());
                bool ope = s.open(root, di, O_READ); 
                if (ope){
                  Serial.println("opened the folder");
                  ListFiles(s, clientRef, LS_R, 2);
                }
                Serial.println(root.curPosition());
//                root.seekSet(416);
                
                isFolder = true;
                k=20;
                
              }
            }else if(secondLayer){
              if(folder2[k].equals((String)direc))
              {
                
                Serial.println("2nd Layer");

               char di[folder[folderIndex].length()+1];
               folder[folderIndex].toCharArray(di,folder[folderIndex].length()+1);
                
               char di2[folder2[k].length()+1];
               folder2[k].toCharArray(di2,folder2[k].length()+1);
               
               const char* DI=(const char*)di;
               const char* DI2=(const char*)di2;
                Serial.println(di);
                Serial.println(di2); 
                
                // send a standard http response header
                clientRef.println("HTTP/1.1 200 OK");
                clientRef.println("Content-Type: text/html");
                clientRef.println();
                SdFile s;
                root.seekSet(416);
                Serial.println(root.curPosition());
                
                delay(20);
                bool ope = s.open(&root ,di ,O_READ);
                clientRef.println("<h2>Files:</h2>");
                if (ope){
                  s.rewind();
                  Serial.println("step1");
                  SdFile d;
                  delay(20);
                  bool ope1 = d.open(&s ,di2 ,O_READ);
                  if(ope1){
                  Serial.println("opened the folder");
                  ListFiles(d, clientRef, LS_R, 2);}
                }
                isFolder = true;
                k=20;
                
                firstLayer = true;
                secondLayer = false;
              }
            }
          }
          
          if(!isFolder)
          {
            // print the file we want
          if (! file.open(&root, direc, O_READ)) {
            clientRef.println("HTTP/1.1 404 Not Found");
            clientRef.println("Content-Type: text/html");
            clientRef.println();
            clientRef.println("<h2>File Not Found!</h2>");
            break;
          }
          
          Serial.println("Opened!");
 
          clientRef.println("HTTP/1.1 200 OK");
          clientRef.println("Content-Type: text/plain");
          clientRef.println();
 
          int16_t c;
          while ((c = file.read()) > 0) {
              // uncomment the serial to debug (slow!)
              //Serial.print((char)c);
              clientRef.print((char)c);
          }
          file.close();
          }
            
//              
//          
//          (strstr(clientline, " HTTP"))[0] = 0;
//          String direct_ = "/"+(String)direc+"/";
//          char di[direct_.length()+1];
//          direct_.toCharArray(di,direct_.length()+1);
//          
//          // print all the files, use a helper to keep it clean
//          clientRef.println("<h2>Files:</h2>");
//          ROOT = SD.open(di);
//          printDirectory(clientRef,ROOT); 
        
        } else {
          // everything else is a 404
          clientRef.println("HTTP/1.1 404 Not Found");
          clientRef.println("Content-Type: text/html");
          clientRef.println();
          clientRef.println("<h2>File Not Found!</h2>");
        }
        break;
      }
    }
    // give the web browser time to receive the data
    delay(100);
    clientRef.close();
  }

  if(!stopclock)
  {
    digitalClockDisplay(); //display clock on LCD
  
   
  uint8_t success;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
    
  
  //waiting for a tag 
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  

  lcd.setCursor(0, 0);
 
  
  if (success) {
    
    if (uidLength == 4)
    {
          
      // Now we need to try to authenticate it for read/write access
      // default KeyA: 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
      // default KeyB: 0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7
      uint8_t keyab[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
          
      // Start with block 4 (the first block of sector 1) since sector 0
      // contains the manufacturer data and it's probably better just
      // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
          
      if (success)
      {      

        uint8_t data[16];
        uint8_t data1[16];
        uint8_t data2[16];
        uint8_t w,e,r;
        String dataString = "";
        
        // If you want to write something to block 4 to test with, uncomment
        // the following line and this text should be read back in a minute
	// data = { 'a', 'd', 'a', 'f', 'r', 'u', 'i', 't', '.', 'c', 'o', 'm', 0, 0, 0, 0};
	// success = nfc.mifareclassic_WriteDataBlock (4, data);

        // Try to read the contents of block 4,5 & 6
        success = nfc.mifareclassic_ReadDataBlock(4, data);
        success = nfc.mifareclassic_ReadDataBlock(5, data1);
        success = nfc.mifareclassic_ReadDataBlock(6, data2);
        w=clock.getHour();
        e=clock.getMinute();
        r=clock.getSecond();
        uint8_t buff = data2[0];
         
        
        String time_((String)w+":"+(String)e+":"+(String)r);
        int yearIn2Digit = clock.getYear() - 2000;
        String dateS("***->"+(String)clock.getDate()+"*");
        String block = time_ + dateS;
        
        
        char blockChar[16];
        block.toCharArray(blockChar, block.length()+1);
        

        const char * blockTo = blockChar;
        //write all 4 punch in a day inside NFC tag.
        //write to cloud when 4 punch recorded.
        if(data2[15] == 0)
        {
	  //write IN/OUT to NFC tag in block 6
          if(data2[0] == 'I')
          {
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
            delay(10);
            uint8_t inout[16]={'O','U','T',0,0,0,0,0,0,0,0,0,0,0,0,'0'};
            nfc.mifareclassic_WriteDataBlock (6, inout);
          }
          else
          {
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
            delay(10);
            uint8_t inout[16]={'I','N',0,0,0,0,0,0,0,0,0,0,0,0,0,'1'};
            nfc.mifareclassic_WriteDataBlock (6, inout);
            
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 12, 1, keyab);
            delay(10);
            nfc.mifareclassic_WriteNDEFString (13, blockTo);
          }
        }
        else if(data2[15] == '1')
        {
	  //write IN/OUT to NFC tag in block 6
          if(data2[0] == 'I')
          {
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
            delay(10);
            uint8_t inout[16]={'O','U','T',0,0,0,0,0,0,0,0,0,0,0,0,'2'};
            nfc.mifareclassic_WriteDataBlock (6, inout);
            
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 12, 1, keyab);
            delay(10);
            nfc.mifareclassic_WriteNDEFString (14, blockTo);
          }
          else
          {
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
            delay(10);
            uint8_t inout[16]={'I','N',0,0,0,0,0,0,0,0,0,0,0,0,0,'1'};
            nfc.mifareclassic_WriteDataBlock (6, inout);
          }
        }
        else if(data2[15] == '2')
        {
	  //write IN/OUT to NFC tag in block 6
          if(data2[0] == 'I')
          {
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
            delay(10);
            uint8_t inout[16]={'O','U','T',0,0,0,0,0,0,0,0,0,0,0,0,'2'};
            nfc.mifareclassic_WriteDataBlock (6, inout);
          }
          else
          {
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
            delay(10);
            uint8_t inout[16]={'I','N',0,0,0,0,0,0,0,0,0,0,0,0,0,'3'};
            nfc.mifareclassic_WriteDataBlock (6, inout);
            
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 16, 1, keyab);
            delay(10);
            nfc.mifareclassic_WriteNDEFString (17, blockTo);
          }
        }
        else if(data2[15] == '3')
        {
	  //write IN/OUT to NFC tag in block 6
          if(data2[0] == 'I')
          {
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
            delay(10);
            uint8_t inout[16]={'O','U','T',0,0,0,0,0,0,0,0,0,0,0,0,'4'};
            nfc.mifareclassic_WriteDataBlock (6, inout);
            
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 16, 1, keyab);
            delay(10);
            nfc.mifareclassic_WriteNDEFString (18, blockTo);
          }
          else
          {
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
            delay(10);
            uint8_t inout[16]={'I','N',0,0,0,0,0,0,0,0,0,0,0,0,0,'3'};
            nfc.mifareclassic_WriteDataBlock (6, inout);
          }
        }
        else if(data2[15] == '4')
        {
	  //write IN/OUT to NFC tag in block 6
          if(data2[0] == 'I')
          {
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
            delay(10);
            uint8_t inout[16]={'O','U','T',0,0,0,0,0,0,0,0,0,0,0,0,'4'};
            nfc.mifareclassic_WriteDataBlock (6, inout);
          }
          else
          {
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
            delay(10);
            uint8_t inout[16]={'I','N',0,0,0,0,0,0,0,0,0,0,0,0,0,'1'};
            nfc.mifareclassic_WriteDataBlock (6, inout);
            
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 12, 1, keyab);
            delay(10);
            nfc.mifareclassic_WriteNDEFString (13, blockTo);
            
            const char *blank = "Blank";
            nfc.mifareclassic_WriteNDEFString (14, blank);
            
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 16, 1, keyab);
            delay(10);
            nfc.mifareclassic_WriteNDEFString (17, blank);
            nfc.mifareclassic_WriteNDEFString (18, blank);     
          }
        }
        else
        {
          if(data2[0] == 'I')
          {
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
            delay(10);
            uint8_t inout[16]={'O','U','T',0,0,0,0,0,0,0,0,0,0,0,0,0};
            nfc.mifareclassic_WriteDataBlock (6, inout);
          }
          else
          {
            nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
            delay(10);
            uint8_t inout[16]={'I','N',0,0,0,0,0,0,0,0,0,0,0,0,0,'1'};
            nfc.mifareclassic_WriteDataBlock (6, inout);
          }
        }
        nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 1, keyab);
        delay(10);
        //reread block 6 to make sure IN/OUT written
        nfc.mifareclassic_ReadDataBlock(6, data2); 
        
       if(data2[15]=='4')
       { 
        uint8_t Time_1[16];
        uint8_t Time_2[16];
        uint8_t Time_3[16];
        uint8_t Time_4[16];
        
        nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 12, 1, keyab);
        delay(10);
        nfc.mifareclassic_ReadDataBlock (13, Time_1);
        nfc.mifareclassic_ReadDataBlock (14, Time_2); 
        
        nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 16, 1, keyab);
        delay(10);
        nfc.mifareclassic_ReadDataBlock (17, Time_3);
        nfc.mifareclassic_ReadDataBlock (18, Time_4);  
       
        First = processTime(Time_1,&dateInt1);
        Second = processTime(Time_2,&dateInt2);
        Third = processTime(Time_3,&dateInt3);
        Forth = processTime(Time_4,&dateInt4);
        
	}	
	//compare the value to check whether written or not
	//then display the name of employees
        if (buff != data2[0])
        {
          digitalWrite(7,LOW);
          greenLed();
          lcd.clear();
		
          //print the name to LCD
          for(uint8_t y=0; y<5; y++)
          {
          lcd.print((char)data[11+y]);
          dataString += String((char)data[11+y]);
          }
        
                  
           for(uint8_t y=0; y<16; y++)
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
        
	//print time on LCD
        lcd.print(w);
        printDigits(e);
        printDigits(r);
        lcd.print(' ');

        //print IN/OUT to LCD
        for(uint8_t y=0; y<16; y++)
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
        
        
        String date_((String)clock.getDate()+clock.getMonth()+(String)clock.getYear());
        String monthfileName((String)clock.getDate()+clock.getMonth()+(String)(clock.getYear()-2000));
        String filename("/"+(String)clock.getYear()+"/"+clock.getMonth()+"/"+monthfileName+".txt");
        String filepath("/"+(String)clock.getYear()+"/"+clock.getMonth()+"/");
        
        char filename_1[filename.length()+1];
        filename.toCharArray(filename_1,filename.length()+1);
        Serial.println(filename);
        
        char filepath_[filepath.length()+1];
        filepath.toCharArray(filepath_,filepath.length()+1);
        
        SD.mkdir(filepath_);  
	dataFile = SD.open(filename_1, FILE_WRITE);
         
       if(dataFile){
       write2sd(dataString);
       write2sd(time_);
       write2sd(clock.getWeek());
       write2sd(date_);

       if(data2[0]=='I')
       {
         dataFile.println("IN");
       }
       else
       {
         dataFile.println("OUT");
       }

       dataFile.close();

       }  
       else
       {
         Serial.println("Could't write to SD");
       }
       
       if(data2[15]=='4')
       {
         String monthfileName1(clock.getMonth()+(String)(clock.getYear()-2000));
         String filename2("/"+(String)clock.getYear()+"/"+clock.getMonth()+"/"+monthfileName1+".txt");
         String filepath("/"+(String)clock.getYear()+"/"+clock.getMonth()+"/");
         char filename_2[filename2.length()+1];
         
         filename2.toCharArray(filename_2,filename2.length()+1);
         Serial.println(filename2);
        
        char filepath_[filepath.length()+1];
        filepath.toCharArray(filepath_,filepath.length()+1);
        
        SD.mkdir(filepath_);     
	dataFile = SD.open(filename_2, FILE_WRITE);
        if(dataFile)
        {
          write2sd(dataString);
          write2sd(First);
          write2sd(Second);
          write2sd(Third);
          write2sd(Forth);
          write2sd(clock.getWeek());
          write2sd(date_);
          if(data2[0]=='I')
         {
           dataFile.println("IN");
         }
         else
         {
           dataFile.println("OUT");
         }
       }
       else
       {
         Serial.println("Could't write to SD");
       }
      
        dataFile.close();
       }
       

         
#if defined INTERNET
       //write to Google SpreadSheet
        if(data2[0]=='I')
        {
          push2drive(dataString,time_,clock.getWeek(),date_,"IN");
        }
        else
        {
          push2drive(dataString,time_,clock.getWeek(),date_,"OUT");
        }
#endif
         
          
          // Wait a bit before reading the card again
         
         closeAll();
         delay(1000);
         lcd.clear();
        }
        else
        {
          redLed();
          lcd.clear();
          lcd.print("     Error!");
          lcd.setCursor(0,1);
          lcd.print("  Punch Again!");
          delay(500);
          lcd.setCursor(0,0);

        }
      }

    }
 
  }
  }
  
  
/*setting time manually*/
//read button press
 buttons = lcd.readButtons();
    
  if (buttons) {
    stopclock = true;
    lcd.clear();
    lcd.setCursor(0,0);
	
    //if button SELECT pressed?
    if (buttons & BUTTON_SELECT)
    {
      //to close the time display and card punch function
      SET = true; 
      
      
      while(SET)
      {
                
        buttons = lcd.readButtons();
               
        if (buttons)
        {
                 
          if(setting==0)
          {
            _year=processbutton(buttons,_year,"Year =",2050);
          }
          else if(setting==1)
          {
            _month=processbutton(buttons,_month,"Month =",12);
          }
          else if(setting==2)
          { 
            _date=processbutton(buttons,_date,"Day =",31);
          }
          else if(setting==3)
          {
            _week=processbutton(buttons,_week,"Week =",7);
          }
          else if(setting==4)
          { 
            _hour=processbutton(buttons,_hour,"Hour =",23);
          }
          else if(setting==5)
          { 
            _min=processbutton(buttons,_min,"Minute =",59);
          }
	 else if(setting==6)
	 {
	    _sec=processbutton(buttons,_sec,"Second =",59);
	  }
          else if(setting==7)
          { 
            String time_week((String)_hour+":"+(String)_min+":"+(String)_sec+"   "+(String)_week);
	    String date_1((String)_date+_month+(String)_year);
		  
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(time_week);
            lcd.setCursor(0,1);
	    lcd.print(date_1);
            
            if (buttons & BUTTON_LEFT)
            {
               setting=setting-1;
               delay(100);
            }
            else if (buttons & BUTTON_RIGHT)
            {
              setting=setting+1;
              delay(100);
            }
            else if (buttons & BUTTON_SELECT)
            {
               SET = false;
               stopclock = false;
               lcd.clear();
         
               //manually set time and date
               clock.set_time(_sec,_min,_hour,_week,_date,_month,_year);
            }
          }
          else if(setting==8 ||setting==-1)
          {
            setting=0;
            delay(100);
          }
          
          String time_2((String)_hour+":"+(String)_min+":"+(String)_sec+"   "+(String)_week);
	  String date_2((String)_date+_month+(String)_year);
          
          switch(setting)
          {
          case 0:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Year =");
            lcd.setCursor(0,1);
            lcd.print(_year);
            break;
          case 1:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Month =");
            lcd.setCursor(0,1);
            lcd.print(_month);
            break;
           case 2:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Day =");
            lcd.setCursor(0,1);
            lcd.print(_date);
            break;
           case 3:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Week =");
            lcd.setCursor(0,1);
            lcd.print(_week);
            break;
           case 4:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Hour =");
            lcd.setCursor(0,1);
            lcd.print(_hour);
            break;
           case 5:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Minute =");
            lcd.setCursor(0,1);
            lcd.print(_min);
            break;
           case 6:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print("Second =");
            lcd.setCursor(0,1);
            lcd.print(_sec);
            break;
           case 7:
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(time_2);
            lcd.setCursor(0,1);
	    lcd.print(date_2);
            break;
           
        }
           
        }
        
        }
        
        lcd.clear();
      }
    }
}


//----------------------------------------------------------
/*Application Level*/
//----------------------------------------------------------
/*Get Online time*/
//----------------------------------------------------------
String getTimeString(char dd[],int a, int b)
{
  String d="";
  for(a;a<b+1;a++)
  {
    d += dd[a];
  }
  return d;
}

int getweekdayInt(String weekdayStr)
{
  int wdInt = 0;
  
    if(weekdayStr == "Mon")
    {
      wdInt = 1;
    }
    else if(weekdayStr == "Tue")
    {
      wdInt = 2;
    }
    else if(weekdayStr == "Wed")
    {
      wdInt = 3;
    }
    else if(weekdayStr == "Thr")
    {
      wdInt = 4;
    }
    else if(weekdayStr == "Fri")
    {
      wdInt = 5;
    }
    else if(weekdayStr == "Sat")
    {
      wdInt = 6;
    }
    else if(weekdayStr == "Sun")
    {
      wdInt = 7;
    }
  return wdInt;
}

int getmonthInt(String monthStr)
{
  int mInt = 0;
  if(monthStr == "Jan")
  {
      mInt = 1;
  }
  else if(monthStr == "Feb")
  {
      mInt = 2;
  }
  else if(monthStr == "Mac")
  {
      mInt = 3;
  }
  else if(monthStr == "Apr")
  {
      mInt = 4;
  }
  else if(monthStr == "May")
  {
      mInt = 5;
  }
  else if(monthStr == "Jun")
  {
      mInt = 6;
  }
  else if(monthStr == "Jul")
  {
      mInt = 7;
  }
  else if(monthStr == "Aug")
  {
      mInt = 8;
  }
  else if(monthStr == "Sep")
  {
      mInt = 9;
  }
  else if(monthStr == "Oct")
  {
      mInt = 10;
  }
  else if(monthStr == "Nov")
  {
      mInt = 11;
  }
  else if(monthStr == "Dec")
  {
      mInt = 12;
  }  
  return mInt;
}
  
boolean getOnlineTime(String *timeStr)
{
  boolean returnValue = false;
  if (! cc3000.getHostByName(timeServer, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    
  client = cc3000.connectTCP(ip, 80);
  if (client.connected()) {
    client.fastrprint(F("GET /Beijing HTTP/1.1\r\n"));
    client.fastrprint(F("Host: time.is\r\n"));
    client.fastrprint(F("User-Agent: Arduino\r\n"));
    client.fastrprint(F("Content-Type: text/html\r\n"));
    client.fastrprint(F("Connection: close\r\n"));
    client.fastrprint(F("\r\n"));
    client.println();
  } 
  else
  {
    Serial.println("Could't Get online Time!");
  }
  
  unsigned long lastRead = millis();
  while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (client.available()) {
      char c = client.read();
      if(c =='E')
      {
        c = client.read();
        if(c =='x')
        {
        c = client.read();
        if(c =='p')
        {
        c = client.read();
        if(c =='i')
        {
        c = client.read();
        if(c == 'r')
        {
          c = client.read();
          if(c == 'e')
          {
            c = client.read();
            if(c == 's')
            {
              c = client.read();
              if(c == ':')
              {
                while( c != '\n')
                {
                  c=client.read();
                  Serial.print(c);
                  *timeStr += c;
                }
                returnValue = true;
              }
            }
          }
        }
        }
        }
        }
      }
      
      lastRead = millis();
    }
  }
  client.close();
  return returnValue;
}


void setToOnlineTime(String timeStr)
{
  Serial.println(timeStr);
  char d[timeStr.length()+1];
  timeStr.toCharArray(d,timeStr.length()+1);
  
  String weekdayString = getTimeString(d,1,3);
  int weekdayInt = getweekdayInt(weekdayString);
  Serial.println(weekdayInt);
  
  String dayString = getTimeString(d,6,7);
  int dayInt = dayString.toInt();
  Serial.println(dayInt);
  
  String monthString = getTimeString(d,9,11);
  int monthInt = getmonthInt(monthString);
  Serial.println(monthInt);
  
  String yearString = getTimeString(d,13,16);
  int yearInt = yearString.toInt();
  Serial.println(yearInt);
  
  String hourString = getTimeString(d,18,19);
  int hourInt = hourString.toInt();
  Serial.println(hourInt);
  
  String minString = getTimeString(d,21,22);
  int minInt = minString.toInt();
  Serial.println(minInt);
  
  String secString = getTimeString(d,24,25);
  int secInt = secString.toInt();
  Serial.println(secInt);
  
  //Set the time when get the time from time.is/Beijing  
  clock.set_time(secInt,minInt,hourInt,weekdayInt,dayInt,monthInt,yearInt);
  Serial.println("Time had been adjusted Automatically");
}
//----------------------------------------------------------   


//----------------------------------------------------------
/*process Time for write to cloud*/
//----------------------------------------------------------
String processTime(uint8_t timeArray[],int *dateI)
{
  int s=0;
  String Time_Str1="";
  String Date_Str1="";
  for(int h = 0; h<8; h++)
  {
    if(timeArray[h]!='*')
    {
      Time_Str1 += (char)timeArray[h];
      s++;
     }
   }
  s = s+5;
  for(int h=0; h<2;h++)
  {
    if(timeArray[h+s]!='*')
    {
      Date_Str1+=(char)timeArray[h+s];
    }
   }
   *dateI = Date_Str1.toInt();
  return Time_Str1;
}
//----------------------------------------------------------


//----------------------------------------------------------
/*process button*/
//----------------------------------------------------------
int processbutton(int but,int type,String printout,int limit)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(printout);
  lcd.setCursor(0,1);
          
  if (but & BUTTON_UP)
  {
    if(type < limit)
    {
     type=type+1;
     lcd.print(type);
     delay(100);
    }else
    {
      type = 0;
    }
   }
   else if (but & BUTTON_DOWN)
   {
     if(type < 1)
     {
       type = limit;
     }
     else
     {
      type=type-1;
      lcd.print(type);
      delay(100);
     }
    }
    else if (but & BUTTON_LEFT)
    {
       setting=setting-1;
       delay(100);
     }
     else if (but & BUTTON_RIGHT)
     {
        setting=setting+1;
        delay(100);
     }
     else if (but & BUTTON_SELECT)
     {
         SET = false;
         stopclock = false;
         lcd.clear();
                        
     }
     
     return type;
}
//----------------------------------------------------------

//----------------------------------------------------------
/*display clock*/
//----------------------------------------------------------
void digitalClockDisplay(){
  // digital clock display of the time
  lcd.print(clock.getHour());
  printDigits(clock.getMinute());
  printDigits(clock.getSecond());
  lcd.print(" ");
  
  lcd.print(clock.getWeek());
  lcd.print(" ");
  lcd.setCursor(0, 1);
  lcd.print(clock.getDate());
  lcd.print(" ");
  
  lcd.print(clock.getMonth());
  lcd.print(" ");
  lcd.print(clock.getYear()); 
  
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  lcd.print(":");
  if(digits < 10)
    lcd.print('0');
  lcd.print(digits);
}
//----------------------------------------------------------

//----------------------------------------------------------
/*ring the buzzer*/
//----------------------------------------------------------
void beep(unsigned char delayms)
{
  analogWrite(8, 50);
  delay(delayms);
  analogWrite(8, 0);
  delay(delayms);
}
//----------------------------------------------------------

//----------------------------------------------------------
/*light red LED*/
//----------------------------------------------------------
void redLed()
{
  digitalWrite(7,HIGH);
  
}
//----------------------------------------------------------

//----------------------------------------------------------
/*light green LED*/
//----------------------------------------------------------
void greenLed()
{
  digitalWrite(6,HIGH);
  beep(200);
}
//----------------------------------------------------------

//----------------------------------------------------------
/*Close all LED and Buzzer*/
//----------------------------------------------------------
void closeAll()
{
  digitalWrite(8,LOW);
  digitalWrite(6,LOW);
  digitalWrite(7,LOW);
}
//----------------------------------------------------------

//----------------------------------------------------------
/*Write to Google SpreadSheet*/
//----------------------------------------------------------
void push2drive(String emplo,String time,String week, String date, String inout)
{
  ip = 0;
  String website(NFC_Id+"&emplo="+emplo+"&time="+time+"&week="+week+"&date="+date+"&inout="+inout);
  char POSTID[website.length()+1];
  website.toCharArray(POSTID,website.length()+1);

 
  if (! cc3000.getHostByName(server, &ip)) {
      Serial.println(F("Couldn't resolve!"));
  }
  
  client = cc3000.connectTCP(ip, 80);
  
  if (client.connected()) {
    client.fastrprint(F("GET /pushingbox?devid="));
    client.fastrprint(POSTID);
    client.fastrprint(F(" HTTP/1.1\r\n"));
    client.fastrprint(F("Host: api.pushingbox.com\r\n"));
    client.fastrprint(F("User-Agent: Arduino\r\n"));
    client.fastrprint(F("Content-Type: text/html\r\n"));
    client.fastrprint(F("Connection: close\r\n"));
    client.fastrprint(F("\r\n"));
    client.println();
  } 
  else
  {
    Serial.println("Could't Write to CLoud !");
  }
  unsigned long lastRead = millis();
  while (client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
      lastRead = millis();
    }
  }
  client.close();
  delay(1);
}
//----------------------------------------------------------

//----------------------------------------------------------
/*Write to SD card*/
//----------------------------------------------------------
void write2sd(String value)
{
 dataFile.print(value);
 dataFile.print("\t");
}
//----------------------------------------------------------

//----------------------------------------------------------
/*Display connection detail*/
//----------------------------------------------------------
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}
//----------------------------------------------------------

//----------------------------------------------------------
/*handle error for sd card*/
//----------------------------------------------------------
void error_P(const char* str) {
  PgmPrint("error: ");
  SerialPrintln_P(str);
  if (card.errorCode()) {
    PgmPrint("SD error: ");
    Serial.print(card.errorCode(), HEX);
    Serial.print(',');
    Serial.println(card.errorData(), HEX);
  }
  while(1);
}
//----------------------------------------------------------

//----------------------------------------------------------
/*List all the file inside sd card*/
//----------------------------------------------------------
void ListFiles(SdFile root,Adafruit_CC3000_ClientRef client, uint8_t flags, uint8_t indent) {
  // This code is just copied from SdFile.cpp in the SDFat library
  // and tweaked to print to the client output in html!
  dir_t p;
  uint16_t counter =0;
  int folderCount = 0;
  root.rewind();
  client.println("<ul>");
  while (root.readDir(&p) > 0) {
    counter ++;
    // done if past last used entry
    if (p.name[0] == DIR_NAME_FREE) break;
 
    // skip deleted entry and entries for . and  ..
    if (p.name[0] == DIR_NAME_DELETED || p.name[0] == '.') ;//continue;
 
    // only list subdirectories and files
    if (!DIR_IS_FILE_OR_SUBDIR(&p)) ;//continue;
 
    // print any indent spaces
    client.print("<li><a href=\"");
    for (uint8_t i = 0; i < 11; i++) {
      if (p.name[i] == ' ') continue;
      if (i == 8) {
        client.print('.');
      }
      client.print((char)p.name[i]);
    }
    client.print("\">");
    
    // print file name with possible blank fill
    for (uint8_t i = 0; i < 11; i++) {
      if (p.name[i] == ' ') continue;
      if (i == 8) {
        client.print('.');
      }
      client.print((char)p.name[i]);
    }
 
    client.print("</a>");
 
    if (DIR_IS_SUBDIR(&p)) {
      client.print('/');
    }
 
    // print modify date/time if requested
    if (flags & LS_DATE) {
       root.printFatDate(p.lastWriteDate);
       client.print(' ');
       root.printFatTime(p.lastWriteTime);
    }
    // print size if requested
    if (!DIR_IS_SUBDIR(&p) && (flags & LS_SIZE)) {
      client.print(' ');
      client.print(p.fileSize);
    }
    client.println("</li>");
    
    // list subdirectory content if requested
    if ((flags & LS_R) && DIR_IS_SUBDIR(&p)) {
      uint16_t index = root.curPosition()/32 - 1;
      if(firstLayer){
        folder[folderCount] = "";
        for (uint8_t i = 0; i < 11; i++) {
          if (p.name[i] == ' ') continue;
          folder[folderCount] += (char)p.name[i];
        }
      
        Serial.print("Folder = ");
        Serial.println(folder[folderCount]);
        folderCount ++;
        folder[folderCount] = "";
        folder[folderCount] += index;
        folderCount ++;
      }else if(secondLayer){
        folder2[folderCount] = "";
        for (uint8_t i = 0; i < 11; i++) {
          if (p.name[i] == ' ') continue;
          folder2[folderCount] += (char)p.name[i];
        }
      
        Serial.print("Folder = ");
        Serial.println(folder2[folderCount]);
        folderCount ++;
        folder2[folderCount] = "";
        folder2[folderCount] += index;
        folderCount ++;
      }
//      SdFile s;
//      if (s.open(&root, index, O_READ)){
//        ListFiles(s, client, LS_R, indent +2);
//      }
//      root.seekSet(32 * (index + 1));
    }
    
    if(counter>2000)
      {
        counter = 0;
        break;
      }
    
  }
  client.println("</ul>");
}
//----------------------------------------------------------


//----------------------------------------------------------
/*List all the file inside sd card*/
//----------------------------------------------------------
void printDirectory(Adafruit_CC3000_ClientRef client,File dir) {
  
  // Begin at the start of the directory
  dir.rewindDirectory();
  
  client.println("<ul>");
  Serial.println("af ul");
  while(true){
     Serial.println("bf Entry");
     File entry =  dir.openNextFile(FILE_READ);
     Serial.println("af entry");

     if (! entry) {
       // no more files
       //Serial.println("**nomorefiles**");
       Serial.println("will break");
       break;
     }
     
    client.print("<li><a href=\"");
    client.print(entry.name());
    client.print("\">");
    
    // print file name with possible blank fill
    client.print(entry.name());
    client.print("</a>");
     
     // Recurse for directories, otherwise print the file size
     if (entry.isDirectory()) {
       client.println("/</li>");
       folder[folderCount] = "";
       folder[folderCount] += entry.name();
       folderCount ++;
//       printDirectory(client,entry);
     } else {
       // files have sizes, directories do not
       client.print("\t");
       client.print(entry.size(), DEC);
       client.println("</li>");
     } 
     entry.close();  
   }
   
   client.println("</ul>");
}

//----------------------------------------------------------


//----------------------------------------------------------
/*Get internet setting from sdcard*/
//----------------------------------------------------------
boolean getWlanSetting(String *ssid_c,String *pass_c, uint8_t *mode_c) {
  
  boolean retValue = false;
  
  String ssid;
  String pass;
  String mode;
  
  File myFile = SD.open("internet.cfg");
  
  if(myFile){
  char b;
  uint8_t n=0;

  while((b=myFile.read())>0){
//    b = myFile.read();
    if(b=='"'){
      n++;
    }
    if(n==1){
      if(b!='"'){
        ssid = ssid + (String)b;
      }
    }
    else if(n==3){
      if(b!='"'){
        pass = pass + (String)b;
      }
    }
    else if(n==5){
      if(b!='"'){
        mode = mode + (String)b;
      }
    }
  }
  myFile.close();
  
  uint8_t securityMode;

  if(mode.equals("WLAN_SEC_UNSEC")){
    securityMode = 0;
  }
  else if(mode.equals("WLAN_SEC_WEP")){
    securityMode = 1;
  }
  else if(mode.equals("WLAN_SEC_WPA")){
    securityMode = 2;
  }
  else if(mode.equals("WLAN_SEC_WPA2")){
    securityMode = 3;
  }
  retValue = true; 

  *pass_c = pass;
  *ssid_c = ssid;
  *mode_c = securityMode;
 }
  
  return retValue;
}

//----------------------------------------------------------


