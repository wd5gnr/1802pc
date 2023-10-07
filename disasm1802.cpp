
#include "1802.h"
#include "main.h"
#include <cstdio>

void printreg(uint8_t code)
{
    code &= 0xF;
    printf("\tR");
    if (code <= 9)
        code += '0';
    else
        code += 'A' - 10;
    putchar((char)(code));
}

// Table for Branch opcodes (0x3x)
char brtable[] = {'R', 'Q', 'Z', 'F', '1', '2', '3', '4'};

void ophex(uint8_t opcode, unsigned a)
{
    int ct = 0, extra;
    if ((opcode&0xF0==0x30)&&opcode!=38)
        ct = 1;
    if (opcode==0x7C||opcode==0x7D||opcode==0x7F)
        ct = 1;
    if (opcode>=0xC0&&opcode<=0xC3)
        ct = 2;
    if (opcode>=0xc9&&opcode<=0xCB)
        ct = 2;
    if (opcode>=0xF8 && opcode!=0xFE)
        ct = 1;
    if (opcode==0xD4)
        ct = 2;  // fake CALL SEP
    print2hex(opcode, 0);
    putchar(' ');
    extra = ct;
    while (extra--)
    {
        a++;
        print2hex(memread(a));
        if (extra) putchar(' ');
    }
    if (ct!=2)
        putchar('\t');
    putchar('\t');
}

unsigned opcode(uint8_t opcode, unsigned a)
{
    unsigned skip = 0;
    ophex(opcode, a);
    switch (opcode & 0xF0)
    {
    case 0:
        if (opcode == 0)
        {
            printf("IDL");
            return 0;
        }
        printf("LDN");
        printreg(opcode);
        return 0;

    case 0x10:
        printf("INC");
        printreg(opcode);
        return 0;

    case 0x20:
        printf("DEC");
        printreg(opcode);
        return 0;

    case 0x30:
        if (opcode == 0x38)
        {
            printf("SKP");
            return 0;
        }
        printf("B");
        if (opcode > 0x38)
            printf("N");
        if (opcode == 0x33)
            printf("D");
        putchar(brtable[opcode & 0x7]);
        //printff("\t%02X", memread(a + 1));
        print2hex(memread(a + 1), 1);
        return 1;
    case 0x40:
        printf("LDA");
        printreg(opcode);
        return 0;

    case 0x50:
        printf("STR");
        printreg(opcode);
        return 0;

    case 0x60:
        if (opcode == 0x68)
        {
            printf("ILL");
            return 0;
        }
        if (opcode > 0x68)
            printf("INP");
        else
            printf("OUT");
        putchar((char)((opcode & 0x7) + '0'));
        return 0;
    case 0x70:
        switch (opcode)
        {
        case 0x70:
            printf("RET");
            return 0;
        case 0x71:
            printf("DIS");
            return 0;
        case 0x72:
            printf("LDXA");
            return 0;
        case 0x73:
            printf("STXD");
            return 0;
        case 0x74:
            printf("ADC");
            return 0;
        case 0x75:
            printf("SDB");
            return 0;
        case 0x76:
            printf("SHRC");
            return 0;
        case 0x77:
            printf("SMB");
            return 0;
        case 0x78:
            printf("SAV");
            return 0;
        case 0x79:
            printf("MARK");
            return 0;
        case 0x7A:
            printf("REQ");
            return 0;
        case 0x7B:
            printf("SEQ");
            return 0;
        case 0x7C:
            printf("ADCI");
            print2hex(memread(a + 1), 1);
            return 1;
        case 0x7D:
            printf("SDBI");
            print2hex(memread(a + 1), 1);
            return 1;
        case 0x7E:
            printf("SHLC");
            return 0;
        case 0x7F:
            printf("SMBI");
            print2hex(memread(a + 1), 1);
            return 1;
        }
        break;
    case 0x80:
        printf("GLO");
        printreg(opcode);
        return 0;

    case 0x90:
        printf("GHI");
        printreg(opcode);
        return 0;

    case 0xA0:
        printf("PLO");
        printreg(opcode);
        return 0;

    case 0xB0:
        printf("PHI");
        printreg(opcode);
        return 0;
    case 0xC0:
        if ((opcode < 0xC4 || opcode > 0xC8) && opcode < 0xCC)
        {
            if (opcode == 0xC0)
                printf("LBR");
            if ((opcode & 0x3) == 1)
                printf((opcode & 8) ? "LBNQ" : "LBQ");
            if ((opcode & 0x3) == 2)
                printf((opcode & 8) ? "LBNZ" : "LBZ");
            if ((opcode & 0x3) == 3)
                printf((opcode & 8) ? "LBNF" : "LBDF");
            print4hex((memread(a + 1) << 8) | memread(a + 2),1);
            return 2;
        }
        if (opcode == 0xC4)
        {
            printf("NOP");
            return 0;
        }
        if (opcode == 0xCC)
        {
            printf("LSIE");
            return 0;
        }
        if ((opcode & 0x7) == 5)
        {
            printf((opcode & 8) ? "LSQ" : "LSNQ");
            return 0;
        }
        if ((opcode & 0x7) == 6)
        {
            printf((opcode & 8) ? "LSZ" : "LSNZ");
            return 0;
        }
        if ((opcode & 0x7) == 7)
        {
            printf((opcode & 8) ? "LSDF" : "LSNF");
            return 0;
        }

        break;
    case 0xD0:
        if (opcode==0xD4)
        {
            // fake opcode for call
            printf("CALL");
            print4hex((memread(a + 1) << 8) | memread(a + 2),1);
            return 2;
        }
        if (opcode==0xD5)
        {
            // fake opcode for return
            printf("RTN");
            return 0;
        }
        printf("SEP");
        printreg(opcode);
        return 0;
    case 0xE0:
        printf("SEX");
        printreg(opcode);
        return 0;

    case 0xF0:
    {
        int off = 0;
        const char *cs;

        switch (opcode)
        {

        case 0xF0:
            cs = "LDX";
            break;

        case 0xF1:
            cs = "OR";
            break;

        case 0xF2:
            cs = "AND";
            break;

        case 0xF3:
            cs = "XOR";
            break;

        case 0xF4:
            cs = "ADD";
            break;
        case 0xF5:
            cs = "SD";
            break;
        case 0xF6:
            cs = "SHR";
            break;
        case 0xF7:
            cs = "SM";
            break;
        case 0xF8:
            cs = "LDI";
            off = 1;
            break;
        case 0xF9:
            cs = "ORI";
            off = 1;
            break;
        case 0xFA:
            cs = "ANI";
            off = 1;
            break;
        case 0xFB:
            cs = "XRI";
            off = 1;
            break;
        case 0xFC:
            cs = "ADI";
            off = 1;
            break;
        case 0xFD:
            cs = "SDI";
            off = 1;
            break;
        case 0xFE:
            cs = "SHL";
            break;
        case 0xFF:
            cs = "SMI";
            off = 1;
            break;
        }  // end switch
        printf("%s",cs);
        if (off)
            print2hex(memread(a + 1), 1);
        return off;
    }  // end outer switch
    }
    return 0;
}

unsigned disasmline(unsigned mp, int nl)
{
    unsigned rv;
    print4hex(mp);
    putchar('\t');
    rv = opcode(memread(mp), mp);
    if (nl)
        printf("\r\n");
    return rv;
}

void disasm1802(unsigned start, unsigned stop)
{
    unsigned mp;
    for (mp = start; mp <= stop; mp++)
    {
        if (mp < start)
            return; // strange wrap around
        mp+=disasmline(mp,1);
    }
}