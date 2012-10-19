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
QBLD = $(TDIR)\qmmdl.exe
!endif
!ifdef EGADSLIB
DBLD = $(TDIR)\dmmdl.exe
!endif

default:	start $(QBLD) $(DBLD) end

start:
	cd $(ODIR)
	copy $(SDIR)\mmdl.c dmmdl.c	/Y
	copy $(SDIR)\mmdl.c qmmdl.c	/Y

$(TDIR)\dmmdl.exe:	dmmdl.obj $(LDIR)\diamond.lib $(LDIR)\gem.lib
	cl /Fe$(TDIR)\dmmdl.exe dmmdl.obj $(LDIR)\gem.lib $(LDIR)\diamond.lib \
		$(EGADSLIB)\egads.lib $(LOPTS)

dmmdl.obj:	dmmdl.c $(IDIR)\gem.h
        cl /c $(COPTS) -I$(IDIR) dmmdl.c

$(TDIR)\qmmdl.exe:	qmmdl.obj $(LDIR)\quartz.lib $(LDIR)\gem.lib
	cl /Fe$(TDIR)\qmmdl.exe qmmdl.obj $(LDIR)\quartz.lib $(LDIR)\gem.lib \
		$(CAPRILIB)\capriDyn.lib $(CAPRILIB)\dcapri.lib $(LOPTS)

qmmdl.obj:	qmmdl.c $(IDIR)\gem.h
	cl /c $(COPTS) /I$(IDIR) /DQUARTZ qmmdl.c

end:
	-del qmmdl.c dmmdl.c
	cd $(SDIR)

clean:
	-del $(ODIR)\qmmdl.obj $(ODIR)\dmmdl.obj 
	-del $(TDIR)\qmmdl.exe $(TDIR)\dmmdl.exe
