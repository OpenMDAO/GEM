/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             DRep Functions
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
#include "kernel.h"


static int  nReserved    = 6;
static char *reserved[6] = {"xyz", "uv", "d1", "d2", "curv", "inside"};
static int  rreserved[6] = {  3,    2,    6,    9,      8,      1 };



static int
gem_indexName(gemDRep *drep, int bound, int vs, char *name)
{
  int i;

  for (i = 0; i < drep->bound[bound-1].VSet[vs-1].nSets; i++)
    if (strcmp(name, drep->bound[bound-1].VSet[vs-1].Sets[i].name) == 0)
      return i+1;
      
  return 0;
}


static void
gem_freeInt2D(gemInt2D *interp)
{
  if (interp == NULL) return;
  
  gem_free(interp->interp);
  if (interp->uvmap != NULL) gem_free(interp->uvmap);
  gem_free(interp);
}


int
gem_newConn(gemConn **conn)
{
  gemConn *conns;

  *conn = conns = (gemConn *) gem_allocate(sizeof(gemConn));
  if (conns == NULL) return GEM_ALLOC;

  conns->magic     = GEM_MCONN;
  conns->nTris     = 0;
  conns->Tris      = NULL;
  conns->tNei      = NULL;
  conns->nQuad     = 0;
  conns->Quad      = NULL;
  conns->qNei      = NULL;
  conns->meshSz[0] = 0;
  conns->meshSz[1] = 0;
  conns->nSides    = 0;
  conns->sides     = NULL;

  return GEM_SUCCESS;
}


int
gem_freeConn(/*@only@*/ gemConn *conn)
{
  if (conn == NULL) return GEM_NULLOBJ;
  if (conn->magic != GEM_MCONN) return GEM_BADOBJECT;

  if (conn->nTris > 0) {
    gem_free(conn->Tris);
    gem_free(conn->tNei);
  }
  if (conn->nQuad > 0) {
    gem_free(conn->Quad);
    gem_free(conn->qNei);
  }
  gem_free(conn->sides);
  gem_free(conn);

  return GEM_SUCCESS;
}


static void
gem_freeVsets(gemBound bound)
{
  int i, j, n;
  
  for (i = 0; i < bound.nVSet; i++) {

    if (bound.VSet[i].nFaces != 0) {
      n = bound.VSet[i].nFaces;
      if (n == -1) n = 1;
      for (j = 0; j < n; j++) {
        gem_free(bound.VSet[i].faces[j].xyz);
        gem_free(bound.VSet[i].faces[j].uv);
        gem_free(bound.VSet[i].faces[j].guv);
        gem_freeConn(bound.VSet[i].faces[j].conn);
      }
      gem_free(bound.VSet[i].faces);
    }

    for (j = 0; j < bound.VSet[i].nSets; j++) {
      gem_free(bound.VSet[i].Sets[j].name);
      gem_free(bound.VSet[i].Sets[j].data);
      if (bound.VSet[i].Sets[j].interp != NULL)
        gem_freeInt2D(bound.VSet[i].Sets[j].interp);
    }
    if (bound.VSet[i].Sets != NULL) gem_free(bound.VSet[i].Sets);

  }
  gem_free(bound.VSet);
}


static void
gem_xform(gemBRep *brep, int npts, double *pts)
{
  int    i;
  double *xform, xyz[3];
  
  if (brep->ibranch == 0) return;
  xform = brep->xform;
  
  for (i = 0; i < npts; i++) {
    xyz[0]     = pts[3*i  ];
    xyz[1]     = pts[3*i+1];
    xyz[2]     = pts[3*i+2];
    pts[3*i  ] = xform[ 0]*xyz[0] + xform[ 1]*xyz[1] +
                 xform[ 2]*xyz[2] + xform[ 3];
    pts[3*i+1] = xform[ 4]*xyz[0] + xform[ 5]*xyz[1] +
                 xform[ 6]*xyz[2] + xform[ 7];
    pts[3*i+2] = xform[ 8]*xyz[0] + xform[ 9]*xyz[1] +
                 xform[10]*xyz[2] + xform[11];
  }
}


int
gem_newDRep(gemModel *model, gemDRep **drep)
{
  int      i;
  gemCntxt *cntxt;
  gemModel *prv;
  gemDRep  *drp, *prev;
  gemTRep  *trep;

  *drep = NULL;
  if (model == NULL) return GEM_NULLOBJ;
  if (model->magic != GEM_MMODEL) return GEM_BADMODEL;

  /* find the context */

  cntxt = NULL;
  prv   = model->prev;
  while (cntxt == NULL) {
    if  (prv  == NULL) return GEM_BADCONTEXT;
    if ((prv->magic != GEM_MMODEL) && 
        (prv->magic != GEM_MCONTEXT)) return GEM_BADOBJECT;
    if  (prv->magic == GEM_MCONTEXT)  cntxt = (gemCntxt *) prv;
    if  (prv->magic == GEM_MMODEL)    prv   = prv->prev;
  }
  
  /* make the DRep */

  trep = (gemTRep *) gem_allocate(model->nBRep*sizeof(gemTRep));
  if (trep == NULL) return GEM_ALLOC;
  for (i = 0; i < model->nBRep; i++) {
    trep[i].nFaces = 0;
    trep[i].Faces  = NULL;
  }
  drp = (gemDRep *) gem_allocate(sizeof(gemDRep));
  if (drp == NULL) {
    gem_free(trep);
    return GEM_ALLOC;
  }
  
  drp->magic  = GEM_MDREP;
  drp->model  = model;
  drp->nIDs   = 0;
  drp->IDs    = NULL;
  drp->nBReps = model->nBRep;
  drp->TReps  = trep;
  drp->nBound = 0;
  drp->bound  = NULL;
  drp->attr   = NULL;
  drp->prev   = (gemDRep *) cntxt;
  drp->next   = NULL;

  prev = cntxt->drep;
  cntxt->drep = drp;
  if (prev != NULL) {
    drp->next  = prev;
    prev->prev = cntxt->drep;
  }
  
  *drep = drp;
  return GEM_SUCCESS;
}


