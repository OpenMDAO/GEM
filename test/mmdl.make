#
include ../include/$(GEM_ARCH)
QBLD  =
DBLD  = 
ODIR  = $(GEM_BLOC)/obj
LDIR  = $(GEM_BLOC)/lib
TDIR  = $(GEM_BLOC)/test
ifdef CAPRILIB
QBLD  = $(TDIR)/qmmdl
endif
ifdef EGADSLIB
DBLD  = $(TDIR)/dmmdl
endif

default:	$(QBLD) $(DBLD)

$(TDIR)/qmmdl:	$(ODIR)/qmmdl.o $(LDIR)/libquartz.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/qmmdl $(DLINK) $(ODIR)/qmmdl.o \
		-L$(LDIR) -lgem -lquartz -lgem -lquartz \
		-L$(CAPRILIB) -lcapriDyn -ldcapri $(XLIBS) -lm $(EXPRTS)

$(ODIR)/qmmdl.o:	mmdl.c ../include/gem.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -DQUARTZ -I../include \
		mmdl.c -o $(ODIR)/qmmdl.o

$(TDIR)/dmmdl:	$(ODIR)/dmmdl.o $(LDIR)/libdiamond.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/dmmdl $(ODIR)/dmmdl.o -L$(LDIR) -lgem -ldiamond \
		-L$(EGADSLIB) -legads

$(ODIR)/dmmdl.o:	mmdl.c ../include/gem.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include \
		mmdl.c -o $(ODIR)/dmmdl.o

clean:
	-rm $(ODIR)/dmmdl.o $(TDIR)/dmmdl $(ODIR)/qmmdl.o $(TDIR)/qmmdl

lint:
	splint -usedef -realcompare +relaxtypes -compdef -nullassign \
		-retvalint -usereleased -mustfreeonly -branchstate -temptrans \
		-nullstate -compmempass -onlytrans -globstate -statictrans \
		-initsize -type -fixedformalarray -shiftnegative -compdestroy \
		-unqualifiedtrans -warnposix -predboolint \
		mmdl.c -I../include
