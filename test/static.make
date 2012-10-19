#
include ../include/$(GEM_ARCH)
QBLD  =
DBLD  =
ODIR  = $(GEM_BLOC)/obj
LDIR  = $(GEM_BLOC)/lib
TDIR  = $(GEM_BLOC)/test
ifdef CAPRILIB
QBLD  = $(TDIR)/qstatic
endif
ifdef EGADSLIB
DBLD  = $(TDIR)/dstatic
endif

default:	$(QBLD) $(DBLD)

$(TDIR)/qstatic:	$(ODIR)/qstatic.o $(LDIR)/libquartz.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/qstatic $(DLINK) $(ODIR)/qstatic.o \
		-L$(LDIR) -lgem -lquartz -lgem -lquartz \
		-L$(CAPRILIB) -lcapriDyn -ldcapri $(XLIBS) -lm $(EXPRTS)

$(ODIR)/qstatic.o:	static.c ../include/gem.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -DQUARTZ -I../include \
		static.c -o $(ODIR)/qstatic.o

$(TDIR)/dstatic:	$(ODIR)/dstatic.o $(LDIR)/libdiamond.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/dstatic $(ODIR)/dstatic.o -L$(LDIR) -lgem \
		-ldiamond -L$(EGADSLIB) -legads

$(ODIR)/dstatic.o:	static.c ../include/gem.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include \
		static.c -o $(ODIR)/dstatic.o

clean:
	-rm $(ODIR)/dstatic.o $(TDIR)/dstatic $(ODIR)/qstatic.o $(TDIR)/qstatic

lint:
	splint -usedef -realcompare +relaxtypes -compdef -nullassign \
		-retvalint -usereleased -mustfreeonly -branchstate -temptrans \
		-nullstate -compmempass -onlytrans -globstate -statictrans \
		-initsize -type -fixedformalarray -shiftnegative -compdestroy \
		-unqualifiedtrans -warnposix -predboolint \
		static.c -I../include
