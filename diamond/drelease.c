/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Release Model Function -- OpenCSM & EGADS
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

/* OpenCSM Includes */
#include "common.h"
#include "OpenCSM.h"


int
gem_kernelRelease(gemModel *model)
{
  int     i, stat;
  ego     obj;
  gemBRep *brep;
  gemBody *body;

  if (model->handle.index == 0) {

    /* EGADS model */
    obj = (ego) model->handle.ident.ptr;
    if (obj != NULL) return EG_deleteObject(obj);

    /* static Model */
    for (i = 0; i < model->nBRep; i++) {
      brep = model->BReps[i];
      body = brep->body;
      obj  = (ego) body->handle.ident.ptr;
      stat = EG_deleteObject(obj);
      if (stat != EGADS_SUCCESS)
        printf(" Diamond Internal: Delete %d/%d body = %d!\n",
               i+1, model->nBRep, stat);
    }
    return GEM_SUCCESS;
  }

  /* parametric OpenCSM Model */
  for (i = 0; i < model->nBRep; i++) {
    brep = model->BReps[i];
    body = brep->body;
    obj  = (ego) body->handle.ident.ptr;
    stat = EG_deleteObject(obj);
    if (stat != EGADS_SUCCESS)
      printf(" Diamond Internal: delete %d/%d body = %d!\n",
             i+1, model->nBRep, stat);
  }
  return ocsmFree(model->handle.ident.ptr);
}
