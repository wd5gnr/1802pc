/*
Note we use FF01 as a made up SCALL and FF02 as SRET
These should never be transfer control addresses in a real BIOS so...

We also use FF40 and FF41 as a special "read/write" API specific to the emulator

FF3F - Setup SCRT (jump with ret in R6)
FF2D - Set up baud rate
FF66 - Print string
FF03 - Send char with 0ch translation
FF4e - Send char
f809 - Send char


*/

#include <cstdio>
#include <ctime>
#include "main.h"

#define F_UTYPE 0xF809
#define F_UREAD 0xF80C
#define F_UTEST 0xF80F
#define F_GETTOD 0xF815
#define F_SETTOD 0xF818
#define F_RDNVR 0xF81b
#define F_WRNVR 0xF81e

#define F_IDESIZE 0xF821 // rd.0=0/1 m/s | RF=size in MB
#define F_IDEID 0xF824  // RF=buf, RD.0 as above | DF=0 ok

#define F_RTCTEST 0xF82d
#define F_NVRCCHK 0xF836



#define F_CALL 0xFFE0
#define F_CALL_ALT 0xFF01
#define F_RET 0xFFF1
#define F_RET_ALT 0xFF02

#define F_BOOT 0xFF00
#define F_TYPE 0xFF03
#define F_READ 0xFF06
// F_TYPEX Deprecated
#define F_MSG 0xFF09
#define F_INPUT 0xFF0F
#define F_STRCMP 0xFF12
#define F_LTRIM 0xFF15
#define F_STRCPY 0xFF18
#define F_MEMCPY 0xFF1b


#define F_SETBD 0xFF2D

#define F_IDERESET 0xFF36  // no arguments df=0 ok
#define F_IDEWRITE 0xFF39  // rf=buff, r7.0=start sec, r7.1 cyl low, r8.0=cyl high, r8.1 head/device | df=0, d=0 ok
#define F_IDEREAD 0xFF3C   // as above

#define F_INITCALL 0xFF3F

#define F_BOOTIDE 0xFF42  // no arguments, set up scrt/stack,reset IDE, read sector 0 to 100 and jump to 106 

#define F_TTY 0xFF4E
#define F_MINIMON 0xFF54
#define F_FREEMEM 0xFF57
#define F_INMSG 0xFF66
#define F_INPUTL 0xFF69
#define F_BRKTEST 0xFF6C
#define F_GETDEV 0xFF81

// Nonstandard
#define F_READSIMKEY 0xFF40
#define F_WRITESIMKEY 0xFF41


#include "main.h"
#include "1802.h"
#include "pceeprom.h"
#include <ctype.h>


#if BIOS == 1


void init_scrt(uint16_t stack)
{
  if (stack)
  {
    reg[2] = stack;
    x = 2;
  }
    reg[4] = F_CALL;
    reg[5] = F_RET;
    reg[3] = reg[6]; // assume this wasn't a proper call because we were not set up yet...
}



FILE *fide;
uint8_t sector[512];
unsigned long cpos = 0xfffffffful;
int diskinit = 0;
unsigned currentcy = 0xFFFF;
unsigned currsector = 0xFFFF;
char fname[32]; // IDE file name
uint8_t dis_diskled = 0;

uint8_t MAXCYL = 16;


int reset_ide()
{
  if (!diskinit)
    {}
  else
    if (currentcy!=0xFFFF) fclose(fide);
  if (EEPROM.read(MAXCYLEE) == EEPROMSIG)
  {
    MAXCYL = EEPROM.read(MAXCYLEE + 1);
  }
  else
    MAXCYL = 7; // Default is 7 x 128K = about 1M-128K
  diskinit = 1;
  cpos = 0xfffffffful;
  currentcy = currsector = 0xffff;
  *fname = '\0';
  return 0;
}


