/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Evaluate Function -- CAPRI
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
#include "memory.h"


int
gem_kernelEval(gemDRep *drep, int bound, int vs, int i, int inv)
{
  int      j, np, ibrep, iface, vol, face, stat, *errs;
  double   dum[3], *data;
  gemModel *mdl;
  gemBRep  *brep;

  mdl = drep->model;
  np  = drep->bound[bound-1].VSet[vs-1].faces[i].npts;
  if (inv == 0) {
    data = (double *) gem_allocate(3*np*sizeof(double));
    if (data == NULL) return GEM_ALLOC;
  } else {
    data = (double *) gem_allocate(2*np*sizeof(double));
    if (data == NULL) return GEM_ALLOC;
  }

  stat = gi_qBegin();
  if (stat != CAPRI_SUCCESS) {
    gem_free(data);
    return stat;
  }

  ibrep = drep->bound[bound-1].VSet[vs-1].faces[i].index.BRep;
  iface = drep->bound[bound-1].VSet[vs-1].faces[i].index.index;
  brep  = mdl->BReps[ibrep-1];
  vol   = brep->body->faces[iface-1].handle.index;
  face  = brep->body->faces[iface-1].handle.ident.tag;
  for (j = 0; j < drep->bound[bound-1].VSet[vs-1].faces[i].npts; j++)
    if (inv == 0) {
      data[3*j  ] = 0.0;
      data[3*j+1] = 0.0;
      data[3*j+2] = 0.0;
      stat = gi_qPointOnFace(vol, face, 
                             &drep->bound[bound-1].VSet[vs-1].faces[i].uv[2*j],
                             &data[3*j], 0, NULL, NULL, NULL, NULL, NULL);
      if (stat < CAPRI_SUCCESS) 
        printf(" Evals Error: %d gi_qPointOnFace = %d!\n", j+1, stat);
    } else {
      data[2*j  ] = -1.e38;
      data[2*j+1] = -1.e38;
      stat = gi_qNearestOnFace(vol, face,
                               &drep->bound[bound-1].VSet[vs-1].faces[i].xyz[3*j],
                               &data[2*j], dum);
      if (stat < CAPRI_SUCCESS) 
        printf(" Evals Error: %d gi_qNearestOnFace = %d!\n", j+1, stat);
    }
  
  stat = gi_qEnd(&errs);
  if (stat < CAPRI_SUCCESS) {
    gem_free(data);
    return stat;
  }
  if (stat != drep->bound[bound-1].VSet[vs-1].faces[i].npts) {
    gi_free(errs);
    gem_free(data);
    return CAPRI_COMMERR;
  }
  
  for (j = 0; j < stat; j++)
    if (errs[j] != CAPRI_SUCCESS)
      printf(" Evals Warning on %d: %d\n", j+1, errs[j]);
  gi_free(errs);

  if (inv == 0) {
    drep->bound[bound-1].VSet[vs-1].faces[i].xyz = data;
  } else {
    drep->bound[bound-1].VSet[vs-1].faces[i].uv  = data;
  }
  return GEM_SUCCESS;
}
