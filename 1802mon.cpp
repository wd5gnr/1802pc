
#include "1802.h"
#include "main.h"
#if MONITOR == 1
#include <cstdio>
#include <cctype>
#include <cstring>
#include "pceeprom.h"

/*
Commands:

R - Display all registers
R2 - Display register 2
R2=AA - Set register 2 to AA

Note: X=10, P=11, D=12, DF=13, Q=14, and T=15 -- display all registers to see that list

M 400 - Display 100 hex bytes at 400
M 400 10 - Display 10 hex bytes at 400
M 400=20 30 40; - Set data starting at 400 (end with semicolon)

Note that you can use the first line with full backspace:
M 400=20 30 40;

But if you start a new line (you get a colon prompt), you will not be able to backspace past the current byte:

M 400=
: 20 30 40
: 50 60 70;

Backing up while entering 30 can only delete the 30 and not the 20. Also, instead of backing up you can just keep going
as in:

:M 400=
: 200 300 400;

All 3 bytes will then be zero.

G 400 - Goto 400
G 400 3 - Goto 400 with P=3

B - List all enabled breakpoints
B0 - - Disable Breakpoint 0 (that is a dash as in B0 -)
BF @200 - Set breakpoint F to address 200
BF P3 - Set breakpoint when P=3
BF I7A - Set breakpoint when instruction 7A executes

Note would be possible to do data write (or even read) breakpoints
Would also be possible to do ranges of addresses, if desired

N - Execute next instruction


I 2 - Show input from N=2
O 2 10 - Write 10 to output N=2

Note: The keypad and display are dead while the monitor is in control

X - Exit to front panel mode (not running)

C - Exit and continue running (if already running)
Q - Same as C

.+ - Send next character(s) to the front panel interpreter (no spaces)

*/

#include <cstdlib>  //  need exit

static char cmdbuf[33];
static int cb;

static int cmd;       // command
static uint16_t arg;  // one argument
static int terminate; // termination character
static int noread;

// generic sector buffer
uint8_t sbuf[512];

int monactive = 0;
#if 0
int getch(void)
{
  int c;
  int ctr = 0;
  do
  {
    c = Serialread();
    if (c == -1)
    {
 #if 0
      if (++ctr % DISPLAY_DIVISOR == 0)
      {
        updateLEDdata();
        driveLEDs();
        scanKeys();
        ctr = 0;
      }
#endif     
    }

  } while (c == -1);
  return c;
}
#endif

// This skips leading blanks, makes all internal whitespace one space, and trims trailing blanks
// This is very important since some of the parsing assumes there will be 0 or 1 space but no more
// and no tabs etc.

uint8_t readline(int *terminate)
{
  int c;
  cb = 0;
  cmdbuf[0] = '\0';
  while (1)
  {
    c = getch();
    putchar(c);
    if (c == '\r')
    {
      if (terminate)
        *terminate = '\r';
      while (cb && cmdbuf[cb - 1] == ' ')
        cb--;
      cmdbuf[cb] = '\0';
      if (cb)
        cb = 1;
      return *cmdbuf;
    }
    if (c == 0x1b)
    {
      cmdbuf[0] = '\0';
      cb = 0;
      if (terminate)
        *terminate = 0x1b;
      return '\0';
    }
    if (c==0x7f || c==0xFF)
      c = 8;
    if (c == 8 && cb != 0)
    {
      cb--;
      continue;
    }
    if (cb > sizeof(cmdbuf) - 2)
    {
      putchar('\x7');
      continue;
    }

    if (isspace(c) && cb == 0)
      continue;
    if (cb && isspace(c) && cmdbuf[cb - 1] == ' ')
      continue;
    else if (isspace(c))
      c = ' ';
    cmdbuf[cb++] = toupper(c);
  }
}

