/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Base-level Functions
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
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


  static int gem_nContext = 0;


int 
gem_initialize(gemCntxt **context)
{
  int      stat;
  gemCntxt *cntxt;

  *context = NULL;
  if (gem_nContext == 0) {
    stat = gem_kernelInit();
    if (stat != GEM_SUCCESS) return stat;
    gem_nContext++;
  }
  
  cntxt = (gemCntxt *) gem_allocate(sizeof(gemCntxt));
  if (cntxt == NULL) return GEM_ALLOC;

  cntxt->magic = GEM_MCONTEXT;
  cntxt->model = NULL;
  cntxt->drep  = NULL;
  cntxt->attr  = NULL;

  *context = cntxt;
  return GEM_SUCCESS;
}


int
gem_staticModel(gemCntxt *cntxt, gemModel **model)
{
  gemID    gid;
  gemModel *mdl, *prev;

  *model = NULL;
  if (cntxt == NULL) return GEM_NULLOBJ;
  if (cntxt->magic != GEM_MCONTEXT) return GEM_BADCONTEXT;

  mdl = (gemModel *) gem_allocate(sizeof(gemModel));
  if (mdl == NULL) return GEM_ALLOC;

  gid.index      = 0;
  gid.ident.ptr  = NULL;

  mdl->magic     = GEM_MMODEL;
  mdl->handle    = gid;
  mdl->nonparam  = 1;
  mdl->server    = NULL;
  mdl->location  = NULL;
  mdl->modeler   = NULL;
  mdl->nBRep     = 0;
  mdl->BReps     = NULL;
  mdl->nParams   = 0;
  mdl->Params    = NULL;
  mdl->nBranches = 0;
  mdl->Branches  = NULL;
  mdl->attr      = NULL;
  mdl->prev      = (gemModel *) cntxt;
  mdl->next      = NULL;

  prev = cntxt->model;
  cntxt->model = mdl;
  if (prev != NULL) {
    mdl->next  = prev;
    prev->prev = cntxt->model;
  }

  *model = mdl;
  return GEM_SUCCESS;
}


int
gem_loadModel(gemCntxt *cntxt, /*@null@*/ char *server,
              char *location, gemModel **model)
{
  int stat;
  
  if (cntxt == NULL) return GEM_NULLOBJ;
  if (cntxt->magic != GEM_MCONTEXT) return GEM_BADCONTEXT;
  
  stat = gem_kernelLoad(cntxt, server, location, model);
  
  return stat;  
}


int
gem_terminate(gemCntxt *cntxt)
{
  if (cntxt == NULL) return GEM_NULLOBJ;
  if (cntxt->magic != GEM_MCONTEXT) return GEM_BADCONTEXT;

  while (cntxt->drep  != NULL) gem_destroyDRep(cntxt->drep);
  while (cntxt->model != NULL) gem_releaseModel(cntxt->model);
  gem_clrAttribs(&cntxt->attr);

  cntxt->magic = 0;
  gem_free(cntxt);
  gem_nContext--;
  
  if (gem_nContext == 0) gem_kernelClose();

  return GEM_SUCCESS;
}


int
gem_solidBoolean(gemBRep *breps, gemBRep *brept, /*@null@*/ double *xform, 
                 int type, gemModel **model)
{
  int     stat;
  gemBody *bodys, *bodyt;
    
  if (breps == NULL) return GEM_NULLOBJ;
  if (breps->magic != GEM_MBREP) return GEM_BADBREP;
  if (breps->omodel == NULL)     return GEM_NULLVALUE;
  bodys = breps->body;
  if (bodys->type != GEM_SOLID)  return GEM_SOLIDBODY;
  if (brept == NULL) return GEM_NULLOBJ;
  if (brept->magic != GEM_MBREP) return GEM_BADBREP;
  bodyt = brept->body;
  if (bodyt->type != GEM_SOLID)  return GEM_SOLIDBODY;

  *model = breps->omodel;
  stat   = gem_kernelSBO(bodys->handle, bodyt->handle, xform, type,
                         model);
  if (stat != GEM_SUCCESS) *model = NULL;
  
  return stat;
}


