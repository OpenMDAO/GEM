/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Example Triangle Piecewise biLinear Continuous
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "gem.h"


extern int                              /* from triUtils.c */
gemPatchTessel(const gemDRep *drep, gemQuilt *quilt);

/* use this instead of gem_retAttribute because it links cleanly */
extern int
gem_retAttrib(/*@null@*/ gemAttrs *attr, /*@null@*/ char *name, int *aindex,
              int *atype, int *alen, int **integers, double **reals,
              char **string);


#define CROSS(a,b,c)      a[0] = (b[1]*c[2]) - (b[2]*c[1]);\
                          a[1] = (b[2]*c[0]) - (b[0]*c[2]);\
                          a[2] = (b[0]*c[1]) - (b[1]*c[0])
#define DOT(a,b)         (a[0]*b[0] + a[1]*b[1] + a[2]*b[2])


/*
 *
 * This simple (linear) example of a triangle-based discretization has its 
 * geometry positions and its data stored at the geometric vertices (i.e.
 * node-based data). There is a single type throughout.
 *
 * The triangluation is taken from the tessellation stored in the DRep, so
 * it must exist when gemDefineQuilt is called.
 *
 */


void
gemFreeQuilt(gemQuilt *quilt)           /* (in)  ptr to the Quilt to free */
{
  int i;
  
  if (quilt->types != NULL) {
    for (i = 0; i < quilt->nTypes; i++) {
      if (quilt->types[i].gst  != NULL) free(quilt->types[i].gst);
      if (quilt->types[i].tris != NULL) free(quilt->types[i].tris);
    }
    free(quilt->types);
  }
  if (quilt->points != NULL) {
    for (i = 0; i < quilt->nPoints; i++)
      if (quilt->points[i].nFaces > 2)
        if (quilt->points[i].findices.multi != NULL)
          free(quilt->points[i].findices.multi);
    free(quilt->points);
  }
  if (quilt->bfaces  != NULL) free(quilt->bfaces);
  if (quilt->faceUVs != NULL) free(quilt->faceUVs);
  if (quilt->elems   != NULL) free(quilt->elems);
  if (quilt->ptrm    != NULL) free(quilt->ptrm);
  quilt->bfaces       = NULL;
  quilt->types        = NULL;
  quilt->faceUVs      = NULL;
  quilt->points       = NULL;
  quilt->elems        = NULL;
  quilt->ptrm         = NULL;
}


