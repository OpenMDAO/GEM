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
QBLD = $(TDIR)\qstatic.exe
!endif
!ifdef EGADSLIB
DBLD = $(TDIR)\dstatic.exe
!endif

default:	start $(QBLD) $(DBLD) end

start:
	cd $(ODIR)
	copy $(SDIR)\static.c dstatic.c	/Y
	copy $(SDIR)\static.c qstatic.c	/Y

$(TDIR)\dstatic.exe:	dstatic.obj $(LDIR)\diamond.lib $(LDIR)\gem.lib
	cl /Fe$(TDIR)\dstatic.exe dstatic.obj $(LDIR)\gem.lib \
		$(LDIR)\diamond.lib $(EGADSLIB)\egads.lib $(LOPTS)

dstatic.obj:	dstatic.c $(IDIR)\gem.h
        cl /c $(COPTS) -I$(IDIR) dstatic.c

$(TDIR)\qstatic.exe:	qstatic.obj $(LDIR)\quartz.lib $(LDIR)\gem.lib
	cl /Fe$(TDIR)\qstatic.exe qstatic.obj $(LDIR)\quartz.lib \
		$(LDIR)\gem.lib $(CAPRILIB)\capriDyn.lib \
		$(CAPRILIB)\dcapri.lib $(LOPTS)

qstatic.obj:	qstatic.c $(IDIR)\gem.h
	cl /c $(COPTS) /I$(IDIR) /DQUARTZ qstatic.c

end:
	-del qstatic.c dstatic.c
	cd $(SDIR)

clean:
	-del $(ODIR)\qstatic.obj $(ODIR)\dstatic.obj 
	-del $(TDIR)\qstatic.exe $(TDIR)\dstatic.exe
