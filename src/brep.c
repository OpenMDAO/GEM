/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             BRep Functions
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "gem.h"
#include "attribute.h"
#include "kernel.h"


static int
gem_invert4x4(double *b)
{
  int    i, j, k, ind;
  double a[16], val, val2;

#define MAT(m,r,c) ((m)[(c)*4+(r)])

  for (i = 0; i < 16; i++) a[i] = 0.0;
  a[0] = a[5] = a[10] = a[15] = 1.0;

  for (i = 0; i != 4; i++) {

    val = MAT(b,i,i);                   /* find pivot */
    ind = i;
    for (j = i + 1; j != 4; j++) {
      if (fabs(MAT(b,j,i)) > fabs(val)) {
        ind = j;
        val = MAT(b,j,i);
      }
    }

    if (ind != i) {                     /* swap columns */
      for (j = 0; j != 4; j++) {
        val2 = MAT(a,i,j);
        MAT(a,i,j) = MAT(a,ind,j);
        MAT(a,ind,j) = val2;
        val2 = MAT(b,i,j);
        MAT(b,i,j) = MAT(b,ind,j);
        MAT(b,ind,j) = val2;
      }
    }

    if (val == 0.0) {
       for (i = 0; i < 16; i++) b[i] = 0.0;
       b[0] = b[5] = b[10] = b[15] = 1.0;
       return 0;
    }

    for (j = 0; j != 4; j++) {
      MAT(b,i,j) /= val;
      MAT(a,i,j) /= val;
    }

    for (j = 0; j != 4; j++) {          /* eliminate column */
      if (j == i) continue;
      val = MAT(b,j,i);
      for (k = 0; k != 4; k++) {
        MAT(b,j,k) -= MAT(b,i,k) * val;
        MAT(a,j,k) -= MAT(a,i,k) * val;
      }
    }
  }
#undef MAT
  for (i = 0; i < 16; i++) b[i] = a[i];
  return 1;
}


int
gem_invertXform(double *xform, double *invXform)
{
  int    i, stat;
  double a[16];

  for (i = 0; i < 12; i++) a[i] = xform[i];
  a[12] = a[13] = a[14] = 0.0;
  a[15] = 1.0;

  stat = gem_invert4x4(a);
  if (stat == 0) return GEM_BADINVERSE;

  for (i = 0; i < 12; i++) invXform[i] = a[i];

  return GEM_SUCCESS;
}


int
gem_getBRepInfo(gemBRep *brep, double *box, int *type, int *nnode, int *nedge,
                int *nloop, int *nface, int *nshel, int *nattr)
{
  gemBody *body;

  if (brep == NULL) return GEM_NULLOBJ;
  if (brep->magic != GEM_MBREP) return GEM_BADBREP;
  
  body = brep->body;
  
  box[0] = body->box[0];
  box[1] = body->box[1];
  box[2] = body->box[2];
  box[3] = body->box[3];
  box[4] = body->box[4];
  box[5] = body->box[5];
  *type  = body->type;
  *nnode = body->nnode;
  *nedge = body->nedge;
  *nloop = body->nloop;
  *nface = body->nface;
  *nshel = body->nshell;
  *nattr = 0;
  if (body->attr != NULL) *nattr = body->attr->nattrs;
  
  return GEM_SUCCESS;
}


int
gem_getShell(gemBRep *brep, int shell, int *type, int *nface, int **faces, 
             int *nattr)
{
  gemBody *body;

  if (brep == NULL) return GEM_NULLOBJ;
  if (brep->magic != GEM_MBREP) return GEM_BADBREP;
  
  body = brep->body;
  if (body->type == GEM_WIRE) return GEM_WIREBODY;
  if ((shell < 1) || (shell > body->nshell)) return GEM_BADINDEX;
  
  *type  = body->shells[shell-1].type;
  *nface = body->shells[shell-1].nfaces;
  *faces = body->shells[shell-1].faces;
  *nattr = 0;
  if (body->shells[shell-1].attr != NULL) 
    *nattr = body->shells[shell-1].attr->nattrs;
  
  return GEM_SUCCESS;
}


int
gem_getFace(gemBRep *brep, int face, char **ID, double *uvbox, int *norm,
            int *nloops, int **loops, int *nattr)
{
  gemBody *body;

  if (brep == NULL) return GEM_NULLOBJ;
  if (brep->magic != GEM_MBREP) return GEM_BADBREP;
  
  body = brep->body;
  if (body->type == GEM_WIRE) return GEM_WIREBODY;
  if ((face < 1) || (face > body->nface)) return GEM_BADINDEX;

  *ID      = body->faces[face-1].ID;
  uvbox[0] = body->faces[face-1].uvbox[0];
  uvbox[1] = body->faces[face-1].uvbox[1];
  uvbox[2] = body->faces[face-1].uvbox[2];
  uvbox[3] = body->faces[face-1].uvbox[3];
  *norm    = body->faces[face-1].norm;
  *nloops  = body->faces[face-1].nloops;
  *loops   = body->faces[face-1].loops;
  *nattr   = 0;
  if (body->faces[face-1].attr != NULL) 
    *nattr = body->faces[face-1].attr->nattrs;

  return GEM_SUCCESS;
}


