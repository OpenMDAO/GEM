#
include ../include/$(GEM_ARCH)
QBLD  =
DBLD  =
ODIR  = $(GEM_BLOC)/obj
LDIR  = $(GEM_BLOC)/lib
TDIR  = $(GEM_BLOC)/test
ifdef CAPRILIB
QBLD  = $(TDIR)/qdrep
endif
ifdef EGADSLIB
DBLD  = $(TDIR)/ddrep
endif

default:	$(QBLD) $(DBLD)

$(TDIR)/qdrep:	$(ODIR)/qdrep.o $(LDIR)/libquartz.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/qdrep $(DLINK) $(ODIR)/qdrep.o \
		-L$(LDIR) -lgem -lquartz -lgem -lquartz \
		-L$(CAPRILIB) -lcapriDyn -ldcapri $(XLIBS) -lm $(EXPRTS)

$(ODIR)/qdrep.o:	drep.c ../include/gem.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -DQUARTZ -I../include \
		drep.c -o $(ODIR)/qdrep.o

$(TDIR)/ddrep:	$(ODIR)/ddrep.o $(LDIR)/libdiamond.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/ddrep $(ODIR)/ddrep.o -L$(LDIR) -lgem \
		-ldiamond -L$(EGADSLIB) -legads

$(ODIR)/ddrep.o:	drep.c ../include/gem.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include \
		drep.c -o $(ODIR)/ddrep.o

clean:
	-rm $(ODIR)/ddrep.o $(TDIR)/ddrep $(ODIR)/qdrep.o $(TDIR)/qdrep

lint:
	splint -usedef -realcompare +relaxtypes -compdef -nullassign \
		-retvalint -usereleased -mustfreeonly -branchstate -temptrans \
		-nullstate -compmempass -onlytrans -globstate -statictrans \
		-initsize -type -fixedformalarray -shiftnegative -compdestroy \
		-unqualifiedtrans -warnposix -predboolint \
		drep.c -I../include
