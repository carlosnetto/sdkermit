sdker.exe : cfgcomm.c cfgcomm.e cfgscr.c cfgscr.e cfgterm.c cfgterm.e         \
            config.c config.e critical.c critical.e expfile.c expfile.e ker.c \
	    keyboard.c keyboard.e malloc.c malloc.e menutree.c menutree.e     \
	    newmenu.c newmenu.e protocol.c protocol.e readstr.c readstr.e     \
	    serial.c serial.e strtool.c strtool.e terminal.c terminal.e       \
	    types.e writemsg.c writemsg.e scrker.c scrker.e bios.c bios.e     \
	    turboc.cfg 
  tc /M
  sdker

sdkerdsk.zip : sdker.exe
  rem Insert a blank disk in A:
  label a:'RIUNBDLKA!
  copy sdker.exe a:/v
  date 01-01-90
  time 1:01:00
  touch a:sdker.exe
  timer /s
  dsk2file disk.dsk
  pkzip -m sdkerdsk.zip disk.dsk
  
sdker : sdkerdsk.zip
  pkunzip sdkerdsk.zip
  gendsk -l SDK_%07.7ld -c -o Serno.DOC -i 0 -n 0 disk.dsk
  del disk.dsk

demo.exe : sdker.exe
  ren serial.obj *.o
  ren ker.obj *.o
  tcc -DDEMO -c serial.c ker.c
  tcc -eDEMO.EXE *.obj
  del serial.obj
  del ker.obj
  ren *.o *.obj
  demo

demodsk.zip : demo.exe
  rem Insert a blank disk in A:
  label a:'RIUNBDLKA!
  copy demo.exe a:sdker.exe/v
  date 01-02-90
  time 1:01:00
  touch a:sdker.exe
  timer /s
  dsk2file disk.dsk
  pkzip -m demodsk.zip disk.dsk

demo : demodsk.zip
  pkunzip demodsk.zip
  gendsk -c -l SDKermit -n 0 disk.dsk
  del disk.dsk

mysdker.exe : sdker.exe
  ren terminal.obj *.o
  tcc -DMYSDKER -c terminal.c
  tcc -eMYSDKER.EXE *.obj
  del terminal.obj
  ren *.o *.obj
  setserno -i 1 mysdker.exe
  mysdker

mysdker : mysdker.exe
  rem Insert a blank disk in A:
  label a:SD_SDKERMIT
  copy mysdker.exe a:sdker.exe /v
  date 01-03-90
  time 1:01:00
  touch a:sdker.exe
  timer /s

turboc.cfg : tcconfig.tc remdebug.rc
  tcconfig tcconfig.tc

backup :
  - del \fastback\*.cat
  - \fastback\fastback c: \prj\sdker y *.* n
  - del \fastback\*.cat

all : sdkerdsk.zip demodsk.zip mysdker.exe
