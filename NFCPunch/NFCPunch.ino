/*Include Library*/
//-----------------------------------------------------------------------------------
//#include <EthernetUdp.h>
//#include <Time.h>  
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>
#include <Adafruit_NFCShield_I2C.h>
#include <TimeDS1302.h>
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <string.h>
#include "utility/debug.h"
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


/*Setting Up NFC*/
//-----------------------------------------------------------------------------------
#define IRQ   (2)
#define RESET (3)  // Not connected by default on the NFC Shield

Adafruit_NFCShield_I2C nfc(IRQ, RESET);
//-----------------------------------------------------------------------------------
 

 /*Setting Up TimeDS1302*/
 //-----------------------------------------------------------------------------------
#define PIN_SCLK 46
#define PIN_IO 49
#define PIN_CE 47

TimeDS1302 clock(PIN_SCLK, PIN_IO, PIN_CE);
//-----------------------------------------------------------------------------------


/*Setting Up WIFI*/
//-----------------------------------------------------------------------------------
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIV2);
                                         
#define WLAN_SSID       "YOUR_SSID"           // cannot be longer than 32 characters!
#define WLAN_PASS       "YOUR_PASS"

#define WLAN_SECURITY   WLAN_SEC_WPA
#define IDLE_TIMEOUT_MS  3000
//-----------------------------------------------------------------------------------

/*Setting Up Internet*/
//-----------------------------------------------------------------------------------
char server[] = "api.pushingbox.com";//213.186.33.19
String NFC_Id("YOUR_ID");
uint32_t ip;

//IPAddress timeServer(132, 163, 4, 101);
//const int timeZone = +8; 

//byte server[] = { 209, 85, 229, 101 };  //Google IP

//EthernetUDP Udp;
//unsigned int localPort = 8888; 
//-----------------------------------------------------------------------------------


/*Setting Up SD card*/
//-----------------------------------------------------------------------------------
File dataFile;
//-----------------------------------------------------------------------------------


/*Global Variable*/
//-----------------------------------------------------------------------------------
uint8_t buttons;
boolean SET = false;
boolean stopclock = false;
int setting=0;
int _sec=0,_min=0,_hour=0,_week=0,_date=0,_month=0,_year=2013;
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
     if (!SD.exists("datalog.txt")) 
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
    
  //Setup Internet//
    if(!cc3000.begin())
    {
      lcd.clear();
      lcd.print("Could't Start");
      while(1);
    }
    
    if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY))
    {
      lcd.clear();
      lcd.print("Could't Connect");
      while(1);
    }
    
    while(!cc3000.checkDHCP())
    {
      delay(100);
    }
    
  Serial.println("Connected to Internet");
  
  if (! cc3000.getHostByName(server, &ip)) {
      Serial.println(F("Couldn't resolve!"));
    }
    cc3000.printIPdotsRev(ip);
  delay(1000);
  lcd.clear();    
 
  
}


