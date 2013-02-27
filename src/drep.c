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


/* reserved DataSet names */
static int  nReserved    = 8;
static char *reserved[8] = {"xyz","uv","d1","d2","curv","inside","xyzd","uvd"};
static int  rreserved[8] = {  3,   2,   6,   9,    8,      1,      3,     2 };


/* discrete Method dynamic loading */

#define MAXMETHOD 32

typedef int  (*DLLFunc) (void);
typedef void (*fQuilt)  (gemQuilt *);
typedef int  (*dQuilt)  (const gemDRep *, int, gemPair *, gemQuilt *);
typedef void (*fTris)   (gemTri *);
typedef int  (*gTris)   (gemQuilt *, int, gemTri *);
typedef int  (*gInterp) (gemQuilt *, int, int, double *, int, double *,
                         double *);
typedef int  (*gIntegr) (gemQuilt *, int, int, gemCutFrag *, int, double *,
                         double *, double *);

static char   *metName[MAXMETHOD];
static DLL     metDLL[MAXMETHOD];
static fQuilt  freeQuilt[MAXMETHOD];
static dQuilt  defQuilt[MAXMETHOD];
static fTris   freeTris[MAXMETHOD];
static gTris   Triangulate[MAXMETHOD];
static gInterp Interpolate[MAXMETHOD];
static gIntegr Integrate[MAXMETHOD];

static int     met_nDiscr = 0;


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