int
gem_tesselDRep(gemDRep *drep, int brep, double angle, double mxside,
               double sag)
{
  int      i, j, k, stat;
  gemModel *model;
  
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((brep < 0) || (brep > drep->nBReps)) return GEM_BADINDEX;

  model = drep->model;
  if (brep != 0) {
    stat = gem_kernelTessel(model->BReps[brep-1]->body, angle, mxside, sag,
                            drep, brep);
    if (stat == GEM_SUCCESS)
      for (k = 0; k < drep->TReps[brep-1].nFaces; k++)
        gem_xform(model->BReps[brep-1], drep->TReps[brep-1].Faces[k].npts,
                  drep->TReps[brep-1].Faces[k].xyz);
  } else {
    stat = -9999;
    for (i = 0; i < drep->nBReps; i++) {
      j = gem_kernelTessel(model->BReps[i]->body, angle, mxside, sag,
                           drep, i+1);
      if (j == GEM_SUCCESS)
        for (k = 0; k < drep->TReps[i].nFaces; k++)
          gem_xform(model->BReps[i], drep->TReps[i].Faces[k].npts,
                    drep->TReps[i].Faces[k].xyz);
      if (j > stat) stat = j;
    }
  }

  return stat;
}


int
gem_getTessel(gemDRep *drep, gemPair bface, int *nvrt, double **xyz,
              double **uv, gemConn **conn)
{
  *nvrt = 0;
  *xyz  = NULL;
  *uv   = NULL;
  *conn = NULL;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;

  if (drep->TReps == NULL) return GEM_NOTESSEL;
  if ((bface.BRep  < 1) || (bface.BRep > drep->nBReps)) return GEM_BADINDEX;
  if (drep->TReps[bface.BRep-1].Faces == NULL) return GEM_NOTESSEL;
  if ((bface.index < 1) ||
      (bface.index > drep->TReps[bface.BRep-1].nFaces)) return GEM_BADINDEX;

  *nvrt = drep->TReps[bface.BRep-1].Faces[bface.index-1].npts;
  *xyz  = drep->TReps[bface.BRep-1].Faces[bface.index-1].xyz;
  *uv   = drep->TReps[bface.BRep-1].Faces[bface.index-1].uv;
  *conn = drep->TReps[bface.BRep-1].Faces[bface.index-1].conn;
  
  return GEM_SUCCESS;
}


int
gem_destroyDRep(gemDRep *drep)
{
  int      i, j;
  gemCntxt *cntxt;
  gemDRep  *prev, *next;

  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;

  /* find the context */

  cntxt = NULL;
  prev  = drep->prev;
  while (cntxt == NULL) {
    if  (prev  == NULL) return GEM_BADCONTEXT;
    if ((prev->magic != GEM_MDREP) && 
        (prev->magic != GEM_MCONTEXT)) return GEM_BADOBJECT;
    if  (prev->magic == GEM_MCONTEXT)  cntxt = (gemCntxt *) prev;
    if  (prev->magic == GEM_MDREP)     prev  = prev->prev;
  }

  /* fix up the linked list */

  prev = drep->prev;
  next = drep->next;
  if (prev->magic == GEM_MDREP) {
    prev->next  = next;
  } else {
    cntxt->drep = next;
  }
  if (next != NULL) next->prev = prev;
  
  /* remove stuff */
  
  for (i = 0; i < drep->nIDs; i++) gem_free(drep->IDs[i]);
  gem_free(drep->IDs);

  for (i = 0; i < drep->nBReps; i++) {
    if (drep->TReps[i].Faces != NULL) {
      for (j = 0; j < drep->TReps[i].nFaces; j++) {
        gem_free(drep->TReps[i].Faces[j].xyz);
        gem_free(drep->TReps[i].Faces[j].uv);
        gem_free(drep->TReps[i].Faces[j].guv);
        gem_freeConn(drep->TReps[i].Faces[j].conn);
      }
      gem_free(drep->TReps[i].Faces);
    }
  }
  gem_free(drep->TReps);

  if (drep->bound != NULL) {
    for (i = 0; i < drep->nBound; i++) {
      gem_free(drep->bound[i].IDs);
      gem_free(drep->bound[i].indices);
      if (drep->bound[i].surface != NULL) 
        gem_freeInt2D(drep->bound[i].surface);
      gem_freeVsets(drep->bound[i]);
    }
    gem_free(drep->bound);
  }

  gem_clrAttribs(&drep->attr);

  drep->magic = 0;
  gem_free(drep);

  return GEM_SUCCESS;
}


int
gem_clrDReps(gemModel *model, int phase)
{
  int      i, j, hit;
  gemModel *prev;
  gemDRep  *drep;
  gemCntxt *cntxt;
  gemTRep  *trep;
  
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
  
  /* cleanup/refill any DReps attached to this model */

  do {
    hit  = 0;
    drep = cntxt->drep;
    while (drep != NULL) {
      if (drep->model == model) {
        if (phase == 0) {
        
          /* cleanup */
          for (i = 0; i < drep->nBReps; i++) {
            if (drep->TReps[i].Faces != NULL) {
              for (j = 0; j < drep->TReps[i].nFaces; j++) {
                gem_free(drep->TReps[i].Faces[j].xyz);
                gem_free(drep->TReps[i].Faces[j].uv);
                gem_free(drep->TReps[i].Faces[j].guv);
                gem_freeConn(drep->TReps[i].Faces[j].conn);
              }
              gem_free(drep->TReps[i].Faces);
            }
          }
          gem_free(drep->TReps);
          drep->nBReps = 0;
          drep->TReps  = NULL;

          if (drep->bound != NULL) {
            for (i = 0; i < drep->nBound; i++) {
              gem_free(drep->bound[i].IDs);
              gem_free(drep->bound[i].indices);
              if (drep->bound[i].surface != NULL) 
                gem_freeInt2D(drep->bound[i].surface);
              gem_freeVsets(drep->bound[i]);
            }
            gem_free(drep->bound);
            drep->nBound = 0;
            drep->bound  = NULL;
          }
        } else {
        
          /* repopulate */
          trep = (gemTRep *) gem_allocate(model->nBRep*sizeof(gemTRep));
          if (trep != NULL) {
            for (i = 0; i < model->nBRep; i++) {
              trep[i].nFaces = 0;
              trep[i].Faces  = NULL;
            }
            drep->nBReps = model->nBRep;
            drep->TReps  = trep;  
          }
        }
        hit++;
        break;
      }
      drep = drep->next;
    }
  } while (hit != 0);

  return GEM_SUCCESS;
}


