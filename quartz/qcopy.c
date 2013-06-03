/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Copy Function -- CAPRI
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
#include "memory.h"


  extern int  *CAPRI_MdlRefs;
  
  extern void gem_releaseBRep( /*@only@*/ gemBRep *brep );
  extern int  gem_quartzBody( int vol, gemBRep *brep );
  extern int  gem_invertXform( double *xform, double *invXform );
  extern void gem_multMatMat4x4( double *a, double *b, double *c );



int
gem_kernelDelete(/*@unused@*/ gemID handle)
{
  return GEM_SUCCESS;
}


int
gem_kernelCopyMM(gemModel *model)
{
  int i, cmdl, mm;

  cmdl = model->handle.index;
  mm   = gi_fMasterModel(cmdl, 0);
  if (mm <= CAPRI_SUCCESS) return mm;
  
  model->handle.ident.tag = mm;
  for (i = 0; i < model->nBRep; i++)
    model->BReps[i]->phandle = model->handle;
  for (i = 0; i < model->nParams; i++)
    model->Params[i].handle.index = mm;
  for (i = 0; i < model->nBranches; i++)
    model->Branches[i].handle.index = mm;
    
  return GEM_SUCCESS;
}


int
gem_kernelCopy(gemBRep *brep, /*@null@*/ double *xform, gemBRep **newBRep)
{
  int     i, stat, vol, cmdl;
  double  xfo[16], xfx[16];
  gemBRep *nbrep;

  *newBRep =  NULL;
  if (brep == NULL) return GEM_NULLOBJ;
  if (brep->magic != GEM_MBREP) return GEM_BADBREP;
  vol  = brep->body->handle.index;
  stat = gi_uGetModel(vol, &cmdl);
  if (stat != CAPRI_SUCCESS) return stat;
  
  nbrep = (gemBRep *) gem_allocate(sizeof(gemBRep));
  if (nbrep == NULL) return GEM_ALLOC;
  nbrep->magic   = GEM_MBREP;
  nbrep->omodel  = NULL;
  nbrep->phandle = brep->phandle;
  nbrep->ibranch = 0;
  nbrep->inumber = 0;
  nbrep->body    = NULL;
  
  stat = gem_quartzBody(vol, nbrep);
  if (stat != GEM_SUCCESS) {
    gem_free(nbrep);
    return stat;
  }
  if (xform != NULL) {
    for (i = 0; i < 12; i++) {
      xfo[i] = brep->xform[i];
      xfx[i] = xform[i];
    }
    xfo[12] = xfo[13] = xfo[14] = 0.0;
    xfo[15] = 1.0;
    xfx[12] = xfx[13] = xfx[14] = 0.0;
    xfx[15] = 1.0;
    gem_multMatMat4x4(xfo, xfx, xfo);
    for (i = 0; i < 12; i++) nbrep->xform[i] = xfo[i];
    stat = gem_invertXform(nbrep->xform, nbrep->invXform);
    if (stat != GEM_SUCCESS) {
      gem_releaseBRep(nbrep);
      return stat;
    }
  }
  CAPRI_MdlRefs[cmdl-1]++;

  *newBRep = nbrep;
  return GEM_SUCCESS;
}