int ideseek(uint8_t h, uint16_t c, uint8_t s)
{
  long newpos;
  int noseek = 0;
  if (h != 0)
  {
    printf("Bad head\n");
    return -1;
    }

      if (c > MAXCYL)
        return -1;
      newpos = (s & 0x3F) * sizeof(sector);
      if (c != currentcy || ((s&0xC0)!=(currsector&0xC0)))
      {
        int subtrack = (s & 0xC0) >> 6;
        char subname[] = "ABCD";
        sprintf(fname, "%s/ide%02x%c.dsk",drivepfx, c,subname[subtrack]);
        if (currentcy != 0xFFFF)
        {
          fclose(fide);
        }
        fide = fopen(fname, "r+");
        if (!fide)
        {
          fide = fopen(fname, "w+");
          noseek = 1;
        }
        if (!fide)
        {
          return -1;
        }
        currentcy = c;
        currsector = s;
        if (fseek(fide,newpos, SEEK_SET))
         {
           return -1;
         }
       cpos = newpos + sizeof(sector); // for next time
  }
  else  // file already open
  {
    if (cpos!=newpos)
      if (fseek(fide, newpos, SEEK_SET))
      {
        return -1;
      }
    cpos = newpos + sizeof(sector);
    currsector = s;
  }
  return 0; 
}

int read_ide(uint16_t buff, uint8_t h, uint16_t c, uint8_t s)
{
  if (ideseek(h, c, s))
    return -1;
  if (fread(ram+buff,1,sizeof(sector),fide)!=sizeof(sector))
    return -1;
  return 0;
}

int write_ide(uint16_t buff, uint8_t h, uint16_t c, uint8_t s)
{
  int i;
  if (ideseek(h, c, s))
  {
    return -1;
  }
  if (fwrite(ram+buff,1,sizeof(sector),fide)!= sizeof(sector))
  {
    return -1;
  }
  fflush(fide);
  return 0;
}



int get_chs(uint8_t h, uint16_t &c,uint8_t &s)
{
  c = (reg[8] << 8) | (reg[7] >> 8);
  h = (reg[8] >> 8)&1;
  s = reg[7] & 0xFF;
  return 0;
}



