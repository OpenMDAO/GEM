/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Evaluation Function -- EGADS
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
gem_kernelEval(gemDRep *drep, gemPair bface, int npts, double *uvs,
               double *xyzs)
{
  int      i, stat;
  double   results[18];
  gemModel *mdl;
  gemBRep  *brep;
  ego      face;

  mdl  = drep->model;
  brep = mdl->BReps[bface.BRep-1];
  face = (ego) brep->body->faces[bface.index-1].handle.ident.ptr;
  for (i = 0; i < npts; i++) {
    stat = EG_evaluate(face, &uvs[2*i], results);
    if (stat != EGADS_SUCCESS) return stat;
    xyzs[3*i  ] = results[0];
    xyzs[3*i+1] = results[1];
    xyzs[3*i+2] = results[2];
  }

  return GEM_SUCCESS;
}
