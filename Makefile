CXXFLAGS=-g

1802pc : 1802bios.o getch.o pceeprom.o 1802.o 1802io.o 1802black.o 1802exec.o 1802mon.o disasm1802.o ihex1802.o ihexout.o ihex.o
	g++ -o 1802pc -g $^
