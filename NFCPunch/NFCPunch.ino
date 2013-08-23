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
// include the library code:
#include <Wire.h>
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>

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

// single character message tags
#define TIME_HEADER   'T'   // Header tag for serial time sync message
#define FORMAT_HEADER 'F'   // Header tag indicating a date format message
#define FORMAT_SHORT  's'   // short month and day strings
#define FORMAT_LONG   'l'   // (lower case l) long month and day strings

#define TIME_REQUEST  7     // ASCII bell character requests a time sync message 

static boolean isLongFormat = false;

void setup()  {
  Serial.begin(9600);
  
  // set up the LCD's number of columns and rows: 
  lcd.begin(16, 2);
  while (!Serial) ; // Needed for Leonardo only
  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println("Waiting for sync message");
  lcd.print("Hello, world!");
  lcd.setBacklight(WHITE);
}

uint8_t i=0;
void loop(){  

// set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 0);
  // print the number of seconds since reset:
  

  uint8_t buttons = lcd.readButtons();
  
  if (Serial.available()) {
    char c = Serial.read();
    if( c == TIME_HEADER) {
      processSyncMessage();
    }
    else if( c== FORMAT_HEADER) {
      processFormatMessage();
    }
  }
  if (timeStatus()!= timeNotSet) {
    digitalClockDisplay();  
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
  delay(1000);
  
  
  
}

void digitalClockDisplay(){
  // digital clock display of the time
  lcd.print(hour());
  printDigits(minute());
  printDigits(second());
  lcd.print(" ");
  if(isLongFormat)
    lcd.print(dayStr(weekday()));
  else  
   lcd.print(dayShortStr(weekday()));
  lcd.print(" ");
  lcd.setCursor(0, 1);
  lcd.print(day());
  lcd.print(" ");
  if(isLongFormat)
     lcd.print(monthStr(month()));
  else
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

void  processFormatMessage() {
   char c = Serial.read();
   if( c == FORMAT_LONG){
      isLongFormat = true;
      lcd.println("Setting long format");
   }
   else if( c == FORMAT_SHORT){
      isLongFormat = false;   
      lcd.println("Setting short format");
   }
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

