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
  int      i, j, k, ibrep, iface, stat;
  double   results[18];
  gemModel *mdl;
  gemBRep  *brep;
  ego      face;
  
  if (drep->bound[bound-1].VSet[vs-1].quilt == NULL) return GEM_NOTPARAMBND;
  mdl = drep->model;
  
  for (j = 0; j < drep->bound[bound-1].VSet[vs-1].quilt->nGpts; j++) {
    i = j;
    if (drep->bound[bound-1].VSet[vs-1].quilt->geomIndices != NULL)
      i = drep->bound[bound-1].VSet[vs-1].quilt->geomIndices[j] - 1;
    if (drep->bound[bound-1].VSet[vs-1].quilt->points[i].nFaces > 2) {
      k   = drep->bound[bound-1].VSet[vs-1].quilt->points[i].findices.multi[0]-1;
    } else {
      k   = drep->bound[bound-1].VSet[vs-1].quilt->points[i].findices.faces[0]-1;
    }
    ibrep = drep->bound[bound-1].VSet[vs-1].quilt->faceUVs[k].bface.BRep;
    iface = drep->bound[bound-1].VSet[vs-1].quilt->faceUVs[k].bface.index;
    brep  = mdl->BReps[ibrep-1];
    face  = (ego) brep->body->faces[iface-1].handle.ident.ptr;
    stat  = EG_evaluate(face, drep->bound[bound-1].VSet[vs-1].quilt->faceUVs[k].uv,
                        results);
    if (stat != EGADS_SUCCESS) return stat;
    for (k = 0; k < 6; k++) d1[6*j+k] = results[3+k];
    for (k = 0; k < 9; k++) d2[9*j+k] = results[9+k];
  }
  
  return GEM_SUCCESS;
}
