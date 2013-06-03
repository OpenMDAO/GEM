/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             DRep Functions
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifdef WIN32
#include <windows.h>
#define DLL HINSTANCE
#else
#include <dlfcn.h>
#define DLL void *
#endif

#include "gem.h"
#include "memory.h"
#include "attribute.h"
#include "kernel.h"
#include "disMethod.h"
#include "connect.h"


#define CROSS(a,b,c)      a[0] = (b[1]*c[2]) - (b[2]*c[1]);\
                          a[1] = (b[2]*c[0]) - (b[0]*c[2]);\
                          a[2] = (b[0]*c[1]) - (b[1]*c[0])
#define DOT(a,b)         (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])


/* reserved DataSet names */
static int  nReserved    = 7;
static int  nGeomBased   = 5;
static char *reserved[7] = {"xyz", "uv", "d1", "d2", "curv", "xyzd", "uvd"};
static int  rreserved[7] = {  3,    2,    6,    9,     8,      3,      2 };

/* disMethod loaded functions */
static char   *metName[MAXMETHOD];
static DLL     metDLL[MAXMETHOD];
static fQuilt  freeQuilt[MAXMETHOD];
static dQuilt  defQuilt[MAXMETHOD];
static invEval iEval[MAXMETHOD];
static gInterp Interpolate[MAXMETHOD];
static bInterp Interpol_bar[MAXMETHOD];
static gIntegr Integrate[MAXMETHOD];
static bIntegr Integr_bar[MAXMETHOD];

static int     met_nDiscr = 0;

extern int  gem_fillCoeff2D(int nrank, int nu, int nv, double *grid,
                            double *coeff, double *r);
extern int  gem_invInterpolate2D(gemAprx2D *interp, double *sv, double *uv);
extern int  gem_dataTransfer(gemDRep *drep, int bound, int ivsrc, int issrc,
                             int vs, int meth, int *iset,
                             gInterp *Interpolatf, bInterp *Interpol_bf,
                             gIntegr *Integratf,   bIntegr *Integr_bf,
                             invEval *invEvalf);



/* *********************** Dynamic Load Functions *************************** */

static /*@null@*/ DLL metDLopen(const char *name)
{
  int  i, len;
  DLL  dll;
  char *full;
  
  if (name == NULL) {
    printf(" Information: Dynamic Loader invoked with NULL name!\n");
    return NULL;
  }
  len  = strlen(name);
  full = (char *) malloc((len+5)*sizeof(char));
  if (full == NULL) {
    printf(" Information: Dynamic Loader MALLOC Error!\n");
    return NULL;
  }
  for (i = 0; i < len; i++) full[i] = name[i];
  full[len  ] = '.';
#ifdef WIN32
  full[len+1] = 'D';
  full[len+2] = 'L';
  full[len+3] = 'L';
  full[len+4] =  0;
  dll = LoadLibrary(full);
#else
  full[len+1] = 's';
  full[len+2] = 'o';
  full[len+3] =  0;
  dll = dlopen(full, RTLD_NOW /* RTLD_LAZY */);
#endif
  
  if (!dll) {
    printf(" Information: Dynamic Loader Error for %s\n", full);
#ifndef WIN32
    printf("              %s\n", dlerror());
#endif
    free(full);
    return NULL;
  }
  
  free(full);
  return dll;
}


static void metDLclose(/*@only@*/ DLL dll)
{
#ifdef WIN32
  FreeLibrary(dll);
#else
  dlclose(dll);
#endif
}


static DLLFunc metDLget(DLL dll, const char *symname, const char *name)
{
  DLLFunc data;
  
#ifdef WIN32
  data = (DLLFunc) GetProcAddress(dll, symname);
#else
  data = (DLLFunc) dlsym(dll, symname);
#endif
  if (!data)
    printf("               Couldn't get symbol %s in %s\n", symname, name);
  return data;
}


int gem_metDLoaded(const char *name)
{
  int i;
  
  for (i = 0; i < met_nDiscr; i++)
    if (strcmp(name, metName[i]) == 0) return i;
  
  return -1;
}


static int metDYNload(const char *name)
{
  int i, len, ret;
  DLL dll;
  
  if (met_nDiscr >= MAXMETHOD) {
    printf(" Information: Number of Methods > %d!\n", MAXMETHOD);
    return GEM_BADINDEX;
  }
  dll = metDLopen(name);
  if (dll == NULL) return GEM_NULLOBJ;
  
  ret = met_nDiscr;
  freeQuilt[ret]    = (fQuilt)  metDLget(dll, "gemFreeQuilt",       name);
  defQuilt[ret]     = (dQuilt)  metDLget(dll, "gemDefineQuilt",     name);
  iEval[ret]        = (invEval) metDLget(dll, "gemInvEvaluation",   name);
  Interpolate[ret]  = (gInterp) metDLget(dll, "gemInterpolation",   name);
  Interpol_bar[ret] = (bInterp) metDLget(dll, "gemInterpolate_bar", name);
  Integrate[ret]    = (gIntegr) metDLget(dll, "gemIntegration",     name);
  Integr_bar[ret]   = (bIntegr) metDLget(dll, "gemIntegrate_bar",   name);
  if ((freeQuilt[ret]    == NULL) || (defQuilt[ret]   == NULL) ||
      (Interpolate[ret]  == NULL) || (Integrate[ret]  == NULL) ||
      (Interpol_bar[ret] == NULL) || (Integr_bar[ret] == NULL) ||
      (iEval[ret]        == NULL)) {
    metDLclose(dll);
    return GEM_BADOBJECT;
  }
  
  len  = strlen(name) + 1;
  metName[ret] = (char *) malloc(len*sizeof(char));
  if (metName[ret] == NULL) {
    metDLclose(dll);
    return GEM_ALLOC;
  }
  for (i = 0; i < len; i++) metName[ret][i] = name[i];
  metDLL[ret] = dll;
  met_nDiscr++;
  
  return ret;
}

/* ************************************************************************** */


void
gem_drepManagerClose()
{
  int i;
  
  for (i = 0; i < met_nDiscr; i++) {
    metDLclose(metDLL[i]);
    gem_free(metName[i]);
  }
  met_nDiscr = 0;
}


static int
gem_indexName(gemDRep *drep, int bound, int vs, char *name)
{
  int i;

  for (i = 0; i < drep->bound[bound-1].VSet[vs-1].nSets; i++)
    if (strcmp(name, drep->bound[bound-1].VSet[vs-1].sets[i].name) == 0)
      return i+1;
      
  return 0;
}


static void
gem_freeAprx2D(/*@null@*/ /*@only@*/ gemAprx2D *approx)
{
  if (approx == NULL) return;
  
  gem_Aprx2DFree(approx);
  gem_free(approx);
}


static void
gem_freeVsets(gemBound bound)
{
  int i, j, n;
  
  for (i = 0; i < bound.nVSet; i++) {

    if (bound.VSet[i].quilt != NULL) {
      n = gem_metDLoaded(bound.VSet[i].disMethod);
      if (n >= 0) {
        freeQuilt[n](bound.VSet[i].quilt);
        gem_free(bound.VSet[i].quilt);
      }
      if (bound.VSet[i].tris != NULL) gem_free(bound.VSet[i].tris);
      gem_free(bound.VSet[i].disMethod);
    }
    
    if (bound.VSet[i].nonconn != NULL) {
      gem_free(bound.VSet[i].nonconn->data);
      gem_free(bound.VSet[i].nonconn);
    }

    for (j = 0; j < bound.VSet[i].nSets; j++) {
      gem_free(bound.VSet[i].sets[j].name);
      gem_free(bound.VSet[i].sets[j].dset.data);
    }
    if (bound.VSet[i].sets != NULL) gem_free(bound.VSet[i].sets);

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
    trep[i].nEdges = 0;
    trep[i].Edges  = NULL;
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
    if (stat == GEM_SUCCESS) {
      for (k = 0; k < drep->TReps[brep-1].nFaces; k++)
        gem_xform(model->BReps[brep-1], drep->TReps[brep-1].Faces[k].npts,
                  drep->TReps[brep-1].Faces[k].xyzs);
      for (k = 0; k < drep->TReps[brep-1].nEdges; k++)
        gem_xform(model->BReps[brep-1], drep->TReps[brep-1].Edges[k].npts,
                  drep->TReps[brep-1].Edges[k].xyzs);
    }
  } else {
    stat = -9999;
    for (i = 0; i < drep->nBReps; i++) {
      j = gem_kernelTessel(model->BReps[i]->body, angle, mxside, sag,
                           drep, i+1);
      if (j == GEM_SUCCESS) {
        for (k = 0; k < drep->TReps[i].nFaces; k++)
          gem_xform(model->BReps[i], drep->TReps[i].Faces[k].npts,
                    drep->TReps[i].Faces[k].xyzs);
        for (k = 0; k < drep->TReps[i].nEdges; k++)
          gem_xform(model->BReps[i], drep->TReps[i].Edges[k].npts,
                    drep->TReps[i].Edges[k].xyzs);
      }
      if (j > stat) stat = j;
    }
  }

  return stat;
}


int
gem_getDiscrete(gemDRep *drep, gemPair bedge, int *npts, double **xyz)
{
  *npts = 0;
  *xyz  = NULL;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  
  if (drep->TReps == NULL) return GEM_NOTESSEL;
  if ((bedge.BRep  < 1) || (bedge.BRep > drep->nBReps)) return GEM_BADINDEX;
  if (drep->TReps[bedge.BRep-1].Edges == NULL) return GEM_NOTESSEL;
  if ((bedge.index < 1) ||
      (bedge.index > drep->TReps[bedge.BRep-1].nEdges)) return GEM_BADINDEX;
  
  *npts = drep->TReps[bedge.BRep-1].Edges[bedge.index-1].npts;
  *xyz  = drep->TReps[bedge.BRep-1].Edges[bedge.index-1].xyzs;
  
  return GEM_SUCCESS;
}


