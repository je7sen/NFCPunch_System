#include <TimeDS1302.h>
#include <SPI.h>

#define PIN_SCLK 51
#define PIN_IO 49
#define PIN_CE 47

TimeDS1302 clock(PIN_SCLK, PIN_IO, PIN_CE);

void setup() {
  
  pinMode(53,OUTPUT);
  digitalWrite(53,HIGH);
  
  clock.begin();
//  clock.set_time(00,58,22,6,9,11,2013); //set time for the first time
  Serial.begin(9600);
  
  Serial.println("DS1302 Real Time Clock");
  Serial.println("Version 3, November 2013");
  Serial.println();


}

void loop() {
  
  char buffe[80];
  sprintf(buffe,"Time = %02d:%02d:%02d, ",clock.getHour(),clock.getMinute(),clock.getSecond());
  Serial.print(buffe);
  Serial.print("Weekday = ");
  Serial.print(clock.getWeek());
  Serial.print(", ");
  Serial.print("Date = ");
  Serial.print(clock.getDate());
  Serial.print(" ");
  Serial.print(clock.getMonth());
  Serial.print(" ");
  Serial.println(clock.getYear());
  delay(1000);

  
}