int
gem_createBound(gemDRep *drep, int nIDs, char **IDs, int *bound)
{
  int      i, j, k, n, *iIDs;
  char     **ctmp;
  gemPair  *indices;
  gemBound *temp;
  gemModel *model;
  gemBody  *body;

  *bound = 0;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if (nIDs <= 0) return GEM_BADVALUE;
  for (i = 0; i < nIDs-1; i++)
    for (j = i+1; j < nIDs; j++)
      if (strcmp(IDs[i], IDs[j]) == 0) return GEM_DUPLICATE;

  /* update the collection of IDs in the DRep */
  iIDs = (int *) gem_allocate(nIDs*sizeof(int));
  if (iIDs == NULL) return GEM_ALLOC;
  for (i = 0; i < nIDs; i++) iIDs[i] = 0;
  for (i = 0; i < nIDs; i++)
    for (j = 0; j < drep->nIDs; j++)
      if (strcmp(IDs[i], drep->IDs[j]) == 0) {
        iIDs[i] = j+1;
        break;
      }
  for (n = i = 0; i < nIDs; i++)
    if (iIDs[i] == 0) n++;
  if (n != 0) {
    if (drep->nIDs == 0) {
      drep->IDs = (char **) gem_allocate(nIDs*sizeof(char *));
      if (drep->IDs == NULL) {
        gem_free(iIDs);
        return GEM_ALLOC;
      }
      n = nIDs;
    } else {
      n   += drep->nIDs;
      ctmp = (char **) gem_reallocate(drep->IDs, n*sizeof(char *));
      if (ctmp == NULL) {
        gem_free(iIDs);
        return GEM_ALLOC;
      }
      drep->IDs = ctmp;
    }
    j = drep->nIDs;
    for (i = 0; i < nIDs; i++)
      if (iIDs[i] == 0) {
        drep->IDs[j] = gem_strdup(IDs[i]);
        j++;
        iIDs[i] = j;
      }
    drep->nIDs = n;
  }

  /* get the BRep/Face pairs */
  indices = (gemPair *) gem_allocate(nIDs*sizeof(gemPair));
  if (indices == NULL) {
    gem_free(iIDs);
    return GEM_ALLOC;
  }
  for (i = 0; i < nIDs; i++)
    indices[i].BRep = indices[i].index = 0;
  model = drep->model;
  for (i = 0; i < nIDs; i++)
    for (j = 0; j < model->nBRep; j++) {
      body = model->BReps[j]->body;
      for (k = 0; k < body->nface; k++)
        if (strcmp(IDs[i], body->faces[k].ID) == 0) {
          indices[i].BRep  = j+1;
          indices[i].index = k+1;
          break;
        }
      if (indices[i].BRep != 0) break;
    }
    
  /* make the new bound */
  if (drep->nBound == 0) {
    drep->bound = (gemBound *) gem_allocate(sizeof(gemBound));
    if (drep->bound == NULL) {
      gem_free(indices);
      gem_free(iIDs);
      return GEM_ALLOC;
    }
  } else {
    temp = (gemBound *) gem_reallocate( drep->bound,
                                       (drep->nBound+1)*sizeof(gemBound));
    if (temp == NULL) {
      gem_free(indices);
      gem_free(iIDs);
      return GEM_ALLOC;
    }
    drep->bound = temp;
  }
  
  drep->nBound++;
  *bound = drep->nBound;

  /* fill it in */
  drep->bound[*bound-1].nIDs     = nIDs;
  drep->bound[*bound-1].IDs      = iIDs;
  drep->bound[*bound-1].indices  = indices;
  drep->bound[*bound-1].surface  = NULL;
  drep->bound[*bound-1].uvbox[0] = 0.0;
  drep->bound[*bound-1].uvbox[1] = 0.0;
  drep->bound[*bound-1].uvbox[2] = 0.0;
  drep->bound[*bound-1].uvbox[3] = 0.0;
  drep->bound[*bound-1].nVSet    = 0;
  drep->bound[*bound-1].VSet     = NULL;
  
  return GEM_SUCCESS;
}


int
gem_extendBound(gemDRep *drep, int bound, int nIDs, char **IDs)
{
  int      i, j, k, n, *iIDs;
  char     **ctmp;
  gemPair  *indices;
  gemModel *model;
  gemBody  *body;

  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if (nIDs <= 0) return GEM_BADVALUE;
  for (i = 0; i < nIDs-1; i++)
    for (j = i+1; j < nIDs; j++)
      if (strcmp(IDs[i], IDs[j]) == 0) return GEM_DUPLICATE;

  /* update the collection of IDs in the DRep */
  n    = nIDs + drep->bound[bound-1].nIDs;
  iIDs = (int *) gem_allocate(n*sizeof(int));
  if (iIDs == NULL) return GEM_ALLOC;
  for (i = 0; i < nIDs; i++) iIDs[i] = 0;
  for (i = 0; i < nIDs; i++)
    for (j = 0; j < drep->nIDs; j++)
      if (strcmp(IDs[i], drep->IDs[j]) == 0) {
        iIDs[i] = j+1;
        break;
      }
  for (n = i = 0; i < nIDs; i++)
    if (iIDs[i] == 0) n++;
  if (n != 0) {
    if (drep->nIDs == 0) {
      drep->IDs = (char **) gem_allocate(nIDs*sizeof(char *));
      if (drep->IDs == NULL) {
        gem_free(iIDs);
        return GEM_ALLOC;
      }
      n = nIDs;
    } else {
      n   += drep->nIDs;
      ctmp = (char **) gem_reallocate(drep->IDs, n*sizeof(char *));
      if (ctmp == NULL) {
        gem_free(iIDs);
        return GEM_ALLOC;
      }
      drep->IDs = ctmp;
    }
    j = drep->nIDs;
    for (i = 0; i < nIDs; i++)
      if (iIDs[i] == 0) {
        drep->IDs[j] = gem_strdup(IDs[i]);
        j++;
        iIDs[i] = j;
      }
    drep->nIDs = n;
  }
  n = drep->bound[bound-1].nIDs;
  for (i = 0; i < nIDs; i++)
    iIDs[i+n] = iIDs[i];
  for (i = 0; i < n; i++)
    iIDs[i] = drep->bound[bound-1].IDs[i];

  /* get the BRep/Face pairs */
  n       = nIDs + drep->bound[bound-1].nIDs;
  indices = (gemPair *) gem_allocate(n*sizeof(gemPair));
  if (indices == NULL) {
    gem_free(iIDs);
    return GEM_ALLOC;
  }
  for (i = 0; i < nIDs; i++)
    indices[i].BRep = indices[i].index = 0;
  model = drep->model;
  for (i = 0; i < nIDs; i++)
    for (j = 0; j < model->nBRep; j++) {
      body = model->BReps[j]->body;
      for (k = 0; k < body->nface; k++)
        if (strcmp(IDs[i], body->faces[k].ID) == 0) {
          indices[i].BRep  = j+1;
          indices[i].index = k+1;
          break;
        }
      if (indices[i].BRep != 0) break;
    }
    
  /* merge the new data with the old */
  n = drep->bound[bound-1].nIDs;
  for (i = 0; i < nIDs; i++)
    indices[i+n] = indices[i];
  for (i = 0; i < n; i++)
    indices[i] = drep->bound[bound-1].indices[i];

  if (drep->bound[bound-1].surface != NULL) 
    gem_freeInt2D(drep->bound[bound-1].surface);
  gem_freeVsets(drep->bound[bound-1]);
  gem_free(drep->bound[bound-1].IDs);
  gem_free(drep->bound[bound-1].indices);

  drep->bound[bound-1].nIDs     += nIDs;
  drep->bound[bound-1].IDs       = iIDs;
  drep->bound[bound-1].indices   = indices;
  drep->bound[bound-1].surface   = NULL;
  drep->bound[bound-1].uvbox[0]  = 0.0;
  drep->bound[bound-1].uvbox[1]  = 0.0;
  drep->bound[bound-1].uvbox[2]  = 0.0;
  drep->bound[bound-1].uvbox[3]  = 0.0;
  drep->bound[bound-1].nVSet     = 0;
  drep->bound[bound-1].VSet      = NULL;  
  
  return GEM_SUCCESS;
}


