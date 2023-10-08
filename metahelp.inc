// warning: this is included in 1802mon.cpp not compiled separate
const char hlp_commands[] = { 0, 'R','M','G','B','N','I','O','X','C','Q','.','D','$','`','V','W','S' };

const char *hlp_strings[] = {
// cmd 0
  "<R>egister, <M>emory, <G>o, <B>reakpoint, <N>ext, <I>nput, <O>utput, e<X>it\r\n"
  "<C>ontinue, .cccc (send characters to front panel; no space after .\r\n"
  "<D>isassemble <$>OS exit <`> Disk menu <V>isual toggle <W>atch <S>atus refresh\r\n"
  "For help on the R command, for example, try ?R\r\n",
// R command
"R - show all registers\r\nRn - show register n\r\nRn=xxxx - set register n to xxxx\r\n"
"  Note special registers (numbers in hex):\r\n  10=X, 11=P, 12=D, 13=DF, 14=Q, and 15=T\r\n",
// M
"M aaaa [count] - Display count number of bytes from address a\r\n"
"M aaaa = [byte...] ; Set bytes starting at aaaa.\r\n  Use a space or new line between bytes. Semicolon stops entry\r\n"
"  Note: The start address will always be divisible by 16\r\n  and the count adjusted.\r\n"
"  In visual mode, the count is ignored and is always 80 hex.\r\n  Otherwise, the default is 100 hex\r\n"
"  Also, in visual mode, issuing M without an address will\r\n  advance to the next set of addresses or to the last D address.",
// G
"G aaaa [p] - Run at address aaaa. Optionally, set P to p first.\r\n",
// B
"B [n] - List breakpoint n or all breakpoints \r\n"
"B n @aaaa - Set breakpoint n to address aaaa\r\n"
"B n Prr - Set breakpoint when P is set to rr\r\n"
"B n Inn - Set breakpoint when opcode nn executes\r\n"
"B n - - Delete breakpoint (minus sign)\r\n",
// N
"N - Step next instruction\r\n",
// I
"I pp - Show input from port pp\r\n",
// O
"O pp nn - Output nn to port pp\r\n",
//X
"X - Exit (continue running program, same as C)\r\n",
//C
" C - Continue running program\r\n",
// Q
"Q - Quit monitor (not a great idea if you have no front panel)\r\n",
// .
". - Send front panel command (no space after the .)\r\n"
"  Example: .48 enters 48 on the virtual keypad.\r\n"
"  Commands include: ;: - trace mode, *: dump state,\r\n  !: display state, $: EF4 toggle\r\n",
// D
"D aaaa [count] - Disassemble from address aaaa, through count bytes\r\n"
"  Note: The count may not be exact since some instructions\r\n  take multiple bytes\r\n"
"  In visual mode, using D by itself will either advance to\r\n  the next page or use the last M address.\r\n",
// $
"$ - Exit to the host operating system (will confirm)\r\n",
// `
"` - Enter disk submenu\r\n",
// V
"V - Toggle visual mode on and off\r\n",
// W -
"W n @aaaa - Watch 8 bytes at address aaaa\r\n"
"W n - - Remove watch n (minus sign)\r\n"
"  Note: Watches display when you use the R command to dump\r\n  all registers in non-visual mode or always in visual mode\r\n",
// S
"S - Show status (refresh screen in visual mode)\r\n"
};

void print_help(const char *s)
{
    printf("%s",s);
}

void do_help(int cmd) 
{
    int i;
    for (i=0;i<sizeof(hlp_commands)/sizeof(hlp_commands[0]); i++)
    {
        if (hlp_commands[i]==cmd) 
        {
            print_help(hlp_strings[i]);
            return;
        }
    }
    print_help(hlp_strings[0]);
}
