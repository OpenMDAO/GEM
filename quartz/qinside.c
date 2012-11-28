/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Inside Predicate Function -- CAPRI
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "capri.h"
#include "gem.h"


int
gem_kernelInside(gemDRep *drep, int bound, int vs, double *in)
{

  int      i, j, k, ibrep, iface, vol, face, stat, *errs;
  gemModel *mdl;
  gemBRep  *brep;

  if (drep->bound[bound-1].VSet[vs-1].quilt == NULL) return GEM_NOTPARAMBND;
  mdl  = drep->model;
  
  

  stat = gi_qBegin();
  if (stat != CAPRI_SUCCESS) return stat;

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
      vol   = brep->body->faces[iface-1].handle.index;
      face  = brep->body->faces[iface-1].handle.ident.tag;
      stat  = gi_qInFaceUV(vol, face, 
                           drep->bound[bound-1].VSet[vs-1].quilt->faceUVs[k].uv);
      if (stat < CAPRI_SUCCESS) 
        printf(" Inside Error: %d/%d gi_qPointOnFace = %d!\n", i+1, j+1, stat);
    }

  } else {
    
    /* multiple faces -- find appropriate one */

  }
  
  stat = gi_qEnd(&errs);
  if (stat < CAPRI_SUCCESS) return stat;
  if (stat != drep->bound[bound-1].VSet[vs-1].quilt->nGpts) return CAPRI_COMMERR;
  
  for (i = 0; i < drep->bound[bound-1].VSet[vs-1].quilt->nGpts; i++) {
    in[i] = 0.0;
    if (errs[i] == CAPRI_OUTSIDE) {
      in[i] = 1.0;
    } else if (errs[i] != CAPRI_SUCCESS) {
      printf(" Inside Error on %d: %d\n", i+1, errs[i]);
    }
  }
  gi_free(errs);

  return GEM_SUCCESS;
}
