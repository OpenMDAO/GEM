#
include ../include/$(GEM_ARCH)
QBLD  =
DBLD  =
ODIR  = $(GEM_BLOC)/obj
LDIR  = $(GEM_BLOC)/lib
TDIR  = $(GEM_BLOC)/test
ifdef CAPRILIB
QBLD  = $(TDIR)/qmprop
endif
ifdef EGADSLIB
DBLD  = $(TDIR)/dmprop
endif

default:	$(QBLD) $(DBLD)

$(TDIR)/qmprop:	$(ODIR)/qmprop.o $(LDIR)/libquartz.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/qmprop $(DLINK) $(ODIR)/qmprop.o \
		-L$(LDIR) -lgem -lquartz -lgem -lquartz \
		-L$(CAPRILIB) -lcapriDyn -ldcapri $(XLIBS) -lm $(EXPRTS)

$(ODIR)/qmprop.o:	mprop.c ../include/gem.h $(CAPRIINC)/capri.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -DQUARTZ -I../include -I$(CAPRIINC) \
		mprop.c -o $(ODIR)/qmprop.o

$(TDIR)/dmprop:	$(ODIR)/dmprop.o $(LDIR)/libdiamond.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/dmprop $(ODIR)/dmprop.o -L$(LDIR) -lgem \
		-ldiamond -L$(EGADSLIB) -legads

$(ODIR)/dmprop.o:	mprop.c ../include/gem.h $(EGADSINC)/egads.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include -I$(EGADSINC) \
		mprop.c -o $(ODIR)/dmprop.o

clean:
	-rm $(ODIR)/dmprop.o $(TDIR)/dmprop $(ODIR)/qmprop.o $(TDIR)/qmprop

lint:
	splint -usedef -realcompare +relaxtypes -compdef -nullassign \
		-retvalint -usereleased -mustfreeonly -branchstate -temptrans \
		-nullstate -compmempass -onlytrans -globstate -statictrans \
		-initsize -type -fixedformalarray -shiftnegative -compdestroy \
		-unqualifiedtrans -warnposix -predboolint \
		mprop.c -I../include -I$(EGADSINC)
