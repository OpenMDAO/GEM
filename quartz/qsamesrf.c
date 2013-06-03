/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Query if Same Surface -- CAPRI
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
#include "memory.h"


int
gem_kernelSameSurfs(gemModel *model, int nFaces, gemPair *bfaces)
{
  int     i, j, len, vol, face, stat, *ivec, *errs;
  double  **rvec;
  gemBRep *brep;

  ivec = (int *) gem_allocate(7*nFaces*sizeof(int));
  if (ivec == NULL) return GEM_ALLOC;
  rvec = (double **) gem_allocate(nFaces*sizeof(double *));
  if (rvec == NULL) {
    gem_free(ivec);
    return GEM_ALLOC;
  }

  stat = gi_qBegin();
  if (stat != CAPRI_SUCCESS) {
    gem_free(rvec);
    gem_free(ivec);
    return stat;
  }
  for (i = 0; i < nFaces; i++) {
    brep = model->BReps[bfaces[i].BRep-1];
    vol  = brep->body->faces[bfaces[i].index-1].handle.index;
    face = brep->body->faces[bfaces[i].index-1].handle.ident.tag;
    stat = gi_qSurface2NURBS(vol, face, &ivec[7*i], &rvec[i]);
    if (stat < CAPRI_SUCCESS) {
      printf(" SameSurf Error: %d gi_qSurface2NURBS = %d\n", i+1, stat);
      rvec[i] = NULL;
    }
  }
  stat = gi_qEnd(&errs);
  if (stat < CAPRI_SUCCESS) {
    for (i = 0; i < nFaces; i++) gi_free(rvec[i]);
    gem_free(rvec);
    gem_free(ivec);
    return stat;
  }
  if (stat != nFaces) {
    gi_free(errs);
    for (i = 0; i < nFaces; i++) gi_free(rvec[i]);
    gem_free(rvec);
    gem_free(ivec);
    return CAPRI_COMMERR;
  }

  /* this may not work well -- most converters take the trim into account */

  stat = GEM_SUCCESS;
  len  = ivec[3] + ivec[6]  + 3*ivec[2]*ivec[5];
  if ((ivec[0]&2) != 0) len += ivec[2]*ivec[5];

  for (i = 1; i < nFaces; i++) {
    j = ivec[7*i+3] + ivec[7*i+6] + 3*ivec[7*i+2]*ivec[7*i+5];
    if ((ivec[7*i  ]&2) != 0)    j += ivec[7*i+2]*ivec[7*i+5];
    if (j != len) {
      stat = CAPRI_OUTSIDE;
      break;
    }
    for (j = 0; j < len; j++)
      if (rvec[0][j] != rvec[i][j]) {
        stat = CAPRI_OUTSIDE;
        break;
      }
    if (stat == CAPRI_OUTSIDE) break;
  }

  for (i = 0; i < nFaces; i++) gi_free(rvec[i]);
  gem_free(rvec);
  gem_free(ivec);
  return stat;
}
