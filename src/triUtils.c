/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Example Triangle Plugin Utility Functions
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "gem.h"


/* functions for zippering up Edges */

static void
zipit(const gemDRep *drep, int ibrep, int iface, int i0, int i1, int np, int ne,
      int *zipper, int *table)
{
  int i, vid0[2], vid1[2];

  vid0[0] = drep->TReps[ibrep].Faces[iface].vid[2*i0  ];
  vid0[1] = drep->TReps[ibrep].Faces[iface].vid[2*i0+1];
  vid1[0] = drep->TReps[ibrep].Faces[iface].vid[2*i1  ];
  vid1[1] = drep->TReps[ibrep].Faces[iface].vid[2*i1+1];
  for (i = 0; i < ne-1; i++) {
    if (((zipper[6*i+1] == vid0[0]) && (zipper[6*i+2] == vid0[1])) &&
        ((zipper[6*i+4] == vid1[0]) && (zipper[6*i+5] == vid1[1]))) {
      /* mark points to be removed */
      if (table[i0+np] > 0) table[i0+np] = -zipper[6*i  ];
      if (table[i1+np] > 0) table[i1+np] = -zipper[6*i+3];
      return;
    }
    if (((zipper[6*i+1] == vid1[0]) && (zipper[6*i+2] == vid1[1])) &&
        ((zipper[6*i+4] == vid0[0]) && (zipper[6*i+5] == vid0[1]))) {
      /* mark points to be removed */
      if (table[i0+np] > 0) table[i0+np] = -zipper[6*i+3];
      if (table[i1+np] > 0) table[i1+np] = -zipper[6*i  ];
      return;
    }

  }
  printf(" zipit Error: Segment not found %d %d!\n", i0, i1);
}


static void
zipps(const gemDRep *drep, int ibrep, int iface, int i0, int i1, int np, int n,
      int *zipper)
{
  zipper[6*n  ] = i0 + np + 1;
  zipper[6*n+1] = drep->TReps[ibrep].Faces[iface].vid[2*i0  ];
  zipper[6*n+2] = drep->TReps[ibrep].Faces[iface].vid[2*i0+1];
  zipper[6*n+3] = i1 + np + 1;
  zipper[6*n+4] = drep->TReps[ibrep].Faces[iface].vid[2*i1  ];
  zipper[6*n+5] = drep->TReps[ibrep].Faces[iface].vid[2*i1+1];
}


/*
 * geometry is always a continuous discretization:
 * remove duplicates at Edge and Nodes where different Faces touch
 */

