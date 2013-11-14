#ifndef _TIME_DS1302_CPP
#define _TIME_DS1302_CPP

#include "TimeDS1302.h"

/*Application Level*/
//-------------------------------------------------------------------------------------

//Initialize 
TimeDS1302::TimeDS1302(uint8_t sclk, uint8_t io, uint8_t ce)
{
	_sclk = sclk;    // Arduino pin for the Serial Clock
	_io   = io;   // Arduino pin for the Data I/O
	_ce   = ce;   // Arduino pin for the Chip Enable
	
	ds1302_struct rtc;
}

void TimeDS1302::begin()
{
  digitalWrite( _ce, LOW); // default, not enabled
  pinMode( _ce, OUTPUT);  

  digitalWrite( _sclk, LOW); // default, clock low
  pinMode( _sclk, OUTPUT);

  pinMode( _io, OUTPUT);

  digitalWrite( _ce, HIGH); // start the session
  delayMicroseconds( 4);           // tCC = 4us
}


//Set the time for the first use
void TimeDS1302::set_time(int sec,int min, int hour, int weekday, int date, int months, int years)
{      
   ds1302_struct rtc;
   
// Remove the next define, 
// after the right date and time are set.
#define SET_DATE_TIME_JUST_ONCE
#ifdef SET_DATE_TIME_JUST_ONCE
	
  // Start by clearing the Write Protect bit
  // Otherwise the clock data cannot be written
  // The whole register is written, 
  // but the WP-bit is the only bit in that register.
  TimeDS1302::DS1302_write (DS1302_ENABLE, 0);

  // Disable Trickle Charger.
  TimeDS1302::DS1302_write (DS1302_TRICKLE, 0x00);


  // Fill these variables with the date and time.
  int seconds, minutes, hours, dayofweek, dayofmonth, month, year;

  // Example for april 15, 2013, 10:08, monday is 2nd day of Week.
  // Set your own time and date in these variables.
  seconds    = sec;
  minutes    = min;
  hours      = hour;
  dayofweek  = weekday;  // Day of week, any day can be first, counts 1...7
  dayofmonth = date; // Day of month, 1...31
  month      = months;  // month 1...12
  year       = years;

  // Set a time and date
  // This also clears the CH (Clock Halt) bit, 
  // to start the clock.

  // Fill the structure with zeros to make 
  // any unused bits zero
  memset ((char *) &rtc, 0, sizeof(rtc));

  rtc.Seconds    = bin2bcd_l( seconds);
  rtc.Seconds10  = bin2bcd_h( seconds);
  rtc.CH         = 0;      // 1 for Clock Halt, 0 to run;
  rtc.Minutes    = bin2bcd_l( minutes);
  rtc.Minutes10  = bin2bcd_h( minutes);
  // To use the 12 hour format,
  // use it like these four lines:
  //    rtc.h12.Hour   = bin2bcd_l( hours);
  //    rtc.h12.Hour10 = bin2bcd_h( hours);
  //    rtc.h12.AM_PM  = 0;     // AM = 0
  //    rtc.h12.hour_12_24 = 1; // 1 for 24 hour format
  rtc.h24.Hour   = bin2bcd_l( hours);
  rtc.h24.Hour10 = bin2bcd_h( hours);
  rtc.h24.hour_12_24 = 0; // 0 for 24 hour format
  rtc.Date       = bin2bcd_l( dayofmonth);
  rtc.Date10     = bin2bcd_h( dayofmonth);
  rtc.Month      = bin2bcd_l( month);
  rtc.Month10    = bin2bcd_h( month);
  rtc.Day        = dayofweek;
  rtc.Year       = bin2bcd_l( year - 2000);
  rtc.Year10     = bin2bcd_h( year - 2000);
  rtc.WP = 0;  

  // Write all clock data at once (burst mode).
  TimeDS1302::DS1302_clock_burst_write( (uint8_t *) &rtc);
#endif
}

// Return second
int TimeDS1302::getSecond()
{
	ds1302_struct rtc;
	TimeDS1302::DS1302_clock_burst_read( (uint8_t *) &rtc);
	return bcd2bin(rtc.Seconds10, rtc.Seconds);
}

// Return Minute
int TimeDS1302::getMinute()
{
	ds1302_struct rtc;
	TimeDS1302::DS1302_clock_burst_read( (uint8_t *) &rtc);
	return bcd2bin(rtc.Minutes10, rtc.Minutes);
}

