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
QBLD = $(TDIR)\qsbo.exe
!endif
!ifdef EGADSLIB
DBLD = $(TDIR)\dsbo.exe
!endif

default:	start $(QBLD) $(DBLD) end

start:
	cd $(ODIR)
	copy $(SDIR)\sbo.c dsbo.c	/Y
	copy $(SDIR)\sbo.c qsbo.c	/Y

$(TDIR)\dsbo.exe:	dsbo.obj $(LDIR)\diamond.lib $(LDIR)\gem.lib
	cl /Fe$(TDIR)\dsbo.exe dsbo.obj $(LDIR)\gem.lib \
		$(LDIR)\diamond.lib $(EGADSLIB)\egads.lib $(LOPTS)

dsbo.obj:	dsbo.c $(IDIR)\gem.h
        cl /c $(COPTS) -I$(IDIR) dsbo.c

$(TDIR)\qsbo.exe:	qsbo.obj $(LDIR)\quartz.lib $(LDIR)\gem.lib
	cl /Fe$(TDIR)\qsbo.exe qsbo.obj $(LDIR)\quartz.lib \
		$(LDIR)\gem.lib $(CAPRILIB)\capriDyn.lib \
		$(CAPRILIB)\dcapri.lib $(LOPTS)

qsbo.obj:	qsbo.c $(IDIR)\gem.h
	cl /c $(COPTS) /I$(IDIR) /DQUARTZ qsbo.c

end:
	-del qsbo.c dsbo.c
	cd $(SDIR)

clean:
	-del $(ODIR)\qsbo.obj $(ODIR)\dsbo.obj 
	-del $(TDIR)\qsbo.exe $(TDIR)\dsbo.exe