int
gem_getWire(gemBRep *brep, int *nloops, int **loops)
{
  gemBody *body;

  if (brep == NULL) return GEM_NULLOBJ;
  if (brep->magic != GEM_MBREP) return GEM_BADBREP;
  
  body = brep->body;
  if (body->type != GEM_WIRE) return GEM_WIREBODY;

  *nloops = body->faces[0].nloops;
  *loops  = body->faces[0].loops;

  return GEM_SUCCESS;
}


int
gem_getLoop(gemBRep *brep, int loop, int *face, int *type, int *nedge,
            int **edges, int *nattr)
{
  gemBody *body;

  if (brep == NULL) return GEM_NULLOBJ;
  if (brep->magic != GEM_MBREP) return GEM_BADBREP;
  
  body = brep->body;
  if ((loop < 1) || (loop > body->nloop)) return GEM_BADINDEX;
  
  *face  = body->loops[loop-1].face;
  *type  = body->loops[loop-1].type;
  *nedge = body->loops[loop-1].nedges;
  *edges = body->loops[loop-1].edges;
  *nattr = 0;
  if (body->loops[loop-1].attr != NULL) 
    *nattr = body->loops[loop-1].attr->nattrs;
  
  return GEM_SUCCESS;
}


int
gem_getEdge(gemBRep *brep, int edgex, double  *tlimit, int *nodes, 
            int *faces, int *nattr)
{
  int     edge;
  gemBody *body;

  if (brep == NULL) return GEM_NULLOBJ;
  if (brep->magic != GEM_MBREP) return GEM_BADBREP;
  
  body = brep->body;
  edge = edgex;
  if  (edge < 0) edge = -edge;
  if ((edge < 1) || (edge > body->nedge)) return GEM_BADINDEX;
  
  if (edgex < 0) {
    tlimit[0] = body->edges[edge-1].tlimit[1];
    tlimit[1] = body->edges[edge-1].tlimit[0];
    nodes[0]  = body->edges[edge-1].nodes[1];
    nodes[1]  = body->edges[edge-1].nodes[0];
    faces[0]  = body->edges[edge-1].faces[1];
    faces[1]  = body->edges[edge-1].faces[0];
  } else {
    tlimit[0] = body->edges[edge-1].tlimit[0];
    tlimit[1] = body->edges[edge-1].tlimit[1];
    nodes[0]  = body->edges[edge-1].nodes[0];
    nodes[1]  = body->edges[edge-1].nodes[1];
    faces[0]  = body->edges[edge-1].faces[0];
    faces[1]  = body->edges[edge-1].faces[1];
  }
  *nattr = 0;
  if (body->edges[edge-1].attr != NULL)
    *nattr = body->edges[edge-1].attr->nattrs;
  
  return GEM_SUCCESS;
}


int
gem_getNode(gemBRep *brep, int node, double *xyz, int *nattr)
{
  gemBody *body;

  if (brep == NULL) return GEM_NULLOBJ;
  if (brep->magic != GEM_MBREP) return GEM_BADBREP;
  
  body = brep->body;
  if ((node < 1) || (node > body->nnode)) return GEM_BADINDEX;
  
  xyz[0] = body->nodes[node-1].xyz[0];
  xyz[1] = body->nodes[node-1].xyz[1];
  xyz[2] = body->nodes[node-1].xyz[2];
  *nattr = 0;
  if (body->nodes[node-1].attr != NULL)
    *nattr = body->nodes[node-1].attr->nattrs;

  return GEM_SUCCESS;
}


