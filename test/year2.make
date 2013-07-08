#
include ../include/$(GEM_ARCH)
ODIR = $(GEM_BLOC)/obj
LDIR = $(GEM_BLOC)/lib
TDIR = $(GEM_BLOC)/test

$(TDIR)/year2:	$(ODIR)/year2.o $(LDIR)/libdiamond.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/year2 $(ODIR)/year2.o -L$(LDIR) -lgem \
		-ldiamond -L$(EGADSLIB) -legads

$(ODIR)/year2.o:	year2.c ../include/gem.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include -I$(EGADSINC) year2.c \
		-o $(ODIR)/year2.o

clean:
	-rm $(ODIR)/year2.o $(TDIR)/year2

lint:
	splint -usedef -realcompare +relaxtypes -compdef -nullassign \
		-retvalint -usereleased -mustfreeonly -branchstate -temptrans \
		-nullstate -compmempass -onlytrans -globstate -statictrans \
		-initsize -type -fixedformalarray -shiftnegative -compdestroy \
		-unqualifiedtrans -warnposix -predboolint \
		year2.c -I../include -I$(EGADSINC)