int
gemDefineQuilt(const
               gemDRep  *drep,          /* (in)  the DRep pointer - read-only */
               int      nFaces,         /* (in)  number of BRep/Face pairs to
                                                 be filled */
               gemPair  bface[],        /* (in)  index to the pairs in DRep */
               gemQuilt *quilt)         /* (out) ptr to Quilt to be filled */
{
  int    i, j, k, n, iface, npts, ntris;
  int    ibr1, ibr2, ibrep, status, aindex, atype, *storage, *ibs;
  char   *string;
  double *reals;

  quilt->paramFlg = 0;                  /* make a candidate for reParam */

  quilt->verts    = NULL;               /* not required by this method */
  
  quilt->types    = NULL;               /* initialize before filling */
  quilt->bfaces   = NULL;
  quilt->faceUVs  = NULL;
  quilt->points   = NULL;
  quilt->elems    = NULL;
  quilt->ptrm     = NULL;
  
  if (drep->TReps == NULL) return GEM_NOTESSEL;
  
/*
  status = gem_retAttribute((void *) drep, 0, 0, "reParam", &aindex, &atype, &n,
                            &ibs, &reals, &string);
*/
  status = gem_retAttrib(drep->attr, "reParam", &aindex, &atype, &n, &ibs,
                         &reals, &string);
  /* on input quilt->nbface is vertexSet index */
  if (status == GEM_SUCCESS) 
    if (atype == GEM_INTEGER)
      if (n > 0)
        if (ibs[0] == quilt->nbface) quilt->paramFlg = 1;
/*
  status = gem_retAttribute((void *) drep, 0, 0, "BRep", &aindex, &atype, &n,
                            &ibs, &reals, &string);
*/
  status = gem_retAttrib(drep->attr, "BRep", &aindex, &atype, &n, &ibs, 
                         &reals, &string);
  if (status != GEM_SUCCESS) n = 0;
  if (atype  != GEM_INTEGER) n = 0;
  /* on input quilt->nbface is vertexSet index */
  if (2*quilt->nbface <= n) {
    ibr1 = ibs[2*quilt->nbface-2];
    ibr2 = ibs[2*quilt->nbface-1];
  } else {
    n = 0;
  }

  /* store away our bfaces */
  quilt->bfaces = (gemPair *) malloc(nFaces*sizeof(gemPair));
  if (quilt->bfaces == NULL) return GEM_ALLOC;
  for (npts = ntris = i = j = 0; j < nFaces; j++) {
    ibrep = bface[j].BRep  - 1;
    iface = bface[j].index - 1;
    if (ibrep < 0) continue;
    if (n != 0)
      if ((ibrep+1 != ibr1) && (ibrep+1 != ibr2)) continue;
    if ((drep->TReps[ibrep].Faces[iface].npts  == 0) ||
        (drep->TReps[ibrep].Faces[iface].ntris == 0)) continue;
    npts  += drep->TReps[ibrep].Faces[iface].npts;
    ntris += drep->TReps[ibrep].Faces[iface].ntris;
    quilt->bfaces[i] = bface[j];
    i++;
  }
  quilt->nbface = i;
  if ((i != 1) && (quilt->paramFlg == 1)) quilt->paramFlg = 0;
  if  (i == 0) {
    gemFreeQuilt(quilt);
    return GEM_NOTESSEL;
  }
  
  /* specify our single element type */
  quilt->nTypes = 1;
  quilt->types  = (gemEleType *) malloc(sizeof(gemEleType));
  if (quilt->types == NULL) {
    gemFreeQuilt(quilt);
    return GEM_ALLOC;
  }
  quilt->types[0].nref  = 3;
  quilt->types[0].ndata = 0;            /* data at geom reference positions */
  quilt->types[0].ntri  = 1;
  quilt->types[0].nmat  = 0;            /* match points at geom ref positions */
  quilt->types[0].tris  = NULL;
  quilt->types[0].gst   = NULL;
  quilt->types[0].dst   = NULL;
  quilt->types[0].matst = NULL;

  quilt->types[0].tris   = (int *) malloc(3*sizeof(int));
  if (quilt->types[0].tris == NULL) {
    gemFreeQuilt(quilt);
    return GEM_ALLOC;
  }
  quilt->types[0].tris[0] = 1;
  quilt->types[0].tris[1] = 2;
  quilt->types[0].tris[2] = 3;

  quilt->types[0].gst   = (double *) malloc(6*sizeof(double));
  if (quilt->types[0].gst == NULL) {
    gemFreeQuilt(quilt);
    return GEM_ALLOC;
  }
  quilt->types[0].gst[0] = 0.0;
  quilt->types[0].gst[1] = 0.0;
  quilt->types[0].gst[2] = 1.0;
  quilt->types[0].gst[3] = 0.0;
  quilt->types[0].gst[4] = 0.0;
  quilt->types[0].gst[5] = 1.0;
  
  /* store away points -- allow duplicates at Edges/Nodes for now */
  quilt->nPoints  = npts;
  quilt->nVerts   = npts;
  quilt->nFaceUVs = npts;
  quilt->points  = (gemPoints *) malloc(npts*sizeof(gemPoints));
  if (quilt->points == NULL) {
    gemFreeQuilt(quilt);
    return GEM_ALLOC;
  }
  for (i = 0; i < npts; i++) {
    quilt->points[i].nFaces            = 1;
    quilt->points[i].findices.faces[0] = i+1;
    quilt->points[i].findices.faces[1] = 0;
  }
  quilt->faceUVs = (gemFaceUV *) malloc(npts*sizeof(gemFaceUV));
  if (quilt->faceUVs == NULL) {
    gemFreeQuilt(quilt);
    return GEM_ALLOC;
  }
  for (i = j = 0; j < quilt->nbface; j++) {
    ibrep  = quilt->bfaces[j].BRep  - 1;
    iface  = quilt->bfaces[j].index - 1;
    for (k = 0; k < drep->TReps[ibrep].Faces[iface].npts; k++, i++) {
      quilt->faceUVs[i].owner = j+1;
      quilt->faceUVs[i].uv[0] = drep->TReps[ibrep].Faces[iface].uvs[2*k  ];
      quilt->faceUVs[i].uv[1] = drep->TReps[ibrep].Faces[iface].uvs[2*k+1];
      quilt->points[i].xyz[0] = drep->TReps[ibrep].Faces[iface].xyzs[3*k  ];
      quilt->points[i].xyz[1] = drep->TReps[ibrep].Faces[iface].xyzs[3*k+1];
      quilt->points[i].xyz[2] = drep->TReps[ibrep].Faces[iface].xyzs[3*k+2];
      quilt->points[i].lTopo  = 0;
      if (drep->TReps[ibrep].Faces[iface].vid[2*k  ] == 0) {
        quilt->points[i].lTopo = -drep->TReps[ibrep].Faces[iface].vid[2*k+1];
      } else if (drep->TReps[ibrep].Faces[iface].vid[2*k  ] > 0) {
        quilt->points[i].lTopo =  drep->TReps[ibrep].Faces[iface].vid[2*k+1];
      }
    }
  }

  /* store away triangles */
  quilt->nElems = ntris;
  quilt->elems  = (gemElement *) malloc(ntris*sizeof(gemElement));
  if (quilt->elems == NULL) {
    gemFreeQuilt(quilt);
    return GEM_ALLOC;
  }
  storage = (int *) malloc(ntris*3*sizeof(int));
  if (storage == NULL) {
    gemFreeQuilt(quilt);
    return GEM_ALLOC;
  }
  quilt->ptrm = storage;
  for (ntris = npts = i = j = 0; j < quilt->nbface; j++) {
    ibrep  = quilt->bfaces[j].BRep  - 1;
    iface  = quilt->bfaces[j].index - 1;
    for (k = 0; k < drep->TReps[ibrep].Faces[iface].ntris; k++, i++) {
      quilt->elems[i].tIndex    = 1;
      quilt->elems[i].owner     = j+1;
      quilt->elems[i].gIndices  = &storage[3*i];
      quilt->elems[i].dIndices  = NULL;
      storage[3*i  ] = drep->TReps[ibrep].Faces[iface].tris[3*k  ] + npts;
      storage[3*i+1] = drep->TReps[ibrep].Faces[iface].tris[3*k+1] + npts;
      storage[3*i+2] = drep->TReps[ibrep].Faces[iface].tris[3*k+2] + npts;
    }
    npts  += drep->TReps[ibrep].Faces[iface].npts;
    ntris += drep->TReps[ibrep].Faces[iface].ntris;
  }
  if (quilt->nbface == 1) return GEM_SUCCESS;
  
  /* patch up at Edges and Nodes */
  status = gemPatchTessel(drep, quilt);
  if (status != GEM_SUCCESS) {
    gemFreeQuilt(quilt);
    return status;
  }
  quilt->nVerts = quilt->nPoints;

  return GEM_SUCCESS;
}