void loop(){ 
  

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
      uint8_t keyb[6] = { 0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7 };
          
      // Start with block 4 (the first block of sector 1) since sector 0
      // contains the manufacturer data and it's probably better just
      // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keyb);
          
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
        
		//write IN/OUT to NFC tag in block 6
        if(data2[0] == 'I')
        {
          uint8_t inout[16]={'O','U','T',0,0,0,0,0,0,0,0,0,0,0,0,0};
          nfc.mifareclassic_WriteDataBlock (6, inout);
        }
        else
        {
          uint8_t inout[16]={'I','N',0,0,0,0,0,0,0,0,0,0,0,0,0,0};
          nfc.mifareclassic_WriteDataBlock (6, inout);
        }
        
		//reread block 6 to make sure IN/OUT written
        nfc.mifareclassic_ReadDataBlock(6, data2);        
        
		
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
        
        String time_((String)w+":"+(String)e+":"+(String)r);
        String date_((String)clock.getDate()+clock.getMonth()+(String)clock.getYear());
        String filename(date_+".txt");
        char filename_1[12];
        filename.toCharArray(filename_1,12);
        const char *filename_2=filename_1;
          
	dataFile = SD.open(filename_2, FILE_WRITE);
 
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

       //write to Google SpreadSheet
        if(data2[0]=='I')
        {
          push2drive(dataString,time_,clock.getWeek(),date_,"IN");
        }
        else
        {
          push2drive(dataString,time_,clock.getWeek(),date_,"OUT");
        }
         
          
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
  
  
/*etting time manually*/
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
            _year=processbutton(buttons,_year,"Year =");
          }
          else if(setting==1)
          {
            _month=processbutton(buttons,_month,"Month =");
          }
          else if(setting==2)
          { 
            _date=processbutton(buttons,_date,"Day =");
          }
          else if(setting==3)
          {
            _week=processbutton(buttons,_week,"Week =");
          }
          else if(setting==4)
          { 
            _hour=processbutton(buttons,_hour,"Hour =");
          }
          else if(setting==5)
          { 
            _min=processbutton(buttons,_min,"Minute =");
          }
		  else if(setting==6)
		  {
			_sec=processbutton(buttons,_sec,"Second =");
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
          else if(setting>7||setting<=-1)
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



/*Application Level*/
/*process button*/
//----------------------------------------------------------
int processbutton(int but,int type,String printout)
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(printout);
  lcd.setCursor(0,1);
          
  if (but & BUTTON_UP)
  {
     type=type+1;
     lcd.print(type);
     delay(100);
   }
   else if (but & BUTTON_DOWN)
   {
      type=type-1;
      lcd.print(type);
      delay(100);
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

/*display clock*/
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

/*light red LED*/
//----------------------------------------------------------
void redLed()
{
  digitalWrite(7,HIGH);
  
}
//----------------------------------------------------------

/*light green LED*/
//----------------------------------------------------------
void greenLed()
{
  digitalWrite(6,HIGH);
  beep(200);
}
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

/*Write to Google SpreadSheet*/
//----------------------------------------------------------
void push2drive(String emplo,String time,String week, String date, String inout)
{
  String website("/pushingbox?devid="+NFC_Id+"&emplo="+emplo+"&time="+time+"&week="+week+"&date="+date+"&inout="+inout);
  char POSTID[website.length()+1];
  website.toCharArray(POSTID,website.length()+1);
  
  Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 80);
  if (client.connected()) {
    client.fastrprint(F("GET "));
    client.fastrprint(POSTID);
    client.fastrprint(F(" HTTP/1.1\r\n"));
    client.fastrprint(F("Host: api.pushingbox.com\r\n"));
    client.fastrprint(F("User-Agent: Arduino\r\n"));
    client.fastrprint(F("Content-Type: text/html; charset=utf-8; encoding=gzip\r\n"));
    client.fastrprint(F("Connection: close\r\n"));
    client.fastrprint(F("\r\n"));
    client.println();
  } 
  if (client.available()) {
        char c = client.read();
        Serial.write(c);}
  delay(1);
}
//----------------------------------------------------------

/*Write to SD card*/
//----------------------------------------------------------
void write2sd(String value)
{
 dataFile.print(value);
 dataFile.print("\t");
}
//----------------------------------------------------------


///*-------- NTP code ----------*/
//
//const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
//byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets
//
//time_t getNtpTime()
//{
//  while (Udp.parsePacket() > 0) ; // discard any previously received packets
////  Serial.println("Transmit NTP Request");
//  sendNTPpacket(timeServer);
//  uint32_t beginWait = millis();
//  while (millis() - beginWait < 1500) {
//    int size = Udp.parsePacket();
//    if (size >= NTP_PACKET_SIZE) {
////      Serial.println("Receive NTP Response");
//      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
//      unsigned long secsSince1900;
//      // convert four bytes starting at location 40 to a long integer
//      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
//      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
//      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
//      secsSince1900 |= (unsigned long)packetBuffer[43];
//      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
//    }
//  }
////  Serial.println("No NTP Response :-(");
//  return 0; // return 0 if unable to get the time
//}
//
//// send an NTP request to the time server at the given address
//void sendNTPpacket(IPAddress &address)
//{
//  // set all bytes in the buffer to 0
//  memset(packetBuffer, 0, NTP_PACKET_SIZE);
//  // Initialize values needed to form NTP request
//  // (see URL above for details on the packets)
//  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
//  packetBuffer[1] = 0;     // Stratum, or type of clock
//  packetBuffer[2] = 6;     // Polling Interval
//  packetBuffer[3] = 0xEC;  // Peer Clock Precision
//  // 8 bytes of zero for Root Delay & Root Dispersion
//  packetBuffer[12]  = 49;
//  packetBuffer[13]  = 0x4E;
//  packetBuffer[14]  = 49;
//  packetBuffer[15]  = 52;
//  // all NTP fields have been given values, now
//  // you can send a packet requesting a timestamp:                 
//  Udp.beginPacket(address, 123); //NTP requests are to port 123
//  Udp.write(packetBuffer, NTP_PACKET_SIZE);
//  Udp.endPacket();
//}
