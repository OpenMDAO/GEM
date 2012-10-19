#
include ../include/$(GEM_ARCH)
ODIR  = $(GEM_BLOC)/obj
LDIR  = $(GEM_BLOC)/lib
TDIR  = $(GEM_BLOC)/test


$(TDIR)/dserve:	$(ODIR)/serve.o $(LDIR)/libdiamond.a $(LDIR)/libgem.a \
		$(LDIR)/libwsserver.a
	$(CCOMP) -o $(TDIR)/dserve $(ODIR)/serve.o -L$(LDIR) -lgem -ldiamond \
		-L$(EGADSLIB) -legads -lwsserver -lpthread -lz -lm

$(ODIR)/serve.o:	serve.c ../include/gem.h $(EGADSINC)/egads.h \
			$(EGADSINC)/wsserver.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include -I$(EGADSINC) serve.c \
		-o $(ODIR)/serve.o

clean:
	-rm $(ODIR)/serve.o

cleanall:
	-rm $(ODIR)/serve.o $(TDIR)/dserve
