#
include ../include/$(GEM_ARCH)
IDIR  = $(CURDIR)/../include
ODIR  = $(GEM_BLOC)/obj
LDIR  = $(GEM_BLOC)/lib

VPATH = $(ODIR)

OBJS  = attribute.o base.o brep.o drep.o memory.o model.o conjGrad.o \
	fillArea.o approx.o prmCfit.o prmGrid.o prmUV.o transfer.o robustIn.o


default:	$(LDIR)/triConstantDiscontinuous.so \
		$(LDIR)/triLinearContinuous.so \
		$(LDIR)/triLinearDiscontinuous.so \
		$(LDIR)/quadLinearContinuous.so $(LDIR)/libgem.a

$(LDIR)/triConstantDiscontinuous.so:	triConstantDiscontinuous.o triUtils.o \
					$(LDIR)/libgem.a
	-rm $(LDIR)/triConstantDiscontinuous.so
	$(CCOMP) $(SOFLGS) -o $(LDIR)/triConstantDiscontinuous.so \
		$(ODIR)/triConstantDiscontinuous.o $(ODIR)/triUtils.o \
		-L$(LDIR) -lgem

triConstantDiscontinuous.o:	triConstantDiscontinuous.c
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include triConstantDiscontinuous.c \
		-o $(ODIR)/triConstantDiscontinuous.o

$(LDIR)/triLinearContinuous.so:	triLinearContinuous.o triUtils.o \
				$(LDIR)/libgem.a
	-rm $(LDIR)/triLinearContinuous.so
	$(CCOMP) $(SOFLGS) -o $(LDIR)/triLinearContinuous.so \
		$(ODIR)/triLinearContinuous.o $(ODIR)/triUtils.o \
		-L$(LDIR) -lgem

triLinearContinuous.o:	triLinearContinuous.c
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include triLinearContinuous.c \
		-o $(ODIR)/triLinearContinuous.o

$(LDIR)/quadLinearContinuous.so:	quadLinearContinuous.o $(LDIR)/libgem.a
	-rm $(LDIR)/quadLinearContinuous.so
	$(CCOMP) $(SOFLGS) -o $(LDIR)/quadLinearContinuous.so \
		$(ODIR)/quadLinearContinuous.o -L$(LDIR) -lgem

quadLinearContinuous.o:	quadLinearContinuous.c
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include quadLinearContinuous.c \
		-o $(ODIR)/quadLinearContinuous.o

$(LDIR)/triLinearDiscontinuous.so:	triLinearDiscontinuous.o triUtils.o \
					$(LDIR)/libgem.a
	-rm $(LDIR)/triLinearDiscontinuous.so
	$(CCOMP) $(SOFLGS) -o $(LDIR)/triLinearDiscontinuous.so \
		$(ODIR)/triLinearDiscontinuous.o $(ODIR)/triUtils.o \
		-L$(LDIR) -lgem

triLinearDiscontinuous.o:	triLinearDiscontinuous.c
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include triLinearDiscontinuous.c \
		-o $(ODIR)/triLinearDiscontinuous.o

triUtis.o:	triUtils.c
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include triUtils.c \
		-o $(ODIR)/triUtils.o

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