int
gemPatchTessel(const
               gemDRep  *drep,          /* (in)  the DRep pointer - read-only */
               gemQuilt *quilt)         /* (out) ptr to Quilt to be mod'ed */
{
  int i, j, k, m, n, iface, iedge, npts, ntris, nbrep, ne, nt, np, i0, i1;
  int ibrep, *table, *ibs, *ifs, *ies, *zipper;

  npts  = quilt->nPoints;
  ntris = quilt->nElems;
  table = (int *) malloc(npts*sizeof(int));
  if (table == NULL) return GEM_ALLOC;
  for (i = 0; i < npts; i++) table[i] = i+1;

  for (nbrep = j = 0; j < quilt->nbface; j++)
    if (quilt->bfaces[j].BRep > nbrep) nbrep = quilt->bfaces[j].BRep;
  ibs = (int *) malloc(nbrep*sizeof(int));
  if (ibs == NULL) {
    free(table);
    return GEM_ALLOC;
  }
  for (i = 0; i < nbrep; i++) ibs[i] = 0;
  for (j = 0; j < quilt->nbface; j++) ibs[quilt->bfaces[j].BRep-1]++;
  for (k = i = 0; i < nbrep; i++)
    if (ibs[i] > 0) k++;
  if (k > 1) {
    printf(" tri Info: Multiple BReps -- paramFlg = -1!\n");
    quilt->paramFlg = -1;               /* can't be used -- multiple outers */
  }
  
  /* Faces that touch must be from the same BReps */
  for (i = 0; i < nbrep;  i++) {
    if (ibs[i] <= 1) continue;
    
    /* do this from the perspective of the Edge (2 Faces at max) */
    ies = (int *) malloc(2*drep->TReps[i].nEdges*sizeof(int));
    if (ies == NULL) {
      free(ibs);
      free(table);
      return GEM_ALLOC;
    }
    for (k = 0; k < 2*drep->TReps[i].nEdges; k++) ies[k] = 0;
    
    /* test for Face connectivity */
    ifs = NULL;
    if (quilt->paramFlg != -1) {
      ifs = (int *) malloc(drep->TReps[i].nFaces*sizeof(int));
      if (ifs == NULL) {
        free(ies);
        free(ibs);
        free(table);
        return GEM_ALLOC;
      }
      for (k = 0; k < drep->TReps[i].nFaces; k++) ifs[k] = 0;
      for (j = 0; j < quilt->nbface; j++) {
        iface = quilt->bfaces[j].index - 1;
        ifs[iface] = j+1;
      }
    }
    
    /* mark the Faces in the Edge */
    for (j = 0; j < quilt->nbface; j++) {
      ibrep  = quilt->bfaces[j].BRep  - 1;
      iface  = quilt->bfaces[j].index - 1;
      if (ibrep != i) continue;
      for (k = 0; k < drep->TReps[ibrep].Faces[iface].ntris; k++) {
        if (drep->TReps[ibrep].Faces[iface].tric[3*k  ] < 0) {
          iedge = -drep->TReps[ibrep].Faces[iface].tric[3*k  ] - 1;
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
        if (drep->TReps[ibrep].Faces[iface].tric[3*k+1] < 0) {
          iedge = -drep->TReps[ibrep].Faces[iface].tric[3*k+1] - 1;
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
        if (drep->TReps[ibrep].Faces[iface].tric[3*k+2] < 0) {
          iedge = -drep->TReps[ibrep].Faces[iface].tric[3*k+2] - 1;
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
    
    /* mark Face connectivity -- if needed */
    if (ifs != NULL)
      for (j = 0; j < drep->TReps[i].nEdges; j++) {
        if (ies[2*j+1] == 0) continue;
        i0 = ies[2*j  ];
        i1 = ies[2*j+1];
        if (i0 == i1) continue;
        if (i0 >  i1) {
          k  = i0;
          i0 = i1;
          i1 = k;
        }
        for (k = 0; k < drep->TReps[i].nFaces; k++)
          if (ifs[k] == i1) ifs[k] = i0;
      }
    
    /* zipper up matching Faces at the Edges */
    for (j = 0; j < drep->TReps[i].nEdges; j++) {
      if (ies[2*j+1] == 0) continue;
      ne     = drep->TReps[i].Edges[j].npts;
      zipper = (int *) malloc(6*(ne-1)*sizeof(int));
      if (zipper == NULL) {
        if (ifs != NULL) free(ifs);
        free(ies);
        free(ibs);
        free(table);
        return GEM_ALLOC;
      }
      for (nt = np = k = 0; k < ies[2*j]; k++) {
        ibrep = quilt->bfaces[k].BRep  - 1;
        iface = quilt->bfaces[k].index - 1;
        
        /* store data from the source Face -- segment at a time */
        if (k == ies[2*j]-1)
          for (n = m = 0; m < drep->TReps[ibrep].Faces[iface].ntris; m++) {
            if (drep->TReps[ibrep].Faces[iface].tric[3*m  ] == -(j+1)) {
              i0 = drep->TReps[ibrep].Faces[iface].tris[3*m+1]-1;
              i1 = drep->TReps[ibrep].Faces[iface].tris[3*m+2]-1;
              zipps(drep, ibrep, iface, i0, i1, np, n, zipper);
              n++;
            }
            if (drep->TReps[ibrep].Faces[iface].tric[3*m+1] == -(j+1)) {
              i0 = drep->TReps[ibrep].Faces[iface].tris[3*m  ]-1;
              i1 = drep->TReps[ibrep].Faces[iface].tris[3*m+2]-1;
              zipps(drep, ibrep, iface, i0, i1, np, n, zipper);
              n++;
            }
            if (drep->TReps[ibrep].Faces[iface].tric[3*m+2] == -(j+1)) {
              i0 = drep->TReps[ibrep].Faces[iface].tris[3*m  ]-1;
              i1 = drep->TReps[ibrep].Faces[iface].tris[3*m+1]-1;
              zipps(drep, ibrep, iface, i0, i1, np, n, zipper);
              n++;
            }
          }
        np += drep->TReps[ibrep].Faces[iface].npts;
        nt += drep->TReps[ibrep].Faces[iface].ntris;
      }

      /* patch up segments from the destination Face */
      for (nt = np = k = 0; k < ies[2*j+1]; k++) {
        ibrep = quilt->bfaces[k].BRep  - 1;
        iface = quilt->bfaces[k].index - 1;

        if (k == ies[2*j+1]-1)
          for (n = m = 0; m < drep->TReps[ibrep].Faces[iface].ntris; m++) {
            if (drep->TReps[ibrep].Faces[iface].tric[3*m  ] == -(j+1)) {
              i0 = drep->TReps[ibrep].Faces[iface].tris[3*m+1]-1;
              i1 = drep->TReps[ibrep].Faces[iface].tris[3*m+2]-1;
              zipit(drep, ibrep, iface, i0, i1, np, ne, zipper, table);
            }
            if (drep->TReps[ibrep].Faces[iface].tric[3*m+1] == -(j+1)) {
              i0 = drep->TReps[ibrep].Faces[iface].tris[3*m  ]-1;
              i1 = drep->TReps[ibrep].Faces[iface].tris[3*m+2]-1;
              zipit(drep, ibrep, iface, i0, i1, np, ne, zipper, table);
            }
            if (drep->TReps[ibrep].Faces[iface].tric[3*m+2] == -(j+1)) {
              i0 = drep->TReps[ibrep].Faces[iface].tris[3*m  ]-1;
              i1 = drep->TReps[ibrep].Faces[iface].tris[3*m+1]-1;
              zipit(drep, ibrep, iface, i0, i1, np, ne, zipper, table);
            }
          }
        np += drep->TReps[ibrep].Faces[iface].npts;
        nt += drep->TReps[ibrep].Faces[iface].ntris;
      }
      free(zipper);
    }
  
    /* look at complete Face connectivity */
    if (ifs != NULL) {
      for (k = 0; k < drep->TReps[i].nFaces; k++) {
        if (ifs[k] == 0) continue;
        if (ifs[k] != 1) {
          printf(" tri Info: Multiple Outer Loops -- paramFlg = -1!\n");
          quilt->paramFlg = -1;
/*        printf("               ");
          for (k = 0; k < drep->TReps[i].nFaces; k++) printf("%d ", ifs[k]);
          printf("\n");  */
          break;
        }
      }
      free(ifs);
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
          return GEM_ALLOC;
        }
        ies[quilt->points[j].nFaces]    = i+1;
        quilt->points[j].findices.multi = ies;
      }
      quilt->points[j].nFaces++;
    }
  
  /* remove the indices that we don't use anymore */
  for (i = 0; i < ntris; i++) {
    quilt->elems[i].gIndices[0] = abs(table[quilt->elems[i].gIndices[0]-1]);
    quilt->elems[i].gIndices[1] = abs(table[quilt->elems[i].gIndices[1]-1]);
    quilt->elems[i].gIndices[2] = abs(table[quilt->elems[i].gIndices[2]-1]);
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
    quilt->elems[i].gIndices[0] = table[quilt->elems[i].gIndices[0]-1];
    quilt->elems[i].gIndices[1] = table[quilt->elems[i].gIndices[1]-1];
    quilt->elems[i].gIndices[2] = table[quilt->elems[i].gIndices[2]-1];
  }
  free(table);

  return GEM_SUCCESS;
}