uint16_t readhexX(int (*getcfp)(void), int *term, uint16_t def = 0xFFFF)
{
  uint16_t val = def;
  int first = 1;
  int c;

  while (1)
  {
    c = getcfp();
    if (!isxdigit(c) && c != 8)
    {
      if (first && c != '\r' && c != ';' && c != 0x1b)
        continue;
      if (term)
        *term = c;
      noread = first;
      return val;
    }
    if (c == 8)
    {
      val >>= 4; // in case of serial input
      continue;
    }
    c = toupper(c);
    if (first)
      val = 0;
    else
      val <<= 4;
    first = 0;
    if (c >= 'A')
      c = c - 'A' + 10;
    else
      c -= '0';
    val += c;
  }
}

uint16_t readhex(int *term, uint16_t def = 0xFFFF)
{
  return readhexX(getch, term, def);
}

int getbufc(void)
{
  if (cmdbuf[cb] == '\0')
    return '\r';
  return cmdbuf[cb++];
}

uint16_t readhexbuf(int *term, uint16_t def = 0xFFFF)
{
  return readhexX(getbufc, term, def);
}

int diskcfm(void)
{
  printf("\r\nY to proceed, N to abort? \r\n");
  int n = getch();
  return (n == 'y' || n == 'Y');
}

// Super crummy disk monitor to troubleshoot/format/set disk stuff
void diskmon(void)
{
  char diskcmd[128];

 
  diskinit = 1;
  while (1)
  {
    printf(F("Host disk menu (arguments in hex)\r\n"));
    printf(F("S - set max 'track' count for -d: S [max#]\r\n"
                     "F - Format (not required on 1802PC)\r\n"
                     "> - Write disk out in exchange format\r\n"
                     "< - Read disk in from exchange format\r\n"
                     "X - eXit\r\n"));
    int n = readline(NULL);
    n = toupper(n);
    switch (n)
    {
    case 'X':
      diskinit = false;
      diskinit = 0;
      return;
    case 'F':
      if (diskcfm())
        printf("Format not required\r\n");
      break;

      case '>':
      {
 
        unsigned i;
        int rv;
        uint16_t c = 0;
        uint8_t s = 0; 
        if (reset_ide())
        {
          printf("Can't reset disk\r\n");
          break;
        }
        printf("!DISK1:%04X:",MAXCYL);
        do {
          rv = read_mide(sbuf, 0, c, s);
          if (!rv)
          {
            for (i = 0; i < sizeof(sbuf); i++)
            {
              printf("%02X", sbuf[i]);
              if (((i+1)%16)==0)
                printf("\r\n");
            }
            s++;
            if (!s)
              c++;
          }
        } while (rv == 0);
        printf("EOF\r\n");
      }
        break;

      case '<':
      {
        uint16_t c = 0;
        uint8_t s = 0;
        unsigned i;
        int rv;
        char inbuf[3];
        FILE *f;
        // check for empty disk
        // temp use sbuff for file name
        if (fslen==0)  // using -d option or default
        {
          sprintf((char *)sbuf, "%s/ide00A.dsk", drivepfx);
          f = fopen((char *)sbuf, "r");
          if (f)
            {
            fclose(f);
            printf("Disk not empty (use -d or delete old files)\r\n");
            break;
            }
        }
        if (!diskcfm())
          break;
        // read header or error
        while (getch()!='!')
          ;
        if (getch()!='D' || getch()!='I' || getch()!='S'
        || getch()!='K')
        {
          printf("Invalid file format\r\n");
          break;
        }
        if (getch()!='1' || getch()!=':')
        {
          printf("Bad file version\r\n");
          break;
        }
        // set MAXCYL
        sbuf[0] = getch();
        sbuf[1] = getch();
        sbuf[2] = getch();
        sbuf[3] = getch();
        sbuf[4] = '\0';
        if (sscanf((char *)sbuf, "%04X", &rv)!=1)
        {
          printf("Misformed file\r\n");
          break;
        }
        if (fslen==0)
          {
          MAXCYL = rv;
          EEPROM.write(MAXCYLEE, EEPROMSIG);
          EEPROM.write(MAXCYLEE + 1, rv);
          }
        getch();  // better be a colon but not checked
        reset_ide();
        i = 0;
        while (1)
        {
        if (i==sizeof(sbuf))
        {
          i = 0;
          rv = write_mide(sbuf, 0, c, s);
          s++;
          if (s==0)
            c++;
        }
        // read bytes
        inbuf[0] = getch();
        // on CRLF issue Prompt ]
        if (inbuf[0]=='\r' || inbuf[0]=='\n')
          {
            putchar(']');
            continue;
          }
        inbuf[1] = getch();
        if (inbuf[0]=='E' && inbuf[1]=='O')
        {
          // end of file
          // we assume the sender did not send
          // an odd number of bytes so...
          printf("Complete\r\n");
          break;
        }
        inbuf[2] = '\0';
        sscanf(inbuf, "%X", &rv);
        sbuf[i++] = rv;
        }
        reset_ide();
      }
      break;

    case 'D':
    {
      printf("Not available on 1802PC\r\n");
    }
    break;
    case 'S':
    {
      int n;
      EEPROM.write(MAXCYLEE, EEPROMSIG);
      EEPROM.write(MAXCYLEE + 1, n = readhexbuf(NULL, 0x10));
      MAXCYL = n;
      printf("Set max cylinder to %d (0x%x)\r\n", n, n);
      break;
    }

    default:
      printf("?\r\n");
    }
  }
}

