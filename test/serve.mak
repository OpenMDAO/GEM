#
!include ..\include\$(GEM_ARCH)
SDIR = $(MAKEDIR)
IDIR = $(SDIR)\..\include
WDIR = $(EGADSINC)\..\wvServer
ODIR = $(GEM_BLOC)\obj
LDIR = $(GEM_BLOC)\lib
TDIR = $(GEM_BLOC)\test
WS2LIB = "C:\Program Files (x86)\Microsoft SDKs\Windows\v7.0A\Lib\x64\WS2_32.Lib"

default:	$(TDIR)\dserve.exe $(TDIR)\gserve.exe

$(TDIR)\dserve.exe:	$(ODIR)\serve.obj $(LDIR)\diamond.lib $(LDIR)\gem.lib \
			$(EGADSLIB)\egads.lib $(EGADSLIB)\wsserver.lib \
			$(EGADSLIB)\z.lib
	cl /Fe$(TDIR)\dserve.exe $(ODIR)\serve.obj $(LDIR)\gem.lib \
		$(LDIR)\diamond.lib /link /LIBPATH:$(EGADSLIB) egads.lib \
		wsserver.lib z.lib ws2_32.lib /NODEFAULTLIB:LIBCMT
	$(MCOMP) /manifest $(TDIR)\dserve.exe.manifest \
		/outputresource:$(TDIR)\dserve.exe;1

$(TDIR)\gserve.exe:	$(ODIR)\serve.obj $(LDIR)\geode.lib $(LDIR)\gem.lib \
			$(EGADSLIB)\egads.lib $(EGADSLIB)\wsserver.lib \
			$(EGADSLIB)\z.lib
	cl /Fe$(TDIR)\gserve.exe $(ODIR)\serve.obj $(LDIR)\gem.lib \
		$(LDIR)\geode.lib $(LDIR)\geoduckclient.lib \
		$(LDIR)\geoduck.lib /link /LIBPATH:$(EGADSLIB) egads.lib \
		wsserver.lib z.lib $(WS2LIB) /NODEFAULTLIB:LIBCMT
	$(MCOMP) /manifest $(TDIR)\gserve.exe.manifest \
		/outputresource:$(TDIR)\gserve.exe;1

$(ODIR)\serve.obj:	serve.c $(IDIR)\gem.h $(EGADSINC)\egads.h \
			$(EGADSINC)\wsss.h $(EGADSINC)\wsserver.h
	cl /c $(COPTS) -I$(IDIR) -I$(EGADSINC) -I$(WDIR)\win32helpers serve.c \
		/Fo$(ODIR)\serve.obj

clean:
	-del $(ODIR)\serve.obj

cleanall:
	-del $(TDIR)\dserve.exe $(TDIR)\dserve.exe.manifest $(ODIR)\serve.obj
	-del $(TDIR)\gserve.exe $(TDIR)\gserve.exe.manifest
