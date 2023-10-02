# To install Elf/OS

For the PC version if you unzip the elfos4.zip file, you will have a fully running OS with a lot of custom software.
Some are my modifications or tools for 1802Black/PC, some are from the sources below. 

Rough steps:

1. You need to tell the Arduino to reserve some of your flash for flash storage. On a stock PICO I suggest 1M + 1M. On a WaveShare Pico Plus (for example), go for 1M + 15M for disk.
2. Boot to BIOS.
3. Use the SHOW CPU command to enter the metamonitor
4. The disk menu appears when you enter the back quote command (`)
5. From the menu, set the max track. This needs to fit in your flash from step 1. Each track has 256 512-byte sectors (so 128K per track). You probably want a little spare room, too, for file system magic so plan accordingly. On a Pico Plus, I set the track to 10 (16 -- this number is in hex) and that is plenty of room -- about 2GB.
6. Format the storage from the same menu.
7. Exit the disk menu (X)
8. Leave the metamonitor (c or reboot; note: in this verison, you only have to do one C command).
9. Load an Elf-OS RAM installer hex file by doing an ASCII transfer to the STG monitor (>>> prompt). For example: http://www.elf-emulation.com/software/elfos/elfos_ram.hex
10. Enter the command: Run 3000
11. The install menu will appear. You need to do each step in order: format the disk, create a file system, system gen, and install software. You can boot if you like or
12. If you use the above image, you probably also want Mike's supplement hex file: https://groups.io/g/cosmacelf/message/31148. Repeat steps 9-10. This time you only need option 4 and install the software there.
13. Now you can boot into ElfO/OS (thanks to members of the Elf mailing list for instructions about this including Andrew Wasson, David Madole, and others). Stop here if you like.
14. You probably would like to upgrade to 3.2 and then to 4.1 (http://www.elf-emulation.com/software.html). Keep copies of dir, chdir, anx xrb in your root directory while upgrading because EVERYTHING is a command. If you lose sight of your /BIN directory you can't do anything anymore. Note that in the version 4, you use /bin not /BIN so you must rename at least there.
15. Once you have version 4, there are a few things you might want to do:

* Linux-like shell (must be in /bin): http://www.elf-emulation.com/software/picoelf/shell.4
* Install help files in /hlp (see http://www.elf-emulation.com/software.html)
* The turbo accelerator works well with this: https://github.com/dmadole/Elfos-turbo (make /cfg)
* Try init and/or cmd: https://github.com/dmadole/Elfos-init  and https://github.com/fourstix/Elfos-utils  (nice for autoloading turbo, setting home directory) 
* Upgrade to latest kernel: https://github.com/dmadole/Elfos-kernel
* Follow instructions to make kernel upgrades nicer: https://groups.io/g/cosmacelf/wiki/27798 (make /os directory)
* Lot of tips and software links: https://groups.io/g/cosmacelf/wiki/27784
* Many utilities: https://github.com/fourstix/Elfos-utils

Good luck!
