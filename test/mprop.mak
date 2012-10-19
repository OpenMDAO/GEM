#
!include ..\include\$(GEM_ARCH)
QBLD =
DBLD =
SDIR = $(MAKEDIR)
IDIR = $(SDIR)\..\include
ODIR = $(GEM_BLOC)\obj
LDIR = $(GEM_BLOC)\lib
TDIR = $(GEM_BLOC)\test
!ifdef CAPRILIB
QBLD = $(TDIR)\qmprop.exe
!endif
!ifdef EGADSLIB
DBLD = $(TDIR)\dmprop.exe
!endif

default:	start $(QBLD) $(DBLD) end

start:
	cd $(ODIR)
	copy $(SDIR)\mprop.c dmprop.c	/Y
	copy $(SDIR)\mprop.c qmprop.c	/Y

$(TDIR)\dmprop.exe:	dmprop.obj $(LDIR)\diamond.lib $(LDIR)\gem.lib
	cl /Fe$(TDIR)\dmprop.exe dmprop.obj $(LDIR)\gem.lib \
		$(LDIR)\diamond.lib $(EGADSLIB)\egads.lib $(LOPTS)

dmprop.obj:	dmprop.c $(IDIR)\gem.h $(EGADSINC)\egads.h
        cl /c $(COPTS) -I$(IDIR) -I$(EGADSINC) dmprop.c

$(TDIR)\qmprop.exe:	qmprop.obj $(LDIR)\quartz.lib $(LDIR)\gem.lib
	cl /Fe$(TDIR)\qmprop.exe qmprop.obj $(LDIR)\quartz.lib \
		$(LDIR)\gem.lib $(CAPRILIB)\capriDyn.lib \
		$(CAPRILIB)\dcapri.lib $(LOPTS)

qmprop.obj:	qmprop.c $(IDIR)\gem.h $(CAPRIINC)/capri.h
	cl /c $(COPTS) /I$(IDIR) /I$(CAPRIINC) /DQUARTZ qmprop.c

end:
	-del qmprop.c dmprop.c
	cd $(SDIR)

clean:
	-del $(ODIR)\qmprop.obj $(ODIR)\dmprop.obj 
	-del $(TDIR)\qmprop.exe $(TDIR)\dmprop.exe
