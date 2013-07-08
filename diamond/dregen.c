/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Regenerate Function -- OpenCSM & EGADS
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
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

#include "gem.h"
#include "memory.h"
#include "egads.h"

/* OpenCSM Defines & Includes */
#include "common.h"
#include "OpenCSM.h"

#define MAX_BODYS     999
  static int  nbody =   0;		/* number of built Bodys */
  static int  bodyList[MAX_BODYS];	/* array of built Bodys */

  extern ego  dia_context;

  extern void gem_releaseBRep(/*@only@*/ gemBRep *brep);
  extern int  gem_clrDReps(gemModel *model, int phase);
  extern int  gem_diamondBody(ego object, /*@null@*/ char *bID, 
                              gemBRep *brep);


int
gem_kernelRegen(gemModel *model)
{
  int    i, j, k, n, nrow, ncol;
  int    stat, actv, type, buildTo, builtTo, nBRep, ibody;
  char   defn[22], name[129];
  void   *modl;
  double real;
  ego    obj;
  modl_T *MODL;
  gemID  handle;
  
  modl = model->handle.ident.ptr;
  MODL = (modl_T *) modl;
  MODL->context = dia_context;
  for (i = 0; i < model->nParams; i++) {
    if (model->Params[i].changed == 0) continue;
    if (model->Params[i].type != GEM_REAL) return GEM_BADTYPE;
  }
  for (i = 0; i < model->nParams; i++) {
    if (model->Params[i].changed == 0) continue;
    stat = ocsmGetPmtr(modl, i+1, &type, &nrow, &ncol, name);
    if (stat != SUCCESS) return stat;
    if ((nrow == 1) && (ncol == 1)) {
      snprintf(defn, 21, "%20.13le", model->Params[i].vals.real);
      defn[21] = 0;
      stat = ocsmSetValu(modl,i+1, 1, 1, defn);
      if (stat != SUCCESS)
        printf(" GEM/OpenCSM status = %d on changing %s to %s!\n",
               stat, model->Params[i].name, defn);
    } else {
      for (n = j = 0; j < ncol; j++)
        for (k = 0; k < nrow; k++, n++) {
          snprintf(defn, 21, "%20.13le", model->Params[i].vals.reals[n]);
          defn[21] = 0;
          stat = ocsmSetValu(modl,i+1, k+1, j+1, defn);
          if (stat != SUCCESS)
            printf(" GEM/OpenCSM status = %d on changing %s[%d,%d] to %s!\n",
                   stat, model->Params[i].name, k+1, j+1, defn);
        }
    }
    model->Params[i].changed = 0;
  }

  for (i = 0; i < model->nBranches; i++) {
    if  (model->Branches[i].changed == 0) continue;  
    actv = OCSM_ACTIVE;
    if (model->Branches[i].sflag == GEM_SUPPRESSED) actv = OCSM_SUPPRESSED;
    stat = ocsmSetBrch(modl, i, actv);
    if (stat != SUCCESS)
      printf(" GEM/OpenCSM status = %d on changing %s to %d!\n",
             stat, model->Branches[i].name, actv);
    model->Branches[i].changed = 0;
  }
  
  /* remove old geometry, rebuild */
  
  for (i = 0; i < model->nBRep; i++) {
    obj = (ego) model->BReps[i]->body->handle.ident.ptr;
    EG_deleteObject(obj);
    gem_releaseBRep(model->BReps[i]);
  }
  gem_free(model->BReps);
  model->BReps = NULL;
  model->nBRep = 0;
  gem_clrDReps(model, 0);
  
  buildTo = 0;                          /* all */
  nbody   = MAX_BODYS;
  stat    = ocsmBuild(modl, buildTo, &builtTo, &nbody, bodyList);
  EG_deleteObject(dia_context);         /* clean up after build */
  if (stat != SUCCESS) return stat;
  nBRep = nbody;

  /* put away the new BReps */
  model->BReps = (gemBRep **) gem_allocate(nBRep*sizeof(gemBRep *));
  if (model->BReps == NULL) {
    for (i = 0; i < nbody; i++) {
      ibody = bodyList[i];
      obj   = MODL->body[ibody].ebody;
      EG_deleteObject(obj);
    }
    return GEM_ALLOC;
  }
  handle.index     = 0;
  handle.ident.ptr = modl;
  for (i = 0; i < nBRep; i++) {
    model->BReps[i] = (gemBRep *) gem_allocate(sizeof(gemBRep));
    if (model->BReps[i] == NULL) {
      for (j = 0; j < i; j++) gem_free(model->BReps[j]);
      gem_free(model->BReps);
      model->BReps = NULL;
      for (j = 0; j < nbody; j++) {
        ibody = bodyList[j];
        obj   = MODL->body[ibody].ebody;
        EG_deleteObject(obj);
      }
      return GEM_ALLOC;
    }
    model->BReps[i]->magic   = GEM_MBREP;
    model->BReps[i]->omodel  = NULL;
    model->BReps[i]->phandle = handle;
    model->BReps[i]->ibranch = 0;
    model->BReps[i]->inumber = 0;
    model->BReps[i]->body    = NULL;
  }
  
  /* reload the BReps */
  for (j = i = 0; i < nbody; i++) {
    ibody = bodyList[i];
    obj   = MODL->body[ibody].ebody;
    stat  = gem_diamondBody(obj, NULL, model->BReps[j]);
    if (stat != EGADS_SUCCESS) {
      for (j = 0; j < nBRep; j++) gem_releaseBRep(model->BReps[j]);
      gem_free(model->BReps);
      model->BReps = NULL;
      for (j = 0; j < nbody; j++) {
        ibody = bodyList[j];
        obj   = MODL->body[ibody].ebody;
        EG_deleteObject(obj);
      }
      return stat;
    }
  }
  for (i = 0; i < nBRep; i++) model->BReps[i]->omodel = model;
  model->nBRep = nBRep;
  gem_clrDReps(model, 1);
  
  /* refresh the parameters */
  for (i = 0; i < model->nParams; i++) {
    stat = ocsmGetPmtr(modl, i+1, &type, &nrow, &ncol, name);
    if (stat != SUCCESS) continue;
    if ((nrow == 1) && (ncol == 1)) {
      stat = ocsmGetValu(modl, i+1, 1, 1, &real);
      if (stat != SUCCESS) continue;
      model->Params[i].vals.real = real;
    } else {
      for (n = j = 0; j < ncol; j++)
        for (k = 0; k < nrow; k++, n++) {
          stat = ocsmGetValu(modl, i+1, k+1, j+1, &real);
          if (stat != SUCCESS) continue;
          model->Params[i].vals.reals[n] = real;
        }
    }
  }

  return GEM_SUCCESS;
}