int
gem_copyDRep(gemDRep *src, gemModel *model, gemDRep **drep, int *nIDx,
             char ***IDx)
{
  int      i, j, k, n, stat, bound;
  char     **IDs, **list;
  gemCntxt *cntxt;
  gemModel *prv;
  gemDRep  *drp, *prev;
  gemTRep  *trep;

  *drep = NULL;
  *nIDx = 0;
  *IDx  = NULL;
  if (src == NULL) return GEM_NULLOBJ;
  if (src->magic != GEM_MDREP) return GEM_BADDREP;
  if (model == NULL) return GEM_NULLOBJ;
  if (model->magic != GEM_MMODEL) return GEM_BADMODEL;
  if (src->nIDs <= 0) return GEM_NULLVALUE;

  /* find the context */

  cntxt = NULL;
  prv   = model->prev;
  while (cntxt == NULL) {
    if  (prv  == NULL) return GEM_BADCONTEXT;
    if ((prv->magic != GEM_MMODEL) && 
        (prv->magic != GEM_MCONTEXT)) return GEM_BADOBJECT;
    if  (prv->magic == GEM_MCONTEXT)  cntxt = (gemCntxt *) prv;
    if  (prv->magic == GEM_MMODEL)    prv   = prv->prev;
  }
  
  /* make the DRep */

  trep = (gemTRep *) gem_allocate(model->nBRep*sizeof(gemTRep));
  if (trep == NULL) return GEM_ALLOC;
  for (i = 0; i < model->nBRep; i++) {
    trep[i].nFaces = 0;
    trep[i].Faces  = NULL;
  }
  drp = (gemDRep *) gem_allocate(sizeof(gemDRep));
  if (drp == NULL) {
    gem_free(trep);
    return GEM_ALLOC;
  }
  IDs = (char **) gem_allocate(src->nIDs*sizeof(char *));
  if (IDs == NULL) {
    gem_free(drp);
    gem_free(trep);
    return GEM_ALLOC;
  }
  for (i = 0; i < src->nIDs; i++) IDs[i] = gem_strdup(src->IDs[i]);
  
  drp->magic  = GEM_MDREP;
  drp->model  = model;
  drp->nIDs   = src->nIDs;
  drp->IDs    = IDs;
  drp->nBReps = model->nBRep;
  drp->TReps  = trep;
  drp->nBound = 0;
  drp->bound  = NULL;
  drp->attr   = NULL;
  drp->prev   = (gemDRep *) cntxt;
  drp->next   = NULL;

  prev = cntxt->drep;
  cntxt->drep = drp;
  if (prev != NULL) {
    drp->next  = prev;
    prev->prev = cntxt->drep;
  }

  /* copy the Bounds */

  stat = 0;
  for (i = 0; i < src->nBound; i++)
    if (src->bound[i].nIDs > stat) stat = src->bound[i].nIDs;
  list = (char **) gem_allocate(stat*sizeof(char *));
  if (list == NULL) {
    gem_destroyDRep(drp);
    return GEM_ALLOC;
  }

  for (n = i = 0; i < src->nBound; i++) {
    for (j = 0; j < src->bound[i].nIDs; j++)
      list[j] = IDs[src->bound[i].IDs[j]-1];
    stat = gem_createBound(drp, src->bound[i].nIDs, list, &bound);
    if (stat != GEM_SUCCESS) {
      gem_free(list);
      gem_destroyDRep(drp);
      return stat;
    }
    if (drp->bound == NULL) {
      gem_free(list);
      gem_destroyDRep(drp);
      return GEM_NOTFOUND;
    }
    for (j = 0; j < drp->bound[i].nIDs; j++)
      if (drp->bound[i].indices[j].BRep == 0) n++;
  }
  gem_free(list);

  /* set up list of unused IDs */
  
  if (n != 0) {
    if (drp->bound == NULL) {
      gem_destroyDRep(drp);
      return GEM_NOTFOUND;
    }
    list = (char **) gem_allocate(n*sizeof(char *));
    if (list == NULL) {
      gem_destroyDRep(drp);
      return GEM_ALLOC;
    }
    for (n = i = 0; i < drp->nBound; i++) 
      for (j = 0; j < drp->bound[i].nIDs; j++)
        if (drp->bound[i].indices[j].BRep == 0) {
          list[n] = IDs[drp->bound[i].IDs[j]-1];
          for (k = 0; k < n; k++)
            if (strcmp(list[k],list[n]) == 0) break;
          if (k == n) n++;
        }
    *IDx  = list;
    *nIDx = n;
  }
  
  *drep = drp;
  return GEM_SUCCESS;
}


int
gem_createVset(gemDRep *drep, int bound, int *vs)
{
  int     n;
  gemVSet *temp;
  
  *vs = 0;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  n = drep->bound[bound-1].nVSet;
  
  /* make the new bound */
  if (n == 0) {
    drep->bound[bound-1].VSet = (gemVSet *) gem_allocate(sizeof(gemVSet));
    if (drep->bound[bound-1].VSet == NULL) return GEM_ALLOC;
  } else {
    temp = (gemVSet *) gem_reallocate(drep->bound[bound-1].VSet, 
                                      (n+1)*sizeof(gemVSet));
    if (temp == NULL) return GEM_ALLOC;
    drep->bound[bound-1].VSet = temp;
  }
  
  drep->bound[bound-1].nVSet++;
  *vs = n+1;
  drep->bound[bound-1].VSet[n].npts   = 0;
  drep->bound[bound-1].VSet[n].nFaces = 0;
  drep->bound[bound-1].VSet[n].faces  = NULL;
  drep->bound[bound-1].VSet[n].nSets  = 0;
  drep->bound[bound-1].VSet[n].Sets   = NULL;

  return GEM_SUCCESS;
}


