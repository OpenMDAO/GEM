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
gem_kernelEval(gemDRep *drep, gemPair bface, int npts, double *uvs,
               double *xyzs)
{
  int      i, vol, face, stat, *errs;
  gemModel *mdl;
  gemBRep  *brep;
  
  stat = gi_qBegin();
  if (stat != CAPRI_SUCCESS) return stat;

  mdl  = drep->model;
  brep = mdl->BReps[bface.BRep-1];
  vol  = brep->body->faces[bface.index-1].handle.index;
  face = brep->body->faces[bface.index-1].handle.ident.tag;
  for (i = 0; i < npts; i++) {
    stat = gi_qPointOnFace(vol, face, &uvs[2*i], &xyzs[3*i], 0, NULL, NULL,
                           NULL, NULL, NULL);
    if (stat < CAPRI_SUCCESS)
      printf(" InvEval Error: %d gi_qPointOnFace = %d!\n", i+1, stat);
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
