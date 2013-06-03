/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Model Functions
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "gem.h"
#include "memory.h"
#include "attribute.h"
#include "kernel.h"


  extern int  gem_clrDReps(gemModel *model, int phase);


void
gem_releaseBRep(/*@only@*/ gemBRep *brep)
{
  int     i;
  gemBody *body;

  if (brep == NULL) return;
  if (brep->magic != GEM_MBREP) return;
  
  body = brep->body;
  if (body == NULL) return;
  if (brep->inumber == 0) {

    for (i = 0; i < body->nnode; i++)
      gem_clrAttribs(&body->nodes[i].attr);
    gem_free(body->nodes);
    
    for (i = 0; i < body->nedge; i++)
      gem_clrAttribs(&body->edges[i].attr);
    gem_free(body->edges);
    
    for (i = 0; i < body->nloop; i++) {
      gem_free(body->loops[i].edges);
      gem_clrAttribs(&body->loops[i].attr);
    }
    gem_free(body->loops);
    
    for (i = 0; i < body->nface; i++) {
      gem_free(body->faces[i].loops);
      gem_free(body->faces[i].ID);
      gem_clrAttribs(&body->faces[i].attr);
    }
    gem_free(body->faces);
    
    for (i = 0; i < body->nshell; i++) {
      gem_free(body->shells[i].faces);
      gem_clrAttribs(&body->shells[i].attr);
    }
    gem_free(body->shells);

    gem_clrAttribs(&body->attr);
    gem_free(body);
  }

  brep->magic = 0;
  gem_free(brep);
}


void
gem_clrModel(/*@only@*/ gemModel *model)
{
  int i;
  
  if (model->server   != NULL) gem_free(model->server);
  if (model->location != NULL) gem_free(model->location);
  if (model->modeler  != NULL) gem_free(model->modeler);
  for (i = 0; i < model->nBRep; i++) gem_releaseBRep(model->BReps[i]);
  if (model->BReps    != NULL) gem_free(model->BReps);

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
    gem_clrAttribs(&model->Params[i].attr);
  }
  gem_free(model->Params);

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
  
  gem_clrAttribs(&model->attr);
  model->magic = 0;
  gem_free(model);
}


int
gem_releaseModel(/*@only@*/ gemModel *model)
{
  int      i;
  gemModel *prev, *next;
  gemDRep  *drep;
  gemCntxt *cntxt;

  if (model == NULL) return GEM_NULLOBJ;
  if (model->magic != GEM_MMODEL) return GEM_BADMODEL;

  /* find the context */

  cntxt = NULL;
  prev  = model->prev;
  while (cntxt == NULL) {
    if  (prev  == NULL) return GEM_BADCONTEXT;
    if ((prev->magic != GEM_MMODEL) && 
        (prev->magic != GEM_MCONTEXT)) return GEM_BADOBJECT;
    if  (prev->magic == GEM_MCONTEXT)  cntxt = (gemCntxt *) prev;
    if  (prev->magic == GEM_MMODEL)    prev  = prev->prev;
  }
  
  /* fix up the linked list */

  prev = model->prev;
  next = model->next;
  if (prev->magic == GEM_MMODEL) {
    prev->next   = next;
  } else {
    cntxt->model = next;
  }
  if (next != NULL) next->prev = prev;
  
  /* tell our geometry kernel */
  
  gem_kernelRelease(model);
  
  /* remove any DReps attached to this model */

  do {
    i    = 0;
    drep = cntxt->drep;
    while (drep != NULL) {
      if (drep->model == model) {
        gem_destroyDRep(drep);
        i++;
        break;
      }
      drep = drep->next;
    }
  } while (i != 0);
  
  gem_clrModel(model);
  return GEM_SUCCESS;
}