int
gem_emptyVset(gemDRep *drep, int bound, int vs)
{
  int j;

  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet)) return GEM_BADVSETINDEX;
  if (drep->bound[bound-1].VSet[vs-1].nFaces == -1) return GEM_BADVSETINDEX;

  if (drep->bound[bound-1].VSet[vs-1].nFaces > 0) {
    for (j = 0; j < drep->bound[bound-1].VSet[vs-1].nFaces; j++) {
      gem_free(drep->bound[bound-1].VSet[vs-1].faces[j].xyz);
      gem_free(drep->bound[bound-1].VSet[vs-1].faces[j].uv);
      gem_free(drep->bound[bound-1].VSet[vs-1].faces[j].guv);
      gem_freeConn(drep->bound[bound-1].VSet[vs-1].faces[j].conn);
    }
    gem_free(drep->bound[bound-1].VSet[vs-1].faces);
    drep->bound[bound-1].VSet[vs-1].nFaces = 0;
    drep->bound[bound-1].VSet[vs-1].faces  = NULL;
  }

  for (j = 0; j < drep->bound[bound-1].VSet[vs-1].nSets; j++) {
    gem_free(drep->bound[bound-1].VSet[vs-1].Sets[j].name);
    gem_free(drep->bound[bound-1].VSet[vs-1].Sets[j].data);
    if (drep->bound[bound-1].VSet[vs-1].Sets[j].interp != NULL)
      gem_freeInt2D(drep->bound[bound-1].VSet[vs-1].Sets[j].interp);
  }
  gem_free(drep->bound[bound-1].VSet[vs-1].Sets);
  drep->bound[bound-1].VSet[vs-1].nSets = 0;
  drep->bound[bound-1].VSet[vs-1].Sets  = NULL;
  drep->bound[bound-1].VSet[vs-1].npts  = 0;
  
  return GEM_SUCCESS;
}


int
gem_addVset(gemDRep *drep, int bound, int vs, gemPair bface, int npts,
            double *xyz, double *uv, gemConn *conn)
{
  int      i, j;
  double   *xyzs, *uvs;
  char     *ID;
  gemModel *model;
  gemBody  *body;
  gemConn  *conns;
  gemTface *temp;

  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if (conn == NULL) return GEM_NULLOBJ;
  if (conn->magic != GEM_MCONN) return GEM_BADOBJECT;

  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet)) return GEM_BADVSETINDEX;
  if (drep->bound[bound-1].VSet[vs-1].nFaces == -1) return GEM_BADTYPE;
  model = drep->model;
  if ((bface.BRep < 1) || (bface.BRep > model->nBRep)) return GEM_BADINDEX;
  body = model->BReps[bface.BRep-1]->body;
  if ((bface.index < 1) || (bface.index > body->nface)) return GEM_BADINDEX;
  if (npts <= 0) return GEM_BADVALUE;
  if ((xyz == NULL) || (uv == NULL)) return GEM_NULLVALUE;

  ID   = body->faces[bface.index-1].ID;
  xyzs = (double *) gem_allocate(npts*3*sizeof(double));
  if (xyzs == NULL) return GEM_ALLOC;
  for (i = 0; i < 3*npts; i++) xyzs[i] = xyz[i];
  uvs = (double *) gem_allocate(npts*2*sizeof(double));
  if (uvs == NULL) {
    gem_free(xyzs);
    return GEM_ALLOC;
  }
  for (i = 0; i < 2*npts; i++) uvs[i] = uv[i];

  /* copy connectivity */
  conns = (gemConn *) gem_allocate(sizeof(gemConn));
  if (conns == NULL) {
    gem_free(uvs);
    gem_free(xyzs);
    return GEM_ALLOC;
  }
  conns->magic     = GEM_MCONN;
  conns->nTris     = conn->nTris;
  conns->Tris      = NULL;
  conns->tNei      = NULL;
  conns->nQuad     = conn->nQuad;
  conns->Quad      = NULL;
  conns->qNei      = NULL;
  conns->meshSz[0] = conn->meshSz[0];
  conns->meshSz[1] = conn->meshSz[1];
  conns->nSides    = conn->nSides;
  conns->sides     = NULL;
  
  if (conn->nTris != 0) {
    conns->Tris = (int *) gem_allocate(3*conn->nTris*sizeof(int));
    conns->tNei = (int *) gem_allocate(3*conn->nTris*sizeof(int));
    if ((conns->Tris == NULL) || (conns->tNei == NULL)) {
      gem_freeConn(conns);
      gem_free(uvs);
      gem_free(xyzs);
      return GEM_ALLOC;      
    }
    for (i = 0; i < 3*conn->nTris; i++) {
      conns->Tris[i] = conn->Tris[i];
      conns->tNei[i] = conn->tNei[i];
    }
  }
  if (conn->nQuad != 0) {
    conns->Quad = (int *) gem_allocate(4*conn->nQuad*sizeof(int));
    conns->qNei = (int *) gem_allocate(4*conn->nQuad*sizeof(int));
    if ((conns->Quad == NULL) || (conns->qNei == NULL)) {
      gem_freeConn(conns);
      gem_free(uvs);
      gem_free(xyzs);
      return GEM_ALLOC;      
    }
    for (i = 0; i < 4*conn->nQuad; i++) {
      conns->Quad[i] = conn->Quad[i];
      conns->qNei[i] = conn->qNei[i];
    }
  }
  if (conn->nSides != 0) {
    conns->sides = (gemSide *) gem_allocate(conn->nSides*sizeof(gemSide));
    if (conns->sides == NULL) {
      gem_freeConn(conns);
      gem_free(uvs);
      gem_free(xyzs);
      return GEM_ALLOC; 
    }
    for (i = 0; i < conn->nSides; i++) 
      conns->sides[i] = conn->sides[i];
  }
  
  /* add this to the VertexSet */
  
  if (drep->bound[bound-1].VSet[vs-1].nFaces == 0) {
    drep->bound[bound-1].VSet[vs-1].faces = (gemTface *) 
                                            gem_allocate(sizeof(gemTface));
    if (drep->bound[bound-1].VSet[vs-1].faces == NULL) {
      gem_freeConn(conns);
      gem_free(uvs);
      gem_free(xyzs);
      return GEM_ALLOC; 
    }
    i = 0;
  } else {
    i    = drep->bound[bound-1].VSet[vs-1].nFaces;
    temp = (gemTface *) 
           gem_reallocate(drep->bound[bound-1].VSet[vs-1].faces,
                          (i+1)*sizeof(gemTface));
    if (temp == NULL) {
      gem_freeConn(conns);
      gem_free(uvs);
      gem_free(xyzs);
      return GEM_ALLOC; 
    }
    drep->bound[bound-1].VSet[vs-1].faces = temp;
  }
  
  drep->bound[bound-1].VSet[vs-1].faces[i].index = bface;
  drep->bound[bound-1].VSet[vs-1].faces[i].ID    = 0;
  drep->bound[bound-1].VSet[vs-1].faces[i].npts  = npts;
  drep->bound[bound-1].VSet[vs-1].faces[i].xyz   = xyzs;
  drep->bound[bound-1].VSet[vs-1].faces[i].uv    = uvs;
  drep->bound[bound-1].VSet[vs-1].faces[i].guv   = NULL;
  drep->bound[bound-1].VSet[vs-1].faces[i].conn  = conns;
  drep->bound[bound-1].VSet[vs-1].nFaces++;
  drep->bound[bound-1].VSet[vs-1].npts += npts;
  
  for (j = 0; j < drep->nIDs; j++)
    if (strcmp(ID, drep->IDs[j]) == 0) {
      drep->bound[bound-1].VSet[vs-1].faces[i].ID = j+1;
      break;
    }

  return GEM_SUCCESS;
}


