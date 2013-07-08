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
QBLD = $(TDIR)\qdrep.exe
!endif
!ifdef EGADSLIB
DBLD = $(TDIR)\ddrep.exe
!endif

default:	start $(QBLD) $(DBLD) end

start:
	cd $(ODIR)
	copy $(SDIR)\drep.c ddrep.c	/Y
	copy $(SDIR)\drep.c qdrep.c	/Y

$(TDIR)\ddrep.exe:	ddrep.obj $(LDIR)\diamond.lib $(LDIR)\gem.lib
	cl /Fe$(TDIR)\ddrep.exe ddrep.obj $(LDIR)\gem.lib \
		$(LDIR)\diamond.lib $(EGADSLIB)\egads.lib $(LOPTS)

ddrep.obj:	ddrep.c $(IDIR)\gem.h
        cl /c $(COPTS) -I$(IDIR) ddrep.c

$(TDIR)\qdrep.exe:	qdrep.obj $(LDIR)\quartz.lib $(LDIR)\gem.lib
	cl /Fe$(TDIR)\qdrep.exe qdrep.obj $(LDIR)\quartz.lib \
		$(LDIR)\gem.lib $(CAPRILIB)\capriDyn.lib \
		$(CAPRILIB)\dcapri.lib $(LOPTS)

qdrep.obj:	qdrep.c $(IDIR)\gem.h
	cl /c $(COPTS) /I$(IDIR) /DQUARTZ qdrep.c

end:
	-del qdrep.c ddrep.c
	cd $(SDIR)

clean:
	-del $(ODIR)\qdrep.obj $(ODIR)\ddrep.obj 
	-del $(TDIR)\qdrep.exe $(TDIR)\ddrep.exe
