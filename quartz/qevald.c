/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Evaluate Derivatives Function -- CAPRI
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
gem_kernelEvalDs(gemDRep *drep, int bound, int vs, double *d1, double *d2)
{
  int      i, j, np, ibrep, iface, vol, face, stat, *errs;
  double   dum[3];
  gemModel *mdl;
  gemBRep  *brep;

  mdl  = drep->model;
  stat = gi_qBegin();
  if (stat != CAPRI_SUCCESS) return stat;

  for (np = i = 0; i < drep->bound[bound-1].VSet[vs-1].nFaces; i++) {
    ibrep = drep->bound[bound-1].VSet[vs-1].faces[i].index.BRep;
    iface = drep->bound[bound-1].VSet[vs-1].faces[i].index.index;
    brep  = mdl->BReps[ibrep-1];
    vol   = brep->body->faces[iface-1].handle.index;
    face  = brep->body->faces[iface-1].handle.ident.tag;
    for (j = 0; j < drep->bound[bound-1].VSet[vs-1].faces[i].npts; j++, np++) {
      stat = gi_qPointOnFace(vol, face, 
                             &drep->bound[bound-1].VSet[vs-1].faces[i].uv[2*j],
                             dum, 2, &d1[6*np], &d1[6*np+3], 
                             &d2[9*np], &d2[9*np+3], &d2[9*np+6]);
      if (stat < CAPRI_SUCCESS) 
        printf(" EvalDs Error: %d/%d gi_qPointOnFace = %d!\n", i+1, j+1, stat);
    }
  }
  
  stat = gi_qEnd(&errs);
  if (stat < CAPRI_SUCCESS) return stat;
  if (stat != np) {
    gi_free(errs);
    return CAPRI_COMMERR;
  }
  
  for (i = 0; i < np; i++)
    if (errs[i] != CAPRI_SUCCESS)
      printf(" EvalDs Error on %d: %d\n", i+1, errs[i]);
  gi_free(errs);
  
  return GEM_SUCCESS;
}
