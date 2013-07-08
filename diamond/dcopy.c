/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Copy Function -- EGADS & OpenCSM
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

#include "egads.h"
#include "gem.h"
#include "memory.h"

/* OpenCSM Includes */
#include "common.h"
#include "OpenCSM.h"


  extern ego  dia_context;

  extern int  gem_diamondBody(ego obj, /*@null@*/ char *bID, gemBRep *brep);
  extern void gem_releaseBRep(/*@only@*/ gemBRep *brep);


int
gem_kernelDelete(gemID handle)
{
  ego object;
  
  object = (ego) handle.ident.ptr;
  return EG_deleteObject(object);
}


int
gem_kernelCopyMM(gemModel *model)
{
  int  i, stat;
  void *modl, *copy;
  
  modl = model->handle.ident.ptr;
  stat = ocsmCopy(modl, &copy);
  if (stat != SUCCESS) return stat;

  model->handle.ident.ptr = copy;
  for (i = 0; i < model->nBRep; i++)
    model->BReps[i]->phandle = model->handle;
  for (i = 0; i < model->nParams; i++)
    model->Params[i].handle.ident.ptr = copy;
  for (i = 0; i < model->nBranches; i++)
    model->Branches[i].handle.ident.ptr = copy;

  return GEM_SUCCESS;
}


int
gem_kernelCopy(gemBRep *brep, /*@null@*/ double *xform, gemBRep **newBRep)
{
  int      stat, ibody, aindex, atype, alen, *aivec;
  char     *bID, *name;
  double   *arvec;
  ego      body, oform, copy;
  gemBody  *gbody;
  gemBRep  *nbrep;
  gemModel *model;

  *newBRep =  NULL;
  if (brep == NULL) return GEM_NULLOBJ;
  if (brep->magic != GEM_MBREP) return GEM_BADBREP;
  gbody = brep->body;
  body  = (ego) gbody->handle.ident.ptr;
  
  if (xform == NULL) {
    stat = EG_copyObject(body, NULL, &copy);
  } else {
    stat = EG_makeTransform(dia_context, xform, &oform);
    if (stat == EGADS_SUCCESS) {
      stat = EG_copyObject(body, oform, &copy);
      EG_deleteObject(oform);
    }
  }
  if (stat != EGADS_SUCCESS) return stat;
  nbrep = (gemBRep *) gem_allocate(sizeof(gemBRep));
  if (nbrep == NULL) {
    EG_deleteObject(copy);
    return GEM_ALLOC;
  }
  nbrep->magic   = GEM_MBREP;
  nbrep->omodel  = NULL;
  nbrep->phandle = brep->phandle;
  nbrep->ibranch = 0;
  nbrep->inumber = 0;
  nbrep->body    = NULL;
  model = brep->omodel;
  for (ibody = 1; ibody <= model->nBRep; ibody++)
    if (model->BReps[ibody-1] == brep) break;
    
  /* get bID for FaceIDs */
  stat = gem_retAttribute(brep, GEM_BREP, 0, "Name", &aindex, &atype, &alen,
                          &aivec, &arvec, &bID);
  if ((stat != GEM_SUCCESS) || (atype != GEM_STRING)) bID = NULL;
  if (bID == NULL) {
    stat = gem_retAttribute(model, 0, 0, "Name", &aindex, &atype, &alen,
                            &aivec, &arvec, &name);
    if ((stat == GEM_SUCCESS) && (atype == GEM_STRING)) {
      alen = strlen(name)+11;
      bID  = (char *) gem_allocate(alen*sizeof(char));
      if (bID == NULL) {
        gem_releaseBRep(nbrep);
        EG_deleteObject(copy);
        return GEM_ALLOC;
      }
      alen--;
      snprintf(bID, alen, "%s:%d", name, ibody);
      bID[alen] = 0;
    }
  }
  if (bID == NULL) {
    name = model->location;
    if (name == NULL) name = "CopiedModel";
    alen = strlen(name)+11;
    bID  = (char *) gem_allocate(alen*sizeof(char));
    if (bID == NULL) {
      gem_releaseBRep(nbrep);
      EG_deleteObject(copy);
      return GEM_ALLOC;
    }
    alen--;
    snprintf(bID, alen, "%s:%d", name, ibody);
    bID[alen] = 0;
  }
  
  stat = gem_diamondBody(copy, bID, nbrep);
  if (stat != EGADS_SUCCESS) {
    gem_releaseBRep(nbrep);
    EG_deleteObject(copy);
    gem_free(bID);
    return GEM_ALLOC;
  }
  gem_free(bID);

  *newBRep = nbrep;
  return GEM_SUCCESS;
}