int
gem_makeVset(gemDRep *drep, int bound, int npts, double *xyz, int *vs)
{
  int      i, n;
  double   *xyzs;
  gemVSet  *temp;
  gemTface *faces;
  gemPair   bface;
  
  *vs = 0;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  n = drep->bound[bound-1].nVSet;

  faces = (gemTface *) gem_allocate(sizeof(gemTface));
  if (faces == NULL) return GEM_ALLOC;
  xyzs = (double *) gem_allocate(npts*3*sizeof(double));
  if (xyzs == NULL) {
    gem_free(faces);
    return GEM_ALLOC;
  }
  for (i = 0; i < 3*npts; i++) xyzs[i] = xyz[i];

  /* make the new bound */
  if (n == 0) {
    drep->bound[bound-1].VSet = (gemVSet *) gem_allocate(sizeof(gemVSet));
    if (drep->bound[bound-1].VSet == NULL) {
      gem_free(xyzs);
      gem_free(faces);
      return GEM_ALLOC;
    }
  } else {
    temp = (gemVSet *) gem_reallocate(drep->bound[bound-1].VSet, 
                                      (n+1)*sizeof(gemVSet));
    if (temp == NULL) {
      gem_free(xyzs);
      gem_free(faces);
      return GEM_ALLOC;
    }
    drep->bound[bound-1].VSet = temp;
  }
  
  drep->bound[bound-1].nVSet++;
  *vs = n+1;
  drep->bound[bound-1].VSet[n].npts   = npts;
  drep->bound[bound-1].VSet[n].nFaces = -1;
  drep->bound[bound-1].VSet[n].faces  = faces;
  drep->bound[bound-1].VSet[n].nSets  = 0;
  drep->bound[bound-1].VSet[n].Sets   = NULL;
  
  bface.BRep = bface.index = 0;
  drep->bound[bound-1].VSet[n].faces[0].index = bface;
  drep->bound[bound-1].VSet[n].faces[0].ID    = 0;
  drep->bound[bound-1].VSet[n].faces[0].npts  = npts;
  drep->bound[bound-1].VSet[n].faces[0].xyz   = xyzs;
  drep->bound[bound-1].VSet[n].faces[0].uv    = NULL;
  drep->bound[bound-1].VSet[n].faces[0].guv   = NULL;
  drep->bound[bound-1].VSet[n].faces[0].conn  = NULL;

  return GEM_SUCCESS;
}


