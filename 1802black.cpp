// This is mostly Oscar's original KIM UNO code
// Modified by Al Williams for use as an 1802
// For the most part I just hacked away the parts specific to the KIM
// I did add code to the display part to take care of the decimal points
// in the hex out code, and added a few defines and a header.
// But for the most part, all of this code is straight out of the KIM Uno.

// Original version:
// version date: 20160907
#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>  // need atoi

#include "main.h"
#include "1802.h"
#include "pceeprom.h"


#define VERSION "1802PCv1"

#define SERIAL_ESCAPE '|' // turn terminal input into real terminal input

uint8_t curkey = 0;

int autostart = 1;
unsigned speed=0;
const char *eedata = "./eedata.dat";
const char *drivepfx = "./disk/";

void serputc(int c)
{
  putchar(c); 
}

// get and clear a key (from original code)
uint8_t getAkey(void)
{
  return (curkey);
}
void clearkey(void)
{
  curkey = 0;
}



// tick counter for display updates
uint8_t tick = 0;

int romsel = 0;


// Set up everything

void setup()
{
  fprintf(stderr,"Wait\r\n");
  set_conio_terminal_mode();
  // read switches
  initRom(romsel);

  setupUno();
 // RTCStart();  // start real time clock
  reset();     // 1802 reset
  fprintf(stderr,"%s\r\n",VERSION);

  if (autostart)
  {
      fprintf(stderr,"%s\r\n", "Autostart");
      exec1802(KEY_GO);
      noserial = 1;
  }
}

// read serial with or without echo
int Serialread(int echo)
{
  int curkey;
  curkey = getch();
  if (curkey > 0 && echo)
    putchar(curkey);
  return curkey;
}

int serial_avail()
{
  return kbhit();
}
#include <unistd.h>

// main loop
void loop()
{
  unsigned dloop;
  if (noserial == 0 && serial_avail()) // if serial input, process that
  {
    curkey = getchar();
    if (curkey == SERIAL_ESCAPE)
       noserial = 1; // one way ticket
    else
        exec1802(curkey);
    curkey = 0;
  }
  else 
    exec1802(0);
  tick++;
  for (dloop = 0; dloop < speed; dloop++)
  {
    kbhit();  // kill a little time doing something harmless
  }
}

// =================================================================================================
// KIM Uno Board functions are bolted on from here
// =================================================================================================

void setupUno()
{

}

#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>

FILE *fsfile = NULL;
unsigned long fslen = 0;


_EEPROM EEPROM;


void fscloser()
{
  if (fsfile)
    fclose(fsfile);
}

int main(int argc, char *argv[])
{
  int c;
  while ((c = getopt(argc,argv,"ar:ps:d:e:D:?"))!=-1)
  {
    switch(c)
    {
      case 'a':
        autostart = 0;
        break;
      case 's':
        speed = atoi(optarg);
        break;
      case 'r':
        romsel = atoi(optarg);
        break;
      case 'd':
        drivepfx = strdup(optarg);
        break;
      case 'e':
        eedata = strdup(optarg);
        break;

      case 'D':
        fsfile = fopen(optarg,"r+");
        if (!fsfile)
        {
          fprintf(stderr, "Can't open file system file %s\n", optarg);
          perror("");
          exit(2);
        }
        atexit(fscloser);
        fseek(fsfile, 0l, SEEK_END);
        fslen = ftell(fsfile);
        fseek(fsfile, 0l, SEEK_SET);
        break;

      case 'p':
      {
      int mainfd, termfd;
      char *termdev;


      mainfd = posix_openpt(O_RDWR|O_NOCTTY);


    if ( mainfd == -1
        || grantpt (mainfd) == -1
        || unlockpt (mainfd) == -1
        || (termdev = ptsname (mainfd)) == NULL)
        {
          perror("PTY Error");
          exit(10);
        }

        printf("pty device is: %s\n", termdev);

     
      // redirect stdin and out
        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        dup2(mainfd, STDIN_FILENO);
        dup2(mainfd, STDOUT_FILENO);
      }
          case '?':
            printf("Usage: 1802pc [-a] [-p] [-e eefile] [-d disk_prefix] [-D file] [-r rom#] [-s delay]\r\n"
                   "    -a - Do not auto start\r\n"
                   "    -p - Use pty for console\r\n"
                   "    -e - Use eefile as EEPROM data\r\n"
                   "    -d - Use directory for disk drive files (use -D or -d; default -d disk)\r\n"
                   "    -D - Use file for disk drive (use -D or -d)\r\n"
                   "    -r - Use ROM # specified (default 0)\r\n"
                   "    -s - Delay between instructions (unitless; default=0)\r\n");
            exit(2);
            break;
      }
  }
  EEPROM.init(eedata);
  setup();
  while (1)
    loop();
}
void setaddress(unsigned short) {}
void setdata(unsigned char) {}
void setdp(int, int) {}



void NVM_PutChecksum(uint16_t csum)
{
    EEPROM.write(112, csum &0xFF);
    EEPROM.write(113, csum >> 8);
    EEPROM.commit(); // we always change checksum, so...
}

uint16_t NVM_GetChecksum(void)
{
    uint16_t v;
    v=EEPROM.read(112);
    v |= EEPROM.read(113) << 8;
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

