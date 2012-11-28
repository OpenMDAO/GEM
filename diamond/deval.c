/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Evaluation Function -- EGADS
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
#include "memory.h"


int
gem_kernelEval(gemDRep *drep, int bound, int vs, int gflg, int inv,
               double **eval)
{
  int      i, j, k, np, ibrep, iface, stat;
  double   results[18], *vals;
  gemModel *mdl;
  gemBRep  *brep;
  ego      face;
  
  if (drep->bound[bound-1].VSet[vs-1].quilt == NULL) return GEM_NOTPARAMBND;
  mdl = drep->model;
  if (gflg == 1) {
    np = drep->bound[bound-1].VSet[vs-1].quilt->nGpts;
  } else {
    np = drep->bound[bound-1].VSet[vs-1].quilt->nVerts;
  }
  if (inv == 0) {
    vals = (double *) gem_allocate(3*np*sizeof(double));
  } else {
    vals = (double *) gem_allocate(2*np*sizeof(double));
  }
  if (vals == NULL) return GEM_ALLOC;
  
  /* do we have multiple faces in the Vset? */
  for (k = j = 0; j < np; j++) {
    i = j;
    if (gflg == 1) {
      if (drep->bound[bound-1].VSet[vs-1].quilt->geomIndices != NULL)
        i = drep->bound[bound-1].VSet[vs-1].quilt->geomIndices[j] - 1;
    } else {
      if (drep->bound[bound-1].VSet[vs-1].quilt->dataIndices != NULL)
        i = drep->bound[bound-1].VSet[vs-1].quilt->dataIndices[j] - 1;
    }
    if (drep->bound[bound-1].VSet[vs-1].quilt->points[i].nFaces != 1) {
      k++;
      break;
    }
  }
  
  if ((k == 0) || (inv == 0)) {

    /* a single face/evaluation -- use the uvs stored */
    for (j = 0; j < np; j++) {
      i = j;
      if (gflg == 1) {
        if (drep->bound[bound-1].VSet[vs-1].quilt->geomIndices != NULL)
          i = drep->bound[bound-1].VSet[vs-1].quilt->geomIndices[j] - 1;
      } else {
        if (drep->bound[bound-1].VSet[vs-1].quilt->dataIndices != NULL)
          i = drep->bound[bound-1].VSet[vs-1].quilt->dataIndices[j] - 1;
      }
      if (drep->bound[bound-1].VSet[vs-1].quilt->points[i].nFaces > 2) {
        k   = drep->bound[bound-1].VSet[vs-1].quilt->points[i].findices.multi[0]-1;
      } else {
        k   = drep->bound[bound-1].VSet[vs-1].quilt->points[i].findices.faces[0]-1;
      }
      ibrep = drep->bound[bound-1].VSet[vs-1].quilt->faceUVs[k].bface.BRep;
      iface = drep->bound[bound-1].VSet[vs-1].quilt->faceUVs[k].bface.index;
      brep  = mdl->BReps[ibrep-1];
      face  = (ego) brep->body->faces[iface-1].handle.ident.ptr;
      if (inv == 0) {
        stat = EG_evaluate(face,
                           drep->bound[bound-1].VSet[vs-1].quilt->faceUVs[k].uv,
                           results);
        if (stat != EGADS_SUCCESS) {
          printf(" Eval Warning: EG_evaluate = %d\n", stat);
          vals[3*j  ] = 0.0;
          vals[3*j+1] = 0.0;
          vals[3*j+2] = 0.0;
        } else {
          vals[3*j  ] = results[0];
          vals[3*j+1] = results[1];
          vals[3*j+2] = results[2];
        }
      } else {
        vals[2*j  ] = 0.0;
        vals[2*j+1] = 0.0;
        stat = EG_invEvaluate(face, 
                              drep->bound[bound-1].VSet[vs-1].quilt->points[i].xyz,
                              &vals[2*j], results);
        if (stat != EGADS_SUCCESS)
          printf(" Eval Warning: EG_invEvaluate = %d\n", stat);
      }
    }
    *eval = vals;

  } else {
    
    /* multiple faces in the Vset -- must find face for invEval! */
    
    
    *eval = vals;
  }
  return GEM_SUCCESS;
}
