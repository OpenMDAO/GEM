/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel SBO Function -- CAPRI
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "capri.h"
#include "gem.h"
#include "memory.h"


  extern int  gem_quartzBody( int vol, gemBRep *brep );
  extern void gem_releaseBRep( /*@only@*/ gemBRep *brep );
 
  
void
gem_multMatMat4x4(double *a, double *b, double *c)
{
  int    i, j, k;
  double t[16];

#define MAT(m,r,c) ((m)[(c)*4+(r)])

  /* a = bc */
  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++) {
      MAT(t,j,i) = 0.0;
      for (k = 0; k < 4; k++) MAT(t,j,i) += MAT(b,j,k) * MAT(c,k,i);
    }

  for (i = 0; i < 4; i++)
    for (j = 0; j < 4; j++) MAT(a,j,i) = MAT(t,j,i);
#undef MAT

}


int
gem_kernelSBO(gemID src, gemID tool, /*@null@*/ double *xform, int type,
              gemModel **model)
{
  int      i, j, stat, vol1, vol2, cmdl, v1, vn, mfile;
  double   saved[16], mult[16];
  char     *modeler;
  gemID    gid, handle;
  gemModel *mdl, *prev;
  gemBRep  **BReps;
  gemCntxt *cntxt;

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
  vol1 = src.index;
  vol2 = tool.index;
  if (xform != NULL) {
    stat = gi_iGetDisplace(vol2, saved);
    if (stat != CAPRI_SUCCESS) return stat;
    saved[12] = saved[13] = saved[14] = 0.0;
    saved[15] = 1.0;
    for (i = 0; i < 12; i++) mult[i] = xform[i];
    mult[12] = mult[13] = mult[14] = 0.0;
    mult[15] = 1.0;
    gem_multMatMat4x4(mult, saved, mult);
    stat = gi_iSetDisplace(vol2, mult);
    if (stat != CAPRI_SUCCESS) return stat;  
  }
  if (type == GEM_INTERSECT) {
    stat = gi_gInterVolume(vol1, vol2);
  } else if (type == GEM_SUBTRACT) {
    stat = gi_gSubVolume(vol1, vol2);
  } else if (type == GEM_UNION) {
    stat = gi_gUnionVolume(vol1, vol2);
  } else {
    stat = CAPRI_INVALID;
  }
  if (xform != NULL) gi_iSetDisplace(vol2, saved);
  if (stat < CAPRI_SUCCESS) return stat;
  
  /* get the result */
  cmdl = stat;
  stat = gi_dGetModel(cmdl, &v1, &vn, &mfile, &modeler);
  if (stat != CAPRI_SUCCESS) {
    gi_uRelModel(cmdl);
    return stat;
  }

  BReps = (gemBRep **) gem_allocate((vn-v1+1)*sizeof(gemBRep *));
  if (BReps == NULL) {
    gi_uRelModel(cmdl);
    return GEM_ALLOC;
  }
  handle.index     = cmdl;
  handle.ident.tag = 0;
  for (i = 0; i < vn-v1+1; i++) {
    BReps[i] = (gemBRep *) gem_allocate(sizeof(gemBRep));
    if (BReps[i] == NULL) {
      for (j = 0; j < i; j++) gem_free(BReps[j]);
      gem_free(BReps);
      gi_uRelModel(cmdl);
      return GEM_ALLOC;
    }
    BReps[i]->magic   = GEM_MBREP;
    BReps[i]->omodel  = NULL;
    BReps[i]->phandle = handle;
    BReps[i]->ibranch = 0;
    BReps[i]->inumber = 0;
    BReps[i]->body    = NULL;
  }
  for (i = 0; i < vn-v1+1; i++) {
    stat = gem_quartzBody(v1+i, BReps[i]);
    if (stat != CAPRI_SUCCESS) {
      for (j = 0; j <= i; j++)
        gem_releaseBRep(BReps[j]);
      gem_free(BReps);
      gi_uRelModel(cmdl);
      return stat;
    }
  }

  mdl = (gemModel *) gem_allocate(sizeof(gemModel));
  if (mdl == NULL) {
    for (i = 0; i < vn-v1+1; i++)
      gem_releaseBRep(BReps[i]);
    gem_free(BReps);
    gi_uRelModel(cmdl);    
    return GEM_ALLOC;
  }

  gid.index      = cmdl;
  gid.ident.tag  = 0;

  mdl->magic     = GEM_MMODEL;
  mdl->handle    = gid;
  mdl->nonparam  = 1;
  mdl->server    = NULL;
  mdl->location  = NULL;
  mdl->modeler   = gem_strdup(modeler);
  mdl->nBRep     = vn - v1 + 1;
  mdl->BReps     = BReps;
  mdl->nParams   = 0;
  mdl->Params    = NULL;
  mdl->nBranches = 0;
  mdl->Branches  = NULL;
  mdl->attr      = NULL;
  mdl->prev      = (gemModel *) cntxt;
  mdl->next      = NULL;
  for (i = 0; i < vn-v1+1; i++) BReps[i]->omodel = mdl;

  prev = cntxt->model;
  cntxt->model = mdl;
  if (prev != NULL) {
    mdl->next  = prev;
    prev->prev = cntxt->model;
  }

  *model = mdl;
  return CAPRI_SUCCESS;
}