int
gem_getAttribute(void *obj, int etype, int eindex, int aindex, char **name,
                 int *atype, int *alen, int **integers, double **reals, 
                 char **string)
{
  gemCntxt *cntxt;
  gemModel *model;
  gemBRep  *brep;
  gemDRep  *drep;
  gemBody  *body;
  gemAttrs *attr;

  if (obj == NULL) return GEM_NULLOBJ;
  attr  = NULL;
    
  cntxt = (gemCntxt *) obj;
  model = (gemModel *) obj;
  brep  = (gemBRep *)  obj;
  drep  = (gemDRep *)  obj;  
  if (cntxt->magic == GEM_MCONTEXT) {
    attr = cntxt->attr;
  } else if (drep->magic == GEM_MDREP) {
    attr = drep->attr;
  } else if (brep->magic == GEM_MBREP) { 
    body = brep->body;  
    if (etype == GEM_BREP) {
      attr = body->attr;
    } else if (etype == GEM_SHELL) {
      if (body->type == GEM_WIRE) return GEM_WIREBODY;
      if ((eindex < 1) || (eindex > body->nshell)) return GEM_BADINDEX;
      attr = body->shells[eindex-1].attr;
    } else if (etype == GEM_FACE) {
      if (body->type == GEM_WIRE) return GEM_WIREBODY;
      if ((eindex < 1) || (eindex > body->nface)) return GEM_BADINDEX;
      attr = body->faces[eindex-1].attr;
    } else if (etype == GEM_LOOP) {
      if ((eindex < 1) || (eindex > body->nloop)) return GEM_BADINDEX;
      attr = body->loops[eindex-1].attr;
    } else if (etype == GEM_EDGE) {
      if ((eindex < 1) || (eindex > body->nedge)) return GEM_BADINDEX;
      attr = body->edges[eindex-1].attr;
    } else if (etype == GEM_NODE) {
      if ((eindex < 1) || (eindex > body->nnode)) return GEM_BADINDEX;
      attr = body->nodes[eindex-1].attr;
    }
  } else if (model->magic == GEM_MMODEL) {
    if (etype == GEM_BRANCH) {
      if ((eindex < 1) || (eindex > model->nBranches)) return GEM_BADINDEX;
      attr = model->Branches[eindex-1].attr;
    } else if (etype == GEM_PARAM) {
      if ((eindex < 1) || (eindex > model->nParams)) return GEM_BADINDEX;
      attr = model->Params[eindex-1].attr;
    } else if (etype == GEM_MODEL) {
      attr = model->attr;
    }
  }
  if (attr == NULL) return GEM_BADTYPE;

  return gem_getAttrib(attr, aindex, name, atype, alen, integers, reals, 
                       string);
}


int
gem_retAttribute(void *obj, int etype, int eindex, char *name, int *aindex, 
                 int *atype, int *alen, int **integers, double **reals, 
                 char **string)
{
  gemCntxt *cntxt;
  gemModel *model;
  gemBRep  *brep;
  gemDRep  *drep;
  gemBody  *body;
  gemAttrs *attr;

  if (obj == NULL) return GEM_NULLOBJ;
  attr  = NULL;
    
  cntxt = (gemCntxt *) obj;
  model = (gemModel *) obj;
  brep  = (gemBRep *)  obj;
  drep  = (gemDRep *)  obj;  
  if (cntxt->magic == GEM_MCONTEXT) {
    attr = cntxt->attr;
  } else if (drep->magic == GEM_MDREP) {
    attr = drep->attr;
  } else if (brep->magic == GEM_MBREP) { 
    body = brep->body;  
    if (etype == GEM_BREP) {
      attr = body->attr;
    } else if (etype == GEM_SHELL) {
      if (body->type == GEM_WIRE) return GEM_WIREBODY;
      if ((eindex < 1) || (eindex > body->nshell)) return GEM_BADINDEX;
      attr = body->shells[eindex-1].attr;
    } else if (etype == GEM_FACE) {
      if (body->type == GEM_WIRE) return GEM_WIREBODY;
      if ((eindex < 1) || (eindex > body->nface)) return GEM_BADINDEX;
      attr = body->faces[eindex-1].attr;
    } else if (etype == GEM_LOOP) {
      if ((eindex < 1) || (eindex > body->nloop)) return GEM_BADINDEX;
      attr = body->loops[eindex-1].attr;
    } else if (etype == GEM_EDGE) {
      if ((eindex < 1) || (eindex > body->nedge)) return GEM_BADINDEX;
      attr = body->edges[eindex-1].attr;
    } else if (etype == GEM_NODE) {
      if ((eindex < 1) || (eindex > body->nnode)) return GEM_BADINDEX;
      attr = body->nodes[eindex-1].attr;
    }
  } else if (model->magic == GEM_MMODEL) {
    if (etype == GEM_BRANCH) {
      if ((eindex < 1) || (eindex > model->nBranches)) return GEM_BADINDEX;
      attr = model->Branches[eindex-1].attr;
    } else if (etype == GEM_PARAM) {
      if ((eindex < 1) || (eindex > model->nParams)) return GEM_BADINDEX;
      attr = model->Params[eindex-1].attr;
    } else if (etype == GEM_MODEL) {
      attr = model->attr;
    }
  }
  if (attr == NULL) return GEM_BADTYPE;

  return gem_retAttrib(attr, name, aindex, atype, alen, integers, reals, 
                       string);
}