int bios(uint16_t fn)
{
  switch (fn)
  {
  case F_CALL:     // old BIOS SCALL
//  case F_CALL_ALT: // made up SCALL
  {
    uint16_t cv;
    // Some code depends on RE.0 = D after a CALL or RETURN
    reg[0xe] = (reg[0xe] & 0xFF00) | (d & 0xFF);

    cv = memread(reg[3]) << 8;
    cv |= memread(reg[3] + 1);
    x = 2;
    memwrite(reg[2], reg[6]);
    memwrite(reg[2] - 1, reg[6] >> 8);
    reg[2] -= 2;
    reg[6] = reg[3] + 2;
    reg[3] = cv;
    p = 3;
    reg[4] = F_CALL; // reset for next call
  }

  break;

  case F_RET:     // old BIOS SRET
//  case F_RET_ALT: //  made up RET
    // Some code depends on RE.0 = D after a CALL or RETURN
    reg[0xe] = (reg[0xe] & 0xFF00) | (d & 0xFF);
    reg[2]++;
    reg[3] = reg[6];
    reg[6] = memread(reg[2]) << 8;
    reg[6] |= memread(reg[2] + 1);
    reg[2]++;
    p = 3;
    reg[5] = F_RET;
    x = 2;
    break;

  case F_INITCALL: // set up SCRT
    init_scrt(0); 
    p = 3;
    break;

  case F_SETBD:                 // Baud rate, not needed here
    reg[0xe] = reg[0xe] & 0xFF; // select UART
    reg[0xe] |= 0x100;          // turn on echo by default
    p = 5;
    break;

  case F_BRKTEST: // serial break
    df = brkflag ? 1 : 0;
    brkflag = 0;
    p = 5;
    break;

  case F_INMSG: // print a string
  {
    char c;
    do
    {
      c = memread(reg[6]++);
      if (c)
        serputc(c);
    } while (c);
    p = 5;
  }
  break;

  case F_TYPE:
  case F_TTY:
  case F_UTYPE:
  {
    char c = d;
    if (c == 0xC && (fn == 0xf809 || fn == 0xFF03))
    {
      serputc('\x1b');
      serputc('[');
      serputc('2');
      serputc('J');
    }
    else
    {
      serputc(c);
    }
    p = 5;
  }
  break;

  case F_UREAD: // uread
  case F_READ:
  {
    int c;
    do
    {

      c = getch();
      if (c > 0 && (reg[0xe] & 0x100))
      {
        char echo = c;
        serputc(echo);
      }
    } while (c == -1);
    d = c;
    p = 5;
  }
  break;

  case F_MSG:
  {
    char c;
    do
    {
      c = memread(reg[0xf]++);
      if (c)
        serputc(c);
    } while (c);
    fflush(fide);
    p = 5;
  }
  break;

  case F_GETDEV:
    reg[0xf] = 0x139; // capabilities: IDE, UART, RTC, NVR, Ext BIOS at F800
    p = 5;
    break;

  case F_UTEST: // key avail?
  df = kbhit() ? 1 : 0;
  p = 5;
  break;

  case F_INPUT:
  case F_INPUTL:   // real BIOS will update RF to point to terminator so we need to do that also
  {
    int c;
    int n = 0;
    int l = 254;
    uint16_t ptr = reg[0xF];
    char echo;

    if (fn == 0xFF69)
      l = reg[0xc] - 1;
    do
    {
      do
      {
        c = getch();
        echo = c;
      } while (c == -1 || c == 0);
      if (c == 0xD)
      {
        c = 0;
        df = 0;
      }
      if (c == 3)
      {
        c = 0;
        df = 1;
      }
      if (c == 8 || c == 0xFF || c==0x7F)
      {
        if (n)
        {
          ptr--;
          n--;
          if (reg[0xe] & 0x100)
          {
            serputc('\x8');
            serputc(' ');
            serputc('\x8');
          }
        }
        continue;
      }
      if (c && n == l)
        continue;
      memwrite(ptr++, c);
      n++;
      if (c > 0 && (reg[0xe] & 0x100))
        serputc(c);
    } while (c != 0);
    reg[0xf] = ptr - 1;
    p = 5;
  }

  break;

  case F_STRCMP:
  {
    char p1, p2;
    d = 0;
    do
    {
      p1 = memread(reg[0xf]);
      p2 = memread(reg[0xd]);
      if (p1 == p2 && p1 == 0)
        break; // strings equal
      if (p1 == 0 || p1 < p2)
      {
        d = 0xFF;
        break;
      } // I think I got these right
      if (p2 == 0 || p1 > p2)
      {
        d = 1;
        break;
      }
      reg[0xf] = reg[0xf] + 1;
      reg[0xd] = reg[0xd] + 1;
    } while (1);
    p = 5;
  }

  break;

  case F_LTRIM:
  {
    char c;
    do
    {
      c = memread(reg[0xf]++);
    } while (c && isspace(c));
    reg[0xf]--;
    p = 5;
  }
  break;

  case F_STRCPY:
  {
    char c;
    do
    {
      c = memread(reg[0xf]++);
      memwrite(reg[0xd]++, c);
    } while (c);
  }
    p = 5;

    break;

  case F_MEMCPY:
    while (reg[0xC]--)
      memwrite(reg[0xd]++, memread(reg[0xF]++));
    p = 5;
    break;

  case F_MINIMON: // enter monitor
    monitor();
    p = 5;
    break;

  case F_FREEMEM:
    reg[0xf] = 0x7EFF; // last address (ROM takes 7F00 as workspace)
    p = 5;
    break;

  case F_GETTOD: // get time of day
  {
   time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    memwrite(reg[0xf]++, tm.tm_mon+1);
    memwrite(reg[0xf]++, tm.tm_mday);
    memwrite(reg[0xf]++, tm.tm_year - 72);
    memwrite(reg[0xf]++, tm.tm_hour);
    memwrite(reg[0xf]++, tm.tm_min);
    memwrite(reg[0xf]++, tm.tm_sec);
  }
    df = 0;
    p = 5;
    break;

  case F_SETTOD: // set time
  {
    int y, m, d, h, n, s;
    m = memread(reg[0x0f]++);
    d = memread(reg[0xF]++);
    y = memread(reg[0xF]++) + 1972;
    h = memread(reg[0xF]++);
    n = memread(reg[0xF]++);
    s = memread(reg[0xF]++);
    // we don't let you set our RTC -- sorry
   // RTCSet(y, m, d, h, n, s);
  }
    df = 0;
    p = 5;
    break;

    // Note that our NVR is not tied to our RTC
    // So address 0 is address 0 and the time does NOT show up as part of the NVR
    // Nor can you set the time by putting things in NVR
  case F_RDNVR: // read NVR
    if (NVM_Checksum() != NVM_GetChecksum())
    {
      df = 1;
    }
    else
    {
      while (reg[0xC]--)
      {
        memwrite(reg[0xd]++, EEPROM.read(reg[0xF] & 0xFF));
        reg[0xf]++;
      }
      df = 0;
    }
    p = 5;

    break;

  case F_WRNVR: // write NVR
    while (reg[0xC]--)
    {
      EEPROM.write(reg[0xf] & 0xFF, memread(reg[0xd]++));
      reg[0xf]++;
    }
    NVM_PutChecksum(NVM_Checksum()); // this will commit the EEPROM, also
    df = 0;
    p = 5;
    break;

  case F_RTCTEST: // Test for RTC/NVR
  {
    static int eeprominit = 0;
    if (eeprominit == 0)
    {
      eeprominit = 1;
    }
  }
    df = 1;
    d = 114;
    p = 5;
    break;

  case F_NVRCCHK: // checksum NVR (ROM code accesses NVR directly)
  {
    reg[0xf] = NVM_Checksum();
    p = 5;
  }
  break;

  case F_IDESIZE:
  {
    uint16_t sz = 0;
    uint8_t drive = reg[0xd] & 1;
    printf("Warning IDE SIZE CALLED");
    if (drive == 0)
      sz = 8;
    reg[0xF] = sz;   // need to compute this
    df = 0;  // just in case
    p = 5;
    break;
  }
  case F_IDEID:  // optional so I think we will try failing it for now
    printf("WARNING IDE ID CALLED");
    df = 1;
    p = 5;
    break;

  case F_IDERESET:
    reset_ide();
    df = 0;
    p = 5;
    break;

  case F_IDEWRITE:  // data in RF, CHS in R8.0:R7.1 R8.1 R7.0
  // write sector
    uint16_t c;
    uint8_t h, s;
    get_chs(h, c, s);  // this is really device, 24-bit LBA I think
    if (write_ide(reg[0xf], h, c, s))
      d= df = 1;
       else 
      d=df = 0;
       reg[0xF] += 512;  // not doc but necessary
       p = 5;
       break;


  case F_IDEREAD:
  // read sector
  {
    uint16_t c;
    uint8_t h, s;
    get_chs(h, c, s);
    if (read_ide(reg[0xf], h, c, s))
      df = d = 1;
    else
      d=df = 0;
    }
    reg[0xF] += 512; // not doc but necessary
    p = 5;
  break;

  case F_BOOT:
  case F_BOOTIDE:
    init_scrt(0xf0);
    reset_ide();
    read_ide(0x100, 0, 0, 0);
    reg[3] = 0x106;
    p = 3;
    break;

  case F_READSIMKEY: // read sim key
    uint8_t key;
    key = memread(reg[6]++);
    d = io_read_key(key);
    p = 5;
    break;

  case F_WRITESIMKEY: // write sim key
    uint8_t wkey;
    uint16_t wval;
    wkey = memread(reg[6]++);
    if (wkey < 0x80)
      io_write_key(wkey, d);
    else
    {
      wval = memread(reg[6]++) << 8;
      wval |= memread(reg[6]++);
      io_write_key(wkey, wval);
    }
    p = 5;
    break;

  default:
    return 0;
  }

  return 1;
}

#endif
