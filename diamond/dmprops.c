/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Mass Properties Function -- EGADS
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "gem.h"
#include "egads.h"


int
gem_kernelMassProps(gemID handle, int etype, double *props)
{
  int i;

  for (i = 0; i < 14; i++) props[i] = 0.0;
  if ((etype < GEM_FACE) || (etype > GEM_BREP)) return GEM_BADTYPE;

  return EG_getMassProperties((ego) handle.ident.ptr, props);
}
