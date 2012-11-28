/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Example Triangle Discretization Method
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "gem.h"

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
    for (i = 0; i < quilt->nTypes; i++)
      if (quilt->types[i].st != NULL) free(quilt->types[i].st);
    free(quilt->types);
  }
  if (quilt->points != NULL) {
    for (i = 0; i < quilt->nPoints; i++)
      if (quilt->points[i].nFaces > 2)
        if (quilt->points[i].findices.multi != NULL)
          free(quilt->points[i].findices.multi);
    free(quilt->points);
  }
  if (quilt->faceUVs != NULL) free(quilt->faceUVs);
  if (quilt->elems   != NULL) free(quilt->elems);
  if (quilt->ptrm    != NULL) free(quilt->ptrm);
  quilt->types        = NULL;
  quilt->faceUVs      = NULL;
  quilt->points       = NULL;
  quilt->elems        = NULL;
  quilt->ptrm         = NULL;
}


static void
zipit(const gemDRep *drep, int ibrep, int iface, int *storage, int i0, int i1,
      int m, int side, int np, int ne, int *zipper, int *table)
{
  int i, tri, vid0[2], vid1[2];

  vid0[0] = drep->TReps[ibrep].conns[iface].vid[2*i0  ];
  vid0[1] = drep->TReps[ibrep].conns[iface].vid[2*i0+1];
  vid1[0] = drep->TReps[ibrep].conns[iface].vid[2*i1  ];
  vid1[1] = drep->TReps[ibrep].conns[iface].vid[2*i1+1];
  for (i = 0; i < ne-1; i++) {
    if (((zipper[8*i+1] == vid0[0]) && (zipper[8*i+2] == vid0[1])) &&
        ((zipper[8*i+4] == vid1[0]) && (zipper[8*i+5] == vid1[1]))) {
      /* mark points to be removed */
      if (table[i0+np] > 0) table[i0+np] = -zipper[8*i  ];
      if (table[i1+np] > 0) table[i1+np] = -zipper[8*i+3];
      /* readjust neighbors */
      tri = zipper[8*i+6]-1;
      storage[6*tri+zipper[8*i+7]] = m+1;
      storage[6*m+side+3]          = tri+1;
      return;
    }
    if (((zipper[8*i+1] == vid1[0]) && (zipper[8*i+2] == vid1[1])) &&
        ((zipper[8*i+4] == vid0[0]) && (zipper[8*i+5] == vid0[1]))) {
      /* mark points to be removed */
      if (table[i0+np] > 0) table[i0+np] = -zipper[8*i+3];
      if (table[i1+np] > 0) table[i1+np] = -zipper[8*i  ];
      /* readjust neighbors */
      tri = zipper[8*i+6]-1;
      storage[6*tri+zipper[8*i+7]] = m+1;
      storage[6*m+side+3]          = tri+1;
      return;
    }

  }
  printf(" zipit Error: Segment not found %d %d!\n", i0, i1);
}


static void
zipps(const gemDRep *drep, int ibrep, int iface, int i0, int i1,
      int m, int side, int np, int n, int *zipper)
{
  zipper[8*n  ] = i0 + np + 1;
  zipper[8*n+1] = drep->TReps[ibrep].conns[iface].vid[2*i0  ];
  zipper[8*n+2] = drep->TReps[ibrep].conns[iface].vid[2*i0+1];
  zipper[8*n+3] = i1 + np + 1;
  zipper[8*n+4] = drep->TReps[ibrep].conns[iface].vid[2*i1  ];
  zipper[8*n+5] = drep->TReps[ibrep].conns[iface].vid[2*i1+1];
  zipper[8*n+6] = m + 1;
  zipper[8*n+7] = side + 3;
}


