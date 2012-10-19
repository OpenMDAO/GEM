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
QBLD = $(TDIR)\qtess.exe
!endif
!ifdef EGADSLIB
DBLD = $(TDIR)\dtess.exe
!endif

default:	start $(QBLD) $(DBLD) end

start:
	cd $(ODIR)
	copy $(SDIR)\tess.c dtess.c	/Y
	copy $(SDIR)\tess.c qtess.c	/Y

$(TDIR)\dtess.exe:	dtess.obj $(LDIR)\diamond.lib $(LDIR)\gem.lib
	cl /Fe$(TDIR)\dtess.exe dtess.obj $(LDIR)\gem.lib $(LDIR)\diamond.lib \
		$(EGADSLIB)\egads.lib $(EGADSLIB)\gv.lib $(GLIBS) $(LOPTS)

dtess.obj:	dtess.c $(IDIR)\gem.h $(EGADSINC)\gv.h
        cl /c $(COPTS) -I$(IDIR) -I$(EGADSINC) dtess.c

$(TDIR)\qtess.exe:	qtess.obj $(LDIR)\quartz.lib $(LDIR)\gem.lib
	cl /Fe$(TDIR)\qtess.exe qtess.obj $(LDIR)\quartz.lib $(LDIR)\gem.lib \
		$(CAPRILIB)\capriDyn.lib $(CAPRILIB)\dcapri.lib \
		$(CAPRILIB)\gvIMD.lib $(GLIBS) $(LOPTS)

qtess.obj:	qtess.c $(IDIR)\gem.h $(CAPRIINC)\gv.h
	cl /c $(COPTS) /I$(IDIR) /I$(CAPRIINC) /DQUARTZ qtess.c

end:
	-del qtess.c dtess.c
	cd $(SDIR)

clean:
	-del $(ODIR)\qtess.obj $(ODIR)\dtess.obj 
	-del $(TDIR)\qtess.exe $(TDIR)\dtess.exe