int
gem_getTessel(gemDRep *drep, gemPair bface, int *ntris, int *npts, int **tris,
              double **xyz)
{
  *ntris = 0;
  *npts  = 0;
  *tris  = NULL;
  *xyz   = NULL;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;

  if (drep->TReps == NULL) return GEM_NOTESSEL;
  if ((bface.BRep  < 1) || (bface.BRep > drep->nBReps)) return GEM_BADINDEX;
  if (drep->TReps[bface.BRep-1].Faces == NULL) return GEM_NOTESSEL;
  if ((bface.index < 1) ||
      (bface.index > drep->TReps[bface.BRep-1].nFaces)) return GEM_BADINDEX;

  *ntris = drep->TReps[bface.BRep-1].Faces[bface.index-1].ntris;
  *npts  = drep->TReps[bface.BRep-1].Faces[bface.index-1].npts;
  *tris  = drep->TReps[bface.BRep-1].Faces[bface.index-1].tris;
  *xyz   = drep->TReps[bface.BRep-1].Faces[bface.index-1].xyzs;
  
  return GEM_SUCCESS;
}


static void
gem_freeXfer(gemXfer *xfer)
{
  if (xfer->position != NULL) gem_free(xfer->position);
  if (xfer->match    != NULL) gem_free(xfer->match);
  gem_free(xfer);
}