int
gem_setAttribute(void *obj, int etype, int eindex, char *name, int atype, 
                 int alen, /*@null@*/ int *integers, 
                 /*@null@*/ double *reals, /*@null@*/ char *string)
{
  int      stat;
  gemCntxt *cntxt;
  gemModel *model;
  gemBRep  *brep;
  gemDRep  *drep;
  gemBody  *body;
  gemAttrs **attr;
  gemID    handle;

  if (obj == NULL) return GEM_NULLOBJ;
  attr  = NULL;

  cntxt = (gemCntxt *) obj;
  model = (gemModel *) obj;
  brep  = (gemBRep *)  obj;
  drep  = (gemDRep *)  obj;  
  if (cntxt->magic == GEM_MCONTEXT) {
    attr = &cntxt->attr;

  } else if (drep->magic == GEM_MDREP) {
    attr = &drep->attr;
    
  } else if (brep->magic == GEM_MBREP) { 
    body = brep->body;  
    if (etype == GEM_BREP) {
      attr   = &body->attr;
      handle =  body->handle;
    } else if (etype == GEM_SHELL) {
      if (body->type == GEM_WIRE) return GEM_WIREBODY;
      if ((eindex < 1) || (eindex > body->nshell)) return GEM_BADINDEX;
      attr   = &body->shells[eindex-1].attr;
      handle =  body->shells[eindex-1].handle;
    } else if (etype == GEM_FACE) {
      if (body->type == GEM_WIRE) return GEM_WIREBODY;
      if ((eindex < 1) || (eindex > body->nface)) return GEM_BADINDEX;
      attr   = &body->faces[eindex-1].attr;
      handle =  body->faces[eindex-1].handle;
    } else if (etype == GEM_LOOP) {
      if ((eindex < 1) || (eindex > body->nloop)) return GEM_BADINDEX;
      attr   = &body->loops[eindex-1].attr;
      handle =  body->loops[eindex-1].handle;
    } else if (etype == GEM_EDGE) {
      if ((eindex < 1) || (eindex > body->nedge)) return GEM_BADINDEX;
      attr   = &body->edges[eindex-1].attr;
      handle =  body->edges[eindex-1].handle;
    } else if (etype == GEM_NODE) {
      if ((eindex < 1) || (eindex > body->nnode)) return GEM_BADINDEX;
      attr   = &body->nodes[eindex-1].attr;
      handle =  body->nodes[eindex-1].handle;
    }
    if (attr == NULL) return GEM_BADTYPE;
    stat = gem_kernelBRepAttr(handle, etype, name, atype, alen, integers, 
                              reals, string);
    if (stat != GEM_SUCCESS) return stat;
    
  } else if (model->magic == GEM_MMODEL) {
    if (etype == GEM_BRANCH) {
      if ((eindex < 1) || (eindex > model->nBranches)) return GEM_BADINDEX;
      attr   = &model->Branches[eindex-1].attr;
      handle =  model->Branches[eindex-1].handle;
      stat   = gem_kernelBranchAttr(handle, eindex, name, atype, alen, 
                                    integers, reals, string);
      if (stat != GEM_SUCCESS) return stat;
    } else if (etype == GEM_PARAM) {
      if ((eindex < 1) || (eindex > model->nParams)) return GEM_BADINDEX;
      attr = &model->Params[eindex-1].attr;
    } else if (etype == GEM_MODEL) {
      attr = &model->attr;
    }
  }
  if (attr == NULL) return GEM_BADTYPE;

  return gem_setAttrib(attr, name, atype, alen, integers, reals, string);
}


