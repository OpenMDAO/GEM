/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Release Function -- CAPRI
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


  extern int *CAPRI_MdlRefs;


int
gem_kernelRelease(gemModel *mdl)
{
  int i, model, stat;

  model = mdl->handle.index;
  
  /* static model */
  if (model == 0) {
    for (i = 0; i < mdl->nBRep; i++) {
      model = mdl->BReps[i]->phandle.index;
      if (CAPRI_MdlRefs != NULL) {
        CAPRI_MdlRefs[model-1]--;
        if (CAPRI_MdlRefs[model-1] > 0) continue;
      }
      stat = gi_uRelModel(model);
      if (stat != CAPRI_SUCCESS) 
        printf(" GEM Quartz Internal: RelModel = %d\n", stat);   
    }
    return GEM_SUCCESS;
  }
  
  /* normal CAPRI model */
  if (CAPRI_MdlRefs != NULL) {
    CAPRI_MdlRefs[model-1]--;
    if (CAPRI_MdlRefs[model-1] > 0) return CAPRI_SUCCESS;
  }
  return gi_uRelModel(model);
}