int
gem_destroyDRep(gemDRep *drep)
{
  int      i, j;
  gemCntxt *cntxt;
  gemDRep  *prev, *next;
  gemXfer  *xfer, *last;

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
        gem_free(drep->TReps[i].Faces[j].xyzs);
        gem_free(drep->TReps[i].Faces[j].tris);
        gem_free(drep->TReps[i].Faces[j].tric);
        gem_free(drep->TReps[i].Faces[j].uvs);
        gem_free(drep->TReps[i].Faces[j].vid);
      }
      gem_free(drep->TReps[i].Faces);
    }
    if (drep->TReps[i].Edges != NULL) {
      for (j = 0; j < drep->TReps[i].nEdges; j++) {
        gem_free(drep->TReps[i].Edges[j].xyzs);
        gem_free(drep->TReps[i].Edges[j].ts);
      }
      gem_free(drep->TReps[i].Edges);
    }
  }
  gem_free(drep->TReps);

  if (drep->bound != NULL) {
    for (i = 0; i < drep->nBound; i++) {
      gem_free(drep->bound[i].IDs);
      gem_free(drep->bound[i].indices);
      if (drep->bound[i].surface != NULL)
        gem_freeAprx2D(drep->bound[i].surface);
      gem_freeVsets(drep->bound[i]);
      xfer = drep->bound[i].xferList;
      while (xfer != NULL) {
        last = xfer;
        xfer = xfer->next;
        gem_freeXfer(last);
      }
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
  int      i, j;
  gemModel *prev;
  gemDRep  *drep;
  gemCntxt *cntxt;
  gemTRep  *trep;
  gemXfer  *xfer, *last;
  
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

  drep = cntxt->drep;
  while (drep != NULL) {
    if (drep->model == model) {
      if (phase == 0) {
        
        /* cleanup */
        for (i = 0; i < drep->nBReps; i++) {
          if (drep->TReps[i].Faces != NULL) {
            for (j = 0; j < drep->TReps[i].nFaces; j++) {
              gem_free(drep->TReps[i].Faces[j].xyzs);
              gem_free(drep->TReps[i].Faces[j].tris);
              gem_free(drep->TReps[i].Faces[j].tric);
              gem_free(drep->TReps[i].Faces[j].uvs);
              gem_free(drep->TReps[i].Faces[j].vid);
            }
            gem_free(drep->TReps[i].Faces);
          }
          if (drep->TReps[i].Edges != NULL) {
            for (j = 0; j < drep->TReps[i].nEdges; j++) {
              gem_free(drep->TReps[i].Edges[j].xyzs);
              gem_free(drep->TReps[i].Edges[j].ts);
            }
            gem_free(drep->TReps[i].Edges);
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
              gem_freeAprx2D(drep->bound[i].surface);
            gem_freeVsets(drep->bound[i]);
            xfer = drep->bound[i].xferList;
            while (xfer != NULL) {
              last = xfer;
              xfer = xfer->next;
              gem_freeXfer(last);
            }
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
            trep[i].nEdges = 0;
            trep[i].Edges  = NULL;
          }
          drep->nBReps = model->nBRep;
          drep->TReps  = trep;  
        }
      }
    }
    drep = drep->next;
  }

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
  drep->bound[*bound-1].nIDs         = nIDs;
  drep->bound[*bound-1].IDs          = iIDs;
  drep->bound[*bound-1].indices      = indices;
  drep->bound[*bound-1].single.BRep  = 0;
  drep->bound[*bound-1].single.index = 0;
  drep->bound[*bound-1].surface      = NULL;
  drep->bound[*bound-1].uvbox[0]     = 0.0;
  drep->bound[*bound-1].uvbox[1]     = 0.0;
  drep->bound[*bound-1].uvbox[2]     = 0.0;
  drep->bound[*bound-1].uvbox[3]     = 0.0;
  drep->bound[*bound-1].nVSet        = 0;
  drep->bound[*bound-1].VSet         = NULL;
  drep->bound[*bound-1].xferList     = NULL;
  
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
  gemXfer  *xfer, *last;

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
    gem_freeAprx2D(drep->bound[bound-1].surface);
  gem_freeVsets(drep->bound[bound-1]);
  xfer= drep->bound[bound-1].xferList;
  while (xfer != NULL) {
    last = xfer;
    xfer = xfer->next;
    gem_freeXfer(last);
  }
  gem_free(drep->bound[bound-1].IDs);
  gem_free(drep->bound[bound-1].indices);

  drep->bound[bound-1].nIDs        += nIDs;
  drep->bound[bound-1].IDs          = iIDs;
  drep->bound[bound-1].indices      = indices;
  drep->bound[bound-1].single.BRep  = 0;
  drep->bound[bound-1].single.index = 0;
  drep->bound[bound-1].surface      = NULL;
  drep->bound[bound-1].uvbox[0]     = 0.0;
  drep->bound[bound-1].uvbox[1]     = 0.0;
  drep->bound[bound-1].uvbox[2]     = 0.0;
  drep->bound[bound-1].uvbox[3]     = 0.0;
  drep->bound[bound-1].nVSet        = 0;
  drep->bound[bound-1].VSet         = NULL;
  drep->bound[bound-1].xferList     = NULL;
  
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
gem_createVset(gemDRep *drep, int bound, char *disMethod, int *vs)
{
  int     stat, n;
  gemVSet *temp;
  
  *vs = 0;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if (disMethod == NULL) return GEM_NULLNAME;
  
  /* deal with discretization method */
  stat = gem_metDLoaded(disMethod);
  if (stat == -1) {
    stat = metDYNload(disMethod);
    if (stat < 0) return stat;
  }
  
  /* make the new VertexSet */
  n = drep->bound[bound-1].nVSet;
  if (n == 0) {
    drep->bound[bound-1].VSet = (gemVSet *) gem_allocate(sizeof(gemVSet));
    if (drep->bound[bound-1].VSet == NULL) return GEM_ALLOC;
  } else {
    temp = (gemVSet *) gem_reallocate(drep->bound[bound-1].VSet, 
                                      (n+1)*sizeof(gemVSet));
    if (temp == NULL) return GEM_ALLOC;
    drep->bound[bound-1].VSet = temp;
  }
  drep->bound[bound-1].VSet[n].disMethod = gem_strdup(disMethod);
  if (drep->bound[bound-1].VSet[n].disMethod == NULL) return GEM_ALLOC;
  drep->bound[bound-1].VSet[n].quilt   = NULL;
  drep->bound[bound-1].VSet[n].nonconn = NULL;
  drep->bound[bound-1].VSet[n].ntris   = 0;
  drep->bound[bound-1].VSet[n].tris    = NULL;
  drep->bound[bound-1].VSet[n].nSets   = 0;
  drep->bound[bound-1].VSet[n].sets    = NULL;

  drep->bound[bound-1].nVSet++;
  *vs = n+1;

  return GEM_SUCCESS;
}


static int
gem_fillNonConn(gemDRep *drep, gemBound *bound, int vsi)
{
  int       i, stat;
  double    coor[3], *xyzs, *uvs, *txyz, *xform;
  gemCollct *collect;
  gemDSet   *sets;
  gemModel  *model;
  gemBRep   *brep;
  
  collect = bound->VSet[vsi].nonconn;
  xyzs    = (double *) gem_allocate(3*collect->npts*sizeof(double));
  if (xyzs == NULL) return GEM_ALLOC;
  uvs     = (double *) gem_allocate(2*collect->npts*sizeof(double));
  if (uvs  == NULL) {
    gem_free(xyzs);
    return GEM_ALLOC;
  }
  sets    = (gemDSet *) gem_allocate(2*sizeof(gemDSet));
  if (sets == NULL) {
    gem_free(uvs);
    gem_free(xyzs);
    return GEM_ALLOC;
  }
  for (i = 0; i < 3*collect->npts; i++) xyzs[i] = collect->data[i];
  sets[0].ivsrc     = 0;
  sets[0].name      = gem_strdup("xyz");
  sets[0].dset.npts = collect->npts;
  sets[0].dset.rank = 3;
  sets[1].ivsrc     = 0;
  sets[1].name      = gem_strdup("uv");
  sets[1].dset.npts = collect->npts;
  sets[1].dset.rank = 2;
  if ((sets[0].name == NULL) || (sets[1].name == NULL)) {
    if (sets[1].name != NULL) gem_free(sets[1].name);
    if (sets[0].name != NULL) gem_free(sets[0].name);
    gem_free(sets);
    gem_free(uvs);
    gem_free(xyzs);
    return GEM_ALLOC;
  }
  
  if (bound->surface == NULL) {
    
    /* single Face */
    txyz = gem_allocate(3*collect->npts*sizeof(double));
    if (txyz == NULL) {
      gem_free(sets[1].name);
      gem_free(sets[0].name);
      gem_free(sets);
      gem_free(uvs);
      gem_free(xyzs);
      return GEM_ALLOC;
    }
    model = drep->model;
    brep  = model->BReps[bound->single.BRep-1];
    xform = brep->invXform;
    for (i = 0; i < collect->npts; i++) {
      txyz[3*i  ] = xform[ 0]*xyzs[3*i  ] + xform[ 1]*xyzs[3*i+1] +
                    xform[ 2]*xyzs[3*i+2] + xform[ 3];
      txyz[3*i+1] = xform[ 4]*xyzs[3*i  ] + xform[ 5]*xyzs[3*i+1] +
                    xform[ 6]*xyzs[3*i+2] + xform[ 7];
      txyz[3*i+2] = xform[ 8]*xyzs[3*i  ] + xform[ 9]*xyzs[3*i+1] +
                    xform[10]*xyzs[3*i+2] + xform[11];
/*    txyz[3*i  ] = xyzs[3*i  ];
      txyz[3*i+1] = xyzs[3*i+1];
      txyz[3*i+2] = xyzs[3*i+2];  */
    }
    stat = gem_kernelInvEval(drep, bound->single, collect->npts, txyz, uvs);
    gem_free(txyz);
    if (stat != GEM_SUCCESS) {
      gem_free(sets[1].name);
      gem_free(sets[0].name);
      gem_free(sets);
      gem_free(uvs);
      gem_free(xyzs);
      return stat;
    }

  } else {
    
    /* reparameterized */
    for (i = 0; i < collect->npts; i++) {
      coor[0] = xyzs[3*i  ];
      coor[1] = xyzs[3*i+1];
      coor[2] = xyzs[3*i+2];
      stat    = gem_invInterpolate2D(bound->surface, coor, &uvs[2*i]);
      if (stat != GEM_SUCCESS) {
        gem_free(sets[1].name);
        gem_free(sets[0].name);
        gem_free(sets);
        gem_free(uvs);
        gem_free(xyzs);
        return stat;
      }
    }

  }
  
  sets[0].dset.data      = xyzs;
  sets[1].dset.data      = uvs;
  bound->VSet[vsi].sets  = sets;
  bound->VSet[vsi].nSets = 2;

  return GEM_SUCCESS;
}


int
gem_makeVset(gemDRep *drep, int bound, int npts, double *xyz, int *vs)
{
  int       i, n, stat;
  double    *xyzs;
  gemVSet   *temp;
  gemCollct *collct;
  
  *vs = 0;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  n = drep->bound[bound-1].nVSet;

  collct = (gemCollct *) gem_allocate(sizeof(gemCollct));
  if (collct == NULL) return GEM_ALLOC;
  xyzs = (double *) gem_allocate(npts*3*sizeof(double));
  if (xyzs == NULL) {
    gem_free(collct);
    return GEM_ALLOC;
  }
  for (i = 0; i < 3*npts; i++) xyzs[i] = xyz[i];

  /* make the new bound */
  if (n == 0) {
    drep->bound[bound-1].VSet = (gemVSet *) gem_allocate(sizeof(gemVSet));
    if (drep->bound[bound-1].VSet == NULL) {
      gem_free(xyzs);
      gem_free(collct);
      return GEM_ALLOC;
    }
  } else {
    temp = (gemVSet *) gem_reallocate(drep->bound[bound-1].VSet, 
                                      (n+1)*sizeof(gemVSet));
    if (temp == NULL) {
      gem_free(xyzs);
      gem_free(collct);
      return GEM_ALLOC;
    }
    drep->bound[bound-1].VSet = temp;
  }
  
  collct->npts = npts;
  collct->rank = 3;
  collct->data = xyzs;
  
  drep->bound[bound-1].VSet[n].disMethod = NULL;
  drep->bound[bound-1].VSet[n].quilt     = NULL;
  drep->bound[bound-1].VSet[n].nonconn   = collct;
  drep->bound[bound-1].VSet[n].ntris     = 0;
  drep->bound[bound-1].VSet[n].tris      = NULL;
  drep->bound[bound-1].VSet[n].nSets     = 0;
  drep->bound[bound-1].VSet[n].sets      = NULL;
  if ((drep->bound[bound-1].uvbox[0] != 0.0) ||
      (drep->bound[bound-1].uvbox[1] != 0.0) ||
      (drep->bound[bound-1].uvbox[2] != 0.0) ||
      (drep->bound[bound-1].uvbox[3] != 0.0)) {
    stat = gem_fillNonConn(drep, &drep->bound[bound-1], n);
    if (stat != GEM_SUCCESS) {
      gem_free(drep->bound[bound-1].VSet[n].nonconn->data);
      gem_free(drep->bound[bound-1].VSet[n].nonconn);
      return stat;
    }
  }

  drep->bound[bound-1].nVSet++;
  *vs = n+1;

  return GEM_SUCCESS;
}


static int
gem_makeNeighbors(int npts, int ntris, prmTri *tris)
{
  int      i, nside, *vtab;
  gemNeigh *stab;
  
  vtab = (int *) gem_allocate(npts*sizeof(int));
  if (vtab == NULL) return GEM_ALLOC;
  stab = (gemNeigh *) gem_allocate(ntris*3*sizeof(gemNeigh));
  if (stab == NULL) {
    gem_free(vtab);
    return GEM_ALLOC;
  }
  
  nside = NOTFILLED;
  for (i = 0; i < npts; i++) vtab[i] = NOTFILLED;
  for (i = 0; i < ntris; i++) {
    tris[i].neigh[0] = i + 1;
    tris[i].neigh[1] = i + 1;
    tris[i].neigh[2] = i + 1;
  }
  for (i = 0; i < ntris;  i++) {
    if ((tris[i].indices[0] < 1) || (tris[i].indices[1] < 1) ||
        (tris[i].indices[2] < 1))
      printf(" %d/%d:  %d %d %d\n", i, ntris, tris[i].indices[0],
             tris[i].indices[1], tris[i].indices[2]);
    gem_fillNeighbor( tris[i].indices[1], tris[i].indices[2],
                     &tris[i].neigh[0], &nside, vtab, stab);
    gem_fillNeighbor( tris[i].indices[0], tris[i].indices[2],
                     &tris[i].neigh[1], &nside, vtab, stab);
    gem_fillNeighbor( tris[i].indices[0], tris[i].indices[1],
                     &tris[i].neigh[2], &nside, vtab, stab);
  }
  
  /* zero out sides with only one neighbor */
  for (i = 0; i <= nside; i++)
    if (stab[i].tri != NULL) *stab[i].tri = 0;
  
  gem_free(stab);
  gem_free(vtab);
  return GEM_SUCCESS;
}


static int
gem_checkQuilt(gemQuilt *quilt, int *ntri, prmTri **tri)
{
  int    i, j, k, n, t, dref, ntris, own, stat, *fuvs, err = 0;
  prmTri *tris;
  
  /* check type indices */
  for (dref = i = 0; i < quilt->nElems; i++) {
    t = quilt->elems[i].tIndex;
    if ((t <= 0) || (t >  quilt->nTypes)) {
      printf(" Quilt Error: Element %d Type = %d [1-%d]!\n",
             i+1, t, quilt->nTypes);
      err++;
    } else {
      if (quilt->types[t-1].ndata != 0) dref++;
    }
  }
  if (err != 0) return GEM_BADINDEX;
  
  /* check tri in elem type */
  for (i = 0; i < quilt->nTypes; i++)
    for (j = 0; j < quilt->types[i].ntri; j++)
      for (k = 0; k < 3; k++) {
        t = quilt->types[i].tris[3*j+k];
        if ((t <= 0) || (t > quilt->types[i].nref)) {
          printf(" Quilt Error: Type %d Tri Index %d/%d = %d [1-%d]!\n",
                 i+1, j, k, t, quilt->types[i].nref);
          err++;
        }
      }
  if (err != 0) return GEM_BADINDEX;
  
  /* see if we have separate data from geom */
  if (dref != 0) {
    if (quilt->verts == NULL) {
      printf(" Quilt Error: data ref required without verts!\n");
      return GEM_BADINDEX;
    }
    for (i = 0; i < quilt->nVerts; i++)
      if ((quilt->verts[i].owner < 1) ||
          (quilt->verts[i].owner > quilt->nbface)) {
        printf(" Quilt Error: verts %d/%d owner = %d [1-%d]!\n",
               i+1, j+1, quilt->verts[i].owner, quilt->nbface);
        err++;
      }
    if (err != 0) return GEM_BADINDEX;
  }

  /* check for point faceUVs in bounds */
  for (i = 0; i < quilt->nPoints; i++) {
    if (quilt->points[i].nFaces < 3) {
      fuvs = quilt->points[i].findices.faces;
    } else {
      fuvs = quilt->points[i].findices.multi;
    }
    for (j = 0; j < quilt->points[i].nFaces; j++) {
      if ((fuvs[j] < 1) || (fuvs[j] > quilt->nFaceUVs)) {
        printf(" Quilt Error: FaceUV %d/%d index = %d [1-%d]!\n",
               i+1, j+1, fuvs[j], quilt->nFaceUVs);
        err++;
      } else {
        if ((quilt->faceUVs[fuvs[j]-1].owner < 1) ||
            (quilt->faceUVs[fuvs[j]-1].owner > quilt->nbface)) {
          printf(" Quilt Error: FaceUV %d/%d owner = %d [1-%d]!\n",
                 i+1, j+1, quilt->faceUVs[fuvs[j]-1].owner, quilt->nbface);
          err++;
        }
      }
    }
  }
  if (err != 0) return GEM_BADINDEX;
  
  /* check element indices */
  for (i = 0; i < quilt->nElems; i++) {
    t = quilt->elems[i].tIndex - 1;
    for (j = 0; j < quilt->types[t].nref; j++)
      if ((quilt->elems[i].gIndices[j] < 1) ||
          (quilt->elems[i].gIndices[j] > quilt->nPoints)) {
        printf(" Quilt Error: Element %d/%d index = %d [1-%d]!\n",
               i+1, j+1, quilt->elems[i].gIndices[j], quilt->nPoints);
        err++;
      }
    if (quilt->types[t].ndata != 0)
      for (j = 0; j < quilt->types[t].ndata; j++)
        if ((quilt->elems[i].dIndices[j] < 1) ||
            (quilt->elems[i].dIndices[j] > quilt->nVerts)) {
          printf(" Quilt Error: Element data %d/%d index = %d [1-%d]!\n",
                 i+1, j+1, quilt->elems[i].dIndices[j], quilt->nVerts);
          err++;
        }
  }
  if (err != 0) return GEM_BADINDEX;
  
  /* make triangles and neighbors if possibly used for reparametrization */
  
  if (quilt->paramFlg == -1) return GEM_SUCCESS;
  if (quilt->nbface   ==  1) return GEM_SUCCESS;
  
  for (ntris = i = 0; i < quilt->nElems; i++) {
    t      = quilt->elems[i].tIndex - 1;
    ntris += quilt->types[t].ntri;
  }
  tris = (prmTri *) gem_allocate(ntris*sizeof(prmTri));
  if (tris == NULL) return GEM_ALLOC;
  
  for (j = i = 0; i < quilt->nElems; i++) {
    t   = quilt->elems[i].tIndex - 1;
    own = quilt->elems[i].owner;
    for (k = 0; k < quilt->types[t].ntri; k++, j++) {
      tris[j].own = own;
      n = quilt->types[t].tris[3*k  ] - 1;
      tris[j].indices[0] = quilt->elems[i].gIndices[n];
      n = quilt->types[t].tris[3*k+1] - 1;
      tris[j].indices[1] = quilt->elems[i].gIndices[n];
      n = quilt->types[t].tris[3*k+2] - 1;
      tris[j].indices[2] = quilt->elems[i].gIndices[n];
    }
  }
  stat = gem_makeNeighbors(quilt->nPoints, ntris, tris);
  if (stat != GEM_SUCCESS) {
    gem_free(tris);
    return stat;
  }
  
  *ntri = ntris;
  *tri  = tris;
  return GEM_SUCCESS;
}


static void
gem_getUVs(gemQuilt *quilt, int iface, int ipt, double *uv)
{
  int i, *fuvs;

  if (quilt->points[ipt].nFaces < 3) {
    fuvs = quilt->points[ipt].findices.faces;
  } else {
    fuvs = quilt->points[ipt].findices.multi;
  }
  for (i = 0; i < quilt->points[ipt].nFaces; i++)
    if (quilt->faceUVs[fuvs[i]-1].owner == iface) {
      uv[0] = quilt->faceUVs[fuvs[i]-1].uv[0];
      uv[1] = quilt->faceUVs[fuvs[i]-1].uv[1];
      return;
    }
  
  printf(" getUVs Error: Index %d not found in Point %d!\n", iface, ipt+1);
}


static int
gem_paramQuilt(gemBound *bound, int ivs)
{
  int       j, n, stat, npts, ntris, own, nu, nv, per, *ppnts;
  double    params[2], box[6], tol, rmserr, maxerr, dotmin;
  double    *grid, *xyz, *r;
  gemQuilt  *quilt;
  gemAprx2D *surface;
  prmUVF    *uvf;
  prmUV     *uv;
  
  quilt = bound->VSet[ivs].quilt;
  npts  = quilt->nPoints;
  ntris = bound->VSet[ivs].ntris;
  
  uv    = (prmUV *) gem_allocate(npts*sizeof(prmUV));
  if (uv   == NULL) return GEM_ALLOC;
  uvf   = (prmUVF *) gem_allocate(ntris*sizeof(prmUVF));
  if (uvf  == NULL) {
    gem_free(uv);
    return GEM_ALLOC;
  }
  xyz   = (double *) gem_allocate(3*npts*sizeof(prmXYZ));
  if (xyz  == NULL) {
    gem_free(uvf);
    gem_free(uv);
    return GEM_ALLOC;
  }

  for (j = 0; j < ntris; j++) {
    own = bound->VSet[ivs].tris[j].own;
    gem_getUVs(quilt, own, bound->VSet[ivs].tris[j].indices[0]-1, params);
    uvf[j].u0 = params[0];
    uvf[j].v0 = params[1];
    gem_getUVs(quilt, own, bound->VSet[ivs].tris[j].indices[1]-1, params);
    uvf[j].u1 = params[0];
    uvf[j].v1 = params[1];
    gem_getUVs(quilt, own, bound->VSet[ivs].tris[j].indices[2]-1, params);
    uvf[j].u2 = params[0];
    uvf[j].v2 = params[1];
  }
  for (j = 0; j < quilt->nPoints; j++) {
    xyz[3*j  ] = quilt->points[j].xyz[0];
    xyz[3*j+1] = quilt->points[j].xyz[1];
    xyz[3*j+2] = quilt->points[j].xyz[2];
  }

  box[0] = box[3] = xyz[0];
  box[1] = box[4] = xyz[1];
  box[2] = box[5] = xyz[2];
  for (j = 1; j < npts; j++) {
    if (xyz[3*j  ] < box[0]) box[0] = xyz[3*j  ];
    if (xyz[3*j  ] > box[3]) box[3] = xyz[3*j  ];
    if (xyz[3*j+1] < box[1]) box[1] = xyz[3*j+1];
    if (xyz[3*j+1] > box[4]) box[4] = xyz[3*j+1];
    if (xyz[3*j+2] < box[2]) box[2] = xyz[3*j+2];
    if (xyz[3*j+2] > box[5]) box[5] = xyz[3*j+2];
  }
  n     = 1;
  grid  = NULL;
  ppnts = NULL;
  tol   = 1.e-7*sqrt((box[3]-box[0])*(box[3]-box[0]) +
                     (box[4]-box[1])*(box[4]-box[1]) +
                     (box[5]-box[2])*(box[5]-box[2]));
  stat  = prm_CreateUV(0, ntris, bound->VSet[ivs].tris, uvf, npts, NULL, NULL,
                       uv, (prmXYZ *) xyz, &per, &ppnts);
#ifdef DEBUG
  printf(" gem_paramBound: prm_CreateUV = %d  per = %d\n", stat, per);
#endif
  if (stat > 0) {
    n    = 2;
    stat = prm_SmoothUV(3, per, ppnts, ntris, bound->VSet[ivs].tris,
                        npts, 3, uv, xyz);
#ifdef DEBUG
    printf(" gem_paraBound: prm_SmoothUV = %d\n", stat);
#endif
    if (stat == GEM_SUCCESS) {
      n    = 3;
      stat = prm_NormalizeUV(0.0, per, npts, uv);
#ifdef DEBUG
      printf(" gem_paraBound: prm_NormalizeUV = %d\n", stat);
#endif
      if (stat == GEM_SUCCESS) {
        n    = 4;
        nu   = 2*npts;
        nv   = 0;
        stat = prm_BestGrid(npts, 3, uv, xyz, ntris, bound->VSet[ivs].tris, tol,
                            per, ppnts, &nu, &nv, &grid,
                            &rmserr, &maxerr, &dotmin);
        if (stat == PRM_TOLERANCEUNMET) {
          printf(" gem_paramBound: Tolerance not met: %lf (%lf)!\n",
                 maxerr, tol);
          stat = GEM_SUCCESS;
        }
#ifdef DEBUG
        printf(" gem_paramBound: prm_BestGrid = %d  %d %d  %lf %lf (%lf)\n",
               stat, nu, nv, rmserr, maxerr, tol);
#endif
      }
    }
  }
  if (ppnts != NULL) gem_free(ppnts);
  gem_free(uvf);
  gem_free(xyz);
  gem_free(uv);
  if ((stat != GEM_SUCCESS) || (grid == NULL)) {
    printf(" gem_paramBound: Create/Smooth/Normalize/BestGrid %d = %d!\n",
           n, stat);
    return stat;
  }
  
  /* make the surface approximation */
  surface = (gemAprx2D *) gem_allocate(sizeof(gemAprx2D));
  if (surface == NULL) {
    gem_free(grid);
    return GEM_ALLOC;
  }
  surface->nrank     = 3;
  surface->periodic  = per;
  surface->nus       = nu;
  surface->nvs       = nv;
  surface->urange[0] = 0.0;
  surface->urange[1] = nu-1;
  surface->vrange[0] = 0.0;
  surface->vrange[1] = nv-1;
  surface->num       = 0;
  surface->nvm       = 0;
  surface->uvmap     = NULL;
  surface->interp    = (double *) gem_allocate(3*4*nu*nv*sizeof(double));
  if (surface->interp == NULL) {
    gem_free(surface);
    gem_free(grid);
    return GEM_ALLOC;
  }
  n = nu;
  if (nv > nu) n = nv;
  r = (double *) gem_allocate(6*n*sizeof(double));
  if (r == NULL) {
    gem_freeAprx2D(surface);
    gem_free(grid);
    return GEM_ALLOC;
  }
  stat = gem_fillCoeff2D(3, nu, nv, grid, surface->interp, r);
  gem_free(r);
  gem_free(grid);
  if (stat == 1) {
    gem_freeAprx2D(surface);
    return GEM_DEGENERATE;
  }
  bound->surface = surface;

  return ivs;
}


static int
gem_pickQuilt(gemBound *bound)
{
  int      i, j, k, n, stat, t, in[3];
  double   *areas, x1[3], x2[3], x3[3], big;
  gemQuilt *quilt;

  /* look for "use" quilts */

  for (j = i = 0; i < bound->nVSet; i++) {
    if (bound->VSet[i].nonconn != NULL) continue;
    quilt = bound->VSet[i].quilt;
    if (quilt->paramFlg == 1) j++;
  }
  if (j > 0) {
    if (j > 1)
      printf(" GEM Warning: %d VertexSets with a Use Param-Flag!\n", j);
    for (i = 0; i < bound->nVSet; i++) {
      if (bound->VSet[i].nonconn != NULL) continue;
      quilt = bound->VSet[i].quilt;
      if (quilt->paramFlg == 1) {
        if (quilt->nbface == 1) {
          bound->single = quilt->bfaces[0];
          return i;
        }
        if (bound->VSet[i].tris == NULL) return GEM_NOTESSEL;
        return gem_paramQuilt(bound, i);
      }
    }
  }

  /* examine "candidate" quilts */
  
  areas = (double *) gem_allocate(bound->nVSet*sizeof(double));
  if (areas == NULL) return GEM_ALLOC;

  for (i = 0; i < bound->nVSet; i++) areas[i] = 0.0;
  for (i = 0; i < bound->nVSet; i++) {
    if (bound->VSet[i].nonconn != NULL) continue;
    quilt = bound->VSet[i].quilt;
    if (quilt->paramFlg == -1) continue;
    for (j = 0; j < quilt->nElems; j++) {
      t = quilt->elems[j].tIndex - 1;
      for (k = 0; k < quilt->types[t].ntri; k++) {
        n     = quilt->types[t].tris[3*k  ] - 1;
        in[0] = quilt->elems[j].gIndices[n] - 1;
        n     = quilt->types[t].tris[3*k+1] - 1;
        in[1] = quilt->elems[j].gIndices[n] - 1;
        n     = quilt->types[t].tris[3*k+2] - 1;
        in[2] = quilt->elems[j].gIndices[n] - 1;
        x1[0] = quilt->points[in[1]].xyz[0] - quilt->points[in[0]].xyz[0];
        x2[0] = quilt->points[in[2]].xyz[0] - quilt->points[in[0]].xyz[0];
        x1[1] = quilt->points[in[1]].xyz[1] - quilt->points[in[0]].xyz[1];
        x2[1] = quilt->points[in[2]].xyz[1] - quilt->points[in[0]].xyz[1];
        x1[2] = quilt->points[in[1]].xyz[2] - quilt->points[in[0]].xyz[2];
        x2[2] = quilt->points[in[2]].xyz[2] - quilt->points[in[0]].xyz[2];
        CROSS(x3, x1, x2);
        areas[i] += sqrt(DOT(x3, x3))/2.0;
      }
    }
  }
  
  /* check biggest area candidate quilts first */
  do {
    big = 0.0;
    for (i = 0; i < bound->nVSet; i++)
      if (areas[i] > big) {
        big = areas[i];
        j   = i;
      }
    if (big == 0.0) continue;
    areas[j] = 0.0;
    quilt    = bound->VSet[j].quilt;
    if (quilt->nbface == 1) {
      bound->single = quilt->bfaces[0];
      gem_free(areas);
      return j;
    }
    if (bound->VSet[j].tris == NULL) continue;
    stat = gem_paramQuilt(bound, j);
    if (stat >= GEM_SUCCESS) {
      gem_free(areas);
      return stat;
    }

  } while (big != 0.0);
  
  gem_free(areas);
  printf(" GEM Info: No Candidate Quilts Found (gem_paramBound)!\n");
  return GEM_NOTFOUND;
}


int
gem_paramBound(gemDRep *drep, int boundx)
{
  int      i, j, k, n, stat, mindex, single, ivs, bound, owner;
  double   uvbox[4];
  gemPair  *pairs;
  gemQuilt *quilt;
  gemXfer  *xfer, *last;
  gemDSet  *sets;

  bound = abs(boundx);
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if (drep->bound[bound-1].nVSet == 0) return GEM_NOTCONNECT;
  if (drep->bound[bound-1].nIDs  <= 0) return GEM_NULLOBJ;
  
  /* invalidate old parameterization, if any */
  drep->bound[bound-1].uvbox[0] = drep->bound[bound-1].uvbox[1] = 0.0;
  drep->bound[bound-1].uvbox[2] = drep->bound[bound-1].uvbox[3] = 0.0;
  xfer = drep->bound[bound-1].xferList;
  while (xfer != NULL) {
    last = xfer;
    xfer = xfer->next;
    gem_freeXfer(last);
  }
  drep->bound[bound-1].xferList = NULL;
  if (drep->bound[bound-1].surface != NULL) {
    gem_freeAprx2D(drep->bound[bound-1].surface);
    drep->bound[bound-1].surface = NULL;
  }
  
  /* remove any old Dsets from VSets in the bound */
  for (i = 0; i < drep->bound[bound-1].nVSet; i++) {
    for (j = 0; j < drep->bound[bound-1].VSet[i].nSets; j++) {
      gem_free(drep->bound[bound-1].VSet[i].sets[j].name);
      gem_free(drep->bound[bound-1].VSet[i].sets[j].dset.data);
    }
    if (drep->bound[bound-1].VSet[i].sets != NULL)
      gem_free(drep->bound[bound-1].VSet[i].sets);
    drep->bound[bound-1].VSet[i].sets  = NULL;
    drep->bound[bound-1].VSet[i].nSets = 0;
  }
  
  /* remove any old quilts from connected VSets in the bound */
  for (i = 0; i < drep->bound[bound-1].nVSet; i++) {
    if (drep->bound[bound-1].VSet[i].nonconn != NULL) continue;
    mindex = gem_metDLoaded(drep->bound[bound-1].VSet[i].disMethod);
    if (mindex < 0) return mindex;
    if (drep->bound[bound-1].VSet[i].quilt != NULL) {
      freeQuilt[mindex](drep->bound[bound-1].VSet[i].quilt);
      gem_free(drep->bound[bound-1].VSet[i].quilt);
      drep->bound[bound-1].VSet[i].quilt   = NULL;
    }
    if (drep->bound[bound-1].VSet[i].tris != NULL)
      gem_free(drep->bound[bound-1].VSet[i].tris);
    drep->bound[bound-1].VSet[i].ntris = 0;
    drep->bound[bound-1].VSet[i].tris  = NULL;
  }

  /* get updated quilt info */
  for (n = i = 0; i < drep->bound[bound-1].nVSet; i++) {
    if (drep->bound[bound-1].VSet[i].nonconn != NULL) continue;
    mindex = gem_metDLoaded(drep->bound[bound-1].VSet[i].disMethod);
    quilt  = (gemQuilt *) gem_allocate(sizeof(gemQuilt));
    if (quilt == NULL) {
      for (j = 0; j < i; j++) {
        if (drep->bound[bound-1].VSet[j].nonconn != NULL) continue;
        freeQuilt[mindex](drep->bound[bound-1].VSet[j].quilt);
        gem_free(drep->bound[bound-1].VSet[j].quilt);
        drep->bound[bound-1].VSet[j].quilt = NULL;
      }
      return GEM_ALLOC;
    }
    quilt->nbface = i+1;
    stat = defQuilt[mindex](drep, drep->bound[bound-1].nIDs,
                            drep->bound[bound-1].indices, quilt);
    if (stat != GEM_SUCCESS) {
      printf(" GEM Warning: %s returns %d!\n",
             drep->bound[bound-1].VSet[i].disMethod, stat);
      gem_free(quilt);
      for (j = 0; j < i; j++) {
        if (drep->bound[bound-1].VSet[j].nonconn != NULL) continue;
        freeQuilt[mindex](drep->bound[bound-1].VSet[j].quilt);
        gem_free(drep->bound[bound-1].VSet[j].quilt);
        drep->bound[bound-1].VSet[j].quilt = NULL;
      }
      return stat;
    }
    stat = gem_checkQuilt(quilt, &drep->bound[bound-1].VSet[i].ntris,
                                 &drep->bound[bound-1].VSet[i].tris);
    if (stat != GEM_SUCCESS) {
      printf(" GEM Warning: %s quilt check = %d!\n",
             drep->bound[bound-1].VSet[i].disMethod, stat);
      freeQuilt[mindex](quilt);
      if (drep->bound[bound-1].VSet[i].tris != NULL)
        gem_free(drep->bound[bound-1].VSet[i].tris);
      drep->bound[bound-1].VSet[i].ntris = 0;
      drep->bound[bound-1].VSet[i].tris  = NULL;
      gem_free(quilt);
      for (j = 0; j < i; j++) {
        if (drep->bound[bound-1].VSet[j].nonconn != NULL) continue;
        freeQuilt[mindex](drep->bound[bound-1].VSet[j].quilt);
        gem_free(drep->bound[bound-1].VSet[j].quilt);
        if (drep->bound[bound-1].VSet[i].tris != NULL)
          gem_free(drep->bound[bound-1].VSet[i].tris);
        drep->bound[bound-1].VSet[j].quilt = NULL;
        drep->bound[bound-1].VSet[j].ntris = 0;
        drep->bound[bound-1].VSet[j].tris  = NULL;
      }
      return stat;
    }
    drep->bound[bound-1].VSet[i].quilt = quilt;
    if (quilt->nbface > n) n = quilt->nbface;
  }
  
  /* do we need to reparameterize? */
  single = 0;
  if (boundx < 0) single = 1;
  if (n == 1) {
    single = 1;
    if (drep->bound[bound-1].nVSet > 1) {
      pairs = (gemPair *) gem_allocate(drep->bound[bound-1].nVSet*
                                       sizeof(gemPair));
      if (pairs == NULL) {
        single = 0;
      } else {
        for (j = i = 0; i < drep->bound[bound-1].nVSet; i++) {
          if (drep->bound[bound-1].VSet[i].nonconn != NULL) continue;
          pairs[j] = drep->bound[bound-1].VSet[i].quilt->bfaces[0];
          j++;
        }
        stat = gem_kernelSameSurfs(drep->model, j, pairs);
        if (stat != GEM_SUCCESS) single = 0;
        gem_free(pairs);
      }
    }
  }

  if (single == 1) {
    for (i = 0; i < drep->bound[bound-1].nVSet; i++) {
      if (drep->bound[bound-1].VSet[i].nonconn != NULL) continue;
      quilt = drep->bound[bound-1].VSet[i].quilt;
      n     = 4;
      if (quilt->verts == NULL) n = 2;
      drep->bound[bound-1].single = quilt->bfaces[0];
      sets = (gemDSet *) gem_allocate(n*sizeof(gemDSet));
      if (sets == NULL) return GEM_ALLOC;
      sets[0].ivsrc     = 0;
      sets[0].name      = gem_strdup("xyz");
      sets[0].dset.npts = quilt->nPoints;
      sets[0].dset.rank = 3;
      sets[0].dset.data = (double *)
                          gem_allocate(3*sets[0].dset.npts*sizeof(double));
      sets[1].ivsrc     = 0;
      sets[1].name      = gem_strdup("uv");
      sets[1].dset.npts = quilt->nPoints;
      sets[1].dset.rank = 2;
      sets[1].dset.data = (double *)
                          gem_allocate(2*sets[1].dset.npts*sizeof(double));
      if (n == 4) {
        sets[2].ivsrc     = 0;
        sets[2].name      = gem_strdup("xyzd");
        sets[2].dset.npts = quilt->nVerts;
        sets[2].dset.rank = 3;
        sets[2].dset.data = (double *)
                            gem_allocate(3*sets[2].dset.npts*sizeof(double));
        sets[3].ivsrc     = 0;
        sets[3].name      = gem_strdup("uvd");
        sets[3].dset.npts = quilt->nVerts;
        sets[3].dset.rank = 2;
        sets[3].dset.data = (double *)
                            gem_allocate(2*sets[3].dset.npts*sizeof(double));
      }
      stat = GEM_SUCCESS;
      if (sets[0].name      == NULL) stat = GEM_ALLOC;
      if (sets[0].dset.data == NULL) stat = GEM_ALLOC;
      if (sets[1].name      == NULL) stat = GEM_ALLOC;
      if (sets[1].dset.data == NULL) stat = GEM_ALLOC;
      if (n == 4) {
        if (sets[2].name      == NULL) stat = GEM_ALLOC;
        if (sets[2].dset.data == NULL) stat = GEM_ALLOC;
        if (sets[3].name      == NULL) stat = GEM_ALLOC;
        if (sets[3].dset.data == NULL) stat = GEM_ALLOC;
      }
      if (stat == GEM_ALLOC) {
        gem_free(sets[0].name);
        gem_free(sets[0].dset.data);
        gem_free(sets[1].name);
        gem_free(sets[1].dset.data);
        if (n == 4) {
          gem_free(sets[2].name);
          gem_free(sets[2].dset.data);
          gem_free(sets[3].name);
          gem_free(sets[3].dset.data);
        }
        gem_free(sets);
        return stat;
      }
      
      for (j = 0; j < sets[0].dset.npts; j++) {
        k = quilt->points[j].findices.faces[0] - 1;
        sets[0].dset.data[3*j  ] = quilt->points[j].xyz[0];
        sets[0].dset.data[3*j+1] = quilt->points[j].xyz[1];
        sets[0].dset.data[3*j+2] = quilt->points[j].xyz[2];
        sets[1].dset.data[2*j  ] = quilt->faceUVs[k].uv[0];
        sets[1].dset.data[2*j+1] = quilt->faceUVs[k].uv[1];
        if (j == 0) {
          uvbox[0] = uvbox[1] = sets[1].dset.data[2*j  ];
          uvbox[2] = uvbox[3] = sets[1].dset.data[2*j+1];
        } else {
          if (sets[1].dset.data[2*j  ] < uvbox[0])
            uvbox[0] = sets[1].dset.data[2*j  ];
          if (sets[1].dset.data[2*j  ] > uvbox[1])
            uvbox[1] = sets[1].dset.data[2*j  ];
          if (sets[1].dset.data[2*j+1] < uvbox[2])
            uvbox[2] = sets[1].dset.data[2*j+1];
          if (sets[1].dset.data[2*j+1] > uvbox[3])
            uvbox[3] = sets[1].dset.data[2*j+1];
        }
      }
      if (quilt->verts != NULL) {
        for (j = 0; j < sets[3].dset.npts; j++) {
          sets[3].dset.data[2*j  ] = quilt->verts[j].uv[0];
          sets[3].dset.data[2*j+1] = quilt->verts[j].uv[1];
          if (sets[3].dset.data[2*j  ] < uvbox[0])
            uvbox[0] = sets[3].dset.data[2*j  ];
          if (sets[3].dset.data[2*j  ] > uvbox[1])
            uvbox[1] = sets[3].dset.data[2*j  ];
          if (sets[3].dset.data[2*j+1] < uvbox[2])
            uvbox[2] = sets[3].dset.data[2*j+1];
          if (sets[3].dset.data[2*j+1] > uvbox[3])
            uvbox[3] = sets[3].dset.data[2*j+1];
        }
        stat = gem_kernelEval(drep, quilt->bfaces[0], sets[3].dset.npts,
                              sets[3].dset.data, sets[2].dset.data);
        if (stat != GEM_SUCCESS) {
          gem_free(sets[0].name);
          gem_free(sets[0].dset.data);
          gem_free(sets[1].name);
          gem_free(sets[1].dset.data);
          gem_free(sets[2].name);
          gem_free(sets[2].dset.data);
          gem_free(sets[3].name);
          gem_free(sets[3].dset.data);
          gem_free(sets);
          return stat;
        }
      }

      drep->bound[bound-1].VSet[i].sets  = sets;
      drep->bound[bound-1].VSet[i].nSets = n;
    }
    /* set uvbox */
    drep->bound[bound-1].uvbox[0] = uvbox[0];
    drep->bound[bound-1].uvbox[1] = uvbox[1];
    drep->bound[bound-1].uvbox[2] = uvbox[2];
    drep->bound[bound-1].uvbox[3] = uvbox[3];
    /* set uvs for nonconnected VSets */
    for (i = 0; i < drep->bound[bound-1].nVSet; i++) {
      if (drep->bound[bound-1].VSet[i].nonconn == NULL) continue;
      stat = gem_fillNonConn(drep, &drep->bound[bound-1], i);
      if (stat != GEM_SUCCESS) return stat;
    }
    return GEM_SUCCESS;
  }

  /* reparameterize -- select the basis quilt */
  
  stat = gem_pickQuilt(&drep->bound[bound-1]);
  if (stat < GEM_SUCCESS) return stat;
  ivs = stat;
  printf(" GEM Info: VSet %d selected for reParam (gem_paramBound)!\n", ivs+1);
  
  /* populate "xyz" and "uv" and optionally "xyzd" and "uvd" in VSets */

  for (i = 0; i < drep->bound[bound-1].nVSet; i++) {
    if (drep->bound[bound-1].VSet[i].nonconn != NULL) continue;
    quilt = drep->bound[bound-1].VSet[i].quilt;
    n     = 4;
    if (quilt->verts == NULL) n = 2;
    sets  = (gemDSet *) gem_allocate(n*sizeof(gemDSet));
    if (sets == NULL) return GEM_ALLOC;
    sets[0].ivsrc     = 0;
    sets[0].name      = gem_strdup("xyz");
    sets[0].dset.npts = quilt->nPoints;
    sets[0].dset.rank = 3;
    sets[0].dset.data = (double *)
                        gem_allocate(3*sets[0].dset.npts*sizeof(double));
    sets[1].ivsrc     = 0;
    sets[1].name      = gem_strdup("uv");
    sets[1].dset.npts = quilt->nPoints;
    sets[1].dset.rank = 2;
    sets[1].dset.data = (double *)
                        gem_allocate(2*sets[1].dset.npts*sizeof(double));
    if (n == 4) {
      sets[2].ivsrc     = 0;
      sets[2].name      = gem_strdup("xyzd");
      sets[2].dset.npts = quilt->nVerts;
      sets[2].dset.rank = 3;
      sets[2].dset.data = (double *)
                          gem_allocate(3*sets[2].dset.npts*sizeof(double));
      sets[3].ivsrc     = 0;
      sets[3].name      = gem_strdup("uvd");
      sets[3].dset.npts = quilt->nVerts;
      sets[3].dset.rank = 2;
      sets[3].dset.data = (double *)
                          gem_allocate(2*sets[3].dset.npts*sizeof(double));
    }
    stat = GEM_SUCCESS;
    if (sets[0].name      == NULL) stat = GEM_ALLOC;
    if (sets[0].dset.data == NULL) stat = GEM_ALLOC;
    if (sets[1].name      == NULL) stat = GEM_ALLOC;
    if (sets[1].dset.data == NULL) stat = GEM_ALLOC;
    if (n == 4) {
      if (sets[2].name      == NULL) stat = GEM_ALLOC;
      if (sets[2].dset.data == NULL) stat = GEM_ALLOC;
      if (sets[3].name      == NULL) stat = GEM_ALLOC;
      if (sets[3].dset.data == NULL) stat = GEM_ALLOC;
    }
    if (stat == GEM_ALLOC) {
      gem_free(sets[0].name);
      gem_free(sets[0].dset.data);
      gem_free(sets[1].name);
      gem_free(sets[1].dset.data);
      if (n == 4) {
        gem_free(sets[2].name);
        gem_free(sets[2].dset.data);
        gem_free(sets[3].name);
        gem_free(sets[3].dset.data);
      }
      gem_free(sets);
      return stat;
    }

    for (j = 0; j < sets[0].dset.npts; j++) {
      sets[0].dset.data[3*j  ] = quilt->points[j].xyz[0];
      sets[0].dset.data[3*j+1] = quilt->points[j].xyz[1];
      sets[0].dset.data[3*j+2] = quilt->points[j].xyz[2];
    }
    if (drep->bound[bound-1].surface == NULL) {
      
      if (ivs == i) {
        for (j = 0; j < sets[1].dset.npts; j++) {
          k = quilt->points[j].findices.faces[0] - 1;
          sets[1].dset.data[2*j  ] = quilt->faceUVs[k].uv[0];
          sets[1].dset.data[2*j+1] = quilt->faceUVs[k].uv[1];
        }
      } else {
        stat = gem_kernelInvEval(drep, drep->bound[bound-1].single,
                                 quilt->nPoints, sets[0].dset.data,
                                                 sets[1].dset.data);
        if (stat != GEM_SUCCESS) {
          gem_free(sets[0].name);
          gem_free(sets[0].dset.data);
          gem_free(sets[1].name);
          gem_free(sets[1].dset.data);
          if (n == 4) {
            gem_free(sets[2].name);
            gem_free(sets[2].dset.data);
            gem_free(sets[3].name);
            gem_free(sets[3].dset.data);
          }
          gem_free(sets);
          return stat;
        }
      }
      
    } else {
  
      for (j = 0; j < sets[0].dset.npts; j++)
        gem_invInterpolate2D(drep->bound[bound-1].surface,
                             &sets[0].dset.data[3*j], &sets[1].dset.data[2*j]);
    }
    
    if (i == 0) {
      uvbox[0] = uvbox[1] = sets[1].dset.data[0];
      uvbox[2] = uvbox[3] = sets[1].dset.data[1];
    }
    for (j = 0; j < sets[1].dset.npts; j++) {
      if (sets[1].dset.data[2*j  ] < uvbox[0])
        uvbox[0] = sets[1].dset.data[2*j  ];
      if (sets[1].dset.data[2*j  ] > uvbox[1])
        uvbox[1] = sets[1].dset.data[2*j  ];
      if (sets[1].dset.data[2*j+1] < uvbox[2])
        uvbox[2] = sets[1].dset.data[2*j+1];
      if (sets[1].dset.data[2*j+1] > uvbox[3])
        uvbox[3] = sets[1].dset.data[2*j+1];
    }
    
    /* fill in data related sets */
    if (quilt->verts != NULL) {
      for (j = 0; j < sets[3].dset.npts; j++) {
        sets[3].dset.data[2*j  ] = quilt->verts[j].uv[0];
        sets[3].dset.data[2*j+1] = quilt->verts[j].uv[1];
      }
      j = 0;
      do {
        owner = quilt->verts[j].owner;
        for (k = j; k < quilt->nVerts; k++)
          if (quilt->verts[k].owner != owner) break;

        stat = gem_kernelEval(drep, quilt->bfaces[owner-1], k-j,
                              &sets[3].dset.data[2*j], &sets[2].dset.data[3*j]);
        if (stat != GEM_SUCCESS) {
          gem_free(sets[0].name);
          gem_free(sets[0].dset.data);
          gem_free(sets[1].name);
          gem_free(sets[1].dset.data);
          gem_free(sets[2].name);
          gem_free(sets[2].dset.data);
          gem_free(sets[3].name);
          gem_free(sets[3].dset.data);
          gem_free(sets);
          return stat;
        }
/*      printf("  owner = %d, start = %d, end = %d (%d)\n",
               owner, j, k, quilt->nVerts);  */
        j = k;
      } while (k < quilt->nVerts);

      if (drep->bound[bound-1].surface == NULL) {
        if (ivs != i) {
          stat = gem_kernelInvEval(drep, drep->bound[bound-1].single,
                                   quilt->nVerts, sets[2].dset.data,
                                   sets[3].dset.data);
          if (stat != GEM_SUCCESS) {
            gem_free(sets[0].name);
            gem_free(sets[0].dset.data);
            gem_free(sets[1].name);
            gem_free(sets[1].dset.data);
            gem_free(sets[2].name);
            gem_free(sets[2].dset.data);
            gem_free(sets[3].name);
            gem_free(sets[3].dset.data);
            gem_free(sets);
            return stat;
          }
        }
        
      } else {
        
        for (j = 0; j < sets[3].dset.npts; j++)
          gem_invInterpolate2D(drep->bound[bound-1].surface,
                               &sets[2].dset.data[3*j], &sets[3].dset.data[2*j]);
      }
      for (j = 0; j < sets[3].dset.npts; j++) {
        if (sets[3].dset.data[2*j  ] < uvbox[0])
          uvbox[0] = sets[3].dset.data[2*j  ];
        if (sets[3].dset.data[2*j  ] > uvbox[1])
          uvbox[1] = sets[3].dset.data[2*j  ];
        if (sets[3].dset.data[2*j+1] < uvbox[2])
          uvbox[2] = sets[3].dset.data[2*j+1];
        if (sets[3].dset.data[2*j+1] > uvbox[3])
          uvbox[3] = sets[3].dset.data[2*j+1];
      }

    }
    
    drep->bound[bound-1].VSet[i].sets  = sets;
    drep->bound[bound-1].VSet[i].nSets = n;
  }
  
  /* set uvbox */
  if (drep->bound[bound-1].surface == NULL) {
    drep->bound[bound-1].uvbox[0] = uvbox[0];
    drep->bound[bound-1].uvbox[1] = uvbox[1];
    drep->bound[bound-1].uvbox[2] = uvbox[2];
    drep->bound[bound-1].uvbox[3] = uvbox[3];
  } else {
    drep->bound[bound-1].uvbox[0] = drep->bound[bound-1].surface->urange[0];
    drep->bound[bound-1].uvbox[1] = drep->bound[bound-1].surface->urange[1];
    drep->bound[bound-1].uvbox[2] = drep->bound[bound-1].surface->vrange[0];
    drep->bound[bound-1].uvbox[3] = drep->bound[bound-1].surface->vrange[1];
  }
  /* set uvs for nonconnected VSets */
  for (i = 0; i < drep->bound[bound-1].nVSet; i++) {
    if (drep->bound[bound-1].VSet[i].nonconn == NULL) continue;
    stat = gem_fillNonConn(drep, &drep->bound[bound-1], i);
    if (stat != GEM_SUCCESS) return stat;
  }
  return GEM_SUCCESS;
}


int
gem_putData(gemDRep *drep, int bound, int vs, char *name, int nverts, int rank,
            double *data)
{
  int     i, k, iset;
  char    *dname;
  double  *ds;
  gemDSet *sets;

  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;

  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet)) return GEM_BADVSETINDEX;
  if (drep->bound[bound-1].VSet[vs-1].nonconn != NULL) return GEM_BADOBJECT;
  if (drep->bound[bound-1].VSet[vs-1].quilt   == NULL) return GEM_NOTCONNECT;
  if (drep->bound[bound-1].VSet[vs-1].nSets   == 0)    return GEM_NOTPARAMBND;
  if (name == NULL) return GEM_NULLNAME;
  if (data == NULL) return GEM_NULLVALUE;
  
  /* check the validity of the name */
  
  for (i = 0; i < nReserved; i++)
    if (strcmp(name, reserved[i]) == 0) return GEM_BADDSETNAME;
  if (strlen(name) > 3)
    if ((name[0] == 'd') && (name[1] == '/') && (name[2] == 'd'))
      return GEM_BADDSETNAME;

  for (iset = i = 0; i < drep->bound[bound-1].VSet[vs-1].nSets; i++)
    if (strcmp(name, drep->bound[bound-1].VSet[vs-1].sets[i].name) == 0)
      if (drep->bound[bound-1].VSet[vs-1].sets[i].ivsrc == 0) {
        iset = i+1;
        break;
      }

  if (iset == 0) {
    /* the name does not exist in this Vset -- check others */
    for (k = 0; k < drep->bound[bound-1].nVSet; k++) {
      if (k+1 == vs) continue;
      for (i = 0; i < drep->bound[bound-1].VSet[k].nSets; i++)
        if (strcmp(name, drep->bound[bound-1].VSet[k].sets[i].name) == 0)
          return GEM_BADDSETNAME;
    }
  } else {
    /* the name does exist in this Vset -- check rank */
    if (drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.rank != rank)
      return GEM_BADRANK;
    /* check length */
    if (drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.npts != nverts)
      return GEM_FIXEDLEN;
    /* fill */
    for (i = 0; i < rank*nverts; i++)
      drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.data[i] = data[i];
    
    return GEM_SUCCESS;
  }

  /* put the new data */
  
  dname = gem_strdup(name);
  if (dname == NULL) return GEM_ALLOC;
  ds    = (double *) gem_allocate(rank*nverts*sizeof(double));
  if (ds == NULL) {
    gem_free(dname);
    return GEM_ALLOC;
  }
  for (i = 0; i < rank*nverts; i++) ds[i] = data[i];

  iset = drep->bound[bound-1].VSet[vs-1].nSets+1;
  if (iset == 1) {
    drep->bound[bound-1].VSet[vs-1].sets = (gemDSet *)
                                           gem_allocate(sizeof(gemDSet));
    if (drep->bound[bound-1].VSet[vs-1].sets == NULL) {
      gem_free(ds);
      gem_free(dname);
      return GEM_ALLOC;
    }
  } else {
    sets = (gemDSet *) gem_reallocate(drep->bound[bound-1].VSet[vs-1].sets,
                                      iset*sizeof(gemDSet));
    if (sets == NULL) {
      gem_free(ds);
      gem_free(dname);
      return GEM_ALLOC;
    }
    drep->bound[bound-1].VSet[vs-1].sets = sets;
  }
  drep->bound[bound-1].VSet[vs-1].sets[iset-1].ivsrc     = 0;
  drep->bound[bound-1].VSet[vs-1].sets[iset-1].name      = dname;
  drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.rank = rank;
  drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.npts = nverts;
  drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.data = ds;
  drep->bound[bound-1].VSet[vs-1].nSets++;

  return GEM_SUCCESS;
}


