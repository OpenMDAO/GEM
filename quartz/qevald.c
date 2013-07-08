/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Evaluate Derivatives Function -- CAPRI
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
gem_kernelEvalDs(gemDRep *drep, int bound, int vs, double *d1, double *d2)
{
  int      b, i, j, k, m, vol, face, stat, *errs;
  double   dum[3];
  gemPair  bface;
  gemModel *mdl;
  gemBRep  *brep;

  b = bound - 1;
  if (drep->bound[b].VSet[vs-1].quilt == NULL) return GEM_NOTPARAMBND;
  mdl  = drep->model;
  stat = gi_qBegin();
  if (stat != CAPRI_SUCCESS) return stat;

  for (j = 0; j < drep->bound[b].VSet[vs-1].quilt->nPoints; j++) {
    if (drep->bound[b].VSet[vs-1].quilt->points[j].nFaces > 2) {
      k   = drep->bound[b].VSet[vs-1].quilt->points[j].findices.multi[0]-1;
    } else {
      k   = drep->bound[b].VSet[vs-1].quilt->points[j].findices.faces[0]-1;
    }
    m     = drep->bound[b].VSet[vs-1].quilt->faceUVs[k].owner-1;
    bface = drep->bound[b].VSet[vs-1].quilt->bfaces[m];
    brep  = mdl->BReps[bface.BRep-1];
    vol   = brep->body->faces[bface.index-1].handle.index;
    face  = brep->body->faces[bface.index-1].handle.ident.tag;
    stat  = gi_qPointOnFace(vol, face, 
                            drep->bound[b].VSet[vs-1].quilt->faceUVs[k].uv,
                            dum, 2, &d1[6*j], &d1[6*j+3],
                            &d2[9*j], &d2[9*j+3], &d2[9*j+6]);
    if (stat < CAPRI_SUCCESS) 
      printf(" EvalDs Error: %d/%d gi_qPointOnFace = %d!\n", i+1, j+1, stat);
  }
  
  stat = gi_qEnd(&errs);
  if (stat < CAPRI_SUCCESS) return stat;
  if (stat != drep->bound[b].VSet[vs-1].quilt->nPoints) {
    gi_free(errs);
    return CAPRI_COMMERR;
  }
  
  for (i = 0; i < drep->bound[b].VSet[vs-1].quilt->nPoints; i++)
    if (errs[i] != CAPRI_SUCCESS)
      printf(" EvalDs Error on %d: %d\n", i+1, errs[i]);
  gi_free(errs);
  
  return GEM_SUCCESS;
}
