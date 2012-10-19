/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Tessellate Function -- CAPRI
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "capri.h"
#include "gem.h"
#include "memory.h"


  static int  sides[3][2] = {1,2, 2,0, 0,1};


static void
gem_destroyTRep(gemTRep *trep)
{
  int i;
  
  for (i = 0; i < trep->nFaces; i++) {
    gem_free(trep->Faces[i].xyz);
    gem_free(trep->Faces[i].uv);
    gem_free(trep->Faces[i].guv);
    gem_freeConn(trep->Faces[i].conn);
  }
  gem_free(trep->Faces);
  trep->nFaces = 0;
  trep->Faces  = NULL;  
}


int
gem_kernelTessel(gemBody *body, double angle, double mxside, double sag,
                 gemDRep *drep, int brep)
{
  int     i, j, k, vol, stat, nface, v0, v1, n0, n1, nside, npts, ntri;
  int     *pindex, *ptype, *tris, *tric, len, nds[2];
  double  *xyzs, *uvs, *xyze, *te, trange[2], t0, t1;
  gemConn *conn;
  gemTRep *trep;

  trep = &drep->TReps[brep-1];
  if (trep == NULL) return GEM_NULLVALUE;
  if (trep->Faces != NULL) gem_destroyTRep(trep);
  nface = body->nface;
  if (nface == 0) return GEM_SUCCESS;
  
  vol  = body->handle.index;
  stat = gi_uTesselate(vol, 0, 0, angle, mxside, sag);
  if (stat != CAPRI_SUCCESS) return stat;

  /* get the GEM storage */  
  trep->Faces = (gemTface *) gem_allocate(nface*sizeof(gemTface));
  if (trep->Faces == NULL) return GEM_ALLOC;
  for (i = 0; i < nface; i++) {
    trep->Faces[i].index.BRep  = brep+1;
    trep->Faces[i].index.index = i+1;
    trep->Faces[i].ID          = 0;
    trep->Faces[i].npts        = 0;
    trep->Faces[i].xyz         = NULL;
    trep->Faces[i].uv          = NULL;
    trep->Faces[i].guv         = NULL;
    trep->Faces[i].conn        = NULL;
    if (body->faces[i].ID != NULL)
      for (j = 0; j < drep->nIDs; j++)
        if (drep->IDs[j] != NULL)
          if (strcmp(body->faces[i].ID, drep->IDs[j]) == 0) {
            trep->Faces[i].ID = j+1;
            break;
          }
  }
  trep->nFaces = nface;
  
  /* fill it in */
  for (i = 0; i < nface; i++) {
    stat = gi_dTesselFace(vol, i+1, &ntri, &tris, &tric, 
                          &npts, &xyzs, &ptype, &pindex, &uvs);
    if (stat != CAPRI_SUCCESS) {
      gem_destroyTRep(trep);
      return stat;
    }
    if ((npts == 0) || (ntri == 0)) continue;
    trep->Faces[i].npts = npts;

    trep->Faces[i].xyz  = (double *) gem_allocate(3*npts*sizeof(double));
    if (trep->Faces[i].xyz == NULL) return GEM_ALLOC;
    for (j = 0; j < 3*npts; j++) trep->Faces[i].xyz[j] = xyzs[j];
    trep->Faces[i].uv   = (double *) gem_allocate(2*npts*sizeof(double));
    if (trep->Faces[i].uv == NULL) {
      gem_destroyTRep(trep);
      return GEM_ALLOC;
    }
    for (j = 0; j < 2*npts; j++) trep->Faces[i].uv[j] = uvs[j];
    trep->Faces[i].conn = conn = (gemConn *) gem_allocate(sizeof(gemConn));
    if (trep->Faces[i].conn == NULL) {
      gem_destroyTRep(trep);
      return GEM_ALLOC;
    }
    for (nside = j = 0; j < 3*ntri; j++)
      if (tric[j] < 0) nside++;
    
    conn->magic     = GEM_MCONN;
    conn->nTris     = ntri;
    conn->tNei      = NULL;
    conn->nQuad     = 0;
    conn->Quad      = NULL;
    conn->qNei      = NULL;
    conn->meshSz[0] = conn->meshSz[1] = 0;
    conn->nSides    = nside;
    conn->sides     = NULL;
    conn->Tris      = (int *) gem_allocate(3*ntri*sizeof(int));
    if (conn->Tris == NULL) {
      gem_destroyTRep(trep);
      return GEM_ALLOC;
    }
    for (j = 0; j < 3*ntri; j++) conn->Tris[j] = tris[j];
    conn->tNei = (int *) gem_allocate(3*ntri*sizeof(int));
    if (conn->tNei == NULL) {
      gem_destroyTRep(trep);
      return GEM_ALLOC;
    }
    conn->sides = (gemSide *) gem_allocate(nside*sizeof(gemSide));
    if (conn->sides == NULL) {
      gem_destroyTRep(trep);
      return GEM_ALLOC;
    }

    for (nside = j = 0; j < ntri; j++) 
      for (k = 0; k < 3; k++) {
        conn->tNei[3*j+k] = tric[3*j+k];
        if (tric[3*j+k] > 0) continue;

        conn->sides[nside].edge = -tric[3*j+k];
        stat = gi_dTesselEdge(vol, conn->sides[nside].edge, &len, &xyze, &te);
        if (stat != CAPRI_SUCCESS) {
          gem_destroyTRep(trep);
          return stat;
        }
        stat = gi_dGetEdge(vol, conn->sides[nside].edge, trange, nds);
        if (stat != CAPRI_SUCCESS) {
          gem_destroyTRep(trep);
          return stat;
        }
        v0 = tris[3*j+sides[k][0]] - 1;
        v1 = tris[3*j+sides[k][1]] - 1;
        n0 = n1 = 0;
        if ((ptype[v0] == 0) && (ptype[v1] == 0)) {
          n0 = pindex[v0];
          n1 = pindex[v1];
          if (pindex[v0] == nds[0]) {
            t0 = te[0];
            t1 = te[len-1];
          } else {
            t0 = te[len-1];
            t1 = te[0];
          }
        } else if (ptype[v0] == 0) {
          n0 = pindex[v0];
          if (nds[0] == nds[1]) {
            if (ptype[v1] == 2) {
              t0 = te[0];
            } else {
              t0 = te[len-1];
            }
          } else if (n0 == nds[0]) {
            t0 = te[0];
          } else {
            t0 = te[len-1];
          }
          t1 = te[ptype[v1]-1];
        } else if (ptype[v1] == 0) {
          t0 = te[ptype[v0]-1];
          n1 = pindex[v1];
          if (nds[0] == nds[1]) {
            if (ptype[v0] == 2) {
              t1 = te[0];
            } else {
              t1 = te[len-1];
            }          
          } else if (n1 == nds[0]) {
            t1 = te[0];
          } else {
            t1 = te[len-1];
          }
        } else {
          t0 = te[ptype[v0]-1];
          t1 = te[ptype[v1]-1];
        }
        conn->sides[nside].node[0] = n0;
        conn->sides[nside].t[0]    = t0;
        conn->sides[nside].node[1] = n1;
        conn->sides[nside].t[1]    = t1;
        nside++;
        conn->tNei[3*j+k] = -nside;
      }
  }  

  return GEM_SUCCESS;
}