int
gem_getModel(gemModel *model, char **server, char **filnam, char **modeler, 
             int *uptodate, int *nBRep, gemBRep ***BReps, int *nParams,
             int *nBranch, int *nattr)
{
  int i;
  
  *server   = NULL;
  *filnam   = NULL;
  *modeler  = NULL;
  *uptodate = -1;
  *nBRep    = 0;
  *BReps    = NULL;
  *nParams  = 0;
  *nBranch  = 0;
  *nattr    = 0;
  if (model == NULL) return GEM_NULLOBJ;
  if (model->magic != GEM_MMODEL) return GEM_BADMODEL;
  
  if (model->nonparam == 0) {
    *uptodate = 1;
    for (i = 0; i < model->nParams; i++)
      if (model->Params[i].changed != 0) {
        *uptodate = 0;
        break;
      }
    if (*uptodate == 1)
      for (i = 0; i < model->nBranches; i++) 
        if (model->Branches[i].changed != 0) {
          *uptodate = 0;
          break;
        }
  }
  *server  = model->server;
  *filnam  = model->location;
  *modeler = model->modeler;
  *nBRep   = model->nBRep;
  *BReps   = model->BReps;
  *nParams = model->nParams;
  *nBranch = model->nBranches;
  if (model->attr != NULL) *nattr = model->attr->nattrs;
  
  return GEM_SUCCESS;
}


int
gem_getParam(gemModel *model, int param, char **pname, int *bflag,
             int *order, int *ptype, int *plen, int **integers,
             double **reals, char **string, gemSpl **spline, 
             int *nattr)
{
  *pname    = NULL;
  *bflag    = 0;
  *order    = 0;
  *ptype    = 0;
  *plen     = 0;
  *integers = NULL;
  *reals    = NULL;
  *string   = NULL;
  *spline   = NULL;
  *nattr    = 0;
  if (model == NULL) return GEM_NULLOBJ;
  if (model->magic != GEM_MMODEL) return GEM_BADMODEL;
  if ((param < 1) || (param > model->nParams)) return GEM_BADINDEX;
  
  *pname = model->Params[param-1].name;
  *bflag = model->Params[param-1].bitflag;
  *order = model->Params[param-1].order;
  *ptype = model->Params[param-1].type;
  *plen  = model->Params[param-1].len;
  if (*ptype == GEM_SPLINE) {
    *spline = model->Params[param-1].vals.splpnts;  
  } else if (*ptype == GEM_STRING) {
    *string = model->Params[param-1].vals.string;
  } else if (*ptype == GEM_REAL) {
    if (*plen == 1) {
      *reals = &model->Params[param-1].vals.real;
    } else {
      *reals =  model->Params[param-1].vals.reals;
    }
  } else if (*ptype == GEM_INTEGER) {
    if (*plen == 1) {
      *integers = &model->Params[param-1].vals.integer;
    } else {
      *integers =  model->Params[param-1].vals.integers;
    }
  } else {
    if (*plen == 1) {
      *integers = &model->Params[param-1].vals.bool1;
    } else {
      *integers =  model->Params[param-1].vals.bools;
    }
  }
  if (model->Params[param-1].attr != NULL)
    *nattr = model->Params[param-1].attr->nattrs;

  return GEM_SUCCESS;
}


int
gem_getLimits(gemModel *model, int param, int *integers, double *reals)
{
  if (model == NULL) return GEM_NULLOBJ;
  if (model->magic != GEM_MMODEL) return GEM_BADMODEL;
  if ((param < 1) || (param > model->nParams)) return GEM_BADINDEX;
  if ((model->Params[param-1].bitflag&8) == 0) return GEM_BADINDEX;

  if (model->Params[param-1].type == GEM_INTEGER) {
    integers[0] = model->Params[param-1].bnds.ilims[0];
    integers[1] = model->Params[param-1].bnds.ilims[1];
  } else {
    reals[0] = model->Params[param-1].bnds.rlims[0];
    reals[1] = model->Params[param-1].bnds.rlims[1];
  }

  return GEM_SUCCESS;
}


