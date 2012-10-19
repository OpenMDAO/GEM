/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Evaluate Derivatives Function -- EGADS
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "egads.h"
#include "gem.h"


int
gem_kernelEvalDs(gemDRep *drep, int bound, int vs, double *d1, double *d2)
{
  int      i, j, k, np, ibrep, iface, stat;
  double   results[18];
  gemModel *mdl;
  gemBRep  *brep;
  ego      face;
  
  mdl = drep->model;
  
  for (np = i = 0; i < drep->bound[bound-1].VSet[vs-1].nFaces; i++) {
    ibrep = drep->bound[bound-1].VSet[vs-1].faces[i].index.BRep;
    iface = drep->bound[bound-1].VSet[vs-1].faces[i].index.index;
    brep  = mdl->BReps[ibrep-1];
    face  = (ego) brep->body->faces[iface-1].handle.ident.ptr;
    for (j = 0; j < drep->bound[bound-1].VSet[vs-1].faces[i].npts; j++, np++) {
      stat = EG_evaluate(face, &drep->bound[bound-1].VSet[vs-1].faces[i].uv[2*j],
                         results);
      if (stat != EGADS_SUCCESS) return stat;
      for (k = 0; k < 6; k++) d1[6*np+k] = results[3+k];
      for (k = 0; k < 9; k++) d2[6*np+k] = results[9+k];
    }
  }
  
  return GEM_SUCCESS;
}
