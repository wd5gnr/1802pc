#include <cstdio>
#include "vt100.h"

void VT100::cls(void)
{
    printf("\e[2J");
}

void VT100::save(void)
{
    printf("\e[s");
}

void VT100::unsave(void)
{
    printf("\e[u");
}

void VT100::gotorc(unsigned r, unsigned c)
{
    printf("\e[%d;%df", r, c);
}

void VT100::clreol(void)
{
    printf("\e[K");
}

void VT100::clreos(void)
{
    printf("\e[J");
}

