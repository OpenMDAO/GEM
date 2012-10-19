/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Tessellation Function -- EGADS
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "egads.h"
#include "gem.h"
#include "memory.h"


  static int  sides[3][2] = {1,2, 2,0, 0,1};


static int
EG_nodesTessEdge(const ego tess, int index, int *nodes)
{
  egTessel *btess;
  egObject *obj;

  if (tess == NULL)                 return EGADS_NULLOBJ;
  if (tess->magicnumber != MAGIC)   return EGADS_NOTOBJ;
  if (tess->oclass != TESSELLATION) return EGADS_NOTTESS;
  
  btess = (egTessel *) tess->blind;
  if (btess == NULL) {
    printf(" EGADS Error: NULL Blind Object (EG_nodesTessEdge)!\n");  
    return EGADS_NOTFOUND;
  }
  obj = btess->src;
  if (obj == NULL) {
    printf(" EGADS Error: NULL Source Object (EG_nodesTessEdge)!\n");
    return EGADS_NULLOBJ;
  }
  if (obj->magicnumber != MAGIC) {
    printf(" EGADS Error: Source Not an Object (EG_nodesTessEdge)!\n");
    return EGADS_NOTOBJ;
  }
  if (obj->oclass != BODY) {
    printf(" EGADS Error: Source Not Body (EG_nodesTessEdge)!\n");
    return EGADS_NOTBODY;
  }
  if (btess->tess1d == NULL) {
    printf(" EGADS Error: No Edge Tessellations (EG_nodesTessEdge)!\n");
    return EGADS_NODATA;  
  }
  if ((index < 1) || (index > btess->nEdge)) {
    printf(" EGADS Error: Index = %d [1-%d] (EG_nodesTessEdge)!\n",
           index, btess->nEdge);
    return EGADS_INDEXERR;
  }

  nodes[0] = btess->tess1d[index-1].nodes[0];
  nodes[1] = btess->tess1d[index-1].nodes[1];
  return EGADS_SUCCESS;
}


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
  int          i, j, k, v0, v1, n0, n1, stat, nfaces, npts, ntri, nside;
  int          len, nds[2], nobjs, oclass, mtype, nedge, *eindex, *senses;
  const int    *ptype, *pindex, *tris, *tric;
  double       params[3], t0, t1, limits[4];
  const double *xyzs, *uvs, *xyze, *te;
  ego          obj, tess, geom, *objs, *edges;
  gemConn      *conn;
  gemTRep      *trep;

  trep = &drep->TReps[brep-1];
  if (trep == NULL) return GEM_NULLVALUE;
  if (trep->Faces != NULL) gem_destroyTRep(trep);
  nfaces = body->nface;
  if (nfaces == 0) return GEM_SUCCESS;

  /* make the tessellation */
  params[2] = angle;
  params[0] = mxside;
  params[1] = sag;
  obj  = (ego) body->handle.ident.ptr;
  stat = EG_makeTessBody(obj, params, &tess);
  if (stat != EGADS_SUCCESS) return stat;

  edges = NULL;
  stat  = EG_getBodyTopos(obj, NULL, EDGE,  &nedge,  &edges);
  if ((stat != EGADS_SUCCESS) || (edges == NULL)) {
    EG_deleteObject(tess);
    return stat;
  }
  eindex = (int *) gem_allocate(nedge*sizeof(int));
  if (eindex == NULL) {
    EG_free(edges);
    EG_deleteObject(tess);
    return stat;
  }
  for (j = i = 0; i < nedge; i++) {
    stat = EG_getTopology(edges[i], &geom, &oclass, &mtype, limits, &nobjs,
                          &objs, &senses);
    if (stat  != EGADS_SUCCESS) continue;
    if (mtype == DEGENERATE)    continue;
    j++;
    eindex[i] = j;
  }
  EG_free(edges);
  
  /* get the GEM storage */  
  trep->Faces = (gemTface *) gem_allocate(nfaces*sizeof(gemTface));
  if (trep->Faces == NULL) {
    gem_free(eindex);
    EG_deleteObject(tess);
    return GEM_ALLOC;
  }
  for (i = 0; i < nfaces; i++) {
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
  trep->nFaces = nfaces;
  
  /* fill it in */
  for (i = 0; i < nfaces; i++) {
    stat = EG_getTessFace(tess, i+1, &npts, &xyzs, &uvs, &ptype, &pindex, 
                          &ntri, &tris, &tric);
    if (stat != EGADS_SUCCESS) {
      gem_free(eindex);
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return stat;
    }
    if ((npts == 0) || (ntri == 0)) continue;
    trep->Faces[i].npts = npts;

    trep->Faces[i].xyz  = (double *) gem_allocate(3*npts*sizeof(double));
    if (trep->Faces[i].xyz == NULL) {
      gem_free(eindex);
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return GEM_ALLOC;
    }
    for (j = 0; j < 3*npts; j++) trep->Faces[i].xyz[j] = xyzs[j];
    trep->Faces[i].uv   = (double *) gem_allocate(2*npts*sizeof(double));
    if (trep->Faces[i].uv == NULL) {
      gem_free(eindex);
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return GEM_ALLOC;
    }
    for (j = 0; j < 2*npts; j++) trep->Faces[i].uv[j] = uvs[j];
    trep->Faces[i].conn = conn = (gemConn *) gem_allocate(sizeof(gemConn));
    if (trep->Faces[i].conn == NULL) {
      gem_free(eindex);
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
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
      gem_free(eindex);
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return GEM_ALLOC;
    }
    for (j = 0; j < 3*ntri; j++) conn->Tris[j] = tris[j];
    conn->tNei = (int *) gem_allocate(3*ntri*sizeof(int));
    if (conn->tNei == NULL) {
      gem_free(eindex);
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return GEM_ALLOC;
    }
    conn->sides = (gemSide *) gem_allocate(nside*sizeof(gemSide));
    if (conn->sides == NULL) {
      gem_free(eindex);
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return GEM_ALLOC;
    }

    for (nside = j = 0; j < ntri; j++) 
      for (k = 0; k < 3; k++) {
        conn->tNei[3*j+k] = tric[3*j+k];
        if (tric[3*j+k] > 0) continue;

        conn->sides[nside].edge = eindex[-tric[3*j+k]-1];
        stat = EG_getTessEdge(tess, -tric[3*j+k], &len, &xyze, &te);
        if (stat != EGADS_SUCCESS) {
          gem_free(eindex);
          gem_destroyTRep(trep);
          EG_deleteObject(tess);
          return stat;
        }
        stat = EG_nodesTessEdge(tess, -tric[3*j+k], nds);
        if (stat != EGADS_SUCCESS) {
          gem_free(eindex);
          gem_destroyTRep(trep);
          EG_deleteObject(tess);
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

  gem_free(eindex);
  EG_deleteObject(tess);
  return GEM_SUCCESS;
}