int
gem_setParam(gemModel *model, int param, int plen,
             /*@null@*/ int *integers, /*@null@*/ double *reals,
             /*@null@*/ char *string,  /*@null@*/ gemSpl *spline)
{
  int i;
  
  if (model == NULL) return GEM_NULLOBJ;
  if (model->magic != GEM_MMODEL) return GEM_BADMODEL;
  if ((param < 1) || (param > model->nParams)) return GEM_BADINDEX;
  if (plen <= 0) return GEM_BADVALUE;
  if ((model->Params[param-1].bitflag&1) != 0) return GEM_READONLYERR;
  if ((model->Params[param-1].bitflag&2) != 0)
    if (model->Params[param-1].len != plen) return GEM_FIXEDLEN;
    
  if ((model->Params[param-1].bitflag&8) != 0) 
    if (model->Params[param-1].type == GEM_INTEGER) {
      if (integers == NULL) return GEM_NULLVALUE;
      for (i = 0; i < plen; i++)
        if ((integers[i] < model->Params[param-1].bnds.ilims[0]) ||
            (integers[i] > model->Params[param-1].bnds.ilims[1]))
          return GEM_BADVALUE;
    } else {
      if (reals == NULL) return GEM_NULLVALUE;
      for (i = 0; i < plen; i++)
        if ((reals[i] < model->Params[param-1].bnds.rlims[0]) ||
            (reals[i] > model->Params[param-1].bnds.rlims[1]))
          return GEM_BADVALUE;
    }

  if ((model->Params[param-1].type == GEM_BOOL) ||
      (model->Params[param-1].type == GEM_INTEGER)) {
    if (integers == NULL) return GEM_NULLVALUE;
    if (model->Params[param-1].len != 1) 
      if (plen != model->Params[param-1].len) {
        gem_free(model->Params[param-1].vals.integers);
        model->Params[param-1].len = 0;
      }
    if ((plen > 1) && (plen != model->Params[param-1].len)) {
      model->Params[param-1].vals.integers = 
             (int *) gem_allocate(plen*sizeof(int));
      if (model->Params[param-1].vals.integers == NULL) return GEM_ALLOC;
      model->Params[param-1].len = plen;
    }
    if (plen == 1) {
      model->Params[param-1].vals.integer = integers[0];
    } else {
      for (i = 0; i < plen; i++)
        model->Params[param-1].vals.integers[i] = integers[i];
    }

  } else if (model->Params[param-1].type == GEM_REAL) {
    if (reals == NULL) return GEM_NULLVALUE;
    if (model->Params[param-1].len != 1) 
      if (plen != model->Params[param-1].len) {
        gem_free(model->Params[param-1].vals.reals);
        model->Params[param-1].len = 0;
      }
    if ((plen > 1) && (plen != model->Params[param-1].len)) {
      model->Params[param-1].vals.reals = 
             (double *) gem_allocate(plen*sizeof(double));
      if (model->Params[param-1].vals.reals == NULL) return GEM_ALLOC;
      model->Params[param-1].len = plen;
    }
    if (plen == 1) {
      model->Params[param-1].vals.real = reals[0];
    } else {
      for (i = 0; i < plen; i++)
        model->Params[param-1].vals.reals[i] = reals[i];
    }

  } else if (model->Params[param-1].type == GEM_STRING) {
    if (string == NULL) return GEM_NULLVALUE;
    gem_free(model->Params[param-1].vals.string);
    model->Params[param-1].vals.string = gem_strdup(string);

  } else {
    if (spline == NULL) return GEM_NULLVALUE;
    if (plen < 3) return GEM_BADVALUE;
    if (plen != model->Params[param-1].len) {
      gem_free(model->Params[param-1].vals.splpnts);
      model->Params[param-1].len = 0;
      model->Params[param-1].vals.splpnts = 
             (gemSpl *) gem_allocate(plen*sizeof(gemSpl));
      if (model->Params[param-1].vals.splpnts == NULL) return GEM_ALLOC;
      model->Params[param-1].len = plen;
    }
    for (i = 0; i < plen; i++)
      model->Params[param-1].vals.splpnts[i] = spline[i];
  }
  
  model->Params[param-1].changed = 1;
  return GEM_SUCCESS;
}


