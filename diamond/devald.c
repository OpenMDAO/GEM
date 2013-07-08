/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Evaluate Derivatives Function -- EGADS
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
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
  int      b, j, k, m, stat;
  double   results[18];
  gemPair  bface;
  gemModel *mdl;
  gemBRep  *brep;
  ego      face;
  
  mdl = drep->model;
  b   = bound - 1;
  if (drep->bound[b].VSet[vs-1].quilt == NULL) return GEM_NOTPARAMBND;
  
  for (j = 0; j < drep->bound[b].VSet[vs-1].quilt->nPoints; j++) {
    if (drep->bound[b].VSet[vs-1].quilt->points[j].nFaces > 2) {
      k   = drep->bound[b].VSet[vs-1].quilt->points[j].findices.multi[0]-1;
    } else {
      k   = drep->bound[b].VSet[vs-1].quilt->points[j].findices.faces[0]-1;
    }
    m     = drep->bound[b].VSet[vs-1].quilt->faceUVs[k].owner-1;
    bface = drep->bound[b].VSet[vs-1].quilt->bfaces[m];
    brep  = mdl->BReps[bface.BRep-1];
    face  = (ego) brep->body->faces[bface.index-1].handle.ident.ptr;
    stat  = EG_evaluate(face, drep->bound[b].VSet[vs-1].quilt->faceUVs[k].uv,
                        results);
    if (stat != EGADS_SUCCESS) return stat;
    for (k = 0; k < 6; k++) d1[6*j+k] = results[3+k];
    for (k = 0; k < 9; k++) d2[9*j+k] = results[9+k];
  }
  
  return GEM_SUCCESS;
}
