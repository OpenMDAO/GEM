#
include ../include/$(GEM_ARCH)
QBLD  =
DBLD  =
ODIR  = $(GEM_BLOC)/obj
LDIR  = $(GEM_BLOC)/lib
TDIR  = $(GEM_BLOC)/test
ifdef CAPRILIB
QBLD  = $(TDIR)/qtess
endif
ifdef EGADSLIB
DBLD  = $(TDIR)/dtess
endif

default:	$(QBLD) $(DBLD)

$(TDIR)/qtess:	$(ODIR)/qtess.o $(LDIR)/libquartz.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/qtess $(DLINK) $(ODIR)/qtess.o \
		-L$(LDIR) -lgem -lquartz -lgem -lquartz \
		-L$(CAPRILIB) -lcapriDyn -ldcapri -lgv $(GLIBS) -lm $(EXPRTS)

$(ODIR)/qtess.o:	tess.c ../include/gem.h $(CAPRIINC)/gv.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -DQUARTZ -I../include -I$(CAPRIINC) \
		tess.c -o $(ODIR)/qtess.o

$(TDIR)/dtess:	$(ODIR)/dtess.o $(LDIR)/libdiamond.a $(LDIR)/libgem.a
	$(CCOMP) -o $(TDIR)/dtess $(ODIR)/dtess.o -L$(LDIR) -lgem -ldiamond \
		-L$(EGADSLIB) -legads -lgv $(GLIBS)

$(ODIR)/dtess.o:	tess.c ../include/gem.h $(EGADSINC)/gv.h
	$(CCOMP) -c $(COPTS) $(DEFINE) -I../include -I$(EGADSINC) tess.c \
		-o $(ODIR)/dtess.o 

clean:
	-rm $(ODIR)/dtess.o $(TDIR)/dtess $(ODIR)/qtess.o $(TDIR)/qtess

lint:
	splint -usedef -realcompare +relaxtypes -compdef -nullassign \
		-retvalint -usereleased -mustfreeonly -branchstate -temptrans \
		-nullstate -compmempass -onlytrans -globstate -statictrans \
		-initsize -type -fixedformalarray -shiftnegative -compdestroy \
		-unqualifiedtrans -warnposix -predboolint -bufferoverflowhigh \
		-initallelements tess.c -I../include -I$(EGADSINC)
