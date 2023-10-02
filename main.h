#ifndef __MAIN_H
#define __MAIN_H

#define F(s) s
#include <cstdint>
#include "getch.h"


int freeRam(void);
void interpretkeys(void);
void setupUno(void);
uint8_t xkeyPressed(void);
void scanKeys(void);
void driveLEDs(void);
void setaddress(uint16_t a);
void setdata(uint8_t d);
void setdp(int pos, int state);
int Serialread(int echo = 1);  // read and echo
void serputc(int c);  // put a character out the serial port

void RTCStart(void);
void RTCSet(int year, int month, int day, int hour, int min, int sec);
void RTCGet(int &hour, int &min, int &sec);
void RTCGetAll(int &year, int &month, int &day, int &hour, int &min, int &sec);
uint16_t NVM_Checksum(void);
uint16_t NVM_GetChecksum();
void NVM_PutChecksum(uint16_t);

#define KEY_RS 'R'
#define KEY_AD '='
#define KEY_DA 'L'
#define KEY_GO 'G'
#define KEY_PC 'P'
#define KEY_ST 'S'
#define KEY_SST '/'

extern char threeHex[3][2];

extern volatile int brkflag;


void initRom(int alt);

extern int getch(void);

#define EEPROMSIG 0xD1
#define MAXCYLEE  0xFE
extern int diskinit;

// This is a strange function that runs in RAM
// halts the other processor
// and reads the BOOT button
bool get_bootsel_button(void);

// Warning: these are subject to change capriciously
#define LEDPIN 25
#define AUTOSTARTPIN 5
#define ALTROMPIN 1
#define MONITORPIN 14  // use -1 to read bootsel button (slow)
#define DISKACT 25   // disk activity (25 to use Q LED)

void disasm1802(unsigned start, unsigned stop);
unsigned disasmline(unsigned mp, int nl=1);

extern const char *eedata;
extern const char *drivepfx;

#endif