int
gemDefineQuilt(const
               gemDRep  *drep,          /* (in)  the DRep pointer - read-only */
               int      nFaces,         /* (in)  number of BRep/Face pairs to
                                                 be filled */
               gemPair  bface[],        /* (in)  index to the pairs in DRep */
               gemQuilt *quilt)         /* (out) ptr to Quilt to be filled */
{
  int i, j, k, m, n, ibrep, iface, iedge, npts, ntris, nbrep, ne, nt, np, i0, i1;
  int *storage, *table, *ibs, *ies, *zipper;

  quilt->geomIndices = NULL;            /* geometry and data verts match */
  quilt->dataIndices = NULL;
  quilt->edata       = NULL;

  quilt->types       = NULL;            /* initialize before filling */
  quilt->faceUVs     = NULL;
  quilt->points      = NULL;
  quilt->elems       = NULL;
  quilt->ptrm        = NULL;
  
  if (drep->TReps == NULL) return GEM_NOTESSEL;

  for (npts = ntris = j = 0; j < nFaces; j++) {
    ibrep  = bface[j].BRep  - 1;
    iface  = bface[j].index - 1;
    if (ibrep < 0) continue;
    npts  += drep->TReps[ibrep].Faces[iface].npts;
    ntris += drep->TReps[ibrep].Faces[iface].ntris;
  }
  if ((npts == 0) || (ntris == 0)) return GEM_NOTESSEL;
  
  /* specify our single element type */
  quilt->nTypes = 1;
  quilt->types  = (gemEleType *) malloc(sizeof(gemEleType));
  if (quilt->types == NULL) {
    gemFreeQuilt(quilt);
    return GEM_ALLOC;
  }
  quilt->types[0].nside = 3;
  quilt->types[0].nref  = 3;
  quilt->types[0].st    = (double *) malloc(6*sizeof(double));
  if (quilt->types[0].st == NULL) {
    gemFreeQuilt(quilt);
    return GEM_ALLOC;
  }
  quilt->types[0].st[0] = 0.0;
  quilt->types[0].st[1] = 0.0;
  quilt->types[0].st[2] = 1.0;
  quilt->types[0].st[3] = 0.0;
  quilt->types[0].st[4] = 0.0;
  quilt->types[0].st[5] = 1.0;
  
  /* store away points -- allow duplicates at Edges/Nodes for now */
  quilt->nPoints = npts;
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
  for (i = j = 0; j < nFaces; j++) {
    ibrep  = bface[j].BRep  - 1;
    iface  = bface[j].index - 1;
    if (ibrep < 0) continue;
    for (k = 0; k < drep->TReps[ibrep].Faces[iface].npts; k++, i++) {
      quilt->faceUVs[i].bface  =  bface[j];
      quilt->faceUVs[i].uv[0]  =  drep->TReps[ibrep].conns[iface].uvs[2*k  ];
      quilt->faceUVs[i].uv[1]  =  drep->TReps[ibrep].conns[iface].uvs[2*k+1];
      quilt->points[i].xyz[0]  =  drep->TReps[ibrep].Faces[iface].xyzs[3*k  ];
      quilt->points[i].xyz[1]  =  drep->TReps[ibrep].Faces[iface].xyzs[3*k+1];
      quilt->points[i].xyz[2]  =  drep->TReps[ibrep].Faces[iface].xyzs[3*k+2];
      quilt->points[i].lTopo   =  0;
      if (drep->TReps[ibrep].conns[iface].vid[2*k  ] == 0) {
        quilt->points[i].lTopo = -drep->TReps[ibrep].conns[iface].vid[2*k+1];
      } else if (drep->TReps[ibrep].conns[iface].vid[2*k  ] > 0) {
        quilt->points[i].lTopo =  drep->TReps[ibrep].conns[iface].vid[2*k+1];
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
  storage = (int *) malloc(ntris*6*sizeof(int));
  if (storage == NULL) {
    gemFreeQuilt(quilt);
    return GEM_ALLOC;
  }
  quilt->ptrm = storage;
  for (ntris = npts = i = j = 0; j < nFaces; j++) {
    ibrep  = bface[j].BRep  - 1;
    iface  = bface[j].index - 1;
    if (ibrep < 0) continue;
    for (k = 0; k < drep->TReps[ibrep].Faces[iface].ntris; k++, i++) {
      quilt->elems[i].type      = &quilt->types[0];
      quilt->elems[i].indices   = &storage[6*i  ];
      quilt->elems[i].neighbors = &storage[6*i+3];
      storage[6*i  ] = drep->TReps[ibrep].Faces[iface].tris[3*k  ] + npts;
      storage[6*i+1] = drep->TReps[ibrep].Faces[iface].tris[3*k+1] + npts;
      storage[6*i+2] = drep->TReps[ibrep].Faces[iface].tris[3*k+2] + npts;
      storage[6*i+3] = drep->TReps[ibrep].conns[iface].tric[3*k  ];
      storage[6*i+4] = drep->TReps[ibrep].conns[iface].tric[3*k+1];
      storage[6*i+5] = drep->TReps[ibrep].conns[iface].tric[3*k+2];
      if (storage[6*i+3] > 0) storage[6*i+3] += ntris;
      if (storage[6*i+4] > 0) storage[6*i+4] += ntris;
      if (storage[6*i+5] > 0) storage[6*i+5] += ntris;
    }
    npts  += drep->TReps[ibrep].Faces[iface].npts;
    ntris += drep->TReps[ibrep].Faces[iface].ntris;
  }
  if (nFaces == 1) return GEM_SUCCESS;
  
  /*
   * a continuous discretization:
   * remove duplicates at Edge and Nodes where different Faces touch 
   */
  
  table = (int *) malloc(npts*sizeof(int));
  if (table == NULL) {
    gemFreeQuilt(quilt);
    return GEM_ALLOC;
  }
  for (i = 0; i < npts; i++) table[i] = i+1;

  for (nbrep = j = 0; j < nFaces; j++)
    if (bface[j].BRep > nbrep) nbrep = bface[j].BRep;
  ibs = (int *) malloc(nbrep*sizeof(int));
  if (ibs == NULL) {
    free(table);
    gemFreeQuilt(quilt);
    return GEM_ALLOC;
  }
  for (i = 0; i < nbrep;  i++) ibs[i] = 0;
  for (j = 0; j < nFaces; j++)
    if (bface[j].BRep > 0) ibs[bface[j].BRep-1]++;
  
  /* Faces that touch must be from the same BReps */
  for (i = 0; i < nbrep;  i++) {
    if (ibs[i] <= 1) continue;
    
    /* do this from the perspective of the Edge (2 Faces at max) */
    ies = (int *) malloc(2*drep->TReps[i].nEdges*sizeof(int));
    if (ies == NULL) {
      free(ibs);
      free(table);
      gemFreeQuilt(quilt);
      return GEM_ALLOC;
    }
    for (k = 0; k < 2*drep->TReps[i].nEdges; k++) ies[k] = 0;
      
    /* mark the Faces in the Edge */
    for (j = 0; j < nFaces; j++) {
      ibrep  = bface[j].BRep  - 1;
      iface  = bface[j].index - 1;
      if (ibrep != i) continue;
      for (k = 0; k < drep->TReps[ibrep].Faces[iface].ntris; k++) {
        if (drep->TReps[ibrep].conns[iface].tric[3*k  ] < 0) {
          iedge = -drep->TReps[ibrep].conns[iface].tric[3*k  ] - 1;
          if (ies[2*iedge] == 0) {
            ies[2*iedge] = j+1;
          } else if (ies[2*iedge] != j+1) {
            if (ies[2*iedge+1] == 0) {
              ies[2*iedge+1] = j+1;
            } else if (ies[2*iedge+1] != j+1) {
              printf(" Zipper Error: Edge %d has at least 3 Faces!\n", iedge+1);
            }
          }
        }
        if (drep->TReps[ibrep].conns[iface].tric[3*k+1] < 0) {
          iedge = -drep->TReps[ibrep].conns[iface].tric[3*k+1] - 1;
          if (ies[2*iedge] == 0) {
            ies[2*iedge] = j+1;
          } else if (ies[2*iedge] != j+1) {
            if (ies[2*iedge+1] == 0) {
              ies[2*iedge+1] = j+1;
            } else if (ies[2*iedge+1] != j+1) {
              printf(" Zipper Error: Edge %d has at least 3 Faces!\n", iedge+1);
            }
          }
        }
        if (drep->TReps[ibrep].conns[iface].tric[3*k+2] < 0) {
          iedge = -drep->TReps[ibrep].conns[iface].tric[3*k+2] - 1;
          if (ies[2*iedge] == 0) {
            ies[2*iedge] = j+1;
          } else if (ies[2*iedge] != j+1) {
            if (ies[2*iedge+1] == 0) {
              ies[2*iedge+1] = j+1;
            } else if (ies[2*iedge+1] != j+1) {
              printf(" Zipper Error: Edge %d has at least 3 Faces!\n", iedge+1);
            }
          }
        }
      }
    }
      
    /* zipper up matching Faces at the Edges */
    for (j = 0; j < drep->TReps[i].nEdges; j++) {
      if (ies[2*j+1] == 0) continue;
      ne     = drep->TReps[i].Edges[j].npts;
      zipper = (int *) malloc(8*(ne-1)*sizeof(int));
      if (zipper == NULL) {
        free(ies);
        free(ibs);
        free(table);
        gemFreeQuilt(quilt);
        return GEM_ALLOC;
      }
      for (nt = np = k = 0; k < ies[2*j]; k++) {
        ibrep = bface[k].BRep  - 1;
        iface = bface[k].index - 1;
        
        /* store data from the source Face -- segment at a time */
        if (k == ies[2*j]-1)
          for (n = m = 0; m < drep->TReps[ibrep].Faces[iface].ntris; m++) {
            if (drep->TReps[ibrep].conns[iface].tric[3*m  ] == -(j+1)) {
              i0 = drep->TReps[ibrep].Faces[iface].tris[3*m+1]-1;
              i1 = drep->TReps[ibrep].Faces[iface].tris[3*m+2]-1;
              zipps(drep, ibrep, iface, i0, i1, m+nt, 0, np, n, zipper);
              n++;
            }
            if (drep->TReps[ibrep].conns[iface].tric[3*m+1] == -(j+1)) {
              i0 = drep->TReps[ibrep].Faces[iface].tris[3*m  ]-1;
              i1 = drep->TReps[ibrep].Faces[iface].tris[3*m+2]-1;
              zipps(drep, ibrep, iface, i0, i1, m+nt, 1, np, n, zipper);
              n++;
            }
            if (drep->TReps[ibrep].conns[iface].tric[3*m+2] == -(j+1)) {
              i0 = drep->TReps[ibrep].Faces[iface].tris[3*m  ]-1;
              i1 = drep->TReps[ibrep].Faces[iface].tris[3*m+1]-1;
              zipps(drep, ibrep, iface, i0, i1, m+nt, 2, np, n, zipper);
              n++;
            }
          }
        np += drep->TReps[ibrep].Faces[iface].npts;
        nt += drep->TReps[ibrep].Faces[iface].ntris;
      }

      /* patch up segments from the destination Face */
      for (nt = np = k = 0; k < ies[2*j+1]; k++) {
        ibrep = bface[k].BRep  - 1;
        iface = bface[k].index - 1;

        if (k == ies[2*j+1]-1)
          for (n = m = 0; m < drep->TReps[ibrep].Faces[iface].ntris; m++) {
            if (drep->TReps[ibrep].conns[iface].tric[3*m  ] == -(j+1)) {
              i0 = drep->TReps[ibrep].Faces[iface].tris[3*m+1]-1;
              i1 = drep->TReps[ibrep].Faces[iface].tris[3*m+2]-1;
              zipit(drep, ibrep, iface, storage, i0, i1, m+nt, 0, np,
                    ne, zipper, table);
            }
            if (drep->TReps[ibrep].conns[iface].tric[3*m+1] == -(j+1)) {
              i0 = drep->TReps[ibrep].Faces[iface].tris[3*m  ]-1;
              i1 = drep->TReps[ibrep].Faces[iface].tris[3*m+2]-1;
              zipit(drep, ibrep, iface, storage, i0, i1, m+nt, 1, np,
                    ne, zipper, table);
            }
            if (drep->TReps[ibrep].conns[iface].tric[3*m+2] == -(j+1)) {
              i0 = drep->TReps[ibrep].Faces[iface].tris[3*m  ]-1;
              i1 = drep->TReps[ibrep].Faces[iface].tris[3*m+1]-1;
              zipit(drep, ibrep, iface, storage, i0, i1, m+nt, 2, np,
                    ne, zipper, table);
            }
          }
        np += drep->TReps[ibrep].Faces[iface].npts;
        nt += drep->TReps[ibrep].Faces[iface].ntris;
      }
      free(zipper);
    }
    free(ies);
  }
  free(ibs);
  
  /* adjust the point definition to add faceUVs for dups */
  for (i = 0; i < npts; i++)
    if (table[i] < 0) {
      j = -table[i]-1;
      if (quilt->points[j].nFaces == 1) {
        quilt->points[j].findices.faces[1] = i+1;
      } else if (quilt->points[j].nFaces == 2) {
        i0 = quilt->points[j].findices.faces[0];
        i1 = quilt->points[j].findices.faces[1];
        quilt->points[j].findices.multi = (int *) malloc(3*sizeof(int));
        if (quilt->points[j].findices.multi == NULL) {
          free(table);
          gemFreeQuilt(quilt);
          return GEM_ALLOC;
        }
        quilt->points[j].findices.multi[0] = i0;
        quilt->points[j].findices.multi[1] = i1;
        quilt->points[j].findices.multi[2] = i+1;
      } else {
        ies = (int *) realloc( quilt->points[j].findices.multi,
                              (quilt->points[j].nFaces+1)*sizeof(int));
        if (ies == NULL) {
          free(table);
          gemFreeQuilt(quilt);
          return GEM_ALLOC;
        }
        ies[quilt->points[j].nFaces]    = i+1;
        quilt->points[j].findices.multi = ies;
      }
      quilt->points[j].nFaces++;
    }
  /* remove the indices that we don't use anymore */
  for (i = 0; i < ntris; i++) {
    storage[6*i  ] = abs(table[storage[6*i  ]-1]);
    storage[6*i+1] = abs(table[storage[6*i+1]-1]);
    storage[6*i+2] = abs(table[storage[6*i+2]-1]);
  }
  /* crunch the point space */
  for (j = i = 0; i < npts; i++) {
    if (table[i] < 0) continue;
    quilt->points[j] = quilt->points[i];
    j++;
    table[i] = j;
  }
  quilt->nPoints = j;
  /* reassign the triangle indices based on new numbering */
  for (i = 0; i < ntris; i++) {
    storage[6*i  ] = table[storage[6*i  ]-1];
    storage[6*i+1] = table[storage[6*i+1]-1];
    storage[6*i+2] = table[storage[6*i+2]-1];
  }
  free(table);

  return GEM_SUCCESS;
}


void
gemFreeTriangles(gemTri *tessel)        /* (in)  ptr to triangulation to free */
{
  if (tessel->tris != NULL) free(tessel->tris);
  if (tessel->xyzs != NULL) free(tessel->xyzs);
  tessel->tris      = NULL;
  tessel->xyzs      = NULL;
}


int
gemTriangulate(gemQuilt *quilt,         /* (in)  the quilt description */
               /*@unused@*/
               int      geomFlag,       /* (in)  0 - data ref, 1 - geom based */
               gemTri   *tessel)        /* (out) ptr to the triangulation */
{
  int i;

  tessel->ptrm  = NULL;                 /* not used */
  tessel->tris  = NULL;
  tessel->xyzs  = NULL;
  tessel->ntris = quilt->nElems;
  tessel->npts  = quilt->nPoints;

  tessel->tris  = (int *) malloc(3*tessel->ntris*sizeof(int));
  if (tessel->tris == NULL) return GEM_ALLOC;
  for (i = 0; i < tessel->ntris; i++) {
    tessel->tris[3*i  ] = quilt->elems[i].indices[0];
    tessel->tris[3*i+1] = quilt->elems[i].indices[1];
    tessel->tris[3*i+2] = quilt->elems[i].indices[2];
  }
  
  tessel->xyzs  = (double *) malloc(3*tessel->npts*sizeof(double));
  if (tessel->xyzs == NULL) {
    free(tessel->tris);
    tessel->tris = NULL;
    return GEM_ALLOC;
  }
  for (i = 0; i < tessel->npts; i++) {
    tessel->xyzs[3*i  ] = quilt->points[i].xyz[0];
    tessel->xyzs[3*i+1] = quilt->points[i].xyz[1];
    tessel->xyzs[3*i+2] = quilt->points[i].xyz[2];
  }

  return GEM_SUCCESS;
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
  in[0] = quilt->elems[eIndex-1].indices[0] - 1;
  in[1] = quilt->elems[eIndex-1].indices[1] - 1;
  in[2] = quilt->elems[eIndex-1].indices[2] - 1;
  for (i = 0; i < rank; i++)
    result[i] = data[rank*in[0]+i]*we[0] + data[rank*in[1]+i]*we[1] +
                data[rank*in[2]+i]*we[2];
  
  return GEM_SUCCESS;
}


int
gemIntegration(gemQuilt   *quilt,       /* (in)  the quilt to integrate upon */
               int        whichQ,       /* (in)  1 or 2 (cut quilt index) */
               int        nFrags,       /* (in)  # of fragments to integrate */
               gemCutFrag frags[],      /* (in)  cut element fragments */
               int        rank,         /* (in)  data depth */
               double     data[],       /* (in)  values (rank*npts in length) */
               double     xyzs[],       /* (in)  coordinates (3*npts in len) */
               double     result[])     /* (out) integrated result - (rank) */
{
  int    i, j, k, m, i0, i1, i2, eindex, stat;
  double x1[3], x2[3], x3[3], area, *st, *datac, *xyzc;

  for (k = 0; k < rank; k++) result[k] = 0.0;
  for (m = i = 0; i < nFrags; i++)
    if (frags[i].nCutVerts > m) m = frags[i].nCutVerts;
  if (m == 0) return GEM_SUCCESS;

  /* get temp arrays large enough for the biggest fragment */
  datac = (double *) malloc(rank*m*sizeof(double));
  if (datac == NULL) return GEM_ALLOC;
  xyzc  = (double *) malloc(3*m*sizeof(double));
  if (xyzc == NULL) {
    free(datac);
    return GEM_ALLOC;
  }

  /* integrate a fragment at a time */
  for (i = 0; i < nFrags; i++) {
    
    /* get the appropriate element index */
    if (whichQ == 1) {
      eindex = frags[i].e1Index;
    } else {
      eindex = frags[i].e2Index;
    }
    
    /* store away the data/coordinates at the cut positions */
    for (j = 0; j < frags[i].nCutVerts; j++) {
      if (whichQ == 1) {
        st = frags[i].cutVerts[j].st1;
      } else {
        st = frags[i].cutVerts[j].st2;
      }
      stat = gemInterpolation(quilt, 0, eindex, st, rank, data, &datac[j*rank]);
      if (stat != GEM_SUCCESS) {
        free(xyzc);
        free(datac);
        return stat;
      }
      stat = gemInterpolation(quilt, 1, eindex, st, 3, xyzs, &xyzc[j*3]);
      if (stat != GEM_SUCCESS) {
        free(xyzc);
        free(datac);
        return stat;
      }
    }
    
    /* integrate over the fragment cut triangles */
    for (j = 0; j < frags[i].nCutTris; j++) {
      i0    = frags[i].cutTris[j].indices[0]-1;
      i1    = frags[i].cutTris[j].indices[1]-1;
      i2    = frags[i].cutTris[j].indices[2]-1;
      x1[0] = xyzc[3*i1  ] - xyzc[3*i0  ];
      x2[0] = xyzc[3*i2  ] - xyzc[3*i0  ];
      x1[1] = xyzc[3*i1+1] - xyzc[3*i0+1];
      x2[1] = xyzc[3*i2+1] - xyzc[3*i0+1];
      x1[2] = xyzc[3*i1+2] - xyzc[3*i0+2];
      x2[2] = xyzc[3*i2+2] - xyzc[3*i0+2];
      CROSS(x3, x1, x2);
      area = DOT(x3, x3)/6.0;            /* 1/2 for area and then 1/3 for sum */
      
      for (k = 0; k < rank; k++)
        result[k] += (datac[rank*i0+k]+datac[rank*i1+k]+datac[rank*i2+k])*area;

    }
  }
  
  free(xyzc);
  free(datac);
  return GEM_SUCCESS;
}