int
gem_getBranch(gemModel *model, int branch, char **bname, char **btype,
              int *suppress, int *nparent, int **parents, int *nchild,
              int **children, int *nattr)
{
  *bname    = NULL;
  *btype    = NULL;
  *suppress = 0;
  *nparent  = 0;
  *nchild   = 0;
  *nattr    = 0;
  *parents  = NULL;
  *children = NULL;
  if (model == NULL) return GEM_NULLOBJ;
  if (model->magic != GEM_MMODEL) return GEM_BADMODEL;
  if ((branch < 1) || (branch > model->nBranches)) return GEM_BADINDEX;
  
  *bname     = model->Branches[branch-1].name;
  *btype     = model->Branches[branch-1].branchType;
  *suppress  = model->Branches[branch-1].sflag;
  *nparent   = model->Branches[branch-1].nParents;
  *nchild    = model->Branches[branch-1].nChildren;
  *nattr     = 0;
  if (model->Branches[branch-1].attr != NULL)
    *nattr   = model->Branches[branch-1].attr->nattrs;
  if (model->Branches[branch-1].nParents <= 1) {
    *parents = &model->Branches[branch-1].parents.pnode;
  } else {
    *parents =  model->Branches[branch-1].parents.pnodes;
  }
  if (model->Branches[branch-1].nChildren <= 1) {
    *children = &model->Branches[branch-1].children.node;
  } else {
    *children =  model->Branches[branch-1].children.nodes;
  }
    
  return GEM_SUCCESS;
}


int
gem_setSuppress(gemModel *model, int branch, int suppress)
{
  if (model == NULL) return GEM_NULLOBJ;
  if (model->magic != GEM_MMODEL) return GEM_BADMODEL;
  if ((branch < 1) || (branch > model->nBranches)) return GEM_BADINDEX;
  if ((suppress != GEM_ACTIVE) && (suppress != GEM_SUPPRESSED))
    return GEM_BADTYPE;
  if (model->Branches[branch-1].sflag == GEM_READONLY)
    return GEM_READONLYERR;
  if (model->Branches[branch-1].sflag == suppress) 
    return GEM_NOTCHANGED;

  model->Branches[branch-1].sflag   = suppress;
  model->Branches[branch-1].changed = 1;

  return GEM_SUCCESS;
}


int
gem_regenModel(gemModel *model)
{
  int i, changed = 0;

  if (model == NULL) return GEM_NULLOBJ;
  if (model->magic != GEM_MMODEL) return GEM_BADMODEL;
  if (model->nonparam == 1) return GEM_NOTPARMTRIC;
  
  for (i = 0; i < model->nBranches; i++)
    if (model->Branches[i].changed == 1) changed++;
  for (i = 0; i < model->nParams; i++)
    if (model->Params[i].changed   == 1) changed++;
  if (changed == 0) return GEM_NOTCHANGED;

  return gem_kernelRegen(model);
}