static int metDLoaded(const char *name)
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
  freeQuilt[ret]   = (fQuilt)  metDLget(dll, "gemFreeQuilt",     name);
  defQuilt[ret]    = (dQuilt)  metDLget(dll, "gemDefineQuilt",   name);
  freeTris[ret]    = (fTris)   metDLget(dll, "gemFreeTriangles", name);
  Triangulate[ret] = (gTris)   metDLget(dll, "gemTriangulate",   name);
  Interpolate[ret] = (gInterp) metDLget(dll, "gemInterpolation", name);
  Integrate[ret]   = (gIntegr) metDLget(dll, "gemIntegration",   name);
  if ((freeQuilt[ret]   == NULL) || (defQuilt[ret]    == NULL) ||
      (freeTris[ret]    == NULL) || (Triangulate[ret] == NULL) ||
      (Interpolate[ret] == NULL) || (Integrate[ret]   == NULL)) {
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
gem_freeAprx2D(gemAprx2D *approx)
{
  if (approx == NULL) return;
  
  gem_free(approx->interp);
  if (approx->uvmap != NULL) gem_free(approx->uvmap);
  gem_free(approx);
}


static void
gem_freeVsets(gemBound bound)
{
  int i, j, n;
  
  for (i = 0; i < bound.nVSet; i++) {

    if (bound.VSet[i].quilt != NULL) {
      n = metDLoaded(bound.VSet[i].disMethod);
      if (n >= 0) freeQuilt[n](bound.VSet[i].quilt);
      if (bound.VSet[i].gbase != NULL) freeTris[n](bound.VSet[i].gbase);
      if (bound.VSet[i].dbase != NULL) freeTris[n](bound.VSet[i].dbase);
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
    trep[i].Edges = NULL;
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


extern int
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
gem_freeCutter(gemCutter *cutter)
{
  int i;
  
  for (i = 0; i < cutter->nEle; i++) {
    gem_free(cutter->elements[i].cutVerts);
    gem_free(cutter->elements[i].cutTris);
  }
  gem_free(cutter->elements);
}


int
gem_destroyDRep(gemDRep *drep)
{
  int       i, j;
  gemCntxt  *cntxt;
  gemDRep   *prev, *next;
  gemCutter *cutter, *last;

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
        gem_free(drep->TReps[i].conns[j].tric);
        gem_free(drep->TReps[i].conns[j].uvs);
        gem_free(drep->TReps[i].conns[j].vid);
      }
      gem_free(drep->TReps[i].Faces);
      gem_free(drep->TReps[i].conns);
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
      cutter = drep->bound[i].cutList;
      while (cutter != NULL) {
        last   = cutter;
        cutter = cutter->next;
        gem_freeCutter(last);
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
  int       i, j, hit;
  gemModel  *prev;
  gemDRep   *drep;
  gemCntxt  *cntxt;
  gemTRep   *trep;
  gemCutter *cutter, *last;
  
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
                gem_free(drep->TReps[i].Faces[j].xyzs);
                gem_free(drep->TReps[i].Faces[j].tris);
                gem_free(drep->TReps[i].conns[j].tric);
                gem_free(drep->TReps[i].conns[j].uvs);
                gem_free(drep->TReps[i].conns[j].vid);
              }
              gem_free(drep->TReps[i].Faces);
              gem_free(drep->TReps[i].conns);
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
              cutter = drep->bound[i].cutList;
              while (cutter != NULL) {
                last   = cutter;
                cutter = cutter->next;
                gem_freeCutter(last);
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
  drep->bound[*bound-1].cutList  = NULL;
  
  return GEM_SUCCESS;
}


int
gem_extendBound(gemDRep *drep, int bound, int nIDs, char **IDs)
{
  int       i, j, k, n, *iIDs;
  char      **ctmp;
  gemPair   *indices;
  gemModel  *model;
  gemBody   *body;
  gemCutter *cutter, *last;

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
  cutter = drep->bound[bound-1].cutList;
  while (cutter != NULL) {
    last   = cutter;
    cutter = cutter->next;
    gem_freeCutter(last);
  }
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
  drep->bound[bound-1].cutList   = NULL;
  
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
  int      stat, n;
  gemVSet  *temp;
  gemQuilt *quilt;
  
  *vs = 0;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if (disMethod == NULL) return GEM_NULLNAME;
  
  /* deal with discretization method */
  stat = metDLoaded(disMethod);
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
  quilt = (gemQuilt *) gem_allocate(sizeof(gemQuilt));
  if (quilt == NULL) return GEM_ALLOC;
  drep->bound[bound-1].VSet[n].disMethod = gem_strdup(disMethod);
  if (drep->bound[bound-1].VSet[n].disMethod == NULL) {
    gem_free(quilt);
    return GEM_ALLOC;
  }
  stat = defQuilt[stat](drep, drep->bound[bound-1].nIDs,
                              drep->bound[bound-1].indices, quilt);
  if (stat != GEM_SUCCESS) {
    gem_free(drep->bound[bound-1].VSet[n].disMethod);
    gem_free(quilt);
    return stat;
  }

  drep->bound[bound-1].VSet[n].quilt   = quilt;
  drep->bound[bound-1].VSet[n].gbase   = NULL;
  drep->bound[bound-1].VSet[n].dbase   = NULL;
  drep->bound[bound-1].VSet[n].nonconn = NULL;
  drep->bound[bound-1].VSet[n].nSets   = 0;
  drep->bound[bound-1].VSet[n].sets    = NULL;

  drep->bound[bound-1].nVSet++;
  *vs = n+1;

  return GEM_SUCCESS;
}


int
gem_makeVset(gemDRep *drep, int bound, int npts, double *xyz, int *vs)
{
  int       i, n;
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
  
  drep->bound[bound-1].VSet[n].quilt   = NULL;
  drep->bound[bound-1].VSet[n].gbase   = NULL;
  drep->bound[bound-1].VSet[n].dbase   = NULL;
  drep->bound[bound-1].VSet[n].nonconn = collct;
  drep->bound[bound-1].VSet[n].nSets   = 0;
  drep->bound[bound-1].VSet[n].sets    = NULL;

  drep->bound[bound-1].nVSet++;
  *vs = n+1;

  return GEM_SUCCESS;
}


int
gem_paramBound(gemDRep *drep, int bound)
{
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;

  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;

  /* populate "xyz" and "uv" and optionally "xyzd" and "uvd" in Vsets */

  
  return GEM_SUCCESS;
}


int
gem_putData(gemDRep *drep, int bound, int vs, char *name, int nverts, int rank,
            double *data)
{
  int     i, k, iset;
  char    *dname;
  gemDSet *sets;

  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;

  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet)) return GEM_BADVSETINDEX;
  if (drep->bound[bound-1].VSet[vs-1].quilt   == NULL) return GEM_NOTCONNECT;
  if (drep->bound[bound-1].VSet[vs-1].nonconn != NULL) return GEM_BADOBJECT;
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
  iset = drep->bound[bound-1].VSet[vs-1].nSets+1;
  if (iset == 0) {
    drep->bound[bound-1].VSet[vs-1].sets = (gemDSet *)
                                           gem_allocate(sizeof(gemDSet));
    if (drep->bound[bound-1].VSet[vs-1].sets == NULL) {
      gem_free(dname);
      return GEM_ALLOC;
    }
  } else {
    sets = (gemDSet *) gem_reallocate(drep->bound[bound-1].VSet[vs-1].sets,
                                      iset*sizeof(gemDSet));
    if (sets == NULL) {
      gem_free(dname);
      return GEM_ALLOC;
    }
    drep->bound[bound-1].VSet[vs-1].sets = sets;
  }
  drep->bound[bound-1].VSet[vs-1].sets[iset-1].ivsrc     = 0;
  drep->bound[bound-1].VSet[vs-1].sets[iset-1].name      = dname;
  drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.rank = rank;
  drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.npts = nverts;
  for (i = 0; i < rank*nverts; i++)
    drep->bound[bound-1].VSet[vs-1].sets[iset-1].dset.data[i] = data[i];
  drep->bound[bound-1].VSet[vs-1].nSets++;

  return GEM_SUCCESS;
}


int
gem_getData(gemDRep *drep, int bound, int vs, char *name, int xfer, int *npts,
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
  if ((xfer < GEM_INTERP) || (xfer > (GEM_CONSERVE|GEM_MOMENTS)))
    return GEM_BADMETHOD;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet)) return GEM_BADVSETINDEX;
  if ((drep->bound[bound-1].VSet[vs-1].quilt != NULL) &&
      (drep->bound[bound-1].VSet[vs-1].nSets == 0))  return GEM_NOTPARAMBND;
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

    } else if (ires == 0) {
      /* not reserved name */
      if (ivsrc == 0) return GEM_NOTFOUND;
      /* interpolate from source */
      
      
    } else {
      /* reserved name -- do the geom query */
      if (drep->bound[bound-1].VSet[vs-1].quilt != NULL) {
        *npts = drep->bound[bound-1].VSet[vs-1].quilt->nGpts;
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
gem_triVset(gemDRep *drep, int bound, int vs, char *name, int *ntris,
            int *npts, int **tris, double  **xyzs)
{
  int iset;

  *npts = *ntris = 0;
  *tris = NULL;
  *xyzs = NULL;
  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet)) return GEM_BADVSETINDEX;
  if  (drep->bound[bound-1].VSet[vs-1].nonconn != NULL) return GEM_BADOBJECT;
  if  (name == NULL) return GEM_NULLNAME;
  
  /* does the data exist in the Vset? */
  iset = gem_indexName(drep, bound, vs, name);
  if (iset == 0) return GEM_NOTFOUND;
  

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
gem_getVsetInfo(gemDRep *drep, int bound, int vs, int *vstype, int *nGpnt,
                int *nVerts, int *nset, char ***names, int **ivsrc, int **ranks)
{
  int  i;
  char **vnames;
  int  *vranks, *vsrcs;

  if (drep == NULL) return GEM_NULLOBJ;
  if (drep->magic != GEM_MDREP) return GEM_BADDREP;
  if ((bound < 1) || (bound > drep->nBound)) return GEM_BADBOUNDINDEX;
  if ((vs < 1) || (vs > drep->bound[bound-1].nVSet)) return GEM_BADINDEX;
  
  if (drep->bound[bound-1].VSet[vs-1].nonconn != NULL) {
    *vstype = 1;
    *nGpnt  = *nVerts = drep->bound[bound-1].VSet[vs-1].nonconn->npts;
  } else {
    *vstype = 0;
    *nGpnt  = drep->bound[bound-1].VSet[vs-1].quilt->nGpts;
    *nVerts = drep->bound[bound-1].VSet[vs-1].quilt->nVerts;
  }
  *nset  = drep->bound[bound-1].VSet[vs-1].nSets;
  *names = NULL;
  *ranks = NULL;
  *ivsrc = NULL;
  
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