int
gem_getData(gemDRep *drep, int bound, int vs, char *name, int meth, int *npts,
            int *rank, double **data)
{
  int     i, j, stat, iset, ires, ivsrc = 0, issrc = 0;
  double  *sdat1, *sdat2;
  gemDSet *sets;

  *npts = *rank = 0;
  *data = NULL;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;

  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if ((meth < GEM_INTERP) || (meth > GEM_CONSERVE)) return GEM_BADMETHOD;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet))  return GEM_BADVSETINDEX;
  if ((drep->bound[bound-1].VSet[vs-1].nonconn == NULL) &&
      (drep->bound[bound-1].VSet[vs-1].nSets   == 0)) return GEM_NOTPARAMBND;
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
          if (drep->bound[bound-1].VSet[j].sets[i].ivsrc == 0)
            if (strcmp(name, drep->bound[bound-1].VSet[j].sets[i].name) == 0) {
              ivsrc = j+1;
              issrc = i+1;
              break;
            }
      }
    } else {
      if (drep->bound[bound-1].VSet[vs-1].nonconn != NULL)
        return GEM_NOTCONNECT;
    }

  /* not already in the Vset -- add it */
  if (iset == 0) {

    if (ires < 0) {
      
      /* parametric sensitivity -- check for parameter */

      return GEM_UNSUPPORTED;

    } else if (ires == 0) {
      
      /* not reserved name */
      if (ivsrc == 0) return GEM_NOTFOUND;
      /* interpolate from source */
      stat = gem_dataTransfer(drep, bound, ivsrc, issrc, vs, meth, &iset,
                              Interpolate, Interpol_bar,
                              Integrate,   Integr_bar,   iEval);
      if (stat != GEM_SUCCESS) return stat;
      
    } else if (ires >= nGeomBased) {
      
      return GEM_BADNAME;
      
    } else {
      
      /* reserved name -- do the geom query */
      if (drep->bound[bound-1].VSet[vs-1].quilt != NULL) {
        *npts = drep->bound[bound-1].VSet[vs-1].quilt->nPoints;
      } else {
        return GEM_NOTCONNECT;
      }
      if (ires < 5) {

        sdat1 = (double *) gem_allocate(*npts*rreserved[2]*sizeof(double));
        if (sdat1 == NULL) return GEM_ALLOC;
        sdat2 = (double *) gem_allocate(*npts*rreserved[3]*sizeof(double));
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
        sets = (gemDSet *) gem_reallocate(drep->bound[bound-1].VSet[vs-1].sets,
                                          iset*sizeof(gemDSet));
        if (sets == NULL) {
          gem_free(sdat2);
          gem_free(sdat1);
          return GEM_ALLOC;
        }
        drep->bound[bound-1].VSet[vs-1].nSets                  = iset;
        drep->bound[bound-1].VSet[vs-1].sets[iset-2].ivsrc     = ivsrc;
        drep->bound[bound-1].VSet[vs-1].sets[iset-2].name      = gem_strdup(reserved[2]);
        drep->bound[bound-1].VSet[vs-1].sets[iset-2].dset.npts = *npts;
        drep->bound[bound-1].VSet[vs-1].sets[iset-2].dset.rank = rreserved[2];
        drep->bound[bound-1].VSet[vs-1].sets[iset-2].dset.data = sdat1;
        drep->bound[bound-1].VSet[vs-1].sets[iset-1].ivsrc     = ivsrc;
        drep->bound[bound-1].VSet[vs-1].sets[iset-1].name      = gem_strdup(reserved[3]);
        drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.npts = *npts;
        drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.rank = rreserved[3];
        drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.data = sdat2;
        if (ires == 3) iset--;

      } else {
    
        sdat1 = (double *) gem_allocate(*npts*rreserved[ires-1]*sizeof(double));
        if (sdat1 == NULL) return GEM_ALLOC;
        stat = gem_kernelCurvature(drep, bound, vs, sdat1);
        if (stat != GEM_SUCCESS) {
          gem_free(sdat1);
          return stat;
        }
        iset = drep->bound[bound-1].VSet[vs-1].nSets+1;
        sets = (gemDSet *) gem_reallocate(drep->bound[bound-1].VSet[vs-1].sets,
                                          iset*sizeof(gemDSet));
        if (sets == NULL) {
          gem_free(sdat1);
          return GEM_ALLOC;
        }
        drep->bound[bound-1].VSet[vs-1].nSets                  = iset;
        drep->bound[bound-1].VSet[vs-1].sets[iset-1].ivsrc     = ivsrc;
        drep->bound[bound-1].VSet[vs-1].sets[iset-1].name      = gem_strdup(name);
        drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.npts = *npts;
        drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.rank = rreserved[ires-1];
        drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.data = sdat1;
      }
    }
  }

  *npts = drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.npts;
  *rank = drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.rank;
  *data = drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.data;
  return GEM_SUCCESS;
}


