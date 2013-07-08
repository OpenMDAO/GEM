/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Topology Attribute Function -- CAPRI
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "gem.h"
#include "capri.h"


int
gem_kernelBRepAttr(gemID handle, int etype, char *name, int atype, int alen,
                   /*@null@*/ int *integers, /*@null@*/ double *reals,
                   /*@null@*/ char *string)
{
  int vol, tag, type;

  type = etype;
  if ((etype <  GEM_NODE) || (etype >  GEM_BREP)) return GEM_BADTYPE;
  if ((etype == GEM_LOOP) || (etype == GEM_SHELL)) return CAPRI_UNSUPPORT;
  if  (etype == GEM_FACE) type = CAPRI_FACE;
  if  (etype == GEM_BREP) type = CAPRI_VOLUME;

  vol = handle.index;
  tag = handle.ident.tag;

  return gi_qSetEntAttribute(vol, type, tag, name, 0, atype, alen,
                             integers, reals, string);
}
