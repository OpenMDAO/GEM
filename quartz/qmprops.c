/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Mass Properties Function -- CAPRI
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "gem.h"
#include "capri.h"


int
gem_kernelMassProps(gemID handle, int etype, double *props)
{
  int    i, vol, stat, nattr;
  double data[15], box[6];

  for (i = 0; i < 14; i++) props[i] = 0.0;
  if ((etype < GEM_FACE) || (etype > GEM_BREP)) return GEM_BADTYPE;
  if (etype == GEM_SHELL) return CAPRI_UNSUPPORT;

  vol = handle.index;

  if (etype == GEM_FACE) {
    return gi_dEntityProp(vol, CAPRI_FACE, handle.ident.tag, box, &props[1], 
                          &nattr);
  } else {
    stat = gi_dEntityProp(vol, CAPRI_VOLUME, 0, box, data, &nattr);
    if (stat == CAPRI_SUCCESS)
      for (i = 0; i < 14; i++) props[i] = data[i];
    return stat;
  }
}
