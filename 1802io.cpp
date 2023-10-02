
#include "1802.h"
#include "main.h" // need Serialread
#include <sys/time.h>
#include <cstdio>

volatile int brkflag = 0;


uint8_t inp_serial(void)
{
  int rv = Serialread();
  return rv == -1 ? 0 : rv;
}

uint8_t inp_data(void)
{
  return idata;
}

uint8_t inp_temp(void)
{
  float f = 0;
  return (unsigned int)f;
}

void out_serial(uint8_t v)
{
  serputc(v);
}

void out_data(uint8_t v)
{
  data = v;
}

void out_a0(uint8_t v)
{
  adlow = v;
}

void out_a1(uint8_t v)
{
  adhigh = v;
}

void out_ctl(uint8_t v)
{
  noserial = v & 1;      // set 1 to use serial port as I/O instead of front panel
  addisp = (v & 2) == 2; // set bit 1 to use address displays
}

uint8_t (*inputfn[])(void) =
    {
        inp_serial,
        inp_data,
        inp_temp};

void (*outputfn[])(uint8_t) =
    {
        out_serial,
        out_data,
        out_a0,
        out_a1,
        out_ctl};

static unsigned keybuffer64 = 0;
static uint64_t holder;

uint8_t setkeybuffer(void)
{
  int i;
  uint64_t hold;
  if (keybuffer64 != 0)
    for (i = 0; i < 8; i++) // 8 bytes in 64 bits
    {
      hold = (holder >> 8 * i) & 0xFF;
      memwrite(keybuffer64 + (7 - i), hold);
    }
  return holder & 0xFF;
}




#define KEY_RD_BRK 0
#define KEY_RD_BRK_RESET 1
#define KEY_RD_MILLI8 2
#define KEY_RD_MILLI 3
#define KEY_RD_B1 4
#define KEY_RD_B2 5
#define KEY_RD_B3 6
#define KEY_RD_B4 7
#define KEY_RD_RND32 8
#define KEY_RD_PICOW 9
#define KEY_RD_CYCLES 0xA
#define KEY_RD_MICROS 0xB

#include <ctime>
long long current_timestamp() {
    struct timeval te; 
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // calculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}


uint8_t io_read_key(uint8_t key)
{
  switch (key)
  {
  case KEY_RD_BRK:
    return brkflag; // read if break ocurred (no reset)
  case KEY_RD_BRK_RESET:
  {
    uint8_t v = brkflag; // read if break and reset
    brkflag = 0;
    return v;
  }
  case KEY_RD_MILLI8: // get host's millis() count (low 8 bits so 0 to .255 seconds);
  {
    return current_timestamp() & 0xFF;
  }
  case KEY_RD_MILLI: // get hosts's millis count (low, followed by 4,5,6 for coordinated high byte)
  {
    holder = current_timestamp();
    return setkeybuffer();
  }
  case KEY_RD_B1: // get next byte of multibyte response
  {
    return (holder & 0xFF00) >> 8;
  }
  case KEY_RD_B2: // and the next
  {
    return (holder & 0xFF0000) >> 16;
  }
  case KEY_RD_B3: // and the top (32 bits)
  {
    return (holder & 0xFF000000) >> 24;
  }
  case KEY_RD_B4: // load top 32-bits of holder and return bottom part (use 4,5,6 for rest)
  {
    holder >>= 16;
    return holder & 0xFF;
  }
  case KEY_RD_RND32: // random number 32 bits
  {

    unsigned int seed;
    FILE* urandom = fopen("/dev/urandom", "r");
    fread(&seed, sizeof(int), 1, urandom);
    fclose(urandom);
    holder = seed;
    return setkeybuffer();
  }
  case KEY_RD_PICOW: // ask if we are a PICO W
    return 0;
  case KEY_RD_CYCLES:
    holder = 0;    // not avaiable here 
    return setkeybuffer();
  case KEY_RD_MICROS:
    holder = 0;  // not available here
    return setkeybuffer();
  default:
    return 0;
  }
}

uint8_t (*inputmap[])(void) =
    {
        inp_serial,
        inp_data,
        inp_data,
        inp_data,
        inp_data,
        inp_data,
        inp_temp};

void (*outputmap[])(uint8_t) =
    {
        out_serial,
        out_a0,
        out_a1,
        out_data,
        out_data,
        out_data,
        out_ctl};

#define KEY_WR_IMAP 0
#define KEY_WR_OMAP 1
#define KEY_WR_BUFH 2
#define KEY_WR_BUFL 3
#define KEY_WR_DISDISKLED 4
#define KEY_WR_BUF16 0x80
#define KEY_WR_DELAY16 0x81
#define KEY_WR_TRAP16 0x82
#define KEY_WR_TVECTOR16 0x83


void io_write_key(uint8_t key, uint16_t val)
{
  unsigned p, idx;
  switch (key)
  {
  case KEY_WR_IMAP:             // map input port
    p = val >> 5;     // 0-7 where 0 is port 1
    idx = val & 0x1F; // index
    inputmap[p] = inputfn[idx];

    break;

  case KEY_WR_OMAP:             // map output put
    p = val >> 5;     // 0-7 where 0 is port 1
    idx = val & 0x1F; // index
    outputmap[p] = outputfn[idx];
    break;

  case KEY_WR_BUFH: // set high part of keybuffer
    keybuffer64 = (keybuffer64 & 0xFF) | (val << 8);
    break;

  case KEY_WR_BUFL: // low part of keybuffer
    keybuffer64 = (keybuffer64 & 0xFF00) | val;
    break;

  case KEY_WR_DISDISKLED:
    dis_diskled = val;
    break;

  case KEY_WR_BUF16:
    keybuffer64 = val;
    break;
  case KEY_WR_DELAY16:
    //delay(val);
    break;
  case KEY_WR_TRAP16:
    trap_address = val;
    break;
  case KEY_WR_TVECTOR16:
    trap_vector_address = val;
    break;
}
}
// Input from any port gives you the data register
// except port 1 is serial input
uint8_t input(uint8_t port)
{
  return inputmap[port - 1]();
}

// Output to any port writes to the data display
void output(uint8_t port, uint8_t val)
{
  outputmap[port - 1](val);
}


