#
!include ..\include\$(GEM_ARCH)
SDIR = $(MAKEDIR)
IDIR = $(SDIR)\..\include
ODIR = $(GEM_BLOC)\obj
LDIR = $(GEM_BLOC)\lib
TDIR = $(GEM_BLOC)\test

default:	start $(TDIR)\year2.exe end

start:
	cd $(ODIR)
	copy $(SDIR)\year2.c year2.c	/Y

$(TDIR)\year2.exe:	year2.obj $(LDIR)\diamond.lib $(LDIR)\gem.lib
	cl year2.obj $(LDIR)\gem.lib \
		$(LDIR)\diamond.lib $(EGADSLIB)\egads.lib $(LOPTS)

year2.obj:	year2.c $(IDIR)\gem.h
        cl /c $(COPTS) -I$(IDIR) year2.c

end:
	-del year2.c
	cd $(SDIR)

clean:
	-del $(ODIR)\year2.obj $(TDIR)\year2.exe