int
gem_getObject(void *obj, int *otype, int *nattr)
{
  gemCntxt *cntxt;
  gemModel *model;
  gemBRep  *brep;
  gemDRep  *drep;

  *nattr = 0;
  if (obj == NULL) return GEM_NULLOBJ;
  cntxt = (gemCntxt *) obj;
  model = (gemModel *) obj;
  brep  = (gemBRep *)  obj;
  drep  = (gemDRep *)  obj;

  if (cntxt->magic == GEM_MCONTEXT) {
    *otype = GEM_CONTEXT;
    if (cntxt->attr != NULL) *nattr = cntxt->attr->nattrs;
    
  } else if (drep->magic == GEM_MDREP) {
    *otype = GEM_DREP;
    if (drep->attr != NULL) *nattr = drep->attr->nattrs;
    
  } else if (brep->magic == GEM_MBREP) {
    *otype = GEM_BREP;
    if (brep->body->attr != NULL) *nattr = brep->body->attr->nattrs;
    
  } else if (model->magic == GEM_MMODEL) {
    *otype = GEM_MODEL;
    if (model->attr != NULL) *nattr = model->attr->nattrs;
  } else {
    return GEM_BADOBJECT;
  }

  return GEM_SUCCESS;
}


/*@observer@*/ const char *
gem_errorString(int code)
{
  const char *str;

  str = gem_kernelError(code);
  if (str != NULL) return str;

  if (code == GEM_SUCCESS)       return "GEM: Success";
  if (code == GEM_BADDREP)       return "GEM: Bad DRep Object";
  if (code == GEM_BADFACEID)     return "GEM: Bad Face ID";
  if (code == GEM_BADBOUNDINDEX) return "GEM: Bad Bound Index";
  if (code == GEM_BADVSETINDEX)  return "GEM: Bad VertexSet Index";
  if (code == GEM_BADRANK)       return "GEM: Bad Rank";
  if (code == GEM_BADDSETNAME)   return "GEM: Bad DataSet Name";
  if (code == GEM_MISSDISPLACE)  return "GEM: What is this and why is it here?";
  if (code == GEM_NOTFOUND)      return "GEM: Not Found";
  if (code == GEM_BADMODEL)      return "GEM: Bad Model Object";
  if (code == GEM_BADCONTEXT)    return "GEM: Bad Context";
  if (code == GEM_BADBREP)       return "GEM: Bad BRep Object";
  if (code == GEM_BADINDEX)      return "GEM: Bad Index";
  if (code == GEM_NOTCHANGED)    return "GEM: Not Changed";
  if (code == GEM_ALLOC)         return "GEM: Memory Allocation Error";
  if (code == GEM_BADTYPE)       return "GEM: Bad Type";
  if (code == GEM_NULLVALUE)     return "GEM: NULL Value";
  if (code == GEM_NULLNAME)      return "GEM: NULL Name";
  if (code == GEM_NULLOBJ)       return "GEM: NULL Object";
  if (code == GEM_BADOBJECT)     return "GEM: Bad Object";
  if (code == GEM_WIREBODY)      return "GEM: Wire Body (Un)Expected";
  if (code == GEM_SOLIDBODY)     return "GEM: Solid Body (Un)Expected";
  if (code == GEM_NOTESSEL)      return "GEM: Not a Tessellation";
  if (code == GEM_BADVALUE)      return "GEM: Bad Value";
  if (code == GEM_DUPLICATE)     return "GEM: Duplicate";
  if (code == GEM_BADINVERSE)    return "GEM: Bad Matrix Inverse";
  if (code == GEM_NOTPARAMBND)   return "GEM: Bound has Not Been Paramaterized";
  if (code == GEM_NOTCONNECT)    return "GEM: VertexSet is Not Connected";
  if (code == GEM_NOTPARMTRIC)   return "GEM: Model is (Not) Parametric";
  if (code == GEM_READONLYERR)   return "GEM: Read-Only Error";
  if (code == GEM_FIXEDLEN)      return "GEM: Parameter is Fixed Length";
  if (code == GEM_ASSEMBLY)      return "GEM: Assembly Error";
  if (code == GEM_BADNAME)       return "GEM: Bad Name";
  if (code == GEM_UNSUPPORTED)   return "GEM: Unsupported";

  return "Unknown Error!";
}