// Return Hour
int TimeDS1302::getHour()
{
	ds1302_struct rtc;
	TimeDS1302::DS1302_clock_burst_read( (uint8_t *) &rtc);
	return bcd2bin(rtc.h24.Hour10, rtc.h24.Hour);
}

// Return day of the month
int TimeDS1302::getDate()
{
	ds1302_struct rtc;
	TimeDS1302::DS1302_clock_burst_read( (uint8_t *) &rtc);
	return bcd2bin( rtc.Date10, rtc.Date);
}

// Return Weekday of the month
const char* TimeDS1302::getWeek()
{	
	ds1302_struct rtc;
	TimeDS1302::DS1302_clock_burst_read( (uint8_t *) &rtc);
	
	int weekday_t = rtc.Day;
	const char* weekDay;
	switch(weekday_t)
	{
		case 1:
			weekDay = "MON";
			return weekDay;
			break;
		case 2:
			weekDay = "TUE";
			return weekDay;
			break;
		case 3:
			weekDay = "WED";
			return weekDay;
			break;
		case 4:
			weekDay = "THU";
			return weekDay;
			break;
		case 5:
			weekDay = "FRI";
			return weekDay;
			break;
		case 6:
			weekDay = "SAT";
			return weekDay;
			break;
		case 7:
			weekDay = "SUN";
			return weekDay;
			break;
		default:
			break;
	}
}

//Return Month of the year
const char* TimeDS1302::getMonth()
{
	ds1302_struct rtc;
	TimeDS1302::DS1302_clock_burst_read( (uint8_t *) &rtc);
	
	int month_t = bcd2bin( rtc.Month10, rtc.Month);
	const char* Month;
	switch(month_t)
	{
		case 1:
			Month = "JAN";
			return Month;
			break;
		case 2:
			Month = "FEB";
			return Month;
			break;
		case 3:
			Month = "MAR";
			return Month;
			break;
		case 4:
			Month = "APR";
			return Month;
			break;
		case 5:
			Month = "MAY";
			return Month;
			break;
		case 6:
			Month = "JUN";
			return Month;
			break;
		case 7:
			Month = "JUL";
			return Month;
			break;
		case 8:
			Month = "AUG";
			return Month;
			break;
		case 9:
			Month = "SEP";
			return Month;
			break;
		case 10:
			Month = "OCT";
			return Month;
			break;
		case 11:
			Month = "NOV";
			return Month;
			break;
		case 12:
			Month = "DEC";
			return Month;
			break;
		default:
			break;
	}
}

// Return Year
int TimeDS1302::getYear()
{	
	ds1302_struct rtc;
	TimeDS1302::DS1302_clock_burst_read( (uint8_t *) &rtc);
	
	return 2000 + bcd2bin( rtc.Year10, rtc.Year);
}
//------------------------------------------------------------------------------------
/*Low Level Application*/

// --------------------------------------------------------
// DS1302_clock_burst_read
//
// This function reads 8 bytes clock data in burst mode
// from the DS1302.
//
// This function may be called as the first function, 
// also the pinMode is set.
//
void TimeDS1302::DS1302_clock_burst_read( uint8_t *p)
{
  int i;
  
  TimeDS1302::begin();

  // Instead of the address, 
  // the CLOCK_BURST_READ command is issued
  // the I/O-line is released for the data
  TimeDS1302::_DS1302_togglewrite( DS1302_CLOCK_BURST_READ, true);  

  for( i=0; i<8; i++)
  {
    *p++ = TimeDS1302::_DS1302_toggleread();
  }
  TimeDS1302::_DS1302_stop();
}


// --------------------------------------------------------
// DS1302_clock_burst_write
//
// This function writes 8 bytes clock data in burst mode
// to the DS1302.
//
// This function may be called as the first function, 
// also the pinMode is set.
//
void TimeDS1302::DS1302_clock_burst_write( uint8_t *p)
{
  int i;
  
  TimeDS1302::begin();

  // Instead of the address, 
  // the CLOCK_BURST_WRITE command is issued.
  // the I/O-line is not released
  TimeDS1302::_DS1302_togglewrite( DS1302_CLOCK_BURST_WRITE, false);  

  for( i=0; i<8; i++)
  {
    // the I/O-line is not released
    TimeDS1302::_DS1302_togglewrite( *p++, false);  
  }
  TimeDS1302::_DS1302_stop();
}


