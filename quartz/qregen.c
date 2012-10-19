/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Regenerate Function -- CAPRI
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gem.h"
#include "memory.h"
#include "attribute.h"
#include "capri.h"

  extern int  *CAPRI_MdlRefs;
  extern int  mCAPRI_MdlRefs;


  extern void gem_releaseBRep(/*@only@*/ gemBRep *brep);
  extern int  gem_kernelRelease(gemModel *mdl);
  extern int  gem_fillMM(int mmdl, gemModel *mdl);
  extern int  gem_quartzBody(int vol, gemBRep *brep);



int
gem_kernelRegen(gemModel *model)
{
  int      i, j, mm, stat, type, bflag, len, ibranch, omdl, cmdl;
  int      nparams, v1, vn,mfile, *ivec;
  char     *name;
  double   *rvec;
  gemBRep  **BReps;
  gemParam *sparams;
  
  omdl = model->handle.index;
  mm   = model->handle.ident.tag;
  if (mm == 0) return GEM_NOTPARMTRIC;
  
  for (i = 0; i < model->nParams; i++) {
    if (model->Params[i].changed == 0) continue;
    if (model->Params[i].type == GEM_BOOL) {
      if (model->Params[i].len == 1) {
        ivec = &model->Params[i].vals.bool1;
      } else {
        ivec =  model->Params[i].vals.bools;
      }
      stat = gi_fSetBool(mm, i+1, ivec);
    } else if (model->Params[i].type == GEM_INTEGER) {
      if (model->Params[i].len == 1) {
        ivec = &model->Params[i].vals.integer;
      } else {
        ivec =  model->Params[i].vals.integers;
      }
      stat = gi_fSetInteger(mm, i+1, ivec);
    } else if (model->Params[i].type == GEM_REAL) {
      if (model->Params[i].len == 1) {
        rvec = &model->Params[i].vals.real;
      } else {
        rvec =  model->Params[i].vals.reals;
      }
      stat = gi_fSetReal(mm, i+1, rvec);
    } else if (model->Params[i].type == GEM_STRING) {
      stat = gi_fSetString(mm, i+1, model->Params[i].vals.string);    
    } else {
      stat = gi_fGetParam(mm, i+1, &name, &type, &bflag, &len, &ibranch);
      if (stat != CAPRI_SUCCESS) return stat;
      len  = model->Params[i].len;
      switch (type) {
        case CAPRI_SPLINE:
          rvec = (double *) gem_allocate(2*len*sizeof(double));
          if (rvec == NULL) return GEM_ALLOC;
          for (j = 0; j < len; j++) {
            rvec[2*j  ] = model->Params[i].vals.splpnts[j].pos[0];
            rvec[2*j+1] = model->Params[i].vals.splpnts[j].pos[1];
          }
          break;
        case CAPRI_SPEND:
          rvec = (double *) gem_allocate((2*len+4)*sizeof(double));
          if (rvec == NULL) return GEM_ALLOC;
          rvec[0] = model->Params[i].vals.splpnts[0].tan[0];
          rvec[1] = model->Params[i].vals.splpnts[0].tan[1];
          for (j = 0; j < len; j++) {
            rvec[2*j+2] = model->Params[i].vals.splpnts[j].pos[0];
            rvec[2*j+3] = model->Params[i].vals.splpnts[j].pos[1];
          }
          rvec[2*len+2] = model->Params[i].vals.splpnts[len-1].tan[0];
          rvec[2*len+3] = model->Params[i].vals.splpnts[len-1].tan[1];
          break;
        case CAPRI_SPTAN:
          rvec = (double *) gem_allocate(4*len*sizeof(double));
          if (rvec == NULL) return GEM_ALLOC;
          for (j = 0; j < len; j++) {
            rvec[4*j  ] = model->Params[i].vals.splpnts[j].pos[0];
            rvec[4*j+1] = model->Params[i].vals.splpnts[j].pos[1];
            rvec[4*j+2] = model->Params[i].vals.splpnts[j].tan[0];
            rvec[4*j+3] = model->Params[i].vals.splpnts[j].tan[1];
          }
          break;
        case CAPRI_SPCURV:
          rvec = (double *) gem_allocate(6*len*sizeof(double));
          if (rvec == NULL) return GEM_ALLOC;
          for (j = 0; j < len; j++) {
            rvec[6*j  ] = model->Params[i].vals.splpnts[j].pos[0];
            rvec[6*j+1] = model->Params[i].vals.splpnts[j].pos[1];
            rvec[6*j+2] = model->Params[i].vals.splpnts[j].tan[0];
            rvec[6*j+3] = model->Params[i].vals.splpnts[j].tan[1];
            rvec[6*j+4] = model->Params[i].vals.splpnts[j].radc[0];
            rvec[6*j+5] = model->Params[i].vals.splpnts[j].radc[1];
          }
          break;
        case CAPRI_3DSPLINE:
          rvec = (double *) gem_allocate(3*len*sizeof(double));
          if (rvec == NULL) return GEM_ALLOC;
          for (j = 0; j < len; j++) {
            rvec[3*j  ] = model->Params[i].vals.splpnts[j].pos[0];
            rvec[3*j+1] = model->Params[i].vals.splpnts[j].pos[1];
            rvec[3*j+2] = model->Params[i].vals.splpnts[j].pos[2];
          }
          break;
        case CAPRI_3DSPEND:
          rvec = (double *) gem_allocate((3*len+6)*sizeof(double));
          if (rvec == NULL) return GEM_ALLOC;
          rvec[0] = model->Params[i].vals.splpnts[0].tan[0];
          rvec[1] = model->Params[i].vals.splpnts[0].tan[1];
          rvec[2] = model->Params[i].vals.splpnts[0].tan[2];
          for (j = 0; j < len; j++) {
            rvec[3*j+3] = model->Params[i].vals.splpnts[j].pos[0];
            rvec[3*j+4] = model->Params[i].vals.splpnts[j].pos[1];
            rvec[3*j+5] = model->Params[i].vals.splpnts[j].pos[2];
          }
          rvec[3*len+3] = model->Params[i].vals.splpnts[len-1].tan[0];
          rvec[3*len+4] = model->Params[i].vals.splpnts[len-1].tan[1];
          rvec[3*len+5] = model->Params[i].vals.splpnts[len-1].tan[2];
          break;
        case CAPRI_3DSPTAN:
          rvec = (double *) gem_allocate(6*len*sizeof(double));
          if (rvec == NULL) return GEM_ALLOC;
          for (j = 0; j < len; j++) {
            rvec[6*j  ] = model->Params[i].vals.splpnts[j].pos[0];
            rvec[6*j+1] = model->Params[i].vals.splpnts[j].pos[1];
            rvec[6*j+2] = model->Params[i].vals.splpnts[j].pos[2];
            rvec[6*j+3] = model->Params[i].vals.splpnts[j].tan[0];
            rvec[6*j+4] = model->Params[i].vals.splpnts[j].tan[1];
            rvec[6*j+5] = model->Params[i].vals.splpnts[j].tan[2];
          }
          break;
        case CAPRI_3DSPCURV:
          rvec = (double *) gem_allocate(9*len*sizeof(double));
          if (rvec == NULL) return GEM_ALLOC;
          for (j = 0; j < len; j++) {
            rvec[9*j  ] = model->Params[i].vals.splpnts[j].pos[0];
            rvec[9*j+1] = model->Params[i].vals.splpnts[j].pos[1];
            rvec[9*j+2] = model->Params[i].vals.splpnts[j].pos[2];
            rvec[9*j+3] = model->Params[i].vals.splpnts[j].tan[0];
            rvec[9*j+4] = model->Params[i].vals.splpnts[j].tan[1];
            rvec[9*j+5] = model->Params[i].vals.splpnts[j].tan[2];
            rvec[9*j+6] = model->Params[i].vals.splpnts[j].radc[0];
            rvec[9*j+7] = model->Params[i].vals.splpnts[j].radc[1];
            rvec[9*j+8] = model->Params[i].vals.splpnts[j].radc[2];
          }
          break;
      }
      stat = gi_fSetSpline(mm, i+1, len, rvec);
      gem_free(rvec);
    }
    if (stat != CAPRI_SUCCESS) {
      printf(" GEM Quartz Internal: Setting Parameter %d = %d\n",
             i+1, stat);
      return stat;
    }
  }

  for (i = 0; i < model->nBranches; i++) {
    if (model->Branches[i].changed == 0) continue;  
    stat = gi_fSetSupress(mm, i, model->Branches[i].sflag);
    if (stat != CAPRI_SUCCESS) {
      printf(" GEM Quartz Internal: Setting Branch %d = %d\n",
             i+1, stat);
      return stat;
    }    
  }

  /* regenerate */
  
  stat = gi_fRegenerate(mm);
  if (stat < CAPRI_SUCCESS) {
    printf(" GEM Quartz Internal: Regenerate = %d\n", stat);
    return stat;
  }
  printf(" gi_fRegenerate = %d\n", stat);
  cmdl = stat;
  stat = gi_fRelMModel(mm);
  if (stat != CAPRI_SUCCESS)
    printf(" GEM Quartz Internal: RelMModel = %d\n", stat);
  if (omdl != cmdl) gem_kernelRelease(model);
  
  /* clean up old stuff in the model */
  
  for (i = 0; i < model->nBRep; i++) gem_releaseBRep(model->BReps[i]);
  if (model->BReps != NULL) gem_free(model->BReps);

  for (i = 0; i < model->nParams; i++) {
    gem_free(model->Params[i].name);
    if (model->Params[i].type == GEM_SPLINE) {
      gem_free(model->Params[i].vals.splpnts);
    } else if (model->Params[i].type == GEM_STRING) {
      gem_free(model->Params[i].vals.string);
    } else if (model->Params[i].type == GEM_REAL) {
      if (model->Params[i].len > 1) gem_free(model->Params[i].vals.reals);
    } else if (model->Params[i].type == GEM_INTEGER) {
      if (model->Params[i].len > 1) gem_free(model->Params[i].vals.integers);
    } else {
      if (model->Params[i].len > 1) gem_free(model->Params[i].vals.bools);
    }
  }
  sparams = model->Params;
  nparams = model->nParams;

  for (i = 0; i < model->nBranches; i++) {
    gem_free(model->Branches[i].name);
    if (model->Branches[i].nParents > 1)
      gem_free(model->Branches[i].parents.pnodes);
    if (model->Branches[i].nChildren > 1)
      gem_free(model->Branches[i].children.nodes);
    if (model->Branches[i].branchType != NULL) 
      gem_free(model->Branches[i].branchType);
    gem_clrAttribs(&model->Branches[i].attr);
  }
  gem_free(model->Branches);

  /* set the new data */

  model->nonparam         = 1;
  model->handle.index     = cmdl;
  model->handle.ident.tag = 0;

  stat = gi_dGetModel(cmdl, &v1, &vn, &mfile, &name);
  if (stat != CAPRI_SUCCESS) {
    gi_uRelModel(cmdl);
    gem_clrAttribs(&model->attr);
    model->magic = 0;
    gem_free(model);
    for (i = 0; i < nparams; i++)
      gem_clrAttribs(&sparams[i].attr);
    gem_free(sparams);
    return stat;
  }

  BReps = (gemBRep **) gem_allocate((vn-v1+1)*sizeof(gemBRep *));
  if (BReps == NULL) {
    gi_uRelModel(cmdl);
    gem_clrAttribs(&model->attr);
    model->magic = 0;
    gem_free(model);
    for (i = 0; i < nparams; i++)
      gem_clrAttribs(&sparams[i].attr);
    gem_free(sparams);
    return GEM_ALLOC;
  }
  for (i = 0; i < vn-v1+1; i++) {
    BReps[i] = (gemBRep *) gem_allocate(sizeof(gemBRep));
    if (BReps[i] == NULL) {
      for (j = 0; j < i; j++) gem_free(BReps[j]);
      gem_free(BReps);
      gi_uRelModel(cmdl);
      gem_clrAttribs(&model->attr);
      model->magic = 0;
      gem_free(model);
      for (j = 0; j < nparams; j++)
        gem_clrAttribs(&sparams[j].attr);
      gem_free(sparams);
      return GEM_ALLOC;
    }
    BReps[i]->magic   = GEM_MBREP;
    BReps[i]->omodel  = NULL;
    BReps[i]->phandle = model->handle;
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
      gem_clrAttribs(&model->attr);
      model->magic = 0;
      gem_free(model);
      for (i = 0; i < nparams; i++)
        gem_clrAttribs(&sparams[i].attr);
      gem_free(sparams);
      return stat;
    }
  }
  for (i = 0; i < vn-v1+1; i++) BReps[i]->omodel = model;
  model->nBRep = vn - v1 + 1;
  model->BReps = BReps;

  mm = gi_fMasterModel(cmdl, 0);
  if (mm <= CAPRI_SUCCESS) {
    printf(" GEM/CAPRI Internal: fMasterModel = %d -- Model made static!\n",
           stat);
  } else {
    stat = gem_fillMM(mm, model);
    if (stat != GEM_SUCCESS) {
      printf(" GEM/CAPRI Internal: fillMM = %d -- Model made static!\n",
             stat);
    } else {
      model->nonparam         = 0;
      model->handle.ident.tag = mm;
      for (i = 0; i < model->nBRep; i++)
        BReps[i]->phandle = model->handle;
    }
  }

  if (nparams == model->nParams)
    for (i = 0; i < nparams; i++)
      gem_cpyAttribs(sparams[i].attr, &model->Params[i].attr);
  for (i = 0; i < nparams; i++)
    gem_clrAttribs(&sparams[i].attr);
  gem_free(sparams);

  /* set our reference */
  if (CAPRI_MdlRefs != NULL) {
    if (cmdl >= mCAPRI_MdlRefs) {
      i    = cmdl+256;
      ivec = (int *) gem_reallocate(CAPRI_MdlRefs, i*sizeof(int));
      if (ivec == NULL) {
        printf(" GEM/CAPRI Internal: Model References Disabled!\n");
        gem_free(CAPRI_MdlRefs);
         CAPRI_MdlRefs = NULL;
        mCAPRI_MdlRefs = 0;
      }
       CAPRI_MdlRefs = ivec;
      mCAPRI_MdlRefs = i;
    }
    if (omdl != cmdl) CAPRI_MdlRefs[cmdl-1] = 1;
  }

  return GEM_SUCCESS;
}