BP bp[16];

void dispbp(int bpn)
{
  printf(F("\r\nBP %X: "),bpn);
  if (bp[bpn].type == 1)
    putchar('@');
  if (bp[bpn].type == 2)
    putchar('P');
  if (bp[bpn].type == 3)
    putchar('I');
  if (bp[bpn].type == 0)
    printf("Disabled");
  else
    print4hex(bp[bpn].target);
}

int nobreak;

void mon_status(void)
{
//  print4hex(reg[p]);
//  Serial.print(F(": "));
  disasmline(reg[p],0);      
//  print2hex(memread(reg[p]));
  printf(F("\tD=%02X\r\n"),d);

}

bool enterMonitor(void)
{
  static int throttle = 0; // the BOOT button is slow so we throttle and only check every 256 instructions
  if (monactive)
    return true;
#if MONITORPIN >= 0
 // return digitalRead(MONITORPIN) == 0;
  return 0;
#else
  if (++throttle < 256)
    return false;
  throttle = 0;
  return get_bootsel_button();
#endif
}

int mon_checkbp(void)
{
  int i;
  int mon = enterMonitor();
  if (nobreak && !mon)
    return 1;
  if (!nobreak)
    for (i = 0; i < sizeof(bp) / sizeof(bp[0]); i++)
    {
      if (bp[i].type == 0)
        continue;
      else if (bp[i].type == 1 && bp[i].target == reg[p])
        mon = 1;
      else if (bp[i].type == 2 && bp[i].target == p)
        mon = 1;
      else if (bp[i].type == 3 && bp[i].target == memread(reg[p]))
        mon = 1;
    }
  if (mon)
  {
    printf("\r\nBreak\r\n");
    mon_status();
    return monitor();
  }
  return 1;
}

// dump printable characters
static void adump(unsigned a)
{
  int z;
  printf(F("  "));
  for (z = 0; z < 16; z++)
  {
    char b = memread(a + z);
    if (b >= ' ')
      putchar(b);
    else
      putchar('.');
  }
}

