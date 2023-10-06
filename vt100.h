#ifndef __VT100_H
#define __VT100_H


class VT100
{
    public:
        static void cls(void);
        static void save(void);
        static void unsave(void);
        static void gotorc(unsigned r, unsigned c);
        static void clreol(void);
        static void clreos(void);
};

#endif