extern void
gemInvEvaluation(/*@unused@*/
                 gemQuilt *quilt,       /* (in)   the quilt description */
                 /*@unused@*/
                 double   uvs[],        /* (in)   uvs that support the geom
                                                  (2*npts in length) */
                 /*@unused@*/
                 double   uv[],         /* (in)   the uv position to get st */
                 /*@unused@*/
                 int      *eIndex,      /* (both) element index (bias 1) */
                 /*@unused@*/
                 double   st[])         /* (both) the element ref coordinates
                                                  on input -- tri based guess */
{
  /* eIndex and st on input are consistent with linear triangles (geom) */

}


int
gemInterpolation(gemQuilt *quilt,       /* (in)  the quilt description */
                 /*@unused@*/
                 int      geomFlag,     /* (in)  0 - data ref, 1 - geom based */
                 int      eIndex,       /* (in)  element index (bias 1) */
                 double   st[],         /* (in)  the element ref coordinates */
                 int      rank,         /* (in)  data depth */
                 double   data[],       /* (in)  values (rank*npts in length) */
                 double   result[])     /* (out) interpolated result - (rank) */
{
  int    in[3], i;
  double we[3];
  
  we[0] = 1.0 - st[0] - st[1];
  we[1] = st[0];
  we[2] = st[1];
  in[0] = quilt->elems[eIndex-1].gIndices[0] - 1;
  in[1] = quilt->elems[eIndex-1].gIndices[1] - 1;
  in[2] = quilt->elems[eIndex-1].gIndices[2] - 1;
  for (i = 0; i < rank; i++)
    result[i] = data[rank*in[0]+i]*we[0] + data[rank*in[1]+i]*we[1] +
                data[rank*in[2]+i]*we[2];
  
  return GEM_SUCCESS;
}


