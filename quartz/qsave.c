/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Save Function -- CAPRI
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


int
gem_kernelSave(gemModel *model, /*@null@*/ char *filename)
{
  if  (model == NULL) return GEM_NULLOBJ;
  /* make sure we were NOT initiated by staticModel */
  if ((model->handle.index == 0) && 
      (model->handle.ident.ptr == NULL)) return GEM_BADTYPE;

  return gi_uSaveModel(model->handle.index, filename);
}
