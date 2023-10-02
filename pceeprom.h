#ifndef __EEPROM_H
#define __EEPROM_H
#include <cstdio>
#include <cstdint>


class _EEPROM
{
    protected:
        FILE *fp;

    public:
        int init(const char *eename="eeprom.dat");
        uint8_t read(uint16_t address);
        void write(uint16_t address, uint8_t byte);
        void commit(void);
};

extern _EEPROM EEPROM;

#endif