int
gemInterpolate_bar(gemQuilt *quilt,     /* (in)   the quilt description */
                   /*@unused@*/
                   int      geomFlag,   /* (in)   0 - data ref, 1 - geom based */
                   int      eIndex,     /* (in)   element index (bias 1) */
                   double   st[],       /* (in)   the element ref coordinates */
                   int      rank,       /* (in)   data depth */
                   double   res_bar[],  /* (in)   d(objective)/d(result)
                                                  (rank in length) */
                   double   dat_bar[])  /* (both) d(objective)/d(data)
                                                  (rank*npts in len) */
{
  int    in[3], i;
  double we[3];
  
  we[0] = 1.0 - st[0] - st[1];
  we[1] = st[0];
  we[2] = st[1];
  in[0] = quilt->elems[eIndex-1].gIndices[0] - 1;
  in[1] = quilt->elems[eIndex-1].gIndices[1] - 1;
  in[2] = quilt->elems[eIndex-1].gIndices[2] - 1;
  for (i = 0; i < rank; i++) {
/*  result[i] = data[rank*in[0]+i]*we[0] + data[rank*in[1]+i]*we[1] +
                data[rank*in[2]+i]*we[2];  */
    dat_bar[rank*in[0]+i] += we[0]*res_bar[i];
    dat_bar[rank*in[1]+i] += we[1]*res_bar[i];
    dat_bar[rank*in[2]+i] += we[2]*res_bar[i];
  }
  
  return GEM_SUCCESS;
}


int
gemIntegration(gemQuilt *quilt,       /* (in)  the quilt to integrate upon */
               /*@unused@*/
               int      geomFlag,     /* (in)  0 - data ref, 1 - geom based */
               int      eIndex,       /* (in)  element index (bias 1) */
               int      rank,         /* (in)  data depth */
               double   data[],       /* (in)  values (rank*npts in length) */
               double   result[])     /* (out) integrated result - (rank) */
{
  int    i, in[3];
  double x1[3], x2[3], x3[3], area;

  /* element indices */

  in[0] = quilt->elems[eIndex-1].gIndices[0] - 1;
  in[1] = quilt->elems[eIndex-1].gIndices[1] - 1;
  in[2] = quilt->elems[eIndex-1].gIndices[2] - 1;
  
  x1[0] = quilt->points[in[1]].xyz[0] - quilt->points[in[0]].xyz[0];
  x2[0] = quilt->points[in[2]].xyz[0] - quilt->points[in[0]].xyz[0];
  x1[1] = quilt->points[in[1]].xyz[1] - quilt->points[in[0]].xyz[1];
  x2[1] = quilt->points[in[2]].xyz[1] - quilt->points[in[0]].xyz[1];
  x1[2] = quilt->points[in[1]].xyz[2] - quilt->points[in[0]].xyz[2];
  x2[2] = quilt->points[in[2]].xyz[2] - quilt->points[in[0]].xyz[2];
  CROSS(x3, x1, x2);
  area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */

  for (i = 0; i < rank; i++)
    result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] +
                 data[rank*in[2]+i])*area;

  return GEM_SUCCESS;
}


int
gemIntegrate_bar(gemQuilt *quilt,     /* (in)   the quilt to integrate upon */
                 /*@unused@*/
                 int      geomFlag,   /* (in)   0 - data ref, 1 - geom based */
                 int      eIndex,     /* (in)   element index (bias 1) */
                 int      rank,       /* (in)   data depth */
                 double   res_bar[],  /* (in)   d(objective)/d(result)
                                                (rank in length) */
                 double   dat_bar[])  /* (both) d(objective)/d(data)
                                                (rank*npts in len) */
{
  int    i, in[3];
  double x1[3], x2[3], x3[3], area;
  
  /* element indices */
  
  in[0] = quilt->elems[eIndex-1].gIndices[0] - 1;
  in[1] = quilt->elems[eIndex-1].gIndices[1] - 1;
  in[2] = quilt->elems[eIndex-1].gIndices[2] - 1;
  
  x1[0] = quilt->points[in[1]].xyz[0] - quilt->points[in[0]].xyz[0];
  x2[0] = quilt->points[in[2]].xyz[0] - quilt->points[in[0]].xyz[0];
  x1[1] = quilt->points[in[1]].xyz[1] - quilt->points[in[0]].xyz[1];
  x2[1] = quilt->points[in[2]].xyz[1] - quilt->points[in[0]].xyz[1];
  x1[2] = quilt->points[in[1]].xyz[2] - quilt->points[in[0]].xyz[2];
  x2[2] = quilt->points[in[2]].xyz[2] - quilt->points[in[0]].xyz[2];
  CROSS(x3, x1, x2);
  area  = sqrt(DOT(x3, x3))/6.0;      /* 1/2 for area and then 1/3 for sum */
  
  for (i = 0; i < rank; i++) {
/*  result[i] = (data[rank*in[0]+i] + data[rank*in[1]+i] +
                 data[rank*in[2]+i])*area;  */
    dat_bar[rank*in[0]+i] += area*res_bar[i];
    dat_bar[rank*in[1]+i] += area*res_bar[i];
    dat_bar[rank*in[2]+i] += area*res_bar[i];
  }
  
  return GEM_SUCCESS;
}