int
gem_getMassProps(gemBRep *brep, int etype, int eindex, double *props)
{
  int     stat;
  double  A[3][3], x[3], determinant, scale;
  gemBody *body;
  gemID   handle;

  if (brep == NULL) return GEM_NULLOBJ;
  if (brep->magic != GEM_MBREP) return GEM_BADBREP;
  body = brep->body; 
  if (body->type == GEM_WIRE) return GEM_WIREBODY;
  if ((etype != GEM_BREP) && (etype != GEM_SHELL) && 
      (etype != GEM_FACE)) return GEM_BADTYPE;
  if (etype == GEM_SHELL) {
    if ((eindex < 1) || (eindex > body->nshell)) return GEM_BADINDEX;
    handle = body->shells[eindex-1].handle;
  } else if (etype == GEM_FACE) {
    if ((eindex < 1) || (eindex > body->nface)) return GEM_BADINDEX;
    handle = body->faces[eindex-1].handle;
  } else {
    handle = body->handle;
  }

  stat = gem_kernelMassProps(handle, etype, props);
  if (stat != GEM_SUCCESS) return stat;

  /* transform -- assumes a homogeneous orthonormal system */
  
  A[0][0] = brep->xform[ 0];
  A[0][1] = brep->xform[ 1];
  A[0][2] = brep->xform[ 2];
  A[1][0] = brep->xform[ 4];
  A[1][1] = brep->xform[ 5];
  A[1][2] = brep->xform[ 6];
  A[2][0] = brep->xform[ 8];
  A[2][1] = brep->xform[ 9];
  A[2][2] = brep->xform[10];

  determinant = (A[0][0]*A[1][1]*A[2][2] - A[0][2]*A[1][1]*A[2][0] +
                 A[0][1]*A[1][2]*A[2][0] - A[0][1]*A[1][0]*A[2][2] +
                 A[0][2]*A[1][0]*A[2][1] - A[0][0]*A[1][2]*A[2][1]);

  if (determinant < 0.0) {
    scale = pow(-determinant, 1.0/3.0);
  } else {
    scale = pow( determinant, 1.0/3.0);
  }
  x[0]      = brep->xform[ 0]*props[2] + brep->xform[ 1]*props[3] +
              brep->xform[ 2]*props[4] + brep->xform[ 3];
  x[1]      = brep->xform[ 4]*props[2] + brep->xform[ 5]*props[3] +
              brep->xform[ 6]*props[4] + brep->xform[ 7];
  x[2]      = brep->xform[ 8]*props[2] + brep->xform[ 9]*props[3] +
              brep->xform[10]*props[4] + brep->xform[11];
  props[0] *= scale*scale*scale;
  props[1] *= scale*scale;
  props[2]  = x[0];
  props[3]  = x[1];
  props[4]  = x[2];
  A[0][0]   = brep->xform[ 0]*props[ 5] + brep->xform[ 1]*props[ 6] +
              brep->xform[ 2]*props[ 7];
  A[1][0]   = brep->xform[ 4]*props[ 5] + brep->xform[ 5]*props[ 6] +
              brep->xform[ 6]*props[ 7];
  A[2][0]   = brep->xform[ 8]*props[ 5] + brep->xform[ 9]*props[ 6] +
              brep->xform[10]*props[ 7];
  A[0][1]   = brep->xform[ 0]*props[ 8] + brep->xform[ 1]*props[ 9] +
              brep->xform[ 2]*props[10];
  A[1][1]   = brep->xform[ 4]*props[ 8] + brep->xform[ 5]*props[ 9] +
              brep->xform[ 6]*props[10];
  A[2][1]   = brep->xform[ 8]*props[ 8] + brep->xform[ 9]*props[ 9] +
              brep->xform[10]*props[10];
  A[0][2]   = brep->xform[ 0]*props[11] + brep->xform[ 1]*props[12] +
              brep->xform[ 2]*props[13];
  A[1][2]   = brep->xform[ 4]*props[11] + brep->xform[ 5]*props[12] +
              brep->xform[ 6]*props[13];
  A[2][2]   = brep->xform[ 8]*props[11] + brep->xform[ 9]*props[12] +
              brep->xform[10]*props[13];

  x[0]      = brep->xform[ 0]*A[0][0] + brep->xform[ 1]*A[0][1] +
              brep->xform[ 2]*A[0][2];
  x[1]      = brep->xform[ 4]*A[0][0] + brep->xform[ 5]*A[0][1] +
              brep->xform[ 6]*A[0][2];
  x[2]      = brep->xform[ 8]*A[0][0] + brep->xform[ 9]*A[0][1] +
              brep->xform[10]*A[0][2];
  props[ 5] = x[0]*scale*scale*scale;
  props[ 6] = x[1]*scale*scale*scale;
  props[ 7] = x[2]*scale*scale*scale;
  x[0]      = brep->xform[ 0]*A[1][0] + brep->xform[ 1]*A[1][1] +
              brep->xform[ 2]*A[1][2];
  x[1]      = brep->xform[ 4]*A[1][0] + brep->xform[ 5]*A[1][1] +
              brep->xform[ 6]*A[1][2];
  x[2]      = brep->xform[ 8]*A[1][0] + brep->xform[ 9]*A[1][1] +
              brep->xform[10]*A[1][2];
  props[ 8] = x[0]*scale*scale*scale;
  props[ 9] = x[1]*scale*scale*scale;
  props[10] = x[2]*scale*scale*scale;
  x[0]      = brep->xform[ 0]*A[2][0] + brep->xform[ 1]*A[2][1] +
              brep->xform[ 2]*A[2][2];
  x[1]      = brep->xform[ 4]*A[2][0] + brep->xform[ 5]*A[2][1] +
              brep->xform[ 6]*A[2][2];
  x[2]      = brep->xform[ 8]*A[2][0] + brep->xform[ 9]*A[2][1] +
              brep->xform[10]*A[2][2];
  props[11] = x[0]*scale*scale*scale;
  props[12] = x[1]*scale*scale*scale;
  props[13] = x[2]*scale*scale*scale;
  
  return GEM_SUCCESS;
}


