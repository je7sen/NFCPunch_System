#include <Ethernet.h>
//#include <EthernetUdp.h>
#include <Time.h>  
#include <Wire.h>
#include <SPI.h>
//#include <SD.h>
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

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0D, 0xB9, 0xDF }; 
char server[] = "api.pushingbox.com";
String NFC_Id("vB9504FE1A766082");

////IPAddress timeServer(132, 163, 4, 101);
////const int timeZone = +8; 
EthernetClient client;
//byte server[] = { 209, 85, 229, 101 };  //Google IP

//File dataFile;

//EthernetUDP Udp;
//unsigned int localPort = 8888; 

void setup() {
  
  Serial.begin(115200);
  while (!Serial) {
    ; // Needed for Leonardo only
  }
  
  pinMode(9,OUTPUT);
  pinMode(4,OUTPUT);  //sd card
  pinMode(5,OUTPUT);  //buzzer
  pinMode(6,OUTPUT);  //green LED  
  pinMode(7,OUTPUT);  //red LED
  
  delay(400);
   
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
       
  setSyncProvider(requestSync);  //set function to call when sync required
//  Serial.println("Waiting for sync message");
  lcd.print("HL Granite and");
  lcd.setCursor(0,1);
  lcd.print("Marble Sdn. Bhd.");
  lcd.setBacklight(WHITE);
    
//   if (!SD.begin(4)) {
//    lcd.clear();
//    lcd.setCursor(0,0);
//    lcd.print("No Sd card.");
//    delay(1500);
//    // don't do anything more:
//    return;
//    }
//    else{
//      if (!SD.exists("datalog.txt")) 
//      {
//        lcd.clear();
//        lcd.print("No File Found.");
//        delay(1500);
//        return;
//      }
//      else{
      nfc.begin();
      // configure board to read RFID tags
      nfc.SAMConfig();
      nfc.setPassiveActivationRetries(3);
      //  Serial.println("Waiting for an ISO14443A Card ...");
//      }
       
//    }
    
    beep(200);
    digitalWrite(6,HIGH);
    digitalWrite(7,HIGH);
    delay(500);
    digitalWrite(6,LOW);
    digitalWrite(7,LOW);
    
       
//    if (Ethernet.begin(mac) == 0) {
//    // no point in carrying on, so do nothing forevermore:
//    while (1) {
////      Serial.println("Failed to configure Ethernet using DHCP");
//      delay(10000);
//    }
//  }
  
  delay(1000);
 
//  Serial.print("IP number assigned by DHCP is ");
//  Serial.println(Ethernet.localIP());
//  Udp.begin(localPort);
//  Serial.println("waiting for sync");
//  setSyncProvider(getNtpTime);
    
 
  
}

time_t prevDisplay = 0;

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
  
