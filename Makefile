all:
	@echo Sorry.. To make, use the following:
	@echo GUS version: make gus
	@echo generic /dev/dsp version: make other	
	@echo stereo /dev/dsp version: make stereo
	@echo alsa  stereo alsa version: make alsa

REV=R1_5

# 
#      /usr/X11R6/include
#      Tells the compiler where to look for X11R6 include files
#      /usr/X11R6/lib 
#      Tells the compiler where the X11R6 libraries are.
#      This might cause problems...
#
INCL=/usr/X11R6/include
LIBR=/usr/X11R6/lib

F=-O2 -DDRUMS_ROOT_DIR=\"`pwd`\"
#F=-g

gus:
	gcc -c $(F) sbdrum.c -o  sbdrum.o 
	@make xdrum	

other:
	gcc -c $(F) -DNOGUS sbdrum.c -o  sbdrum.o 
	@make xdrum	

stereo:
	gcc -c $(F) -DNOGUS -DSTEREO sbdrum.c -o  sbdrum.o 
	@make xdrum	

alsa:
	gcc -c $(F) -DALSADRUM -DSTEREO -g alsadrum.c -o  alsadrum.o 
	@make modern


files.o:  files.c drum.h
	gcc -c $(F) -o files.o files.c

xdrum.o: xdrum.c drum.h
	gcc -c $(F) -g -o xdrum.o -I$(INCL) xdrum.c

xdrum_imgui.o: xdrum_imgui.cpp drum.h
	g++ -c $(F) -g -o xdrum_imgui.o -I$(INCL) xdrum_imgui.cpp

widget.o: widget.c drum.h
	gcc -c $(F) -o widget.o -I$(INCL) widget.c


sbdrum.o: gdrum.h sbdrum.c drum.h
	gcc -c $(F) sbdrum.c -o  sbdrum.o 

xdrum: sbdrum.o xdrum.o widget.o files.o  
	gcc -L$(LIBR) xdrum.o widget.o sbdrum.o files.o -lXaw -lXmu -lXt -lX11  -o xdrum

modern: alsadrum.o xdrum_imgui.o widget.o files.o  
	g++ -L$(LIBR) -g xdrum_imgui.o widget.o alsadrum.o files.o -lXaw -lXmu -lXt -lX11 -lasound  -o xdrum

clean:
	rm -f  xdrum  *.o

rel:
	tar cvf xdrum.rel.tar README DRUMS DRUMS.1024 DRUMS.GUS play.h drum.h sbdrum.c gdrum.h xdrum.h xdrum.c \
	widget.c files.c files.h gmidi.h gusload.c gusload.h Makefile xbm/*.bit samples/* songs/test.sng

code:
	tar cvf xdrum.code.tar README DRUMS DRUMS.1024 DRUMS.GUS play.h drum.h sbdrum.c gdrum.h xdrum.h xdrum.c \
	widget.c files.c files.h gmidi.h gusload.c gusload.h Makefile xbm/*.bit  songs/test.sng

drums:
	tar cvf xdrum.drums.tar samples/* songs/test.sng

