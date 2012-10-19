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
gem_kernelEval(gemDRep *drep, int bound, int vs, int i, int inv)
{
  int      j, np, ibrep, iface, stat;
  double   results[18];
  gemModel *mdl;
  gemBRep  *brep;
  ego      face;
  
  mdl = drep->model;
  np  = drep->bound[bound-1].VSet[vs-1].faces[i].npts;
  if (inv == 0) {
    drep->bound[bound-1].VSet[vs-1].faces[i].xyz = (double *)
                           gem_allocate(3*np*sizeof(double));
    if (drep->bound[bound-1].VSet[vs-1].faces[i].xyz == NULL) return GEM_ALLOC;
  } else {
    drep->bound[bound-1].VSet[vs-1].faces[i].uv = (double *)
                          gem_allocate(2*np*sizeof(double));
    if (drep->bound[bound-1].VSet[vs-1].faces[i].uv == NULL) return GEM_ALLOC;
  }
  
  ibrep = drep->bound[bound-1].VSet[vs-1].faces[i].index.BRep;
  iface = drep->bound[bound-1].VSet[vs-1].faces[i].index.index;
  brep  = mdl->BReps[ibrep-1];
  face  = (ego) brep->body->faces[iface-1].handle.ident.ptr;
  for (j = 0; j < drep->bound[bound-1].VSet[vs-1].faces[i].npts; j++, np++)
    if (inv == 0) {
      stat = EG_evaluate(face, &drep->bound[bound-1].VSet[vs-1].faces[i].uv[2*j],
                         results);
      if (stat != EGADS_SUCCESS) {
        printf(" Eval Warning: EG_evaluate = %d\n", stat);
        drep->bound[bound-1].VSet[vs-1].faces[i].xyz[3*j  ] = 0.0;
        drep->bound[bound-1].VSet[vs-1].faces[i].xyz[3*j+1] = 0.0;
        drep->bound[bound-1].VSet[vs-1].faces[i].xyz[3*j+2] = 0.0;
      } else {
        drep->bound[bound-1].VSet[vs-1].faces[i].xyz[3*j  ] = results[0];
        drep->bound[bound-1].VSet[vs-1].faces[i].xyz[3*j+1] = results[1];
        drep->bound[bound-1].VSet[vs-1].faces[i].xyz[3*j+2] = results[2];
      }
    } else {
      drep->bound[bound-1].VSet[vs-1].faces[i].uv[2*j  ] = 0.0;
      drep->bound[bound-1].VSet[vs-1].faces[i].uv[2*j+1] = 0.0;
      stat = EG_invEvaluate(face, 
                            &drep->bound[bound-1].VSet[vs-1].faces[i].xyz[3*j],
                            &drep->bound[bound-1].VSet[vs-1].faces[i].uv[2*j],
                            results);
      if (stat != EGADS_SUCCESS)
        printf(" Eval Warning: EG_invEvaluate = %d\n", stat);
    }
  
  return GEM_SUCCESS;
}
