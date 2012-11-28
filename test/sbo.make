#
include ../include/$(GEM_ARCH)
QBLD  =
DBLD  =
ODIR  = $(GEM_BLOC)/obj
LDIR  = $(GEM_BLOC)/lib
TDIR  = $(GEM_BLOC)/test
ifdef CAPRILIB
QBLD  = $(TDIR)/qsbo
endif
ifdef EGADSLIB
DBLD  = $(TDIR)/dsbo
endif

default:	$(QBLD) $(DBLD)

$(TDIR)/qsbo:	$(ODIR)/qsbo.o $(LDIR)/libquartz.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/qsbo $(DLINK) $(ODIR)/qsbo.o \
		-L$(LDIR) -lgem -lquartz -lgem -lquartz \
		-L$(CAPRILIB) -lcapriDyn -ldcapri $(XLIBS) -lm $(EXPRTS)

$(ODIR)/qsbo.o:	sbo.c ../include/gem.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -DQUARTZ -I../include \
		sbo.c -o $(ODIR)/qsbo.o

$(TDIR)/dsbo:	$(ODIR)/dsbo.o $(LDIR)/libdiamond.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/dsbo $(ODIR)/dsbo.o -L$(LDIR) -lgem \
		-ldiamond -L$(EGADSLIB) -legads

$(ODIR)/dsbo.o:	sbo.c ../include/gem.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include \
		sbo.c -o $(ODIR)/dsbo.o

clean:
	-rm $(ODIR)/dsbo.o $(TDIR)/dsbo $(ODIR)/qsbo.o $(TDIR)/qsbo

lint:
	splint -usedef -realcompare +relaxtypes -compdef -nullassign \
		-retvalint -usereleased -mustfreeonly -branchstate -temptrans \
		-nullstate -compmempass -onlytrans -globstate -statictrans \
		-initsize -type -fixedformalarray -shiftnegative -compdestroy \
		-unqualifiedtrans -warnposix -predboolint \
		sbo.c -I../include