//  if (timeStatus() != timeNotSet) {
//    if (now() != prevDisplay) { //update the display only if time has changed
//      prevDisplay = now();
//      digitalClockDisplay();  
//    }
//  }
  
  
  uint8_t success;
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
//    Serial.println("Found an ISO14443A card");
//    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
//    Serial.print("  UID Value: ");
//    nfc.PrintHex(uid, uidLength);
//    Serial.println("");
    
    if (uidLength == 4)
    {
      // We probably have a Mifare Classic card ... 
//      Serial.println("Seems to be a Mifare Classic card (4 byte UID)");
	  
      // Now we need to try to authenticate it for read/write access
      // Try with the factory default KeyA: 0xFF 0xFF 0xFF 0xFF 0xFF 0xFF
//      Serial.println("Trying to authenticate block 4 with default KEYA value");
      uint8_t keya[6] = { 0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7 };
	  
	  // Start with block 4 (the first block of sector 1) since sector 0
	  // contains the manufacturer data and it's probably better just
	  // to leave it alone unless you know what you're doing
      success = nfc.mifareclassic_AuthenticateBlock(uid, uidLength, 4, 0, keya);
	  
      if (success)
      {
//        Serial.println("Sector 1 (Blocks 4..7) has been authenticated");
        uint8_t data[16];
        uint8_t data1[16];
        uint8_t data2[16];
        uint8_t w,e,r;
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
        uint8_t buff = data2[0];
        
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
        
        nfc.mifareclassic_ReadDataBlock(6, data2);	
	
        if (buff != data2[0])
        {
          digitalWrite(7,LOW);
          greenLed();
          lcd.clear();
          // Data seems to have been read ... spit it out
//          Serial.println("Reading Block 4:");
//          nfc.PrintHexChar(data, 16);
//          Serial.println("");
          for(uint8_t y=0; y<5; y++)
          {
          lcd.print((char)data[11+y]);
          dataString += String((char)data[11+y]);
          }
//          nfc.PrintHexChar(data1, 16);
//          Serial.println("");
          
                  
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
        
        lcd.print(w);
        printDigits(e);
        printDigits(r);
        lcd.print(' ');
        lcd.print(' ');
        lcd.print(' ');
        
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
        
        String time_(String(w)+":"+String(e)+":"+String(r));
        String date_(String(day())+monthShortStr(month())+String(year()));
//        nfc.PrintHexChar(data2, 16);
//          Serial.println("");
          
//   dataFile = SD.open("datalog.txt", FILE_WRITE);
//        
//   if (dataFile) {
//        write2sd(dataString);
//        write2sd(time_);
//        write2sd(dayShortStr(weekday()));
//        write2sd(date_);
//
//        if(data2[0]=='I')
//        {
//          dataFile.println("IN");
//        }
//        else
//        {
//          dataFile.println("OUT");
//        }
//         
//        
//        dataFile.close();
//        // print to the serial port too:
//        // Serial.println(dataString);
//        }  
//        // if the file isn't open, pop up an error:
//        else {
//        // Serial.println("error opening datalog.txt");
//        }
        
       if(client.connect(server,80))
        {
                             
          if(data2[0]=='I')
        {
          push2drive(dataString,time_,dayShortStr(weekday()),date_,"IN");
        }
        else
        {
          push2drive(dataString,time_,dayShortStr(weekday()),date_,"OUT");
        }
           delay(1000);
           client.stop();     
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
//          Serial.println("Ooops ... unable to read the requested block.  Try another key?");
        }
      }
      else
      {
//        Serial.println("Ooops ... authentication failed: Try another key?");
      }
    }
 
  }
  
//  uint8_t buttons = lcd.readButtons();
    
  //button function
//  if (buttons) {
//    lcd.clear();
//    lcd.setCursor(0,0);
//    if (buttons & BUTTON_UP) {
//      lcd.print("UP ");
//      lcd.setBacklight(RED);
//    }
//    if (buttons & BUTTON_DOWN) {
//      lcd.print("DOWN ");
//      lcd.setBacklight(YELLOW);
//    }
//    if (buttons & BUTTON_LEFT) {
//      lcd.print("LEFT ");
//      lcd.setBacklight(GREEN);
//    }
//    if (buttons & BUTTON_RIGHT) {
//      lcd.print("RIGHT ");
//      lcd.setBacklight(TEAL);
//    }
//    if (buttons & BUTTON_SELECT) {
//      lcd.print("SELECT ");
//      lcd.setBacklight(VIOLET);
//    }
//  }
 
  
  
  
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

void beep(unsigned char delayms)
{
  analogWrite(5, 50);
  delay(delayms);
  analogWrite(5, 0);
  delay(delayms);
}

void redLed()
{
  digitalWrite(7,HIGH);
  
}

void greenLed()
{
  digitalWrite(6,HIGH);
  beep(200);
//  digitalWrite(5,HIGH);
}

void closeAll()
{
  digitalWrite(5,LOW);
  digitalWrite(6,LOW);
  digitalWrite(7,LOW);
}

time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}

void push2drive(String emplo,String time,String week, String date, String inout)
{
  String postID("GET /pushingbox?devid="+NFC_Id+"&emplo="+emplo+"&time="+time+"&week="+week+"&date="+date+"&inout="+inout+" HTTP/1.1");
  
  client.println(postID);
  client.println("Host: api.pushingbox.com");
  client.println("User-Agent: Arduino");
  client.println("Content-Type: text/html; charset=utf-8; encoding=gzip");
  client.println("Connection: close");
  client.println();
  
  delay(1);
}

//void write2sd(String value)
//{
//  dataFile.print(value);
//  dataFile.print("\t");
//}

//
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