// --------------------------------------------------------
// DS1302_read
//
// This function reads a byte from the DS1302 
// (clock or ram).
//
// The address could be like "0x80" or "0x81", 
// the lowest bit is set anyway.
//
// This function may be called as the first function, 
// also the pinMode is set.
//
uint8_t TimeDS1302::DS1302_read(int address)
{
  uint8_t data;

  // set lowest bit (read bit) in address
  bitSet( address, DS1302_READBIT);  
  
  TimeDS1302::begin();
  
  // the I/O-line is released for the data
  TimeDS1302::_DS1302_togglewrite( address, true);  
  data = TimeDS1302::_DS1302_toggleread();
  TimeDS1302::_DS1302_stop();

  return (data);
}


// --------------------------------------------------------
// DS1302_write
//
// This function writes a byte to the DS1302 (clock or ram).
//
// The address could be like "0x80" or "0x81", 
// the lowest bit is cleared anyway.
//
// This function may be called as the first function, 
// also the pinMode is set.
//
void TimeDS1302::DS1302_write( int address, uint8_t data)
{
  // clear lowest bit (read bit) in address
  bitClear( address, DS1302_READBIT);   
	TimeDS1302::begin();
  // don't release the I/O-line
  TimeDS1302::_DS1302_togglewrite( address, false); 
  // don't release the I/O-line
  TimeDS1302::_DS1302_togglewrite( data, false); 
  TimeDS1302::_DS1302_stop();  
}

// --------------------------------------------------------
// _DS1302_stop
//
// A helper function to finish the communication.
//
void TimeDS1302::_DS1302_stop(void)
{
  // Set CE low
  digitalWrite( _ce, LOW);

  delayMicroseconds( 4);           // tCWH = 4us
}


// --------------------------------------------------------
// _DS1302_toggleread
//
// A helper function for reading a byte with bit toggle
//
// This function assumes that the SCLK is still high.
//
uint8_t TimeDS1302::_DS1302_toggleread( void)
{
  uint8_t i, data;

  data = 0;
  for( i = 0; i <= 7; i++)
  {
    // Issue a clock pulse for the next databit.
    // If the 'togglewrite' function was used before 
    // this function, the SCLK is already high.
    digitalWrite( _sclk, HIGH);
    delayMicroseconds( 1);

    // Clock down, data is ready after some time.
    digitalWrite( _sclk, LOW);
    delayMicroseconds( 1);        // tCL=1000ns, tCDD=800ns

    // read bit, and set it in place in 'data' variable
    bitWrite( data, i, digitalRead( _io)); 
  }
  return( data);
}


// --------------------------------------------------------
// _DS1302_togglewrite
//
// A helper function for writing a byte with bit toggle
//
// The 'release' parameter is for a read after this write.
// It will release the I/O-line and will keep the SCLK high.
//
void TimeDS1302::_DS1302_togglewrite( uint8_t data, uint8_t release)
{
  int i;

  for( i = 0; i <= 7; i++)
  { 
    // set a bit of the data on the I/O-line
    digitalWrite( _io, bitRead(data, i));  
    delayMicroseconds( 1);     // tDC = 200ns

    // clock up, data is read by DS1302
    digitalWrite( _sclk, HIGH);     
    delayMicroseconds( 1);     // tCH = 1000ns, tCDH = 800ns

    if( release && i == 7)
    {
      // If this write is followed by a read, 
      // the I/O-line should be released after 
      // the last bit, before the clock line is made low.
      // This is according the datasheet.
      // I have seen other programs that don't release 
      // the I/O-line at this moment,
      // and that could cause a shortcut spike 
      // on the I/O-line.
      pinMode( _io, INPUT);

      // For Arduino 1.0.3, removing the pull-up is no longer needed.
      // Setting the pin as 'INPUT' will already remove the pull-up.
      // digitalWrite (DS1302_IO, LOW); // remove any pull-up  
    }
    else
    {
      digitalWrite( _sclk, LOW);
      delayMicroseconds( 1);       // tCL=1000ns, tCDD=800ns
    }
  }
}

#endif /*_TIME_DS1308_CPP*/