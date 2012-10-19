/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel SBO Function -- EGADS
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WIN32
#define snprintf _snprintf
#endif

#include "egads.h"
#include "gem.h"
#include "memory.h"


  extern int gem_fillModelE( ego object, char *name, int ext, 
                             int *nBRep, gemBRep ***BReps );


int
gem_kernelSBO(gemID src, gemID tool, /*@null@*/ double *xform, int type,
              gemModel **model)
{
  int      i, stat, nBRep;
  char     name[25];
  ego      context, obj1, obj2, omdl, oform, copy;
  gemID    gid;
  gemModel *mdl, *prev;
  gemBRep  **BReps;
  gemCntxt *cntxt;
  static int   nBool    = 0;
  static char *nType[3] = {"Subtract", "Intersection", "Union"};

  /* find the context */
  mdl    = *model;
  *model = NULL;
  cntxt  = NULL;
  prev   = mdl->prev;
  while (cntxt == NULL) {
    if  (prev  == NULL) return GEM_BADCONTEXT;
    if ((prev->magic != GEM_MMODEL) && 
        (prev->magic != GEM_MCONTEXT)) return GEM_BADOBJECT;
    if  (prev->magic == GEM_MCONTEXT)  cntxt = (gemCntxt *) prev;
    if  (prev->magic == GEM_MMODEL)    prev  = prev->prev;
  }

  /* do the operation */
  obj1 = (ego) src.ident.ptr;
  obj2 = (ego) tool.ident.ptr;
  if (xform != NULL) {
    stat = EG_getContext(obj2, &context);
    if (stat != EGADS_SUCCESS) return stat;
    stat = EG_makeTransform(context, xform, &oform);
    if (stat != EGADS_SUCCESS) return stat;
    stat = EG_copyObject(obj2, oform, &copy);
    EG_deleteObject(oform);
    if (stat != EGADS_SUCCESS) return stat;
    stat = EG_solidBoolean(obj1, copy, type, &omdl);
    EG_deleteObject(copy);
  } else {
    stat = EG_solidBoolean(obj1, obj2, type, &omdl);
  }
  if (stat != EGADS_SUCCESS) return stat;

  /* fill up the BReps */
  nBool++;
  snprintf(name, 25, "%s-%d", nType[type-1], nBool);
  i    = strlen(name);
  stat = gem_fillModelE(omdl, name, i, &nBRep, &BReps);
  if (stat != EGADS_SUCCESS) return stat;

  mdl = (gemModel *) gem_allocate(sizeof(gemModel));
  if (mdl == NULL) return GEM_ALLOC;

  gid.index      = 0;
  gid.ident.ptr  = omdl;

  mdl->magic     = GEM_MMODEL;
  mdl->handle    = gid;
  mdl->nonparam  = 1;
  mdl->server    = NULL;
  mdl->location  = NULL;
  mdl->modeler   = gem_strdup("EGADS");
  mdl->nBRep     = nBRep;
  mdl->BReps     = BReps;
  mdl->nParams   = 0;
  mdl->Params    = NULL;
  mdl->nBranches = 0;
  mdl->Branches  = NULL;
  mdl->attr      = NULL;
  mdl->prev      = (gemModel *) cntxt;
  mdl->next      = NULL;
    
  for (i = 0; i < nBRep; i++) BReps[i]->omodel = mdl;

  /* insert into context */
  prev = cntxt->model;
  cntxt->model = mdl;
  if (prev != NULL) {
    mdl->next  = prev;
    prev->prev = cntxt->model;
  }
  
  *model = mdl;
  return EGADS_SUCCESS;
}
