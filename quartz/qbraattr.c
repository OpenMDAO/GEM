/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Branch Attribute Function -- CAPRI
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "capri.h"
#include "gem.h"


int
gem_kernelBranchAttr(gemID handle, int branch, char *name, int atype, int alen, 
                     /*@null@*/ int *ints, /*@null@*/ double *reals,
                     /*@null@*/ char *string)
{
  return gi_qSetEntAttribute(handle.index, CAPRI_BRANCH, branch, name, 0, 
                             atype, alen, ints, reals, string);
}
