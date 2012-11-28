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


static void
gem_destroyTRep(gemTRep *trep)
{
  int i;

  if (trep->Faces != NULL) {
    for (i = 0; i < trep->nFaces; i++) {
      gem_free(trep->Faces[i].xyzs);
      gem_free(trep->Faces[i].tris);
      gem_free(trep->conns[i].tric);
      gem_free(trep->conns[i].uvs);
      gem_free(trep->conns[i].vid);
    }
    gem_free(trep->Faces);
    gem_free(trep->conns);
    trep->nFaces = 0;
    trep->Faces  = NULL;
    trep->conns  = NULL;
  }

  if (trep->Edges != NULL) {
    for (i = 0; i < trep->nEdges; i++) {
      gem_free(trep->Edges[i].xyzs);
      gem_free(trep->Edges[i].ts);
    }
    gem_free(trep->Edges);
    trep->nEdges = 0;
    trep->Edges  = NULL;
  }

}


int
gem_kernelTessel(gemBody *body, double angle, double mxside, double sag,
                 gemDRep *drep, int brep)
{
  int          i, j, stat, nfaces, nedges, npts, ntri;
  const int    *ptype, *pindex, *tris, *tric;
  double       params[3];
  const double *xyzs, *uvs, *xyze, *te;
  ego          obj, tess;
  gemTRep      *trep;

  trep = &drep->TReps[brep-1];
  if (trep == NULL) return GEM_NULLVALUE;
  if (trep->Faces != NULL) gem_destroyTRep(trep);
  nfaces = body->nface;
  nedges = body->nedge;
  if (nfaces == 0) return GEM_SUCCESS;

  /* make the tessellation */
  params[2] = angle;
  params[0] = mxside;
  params[1] = sag;
  obj  = (ego) body->handle.ident.ptr;
  stat = EG_makeTessBody(obj, params, &tess);
  if (stat != EGADS_SUCCESS) return stat;
  
  /* get the GEM storage */  
  trep->Faces = (gemTri *) gem_allocate(nfaces*sizeof(gemTri));
  if (trep->Faces == NULL) {
    EG_deleteObject(tess);
    return GEM_ALLOC;
  }
  trep->conns = (gemTConn *) gem_allocate(nfaces*sizeof(gemTConn));
  if (trep->conns == NULL) {
    gem_free(trep->Faces);
    trep->Faces = NULL;
    EG_deleteObject(tess);
    return GEM_ALLOC;
  }
  for (i = 0; i < nfaces; i++) {
    trep->Faces[i].ntris = 0;
    trep->Faces[i].npts  = 0;
    trep->Faces[i].tris  = NULL;
    trep->Faces[i].xyzs  = NULL;
    trep->Faces[i].ptrm  = NULL;
    trep->conns[i].tric  = NULL;
    trep->conns[i].uvs   = NULL;
    trep->conns[i].vid   = NULL;
  }

  trep->Edges  = (gemDEdge *) gem_allocate(nedges*sizeof(gemDEdge));
  if (trep->Edges == NULL) {
    gem_free(trep->conns);
    gem_free(trep->Faces);
    trep->conns = NULL;
    trep->Faces = NULL;
    EG_deleteObject(tess);
    return GEM_ALLOC;
  }
  for (i = 0; i < nedges; i++) {
    trep->Edges[i].npts = 0;
    trep->Edges[i].xyzs = NULL;
    trep->Edges[i].ts   = NULL;
  }  
  trep->nFaces = nfaces;
  trep->nEdges = nedges;
  
  /* fill in Faces */
  for (i = 0; i < nfaces; i++) {
    stat = EG_getTessFace(tess, i+1, &npts, &xyzs, &uvs, &ptype, &pindex, 
                          &ntri, &tris, &tric);
    if (stat != EGADS_SUCCESS) {
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return stat;
    }
    if ((npts == 0) || (ntri == 0)) continue;
    trep->Faces[i].npts  = npts;
    trep->Faces[i].ntris = ntri;

    trep->Faces[i].xyzs = (double *) gem_allocate(3*npts*sizeof(double));
    if (trep->Faces[i].xyzs == NULL) {
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return GEM_ALLOC;
    }
    for (j = 0; j < 3*npts; j++) trep->Faces[i].xyzs[j] = xyzs[j];
    trep->Faces[i].tris = (int *) gem_allocate(3*ntri*sizeof(int));
    if (trep->Faces[i].tris == NULL) {
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return GEM_ALLOC;
    }
    for (j = 0; j < 3*ntri; j++) trep->Faces[i].tris[j] = tris[j];

    trep->conns[i].uvs = (double *) gem_allocate(2*npts*sizeof(double));
    if (trep->conns[i].uvs == NULL) {
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return GEM_ALLOC;
    }
    for (j = 0; j < 2*npts; j++) trep->conns[i].uvs[j] = uvs[j];
    trep->conns[i].tric = (int *) gem_allocate(3*ntri*sizeof(int));
    if (trep->conns[i].tric == NULL) {
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return GEM_ALLOC;
    }
    for (j = 0; j < 3*ntri; j++) trep->conns[i].tric[j] = tric[j];
    trep->conns[i].vid = (int *) gem_allocate(2*npts*sizeof(int));
    if (trep->conns[i].vid == NULL) {
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return GEM_ALLOC;
    }
    for (j = 0; j < npts; j++) {
      trep->conns[i].vid[2*j  ] = ptype[j];
      trep->conns[i].vid[2*j+1] = pindex[j];
    }
  }  

  /* fill in Edges */

  for (i = 0; i < nedges; i++) {
    stat = EG_getTessEdge(tess, i+1, &npts, &xyze, &te);
    if (stat != EGADS_SUCCESS) {
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return stat;
    }
    if (npts == 0) continue;
    trep->Edges[i].npts = npts;
    trep->Edges[i].xyzs = (double *) gem_allocate(3*npts*sizeof(double));
    if (trep->Edges[i].xyzs == NULL) {
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return GEM_ALLOC;
    }
    for (j = 0; j < 3*npts; j++) trep->Edges[i].xyzs[j] = xyze[j];
    trep->Edges[i].ts = (double *) gem_allocate(npts*sizeof(double));
    if (trep->Edges[i].ts == NULL) {
      gem_destroyTRep(trep);
      EG_deleteObject(tess);
      return GEM_ALLOC;
    }
    for (j = 0; j < npts; j++) trep->Edges[i].ts[j] = te[j];
  
  }

  EG_deleteObject(tess);
  return GEM_SUCCESS;
}
