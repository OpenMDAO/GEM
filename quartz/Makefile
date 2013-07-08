#
include ../include/$(GEM_ARCH)
ODIR  = $(GEM_BLOC)/obj
LDIR  = $(GEM_BLOC)/lib
TDIR  = $(GEM_BLOC)/test

VPATH = $(ODIR)

OBJS = qbraattr.o qboolean.o qclose.o qequiv.o qinit.o qload.o qmprops.o \
	qrelease.o qtessel.o qtopattr.o qevald.o qcurva.o qregen.o qcopy.o \
	qsave.o qsamesrf.o qeval.o qieval.o

$(TDIR)/qtest:	$(ODIR)/qtest.o $(LDIR)/libquartz.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/qtest $(DLINK) $(ODIR)/qtest.o \
		-L$(LDIR) -lgem -lquartz -lgem -lquartz \
		-L$(CAPRILIB) -lcapriDyn -ldcapri $(XLIBS) -lm $(EXPRTS)

$(ODIR)/qtest.o:	qtest.c ../include/gem.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include -I$(CAPRIINC) qtest.c \
		-o $(ODIR)/qtest.o

$(LDIR)/libquartz.a:	$(OBJS)
	-rm $(LDIR)/libquartz.a
	(cd $(ODIR); ar $(LOPTS) $(LDIR)/libquartz.a $(OBJS); $(RANLB) )

$(OBJS):	../include/gem.h ../include/brep.h ../include/model.h \
		../include/drep.h $(CAPRIINC)/capri.h
.c.o:
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include -I$(CAPRIINC) $< -o $(ODIR)/$@

clean:
	(cd $(ODIR); rm $(OBJS) qtest.o )

cleanall:
	-rm $(LDIR)/libquartz.a $(TDIR)/qtest
	(cd $(ODIR); rm $(OBJS) qtest.o )
