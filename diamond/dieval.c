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

#include "egads.h"
#include "gem.h"


int
gem_kernelInvEval(gemDRep *drep, gemPair bface, int npts, double *xyzs,
                  double *uvs)
{
  int      i, stat, oclass, mtype, nloop, *senses;
  double   results[3], limits[4];
  gemModel *mdl;
  gemBRep  *brep;
  ego      face, surf, *loops;

  mdl  = drep->model;
  brep = mdl->BReps[bface.BRep-1];
  face = (ego) brep->body->faces[bface.index-1].handle.ident.ptr;
  stat = EG_getTopology(face, &surf, &oclass, &mtype, limits, &nloop,
                        &loops, &senses);
  if (stat != EGADS_SUCCESS) return stat;

  for (i = 0; i < npts; i++) {
    stat = EG_invEvaluate(surf, &xyzs[3*i], &uvs[2*i], results);
    if (stat != EGADS_SUCCESS) return stat;
  }

  return GEM_SUCCESS;
}