int
gem_paramBound(gemDRep *drep, int bound, int vs)
{
  int    i, j, np, stat;
  double *sdat1, *sdat2;

  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;

  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet)) return GEM_BADVSETINDEX;
  if (drep->bound[bound-1].VSet[vs-1].nFaces == -1)  return GEM_BADTYPE;
  if (drep->bound[bound-1].VSet[vs-1].nFaces ==  0)  return GEM_BADOBJECT;
  if (drep->bound[bound-1].VSet[vs-1].nSets  !=  0)  return GEM_NOTPARAMBND;

  sdat1 = (double *) gem_allocate(drep->bound[bound-1].VSet[vs-1].npts*
                                  3*sizeof(double));
  if (sdat1 == NULL) return GEM_ALLOC;
  sdat2 = (double *) gem_allocate(drep->bound[bound-1].VSet[vs-1].npts*
                                  2*sizeof(double));
  if (sdat2 == NULL) {
    gem_free(sdat1);
    return GEM_ALLOC;
  }

  if (drep->bound[bound-1].VSet[vs-1].nFaces == 1) {
  
    /* single Face in Vset */
    if (drep->bound[bound-1].VSet[vs-1].faces[0].xyz == NULL) {
      stat = gem_kernelEval(drep, bound, vs, 0, 0);
      if (stat != GEM_SUCCESS) {
        gem_free(sdat2);
        gem_free(sdat1);
        return stat;
      }
    }
    for (i = 0; i < 3*drep->bound[bound-1].VSet[vs-1].faces[0].npts; i++)
      sdat1[i] = drep->bound[bound-1].VSet[vs-1].faces[0].xyz[i];

    if (drep->bound[bound-1].VSet[vs-1].faces[0].uv == NULL) {
      stat = gem_kernelEval(drep, bound, vs, 0, 1);
      if (stat != GEM_SUCCESS) {
        gem_free(sdat2);
        gem_free(sdat1);
        return stat;
      }
    }
    for (i = 0; i < 2*drep->bound[bound-1].VSet[vs-1].faces[0].npts; i++)
      sdat2[i] = drep->bound[bound-1].VSet[vs-1].faces[0].uv[i];
 
  } else {
  
    /* multiple Faces */

    for (np = j = 0; j < drep->bound[bound-1].VSet[vs-1].nFaces; j++) {
      if (drep->bound[bound-1].VSet[vs-1].faces[j].xyz == NULL) {
        stat = gem_kernelEval(drep, bound, vs, j, 0);
        if (stat != GEM_SUCCESS) {
          gem_free(sdat2);
          gem_free(sdat1);
          return stat;
        }
      }
      for (i = 0; i < drep->bound[bound-1].VSet[vs-1].faces[j].npts; i++) {
        sdat1[3*np  ] = drep->bound[bound-1].VSet[vs-1].faces[j].xyz[3*i  ];
        sdat1[3*np+1] = drep->bound[bound-1].VSet[vs-1].faces[j].xyz[3*i+1];
        sdat1[3*np+2] = drep->bound[bound-1].VSet[vs-1].faces[j].xyz[3*i+2];
        np++;
      }
      if (drep->bound[bound-1].VSet[vs-1].faces[j].uv == NULL) {
        stat = gem_kernelEval(drep, bound, vs, j, 1);
        if (stat != GEM_SUCCESS) {
          gem_free(sdat2);
          gem_free(sdat1);
          return stat;
        }
      }
    }
    /* parameterize the "quilt" */
    
  
  }
  
  drep->bound[bound-1].VSet[vs-1].Sets = (gemSet *) 
                                         gem_allocate(2*sizeof(gemSet));
  if (drep->bound[bound-1].VSet[vs-1].Sets == NULL) {
    gem_free(sdat2);
    gem_free(sdat1);
    return GEM_ALLOC;
  }
  drep->bound[bound-1].VSet[vs-1].nSets          = 2;
  drep->bound[bound-1].VSet[vs-1].Sets[0].ivsrc  = 0;
  drep->bound[bound-1].VSet[vs-1].Sets[0].name   = gem_strdup(reserved[0]);
  drep->bound[bound-1].VSet[vs-1].Sets[0].rank   = rreserved[0];
  drep->bound[bound-1].VSet[vs-1].Sets[0].data   = sdat1;
  drep->bound[bound-1].VSet[vs-1].Sets[0].interp = NULL;
  drep->bound[bound-1].VSet[vs-1].Sets[1].ivsrc  = 0;
  drep->bound[bound-1].VSet[vs-1].Sets[1].name   = gem_strdup(reserved[1]);
  drep->bound[bound-1].VSet[vs-1].Sets[1].rank   = rreserved[1];
  drep->bound[bound-1].VSet[vs-1].Sets[1].data   = sdat2;
  drep->bound[bound-1].VSet[vs-1].Sets[1].interp = NULL;
  
  return GEM_SUCCESS;
}


int
gem_putData(gemDRep *drep, int bound, int vs, char *name, int rank,
            double *data)
{
  int i, k, iset;

  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;

  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet)) return GEM_BADVSETINDEX;
  if (drep->bound[bound-1].VSet[vs-1].nFaces == -1) return GEM_NOTCONNECT;
  if (drep->bound[bound-1].VSet[vs-1].nFaces ==  0) return GEM_BADOBJECT;
  if (name == NULL) return GEM_NULLNAME;
  if (data == NULL) return GEM_NULLVALUE;
  
  /* check the validity of the name */
  
  for (i = 0; i < nReserved; i++)
    if (strcmp(name, reserved[i]) == 0) return GEM_BADDSETNAME;
  if (strlen(name) > 3)
    if ((name[0] == 'd') && (name[1] == '/') && (name[2] == 'd'))
      return GEM_BADDSETNAME;

  for (iset = i = 0; i < drep->bound[bound-1].VSet[vs-1].nSets; i++)
    if (strcmp(name, drep->bound[bound-1].VSet[vs-1].Sets[i].name) == 0)
      if (drep->bound[bound-1].VSet[vs-1].Sets[i].ivsrc == 0) {
        iset = i+1;
        break;
      }

  if (iset == 0) {
    /* the name does not exist in this Vset -- check others */
    for (k = 0; k < drep->bound[bound-1].nVSet; k++) {
      if (k+1 == vs) continue;
      for (i = 0; i < drep->bound[bound-1].VSet[k].nSets; i++)
        if (strcmp(name, drep->bound[bound-1].VSet[k].Sets[i].name) == 0)
          return GEM_BADDSETNAME;
    }
  } else {
    /* the name does exist in this Vset -- check rank */
    if (drep->bound[bound-1].VSet[vs-1].Sets[iset-1].rank != rank) 
      return GEM_BADRANK;
  }

  /* overwrite or put the data */
  

  
  return GEM_SUCCESS;
}


