#
include ../include/$(GEM_ARCH)
IDIR  = $(CURDIR)/../include
ODIR  = $(GEM_BLOC)/obj
LDIR  = $(GEM_BLOC)/lib

VPATH = $(ODIR)

OBJS  = attribute.o base.o brep.o drep.o memory.o model.o \
	fillArea.o interp.o prmCfit.o prmGrid.o prmUV.o


$(LDIR)/libgem.a:	$(OBJS)
	-rm $(LDIR)/libgem.a
	(cd $(ODIR); ar $(LOPTS) $(LDIR)/libgem.a $(OBJS); $(RANLB) )

$(OBJS):	$(IDIR)/gem.h  $(IDIR)/brep.h   $(IDIR)/model.h \
		$(IDIR)/drep.h $(IDIR)/memory.h $(IDIR)/attribute.h kernel.h
.c.o:
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include $< -o $(ODIR)/$@

clean:
	(cd $(ODIR); rm $(OBJS) )

cleanall:
	-rm $(LDIR)/libgem.a
	(cd $(ODIR); rm $(OBJS) )
