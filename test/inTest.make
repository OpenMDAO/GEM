#
include ../include/$(GEM_ARCH)
ODIR  = $(GEM_BLOC)/obj
LDIR  = $(GEM_BLOC)/lib
TDIR  = $(GEM_BLOC)/test

$(TDIR)/inTest:	$(ODIR)/inTest.o $(LDIR)/libdiamond.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/inTest $(ODIR)/inTest.o -L$(LDIR) -lgem \
		-ldiamond -L$(EGADSLIB) -legads

$(ODIR)/inTest.o:	inTest.c ../include/gem.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include \
		inTest.c -o $(ODIR)/inTest.o

clean:
	-rm $(ODIR)/inTest.o $(TDIR)/inTest
