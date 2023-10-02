#ifndef __GETCH_H
#define __GETCH_H

void reset_terminal_mode(void);
void set_conio_terminal_mode(void);
int kbhit(void);
int getch(void);
#endif