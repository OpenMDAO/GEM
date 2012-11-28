/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Topology Attribute Function -- EGADS
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "gem.h"
#include "egads.h"


int
gem_kernelBRepAttr(gemID handle, int etype, char *name, int atype, int alen,
                   /*@null@*/ int *integers, /*@null@*/ double *reals,
                   /*@null@*/ char *string)
{
  if ((etype < GEM_NODE) || (etype > GEM_BREP)) return GEM_BADTYPE;

  return EG_attributeAdd((ego) handle.ident.ptr, name, atype, alen, integers,
                         reals, string);
}
