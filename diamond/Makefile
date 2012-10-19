#
include ../include/$(GEM_ARCH)
ODIR  = $(GEM_BLOC)/obj
LDIR  = $(GEM_BLOC)/lib
TDIR  = $(GEM_BLOC)/test

VPATH = $(ODIR)

OBJS =	dbraattr.o dboolean.o dclose.o dequiv.o dinit.o dload.o dmprops.o \
	drelease.o dtessel.o dtopattr.o deval.o devald.o dcurva.o dinside.o \
	dregen.o dcopy.o dsave.o


$(TDIR)/dtest:	$(ODIR)/dtest.o $(LDIR)/libdiamond.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/dtest $(ODIR)/dtest.o -L$(LDIR) -lgem -ldiamond \
		-L$(EGADSLIB) -legads

$(ODIR)/dtest.o:	dtest.c ../include/gem.h $(EGADSINC)/egads.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include -I$(EGADSINC) dtest.c \
		-o $(ODIR)/dtest.o

$(LDIR)/libdiamond.a:	$(OBJS) OpenCSM.o udp.o
	-rm $(LDIR)/libdiamond.a
	(cd $(ODIR); ar $(LOPTS) $(LDIR)/libdiamond.a $(OBJS) OpenCSM.o udp.o; $(RANLB) )

udp.o:	$(OCSM_SRC)/udp.c $(EGADSINC)/egads.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -DGEOM_EGADS -I$(EGADSINC) \
		$(OCSM_SRC)/udp.c -o $(ODIR)/udp.o

OpenCSM.o:	$(OCSM_SRC)/OpenCSM.c $(OCSM_SRC)/common.h \
		$(OCSM_SRC)/OpenCSM.h $(EGADSINC)/egads.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -DGEOM_EGADS -I$(OCSM_SRC) \
		-I$(EGADSINC) $(OCSM_SRC)/OpenCSM.c -o $(ODIR)/OpenCSM.o

$(OBJS):	../include/gem.h ../include/brep.h ../include/model.h \
		../include/drep.h $(OCSM_SRC)/OpenCSM.h $(EGADSINC)/egads.h
.c.o:
	$(CCOMP) -c $(COPTS) $(DEFINE) -DGEOM_EGADS -I../include -I$(OCSM_SRC) \
		-I$(EGADSINC) $< -o $(ODIR)/$@

clean:
	(cd $(ODIR); rm $(OBJS) OpenCSM.o udp.o dtest.o )

cleanall:
	-rm $(LDIR)/libdiamond.a $(TDIR)/dtest $(LDIR)/import.$(SHEXT)
	(cd $(ODIR); rm $(OBJS) OpenCSM.o udp.o dtest.o )