int
gem_getDRepInfo(gemDRep *drep, gemModel **model, int *nIDs, char ***IDs,
                int *nbound, int *nattr)
{
  *model  = NULL;
  *nIDs   = 0;
  *IDs    = NULL;
  *nbound = 0;
  *nattr  = 0;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  
  *model  = drep->model;
  *nIDs   = drep->nIDs;
  *IDs    = drep->IDs;
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
gem_getVsetInfo(gemDRep *drep, int bound, int vs, int *vstype, int *nGpnt,
                int *nVerts, int *nset, char ***names, int **ivsrc, int **ranks)
{
  int  i;
  char **vnames;
  int  *vranks, *vsrcs;

  *vstype = 0;
  *nGpnt  = 0;
  *nVerts = 0;
  *names  = NULL;
  *ranks  = NULL;
  *ivsrc  = NULL;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet)) return GEM_BADINDEX;
  
  if (drep->bound[bound-1].VSet[vs-1].nonconn != NULL) {
    *vstype = 1;
    *nGpnt  = *nVerts = drep->bound[bound-1].VSet[vs-1].nonconn->npts;
  } else if (drep->bound[bound-1].VSet[vs-1].quilt != NULL) {
    *nGpnt  = drep->bound[bound-1].VSet[vs-1].quilt->nPoints;
    *nVerts = drep->bound[bound-1].VSet[vs-1].quilt->nVerts;
  }
  *nset = drep->bound[bound-1].VSet[vs-1].nSets;
  
  if (*nset > 0) {
    vranks = (int *)   gem_allocate(*nset*sizeof(int));
    if (vranks == NULL) return GEM_ALLOC;
    vnames = (char **) gem_allocate(*nset*sizeof(char *));
    if (vnames == NULL) {
      gem_free(vranks);
      return GEM_ALLOC;
    }
    vsrcs  = (int *)   gem_allocate(*nset*sizeof(int));
    if (vsrcs == NULL) {
      gem_free(vnames);
      gem_free(vranks);
      return GEM_ALLOC;
    }
    for (i = 0; i < *nset; i++) {
      vranks[i] = drep->bound[bound-1].VSet[vs-1].sets[i].dset.rank;
      vnames[i] = drep->bound[bound-1].VSet[vs-1].sets[i].name;
      vsrcs[i]  = drep->bound[bound-1].VSet[vs-1].sets[i].ivsrc;
    }
    *names = vnames;
    *ranks = vranks;
    *ivsrc = vsrcs;
  }
  
  return GEM_SUCCESS;
}