static int
gem_copyMModel(gemModel *model, gemModel *mdl)
{
  int      i, j, n, type, len;
  gemParam *params;
  gemFeat  *branches;

  if ((model->nParams   == 0) && 
      (model->nBranches == 0)) return GEM_SUCCESS;
  
  if (model->nBranches > 0) {
    branches = (gemFeat *) gem_allocate(model->nBranches*sizeof(gemFeat));
    if (branches == NULL) return GEM_ALLOC;
    for (i = 0; i < model->nBranches; i++) {
      branches[i].name          = NULL;
      branches[i].handle        = model->Branches[i].handle;
      branches[i].sflag         = model->Branches[i].sflag;
      branches[i].branchType    = NULL;
      branches[i].nParents      = 0;
      branches[i].parents.pnode = 0;
      branches[i].nChildren     = 0;
      branches[i].children.node = 0;
      branches[i].attr          = NULL;
      branches[i].changed       = model->Branches[i].changed;
    }
    mdl->nBranches = model->nBranches;
    mdl->Branches  = branches;
    
    for (i = 0; i < model->nBranches; i++) {
      branches[i].name       = gem_strdup(model->Branches[i].name);
      branches[i].branchType = gem_strdup(model->Branches[i].branchType);
      n = model->Branches[i].nParents;
      if (n == 1) {
        branches[i].parents.pnode = model->Branches[i].parents.pnode;
        branches[i].nParents      = 1;
      } else if (n > 1) {
        branches[i].parents.pnodes = (int *) gem_allocate(n*sizeof(int));
        if (branches[i].parents.pnodes == NULL) return GEM_ALLOC;
        for (j = 0; j < n; j++)
          branches[i].parents.pnodes[j] = model->Branches[i].parents.pnodes[j];
        branches[i].nParents = n;
      }
      n = model->Branches[i].nChildren;
      if (n == 1) {
        branches[i].children.node = model->Branches[i].children.node;
        branches[i].nChildren     = 1;
      } else if (n > 1) {
        branches[i].children.nodes = (int *) gem_allocate(n*sizeof(int));
        if (branches[i].children.nodes == NULL) return GEM_ALLOC;
        for (j = 0; j < n; j++)
          branches[i].children.nodes[j] = model->Branches[i].children.nodes[j];
        branches[i].nChildren = n;
      }
      gem_cpyAttribs(model->Branches[i].attr, &branches[i].attr);
    }
  }
  if (model->nParams <= 0) return GEM_SUCCESS;
  
  params = (gemParam *) gem_allocate(model->nParams*sizeof(gemParam));
  if (params == NULL) return GEM_ALLOC;
  for (i = 0; i < model->nParams; i++) {
    params[i].name       = gem_strdup(model->Params[i].name);
    params[i].handle     = model->Params[i].handle;
    params[i].type       = GEM_REAL;
    params[i].order      = 0;
    params[i].bitflag    = 0;
    params[i].len        = 1;
    params[i].vals.real  = 0.0;
    params[i].attr       = NULL;
    params[i].changed    = model->Params[i].changed;
  }
  mdl->nParams = model->nParams;
  mdl->Params  = params;

  for (i = 0; i < model->nParams; i++) {
    type = model->Params[i].type;
    len  = model->Params[i].len;
    if ((type == GEM_BOOL) || (type == GEM_INTEGER)) {
      if (len > 1) {
        params[i].vals.integers = (int *) gem_allocate(len*sizeof(int));
        if (params[i].vals.integers == NULL) return GEM_ALLOC;
        for (j = 0; j < len; j++)
          params[i].vals.integers[j] = model->Params[i].vals.integers[j];
      } else {
        params[i].vals.integer = model->Params[i].vals.integer;
      }
    } else if (type == GEM_REAL) {
      if (len > 1) {
        params[i].vals.reals = (double *) gem_allocate(len*sizeof(double));
        if (params[i].vals.reals == NULL) return GEM_ALLOC;
        for (j = 0; j < len; j++)
          params[i].vals.reals[j] = model->Params[i].vals.reals[j];
      } else {
        params[i].vals.real = model->Params[i].vals.real;
      }
    } else if (type == GEM_STRING) {
      params[i].vals.string = NULL;
      if (model->Params[i].vals.string != NULL)
        params[i].vals.string = gem_strdup(model->Params[i].vals.string);
    } else {
      params[i].vals.splpnts = (gemSpl *) gem_allocate(len*sizeof(gemSpl));
      if (params[i].vals.splpnts == NULL) return GEM_ALLOC;
      for (j = 0; j < len; j++)
        params[i].vals.splpnts[j] = model->Params[i].vals.splpnts[j];
    }
    params[i].type    = type; 
    params[i].order   = model->Params[i].order;
    params[i].bitflag = model->Params[i].bitflag;
    params[i].len     = len;   
    gem_cpyAttribs(model->Params[i].attr, &params[i].attr);
  }

  return gem_kernelCopyMM(mdl);
}


