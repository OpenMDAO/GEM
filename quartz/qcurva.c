/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Curvature Compute Function -- CAPRI
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "capri.h"
#include "gem.h"
#include "memory.h"


  /* use undocumented CAPRI function */
  extern int gi_orientFace(int vol, int face, int *norm);


static void
gem_kernelCurvCalc(double *results, int sense, double *curva)
{
  int    i;
  double der1[2][3], der2[3][3], norm[3], curv1, curv2, dir1[3], dir2[3];
  double a, b, c, d11, d12, d21, d22, g11, g12, g21, g22, ud, vd, len;
  
  for (i = 0; i < 8; i++) curva[i] = 0.0;
  
  for (i = 0; i < 3; i++) {
    der1[0][i] = results[ 3+i];
    der1[1][i] = results[ 6+i];
    der2[0][i] = results[ 9+i];
    der2[1][i] = results[15+i];
    der2[2][i] = results[12+i];
  }
  
  norm[0]  = der1[0][1]*der1[1][2] - der1[0][2]*der1[1][1];
  norm[1]  = der1[0][2]*der1[1][0] - der1[0][0]*der1[1][2];
  norm[2]  = der1[0][0]*der1[1][1] - der1[0][1]*der1[1][0];
  len = sqrt(norm[0]*norm[0] + norm[1]*norm[1] + norm[2]*norm[2]);
  if (len == 0.0) return;
  norm[0] /= len;
  norm[1] /= len;
  norm[2] /= len;
  if (sense < 0) {
    norm[0] = -norm[0];
    norm[1] = -norm[1];
    norm[2] = -norm[2];
  }

  g11 = g12 = g21 = g22 = 0.0;
  d11 = d12 = d21 = d22 = 0.0;

  for (i = 0; i < 3; i++) {
    g11 += der1[0][i]*der1[0][i];
    g12 += der1[0][i]*der1[1][i];
    g21 += der1[1][i]*der1[0][i];
    g22 += der1[1][i]*der1[1][i];

    d11 += norm[i]*der2[0][i];
    d12 += norm[i]*der2[2][i];
    d21 += norm[i]*der2[2][i];
    d22 += norm[i]*der2[1][i];
  }
  a =   g11*g22 - g21*g12;
  b = -(g11*d22 + d11*g22 - 2*g12*d21);
  c =   d11*d22 - d21*d12;

  curv1 = (-b + sqrt(b*b - 4.0*a*c))/(2.0*a);
  curv2 = (-b - sqrt(b*b - 4.0*a*c))/(2.0*a);

  if (curv1 != curv2) {

    /* find principal direction 1 */
    ud =  (d12 - (curv1)*g12)*100.0;
    vd = -(d11 - (curv1)*g11)*100.0;
    if ((ud == 0.0) && (vd == 0.0)) {
      ud =  (d22 - (curv1)*g22)*100.0;
      vd = -(d21 - (curv1)*g21)*100.0;
    }
    for (i = 0; i < 3; i++) dir1[i] = der1[0][i]*ud + der1[1][i]*vd;
    len = dir1[0]*dir1[0] + dir1[1]*dir1[1] + dir1[2]*dir1[2];
    if (len != 0.0) {
      len = sqrt(1.0/len);
      for (i = 0; i < 3; i++) dir1[i] *= len;
    }
    /* find principal direction 2 */
    ud =  (d12 - (curv2)*g12)*100.0;
    vd = -(d11 - (curv2)*g11)*100.0;
    if ((ud == 0.0) && (vd == 0.0)) {
      ud =  (d22 - (curv2)*g22)*100.0;
      vd = -(d21 - (curv2)*g21)*100.0;
    }
    for (i = 0; i < 3; i++) dir2[i] = der1[0][i]*ud + der1[1][i]*vd;
    len = dir2[0]*dir2[0] + dir2[1]*dir2[1] + dir2[2]*dir2[2];
    if (len != 0.0) {
      len = sqrt(1.0/len);
      for (i = 0; i < 3; i++) dir2[i] *= len;
    }
    
  } else {

    /*  Align principal directions with isocurves */
    len = 0.0;
    for (i = 0; i < 3; i++) {
      dir1[i]  = der1[0][i];
      len     += der1[0][i]*der1[0][i];
    }
    len = sqrt(len);
    if (len != 0.0) {
      len = 1.0/len;
      for (i = 0; i < 3; i++) dir1[i] *= len;
    }
    len = 0.0;
    for (i = 0; i < 3; i++) {
      dir2[i]  = der1[1][i];
      len     += der1[1][i]*der1[1][i];
    }
    len = sqrt(len);
    if (len != 0.0) {
      len = 1.0/len;
      for (i = 0; i < 3; i++) dir2[i] *= len;
    }

  }
  
  curva[0] = curv1;
  curva[1] = dir1[0];
  curva[2] = dir1[1];
  curva[3] = dir1[2];
  curva[4] = curv2;
  curva[5] = dir2[0];
  curva[6] = dir2[1];
  curva[7] = dir2[2];
}


