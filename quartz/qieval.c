/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Inverse Evaluation Function -- EGADS
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "capri.h"
#include "gem.h"


int
gem_kernelInvEval(gemDRep *drep, gemPair bface, int npts, double *xyzs,
                  double *uvs)
{
  int      i, vol, face, stat, *errs;
  double   results[3];
  gemModel *mdl;
  gemBRep  *brep;
  
  stat = gi_qBegin();
  if (stat != CAPRI_SUCCESS) return stat;

  mdl  = drep->model;
  brep = mdl->BReps[bface.BRep-1];
  vol  = brep->body->faces[bface.index-1].handle.index;
  face = brep->body->faces[bface.index-1].handle.ident.tag;
  for (i = 0; i < npts; i++) {
    uvs[2*i  ] = -1.e38;
    uvs[2*i+1] = -1.e38;
    stat = gi_qNearestOnFace(vol, face, &xyzs[3*i], &uvs[2*i], results);
    if (stat < CAPRI_SUCCESS)
      printf(" InvEval Error: %d gi_qNearestOnFace = %d!\n", i+1, stat);
  }
  
  stat = gi_qEnd(&errs);
  if (stat < CAPRI_SUCCESS) return stat;
  if (stat != npts) {
    gi_free(errs);
    return CAPRI_COMMERR;
  }
  stat = CAPRI_SUCCESS;
  for (i = 0; i < npts; i++)
    if (errs[i] != CAPRI_SUCCESS) {
      printf(" InvEval Error on %d: status = %d!\n", i+1, errs[i]);
      stat = errs[i];
    }
  gi_free(errs);

  return stat;
}