int
gem_copyModel(gemModel *model, gemModel **newModel)
{
  int      i, j, stat;
  gemBRep  **BReps = NULL;
  gemModel *mdl, *prev;
  gemCntxt *cntxt;

  *newModel = NULL;
  if (model == NULL) return GEM_NULLOBJ;
  if (model->magic != GEM_MMODEL) return GEM_BADMODEL;
  
  if (model->nBranches > 0)
    if (model->Branches[0].branchType != NULL)
      if (strcmp(model->Branches[0].branchType,"ASSEMBLY") == 0)
        return GEM_ASSEMBLY;
        
  /* find the context */

  cntxt = NULL;
  prev  = model->prev;
  while (cntxt == NULL) {
    if  (prev  == NULL) return GEM_BADCONTEXT;
    if ((prev->magic != GEM_MMODEL) && 
        (prev->magic != GEM_MCONTEXT)) return GEM_BADOBJECT;
    if  (prev->magic == GEM_MCONTEXT)  cntxt = (gemCntxt *) prev;
    if  (prev->magic == GEM_MMODEL)    prev  = prev->prev;
  }
        
  /* make copies of the BReps */
  
  if (model->nBRep > 0) {
    BReps = (gemBRep **) gem_allocate(model->nBRep*sizeof(gemBRep *));
    if (BReps == NULL) return GEM_ALLOC;
  
    for (i = 0; i < model->nBRep; i++) {
      stat = gem_kernelCopy(model->BReps[i], NULL, &BReps[i]);
      if (stat != GEM_SUCCESS) {
        printf(" GEM Internal: kernelCopy = %d for BRep %d!\n", stat, i+1);
        for (j = 0; j < i; j++) {
          gem_kernelDelete(BReps[j]->body->handle);
          gem_releaseBRep(BReps[j]);
        }
        gem_free(BReps);
        return stat;
      }
    }
  }

  /* make copies of internals */
  
  mdl = (gemModel *) gem_allocate(sizeof(gemModel));
  if (mdl == NULL) {
    if (BReps != NULL) {
      for (j = 0; j < model->nBRep; j++) {
        gem_kernelDelete(BReps[j]->body->handle);
        gem_releaseBRep(BReps[j]);
      }
      gem_free(BReps);
    }
    return GEM_ALLOC;
  }

  mdl->magic     = model->magic;
  mdl->handle    = model->handle;
  mdl->nonparam  = model->nonparam;
  mdl->server    = gem_strdup(model->server);
  mdl->location  = gem_strdup(model->location);
  mdl->modeler   = gem_strdup(model->modeler);
  mdl->nBRep     = model->nBRep;
  mdl->BReps     = BReps;
  mdl->nParams   = 0;
  mdl->Params    = NULL;
  mdl->nBranches = 0;
  mdl->Branches  = NULL;
  mdl->attr      = NULL;
  mdl->prev      = (gemModel *) cntxt;
  mdl->next      = NULL;
  if (BReps != NULL)
    for (i = 0; i < model->nBRep; i++) BReps[i]->omodel = mdl;
  gem_cpyAttribs(model->attr, &mdl->attr);

  /* fill in the master-model info */
  stat = gem_copyMModel(model, mdl);
  if (stat != GEM_SUCCESS) {
    gem_releaseModel(mdl);
    return stat;
  }

  prev = cntxt->model;
  cntxt->model = mdl;
  if (prev != NULL) {
    mdl->next  = prev;
    prev->prev = cntxt->model;
  }

  *newModel = mdl;
  return GEM_SUCCESS;
}


int
gem_add2Model(gemModel *model, gemBRep *brep, /*@null@*/ double *xform)
{
  int     stat;
  gemBRep *added, **BReps;

  if (model == NULL) return GEM_NULLOBJ;
  if (model->magic != GEM_MMODEL) return GEM_BADMODEL;
  if (brep == NULL)  return GEM_NULLOBJ;
  if (brep->magic  != GEM_MBREP)  return GEM_BADBREP;
  if (model->nonparam == 0) return GEM_NOTPARMTRIC;
  /* make sure we have been initiated by staticModel */
  if (model->handle.index     != 0)    return GEM_BADTYPE;
  if (model->handle.ident.ptr != NULL) return GEM_BADTYPE;
  
  /* make copy of transformed brep and place it in model */
  
  stat = gem_kernelCopy(brep, xform, &added);
  if (stat != GEM_SUCCESS) return stat;
  added->omodel = model;
  
  if (model->nBRep == 0) {
    BReps = (gemBRep **) gem_allocate(sizeof(gemBRep *));
  } else {
    BReps = (gemBRep **) gem_reallocate( model->BReps,
                                        (model->nBRep+1)*sizeof(gemBRep *));
  }
  if (BReps == NULL) {
    gem_kernelDelete(added->body->handle);
    gem_releaseBRep(added);
    return GEM_ALLOC;
  }
  
  gem_clrDReps(model, 0);
  BReps[model->nBRep] = added;
  model->BReps  = BReps;
  model->nBRep += 1;
  gem_clrDReps(model, 1);

  return GEM_SUCCESS;
}


int
gem_saveModel(gemModel *model, /*@null@*/ char *filename)
{
  if  (model == NULL) return GEM_NULLOBJ;
  if  (model->magic != GEM_MMODEL) return GEM_BADMODEL;  
  
  return gem_kernelSave(model, filename);
}