int monitor(void)
{
  int noarg;
  monactive = 1;
  while (1)
  {
    printf(F("\r\n>"));
    cmd = readline(&terminate);
    if (terminate == 0x1b)
      continue;
    if (!strchr("$DRMGBIOXQCN&?.`", cmd))
    {
      putchar('?');
      continue;
    }
    noarg = 0;
    if (cmdbuf[cb])
      arg = readhexbuf(&terminate);
    else
      noarg = 1;
    switch (cmd)
    {
    case '.':
      for (char *cp = cmdbuf + 1; *cp; cp++)
        exec1802(*cp);
      break;

    case '$':
      if (diskcfm()) exit(0);
      break;

    case '&':
    {
      int y, m, d, h, n, s;
      printf("This command not available on 1802PC\r\n");
    }
    break;
    case '?':
      printf(F("<R>egister, <M>emory, <G>o, <B>reakpoint, <N>ext, <I>nput, <O>utput, e<X>it\r\n"));
      printf(F("<Q>uit, <C>ontinue, .cccc (send characters to front panel; no space after .\r\n)"));
      printf(F("<D>isassemble <$>exit to OS <&> time<`> Disk menu\r\n "));
      printf(F("Examples: R (show all)  RB (show RB)  RB=2F00 (se RB)\r\n"));
      printf(F("M 100 10 (show 16 bytes at 100)   M 100=<CR>AA 55 22; (set memory at 100)\r\n"));
      printf(F("B 0 @101 (Set breakpoint 0 at 101)  .44$$ (Send front panel commands)\r\n"));
      break;

    case '`':
      diskmon();
      break;

    case 'D':
    {
      unsigned arg2 = 0;
      unsigned limit;
      if (noarg)
      {
        printf(F("Usage: D address [length]]\r\n"));
        break;
      }
      printf("\r\n");
      if (terminate != '\r')
        arg2 = readhexbuf(&terminate, 0);
      if (arg2 == 0)
        arg2 = 0x100;
      limit = (arg + arg2) - 1;
      if (limit < arg)
        limit = 0xFFFF; // wrapped around!
      disasm1802(arg, limit);
    }
    break;

    case 'N':
      nobreak = 1;
      mon_status();
      run();
      nobreak = 0;
      break;

    case 'B':
      if (noarg || arg >= 0x10)
      {
        int i;
        for (i = 0; i < sizeof(bp) / sizeof(bp[0]); i++)
          dispbp(i);
        break;
      }
      if (terminate != '\r')
      {
        int cc;
        cc = terminate;
        if (cc == ' ')
          cc = getbufc();
        if (cc == '-')
        {
          bp[arg].type = 0;
          break;
        }
        if (cc == '@')
        {
          cc = readhexbuf(&terminate);
          bp[arg].target = cc;
          bp[arg].type = 1;
          break;
        }
        if (cc == 'p' || cc == 'P')
        {
          cc = readhexbuf(&terminate);
          bp[arg].target = cc & 0xF;
          bp[arg].type = 2;
          break;
        }
        if (cc == 'i' || cc == 'I')
        {
          cc = readhexbuf(&terminate);
          bp[arg].target = cc & 0xFF;
          bp[arg].type = 3;
          break;
        }
      }
      else
      {
        dispbp(arg);
      }
      break;

    case 'R':
      if (noarg)
      {
        int i;
        for (i = 0; i <= 15; i += 4)
        {
          printf("R%X:", i);
          print4hex(reg[i]);
          printf(F("\tR%X:"),i+1);
          print4hex(reg[i + 1]);
          printf("\tR%X:", i + 2);
          print4hex(reg[i + 2]);
          printf(F("\tR%X:"),i+3);
          print4hex(reg[i + 3]);
          printf("\r\n");
        }
        printf(F("(10) X: %X\t(11) P:%X\r\n"),x,p);
        printf(F("(12) D: %X\t(13) DF:%X\r\n"),d,df);
        printf("(14) Q:%X\t(15) T:%X\r\n", q, t);
      }
      else
      {
        if (terminate != '=')
          printf("R");
         if (arg <= 0xF)
        {
          if (terminate == '=')
          {
            uint16_t v = readhexbuf(&terminate);
            reg[arg] = v;
          }
          else
          {
            printf("%X:%04X", arg, reg[arg]);
          }
        }
        else
        {
          switch (arg)
          {
          case 0x10:
            if (terminate == '=')
            {
              uint16_t v = readhexbuf(&terminate);
              x = v;
            }
            else
            {
              printf("X:%X", x);
            }

            break;

          case 0x11:
            if (terminate == '=')
            {
              uint16_t v = readhexbuf(&terminate);
              p = v;
            }
            else
            {
              printf("P:%X", p);;
            }

            break;

          case 0x12:
            if (terminate == '=')
            {
              uint16_t v = readhexbuf(&terminate);
              d = v;
            }
            else
            {
              printf("D:%02X", d);
            }

            break;

          case 0x13:
            if (terminate == '=')
            {
              uint16_t v = readhexbuf(&terminate);
              df = v;
            }
            else
            {
              printf("DF:%X", df);
            }

            break;

          case 0x14:
            if (terminate == '=')
            {
              uint16_t v = readhexbuf(&terminate);
              q = v;
            }
            else
            {
              printf("Q:%X", q);
            }

          case 0x15:
            if (terminate == '=')
            {
              uint16_t v = readhexbuf(&terminate);
              t = v;
            }
            else
            {
              printf("T:%02X", t);
            }

            break;
          }
        }
      }

      break;

    case 'Q':
    {
      runstate = 0;
      monactive = 0;
      return 0;
    }
    case 'C':
    case 'X':
      monactive = 0;
      return 1;
    case 'I':
      print2hex(input(arg));
      break;

    case 'O':
    {
      uint8_t v = readhexbuf(&terminate);
      output(arg, v);
      break;
    }

    case 'G':
    {
      if (terminate != '\r')
        p = readhexbuf(&terminate);
      reg[p] = arg;
      runstate = 1;
      monactive = 0;
      return 0;
    }

    case 'M':
    {
      uint16_t arg2 = 0;
      if (terminate == '=')
      {
        uint8_t d;
        while (cmdbuf[cb] != 0)
        {
          d = readhexbuf(&terminate, 0);
          if (noread == 0)
            memwrite(arg++, d);
          if (terminate == ';')
            break;
        }
        if (terminate == ';')
          break;
        do
        {
          if (terminate == '\r' || terminate == '=')
          {
            printf("\r\n");
            print4hex(arg);
            printf(": ");
          }
          d = readhex(&terminate, 0);
          if (terminate != ';' || noread == 0)
            memwrite(arg++, d);
        } while (terminate != ';');
      }
      else
      {
        uint16_t i, limit;
        unsigned ct = 16;
        if (terminate != '\r')
          arg2 = readhexbuf(&terminate, 0);
        if (arg2 == 0)
          arg2 = 0x100;


        // normalize
        i = arg & 0xF;  // how much off are we?
        arg &= 0xFFF0;
        arg2 += i;
        if (arg2 & 0xF)
        {
          arg2 &= 0xFFF0;
          arg2 += 0x10;  // make sure we have a multiple of 16
        }

        limit = (arg + arg2) - 1;
        if (limit < arg)
          limit = 0xFFFF; // wrapped around!


        printf(F("       0  1  2  3  4  5  6  7   8  9  A  B  C  D  E  F"));
        for (i = arg; i <= limit; i++)
        {
          if (ct % 16 == 0)
          {
            if (ct != 16)
              adump(i - 16);
            printf("\r\n");
            print4hex(i);
            printf(": ");
          }
          else if (ct % 8 == 0)
            putchar(' ');
          ct++;

          print2hex(memread(i));
          putchar(' ');
          if (i == 0xFFFF)
            break; // hit limit
          if (kbhit() && Serialread() == 0x1b)
            break;
        }
        adump(i - 16);
      }
    }
    break;

    default:
      putchar((char)cmd);
      printf(F("? "));
    }
  }
}

#endif