int
gem_getData(gemDRep *drep, int bound, int vs, char *name, int *npts,
            int *rank, double **data)
{
  int    i, j, stat, iset, ires, ivsrc = 0, issrc = 0;
  double *sdat1, *sdat2;
  gemSet *sets;

  *npts = *rank = 0;
  *data = NULL;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;

  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet))  return GEM_BADVSETINDEX;
  if  (drep->bound[bound-1].VSet[vs-1].nFaces ==  0)  return GEM_BADOBJECT;
  if ((drep->bound[bound-1].VSet[vs-1].nFaces != -1) &&
      (drep->bound[bound-1].VSet[vs-1].nSets  ==  0)) return GEM_NOTPARAMBND;
  if (name == NULL) return GEM_NULLNAME;

  /* does the data exist in the Vset? */
  ires = 0;
  iset = gem_indexName(drep, bound, vs, name);
  for (ires = i = 0; i < nReserved; i++)
    if (strcmp(name, reserved[i]) == 0) {
      ires = i+1;
      break;
    }
  if (ires == 0) 
    if (strlen(name) > 3)
      if ((name[0] == 'd') && (name[1] == '/') && (name[2] == 'd'))
        ires = -1;

  if (iset == 0) 
    if (ires == 0) {
      for (j = 0; j < drep->bound[bound-1].nVSet; j++) {
        if (j+1 == vs) continue;
        for (i = 0; i < drep->bound[bound-1].VSet[j].nSets; i++)
          if (strcmp(name, drep->bound[bound-1].VSet[j].Sets[i].name) == 0) {
          ivsrc = j+1;
          issrc = i+1;
          break;
        }      
      }
    } else {
      if (drep->bound[bound-1].VSet[vs-1].nFaces == -1) return GEM_NOTCONNECT;
    }
  
  /* not already in the Vset -- add it */
  if (iset == 0) {

    if (ires < 0) {
      /* parametric sensitivity -- check for parameter */

    } else if (ires == 0) {
      /* not reserved name */
      if (ivsrc == 0) return GEM_NOTFOUND;
      /* interpolate from source */
      
    } else {
      /* reserved name -- do query */
      if (ires < 5) {

        sdat1 = (double *) gem_allocate(drep->bound[bound-1].VSet[vs-1].npts*
                                        rreserved[2]*sizeof(double));
        if (sdat1 == NULL) return GEM_ALLOC;
        sdat2 = (double *) gem_allocate(drep->bound[bound-1].VSet[vs-1].npts*
                                        rreserved[3]*sizeof(double));
        if (sdat2 == NULL) {
          gem_free(sdat1);
          return GEM_ALLOC;
        }
        stat = gem_kernelEvalDs(drep, bound, vs, sdat1, sdat2);
        if (stat != GEM_SUCCESS) {
          gem_free(sdat2);
          gem_free(sdat1);
          return stat;
        }
        iset = drep->bound[bound-1].VSet[vs-1].nSets+2;
        sets = (gemSet *) gem_reallocate(drep->bound[bound-1].VSet[vs-1].Sets,
                                         iset*sizeof(gemSet));
        if (sets == NULL) {
          gem_free(sdat2);
          gem_free(sdat1);
          return GEM_ALLOC;
        }
        drep->bound[bound-1].VSet[vs-1].nSets               = iset;
        drep->bound[bound-1].VSet[vs-1].Sets[iset-2].ivsrc  = ivsrc;
        drep->bound[bound-1].VSet[vs-1].Sets[iset-2].name   = gem_strdup(reserved[2]);
        drep->bound[bound-1].VSet[vs-1].Sets[iset-2].rank   = rreserved[2];
        drep->bound[bound-1].VSet[vs-1].Sets[iset-2].data   = sdat1;
        drep->bound[bound-1].VSet[vs-1].Sets[iset-2].interp = NULL;
        drep->bound[bound-1].VSet[vs-1].Sets[iset-1].ivsrc  = ivsrc;
        drep->bound[bound-1].VSet[vs-1].Sets[iset-1].name   = gem_strdup(reserved[3]);
        drep->bound[bound-1].VSet[vs-1].Sets[iset-1].rank   = rreserved[3];
        drep->bound[bound-1].VSet[vs-1].Sets[iset-1].data   = sdat2;
        drep->bound[bound-1].VSet[vs-1].Sets[iset-1].interp = NULL;
        if (ires == 3) iset--;

      } else {
    
        sdat1 = (double *) gem_allocate(drep->bound[bound-1].VSet[vs-1].npts*
                                        rreserved[ires-1]*sizeof(double));
        if (sdat1 == NULL) return GEM_ALLOC;
        if (ires == 5) {
          stat = gem_kernelCurvature(drep, bound, vs, sdat1);
        } else {
          stat = gem_kernelInside(drep, bound, vs, sdat1);
        }
        if (stat != GEM_SUCCESS) {
          gem_free(sdat1);
          return stat;
        }
        iset = drep->bound[bound-1].VSet[vs-1].nSets+1;
        sets = (gemSet *) gem_reallocate(drep->bound[bound-1].VSet[vs-1].Sets,
                                         iset*sizeof(gemSet));
        if (sets == NULL) {
          gem_free(sdat1);
          return GEM_ALLOC;
        }
        drep->bound[bound-1].VSet[vs-1].nSets               = iset;
        drep->bound[bound-1].VSet[vs-1].Sets[iset-1].ivsrc  = ivsrc;
        drep->bound[bound-1].VSet[vs-1].Sets[iset-1].name   = gem_strdup(name);
        drep->bound[bound-1].VSet[vs-1].Sets[iset-1].rank   = rreserved[ires-1];
        drep->bound[bound-1].VSet[vs-1].Sets[iset-1].data   = sdat1;
        drep->bound[bound-1].VSet[vs-1].Sets[iset-1].interp = NULL;

      }
    }
  }

  *npts = drep->bound[bound-1].VSet[vs-1].npts;
  *rank = drep->bound[bound-1].VSet[vs-1].Sets[iset-1].rank;
  *data = drep->bound[bound-1].VSet[vs-1].Sets[iset-1].data;
  return GEM_SUCCESS;
}


int
gem_getDRepInfo(gemDRep *drep, gemModel **model, int *nbound, int *nattr)
{
  *model  = NULL;
  *nbound = 0;
  *nattr  = 0;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  
  *model  = drep->model;
  *nbound = drep->nBound;
  if (drep->attr != NULL) *nattr = drep->attr->nattrs;
  
  return GEM_SUCCESS;
}


int
gem_getBoundInfo(gemDRep *drep, int bound, int *nIDs, int **iIDs,
                 gemPair **indices, double *uvbox, int *nvs)
{
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  
  *nIDs    = drep->bound[bound-1].nIDs;
  *iIDs    = drep->bound[bound-1].IDs;
  *indices = drep->bound[bound-1].indices;
  uvbox[0] = drep->bound[bound-1].uvbox[0];
  uvbox[1] = drep->bound[bound-1].uvbox[1];
  uvbox[2] = drep->bound[bound-1].uvbox[2];
  uvbox[3] = drep->bound[bound-1].uvbox[3];
  *nvs     = drep->bound[bound-1].nVSet;

  return GEM_SUCCESS;
}


int
gem_getVsetInfo(gemDRep *drep, int bound, int vs, int *vstype, int *npnt,
                int *nset, char ***names, int **ranks)
{
  int  i;
  char **vnames;
  int  *vranks;

  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet)) return GEM_BADINDEX;
  
  *vstype = 0;
  if (drep->bound[bound-1].VSet[vs-1].nFaces == -1) *vstype = 1;
  *npnt  = drep->bound[bound-1].VSet[vs-1].npts;
  *nset  = drep->bound[bound-1].VSet[vs-1].nSets;
  *names = NULL;
  *ranks = NULL;
  
  if (*nset > 0) {
    vranks = (int *)   gem_allocate(*nset*sizeof(int));
    if (vranks == NULL) return GEM_ALLOC;
    vnames = (char **) gem_allocate(*nset*sizeof(char *));
    if (vnames == NULL) {
      gem_free(vranks);
      return GEM_ALLOC;
    }
    for (i = 0; i < *nset; i++) {
      vranks[i] = drep->bound[bound-1].VSet[vs-1].Sets[i].rank;
      vnames[i] = drep->bound[bound-1].VSet[vs-1].Sets[i].name;
    }
    *names = vnames;
    *ranks = vranks;
  }
  
  return GEM_SUCCESS;
}
