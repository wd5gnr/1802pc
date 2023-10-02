#include <Arduino.h>
#include <RP2040_RTC.h>
#include "main.h"
#include <EEPROM.h>

void NVM_PutChecksum(uint16_t csum)
{
    EEPROM.put(112, csum);
    EEPROM.commit(); // we always change checksum, so...
}

uint16_t NVM_GetChecksum(void)
{
    uint16_t v;
    EEPROM.get(112, v);
    return v;
}

uint16_t NVM_Checksum()
{
    int i;
    unsigned csum = 0;
    for (i = 0; i < 114-2;i++) 
        {
            unsigned b = EEPROM.read(i);
            csum += b;
            if (csum>0xFFFF)
                csum++;
            csum <<= 1;
            csum &= 0xFFFF;
        }
        return csum;
}

void RTCSet(int year, int month, int day, int hour, int min, int sec)
{
    datetime_t tm;
    tm.year = year;
    tm.month = month;
    tm.day = day;
    tm.hour = hour;
    tm.min = min;
    tm.sec = sec;
    rtc_set_datetime(tm);
}


void RTCGetAll(int &year, int &month, int &day, int &hour, int &min, int &sec)
{
    datetime_t tm;
    rtc_get_datetime(&tm);
    year = tm.year;
    month = tm.month;
    day = tm.day;
    hour = tm.hour;
    min = tm.min;
    sec = tm.sec;
}

void RTCGet(int &hour, int &min, int &sec)
{
    int year, month, day;
    RTCGetAll(year, month, day, hour, min, sec);
}

void RTCStart(void)
{
    rtc_init();
    RTCSet(2023, 1, 1, 0, 0, 0);
    
}
