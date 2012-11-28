/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Inside Predicate Function -- EGADS
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
gem_kernelInside(gemDRep *drep, int bound, int vs, double *in)
{
  int      i, j, k, ibrep, iface, stat;
  gemModel *mdl;
  gemBRep  *brep;
  ego      face;
  
  if (drep->bound[bound-1].VSet[vs-1].quilt == NULL) return GEM_NOTPARAMBND;
  mdl = drep->model;
  
  /* do we have multiple faces in the Vset? */
  for (k = j = 0; j < drep->bound[bound-1].VSet[vs-1].quilt->nGpts; j++) {
    i = j;
    if (drep->bound[bound-1].VSet[vs-1].quilt->geomIndices != NULL)
      i = drep->bound[bound-1].VSet[vs-1].quilt->geomIndices[j] - 1;
    if (drep->bound[bound-1].VSet[vs-1].quilt->points[i].nFaces != 1) {
      k++;
      break;
    }
  }
  
  if (k == 0) {
    
    /* a single face */
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
      stat  = EG_inFace(face, drep->bound[bound-1].VSet[vs-1].quilt->faceUVs[k].uv);
      if (stat < EGADS_SUCCESS) return stat;
      in[j] = stat;
    }
    
  } else {

    /* multiple faces -- find appropriate one */
    
  }

  return GEM_SUCCESS;
}