int
gem_isEquivalent(int etype, gemBRep *brep1, int eindex1,
                            gemBRep *brep2, int eindex2)
{
  gemBody *body1,  *body2;
  gemID   handle1, handle2;

  if (brep1 == NULL) return GEM_NULLOBJ;
  if (brep1->magic != GEM_MBREP) return GEM_BADBREP;
  if (brep2 == NULL) return GEM_NULLOBJ;
  if (brep2->magic != GEM_MBREP) return GEM_BADBREP;

  body1 = brep1->body; 
  if (etype == GEM_BREP) {
    handle1 = body1->handle;
  } else if (etype == GEM_SHELL) {
    if (body1->type == GEM_WIRE) return GEM_WIREBODY;
    if ((eindex1 < 1) || (eindex1 > body1->nshell)) return GEM_BADINDEX;
    handle1 = body1->shells[eindex1-1].handle;
  } else if (etype == GEM_FACE) {
    if (body1->type == GEM_WIRE) return GEM_WIREBODY;
    if ((eindex1 < 1) || (eindex1 > body1->nface)) return GEM_BADINDEX;
    handle1 = body1->faces[eindex1-1].handle;
  } else if (etype == GEM_LOOP) {
    if ((eindex1 < 1) || (eindex1 > body1->nloop)) return GEM_BADINDEX;
    handle1 = body1->loops[eindex1-1].handle;
  } else if (etype == GEM_EDGE) {
    if ((eindex1 < 1) || (eindex1 > body1->nedge)) return GEM_BADINDEX;
    handle1 = body1->edges[eindex1-1].handle;
  } else if (etype == GEM_NODE) {
    if ((eindex1 < 1) || (eindex1 > body1->nnode)) return GEM_BADINDEX;
    handle1 = body1->nodes[eindex1-1].handle;
  } else {
    return GEM_BADTYPE;
  }
  
  body2 = brep2->body; 
  if (etype == GEM_BREP) {
    handle2 = body2->handle;
  } else if (etype == GEM_SHELL) {
    if (body2->type == GEM_WIRE) return GEM_WIREBODY;
    if ((eindex2 < 1) || (eindex2 > body2->nshell)) return GEM_BADINDEX;
    handle2 = body2->shells[eindex2-1].handle;
  } else if (etype == GEM_FACE) {
    if (body2->type == GEM_WIRE) return GEM_WIREBODY;
    if ((eindex2 < 1) || (eindex2 > body2->nface)) return GEM_BADINDEX;
    handle2 = body2->faces[eindex2-1].handle;
  } else if (etype == GEM_LOOP) {
    if ((eindex2 < 1) || (eindex2 > body2->nloop)) return GEM_BADINDEX;
    handle2 = body2->loops[eindex2-1].handle;
  } else if (etype == GEM_EDGE) {
    if ((eindex2 < 1) || (eindex2 > body2->nedge)) return GEM_BADINDEX;
    handle2 = body2->edges[eindex2-1].handle;
  } else if (etype == GEM_NODE) {
    if ((eindex2 < 1) || (eindex2 > body2->nnode)) return GEM_BADINDEX;
    handle2 = body2->nodes[eindex2-1].handle;
  } else {
    return GEM_BADTYPE;
  }

  return gem_kernelEquivalent(etype, handle1, handle2);
}


int
gem_getBRepOwner(gemBRep *brep, gemModel **model, int *instance, int *branch)
{
  *model    = NULL;
  *instance = -1;
  *branch   =  0;
  if (brep == NULL) return GEM_NULLOBJ;
  if (brep->magic != GEM_MBREP) return GEM_BADBREP;
  
  *model    = (gemModel *) brep->omodel;
  *instance = brep->inumber;
  *branch   = brep->ibranch;
  
  return GEM_SUCCESS;
}