int
gem_kernelCurvature(gemDRep *drep, int bound, int vs, double *curv)
{
  int      b, i, j, k, m, np, vol, face, stat, sense, *senses, *errs;
  double   dum[3], *results;
  gemPair  bface;
  gemModel *mdl;
  gemBRep  *brep;

  b   = bound - 1;
  mdl = drep->model;
  if (drep->bound[b].VSet[vs-1].quilt == NULL) return GEM_NOTPARAMBND;

  np      = drep->bound[b].VSet[vs-1].quilt->nPoints;
  results = (double *) gem_allocate(15*np*sizeof(double));
  if (results == NULL) return GEM_ALLOC;
  senses  = (int *)    gem_allocate(   np*sizeof(int));
  if (senses  == NULL) {
    gem_free(results);
    return GEM_ALLOC;
  }

  stat = gi_qBegin();
  if (stat != CAPRI_SUCCESS) {
    gem_free(senses);
    gem_free(results);
    return stat;
  }

  for (j = 0; j < np; j++) {
    if (drep->bound[b].VSet[vs-1].quilt->points[j].nFaces > 2) {
      k   = drep->bound[b].VSet[vs-1].quilt->points[j].findices.multi[0]-1;
    } else {
      k   = drep->bound[b].VSet[vs-1].quilt->points[j].findices.faces[0]-1;
    }
    m     = drep->bound[b].VSet[vs-1].quilt->faceUVs[k].owner-1;
    bface = drep->bound[b].VSet[vs-1].quilt->bfaces[m];
    brep  = mdl->BReps[bface.BRep-1];
    vol   = brep->body->faces[bface.index-1].handle.index;
    face  = brep->body->faces[bface.index-1].handle.ident.tag;
    stat  = gi_orientFace(vol, face, &sense);
    if (stat != CAPRI_SUCCESS) {
      gem_free(senses);
      gem_free(results);
      return stat;
    }
    senses[np] = sense;
    stat = gi_qPointOnFace(vol, face, 
                           drep->bound[b].VSet[vs-1].quilt->faceUVs[k].uv,
                           dum, 2, &results[15*np], &results[15*np+3], 
                           &results[15*np+6], &results[15*np+9], 
                           &results[15*np+12]);
    if (stat < CAPRI_SUCCESS) 
      printf(" Curvature Error: %d/%d gi_qPointOnFace = %d!\n", i+1, j+1, stat);
  }
  
  stat = gi_qEnd(&errs);
  if (stat < CAPRI_SUCCESS) {
    gem_free(senses);
    gem_free(results);
    return stat;
  }
  if (stat != np) {
    gi_free(errs);
    gem_free(senses);
    gem_free(results);
    return CAPRI_COMMERR;
  }
  
  for (i = 0; i < np; i++) {
    for (j = 0; j < 8; j++) curv[8*i+j] = 0.0;
    if (errs[i] != CAPRI_SUCCESS) {
      printf(" Curvature Error on %d: %d\n", i+1, errs[i]);
    } else {
      gem_kernelCurvCalc(&results[15*i], senses[i], &curv[8*i]);
    }
  }
  gi_free(errs);
  
  gem_free(senses);
  gem_free(results);
  return GEM_SUCCESS;
}
