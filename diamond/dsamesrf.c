/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Query if Same Surface -- EGADS
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "egads.h"
#include "gem.h"


int
gem_kernelSameSurfs(gemModel *model, int nFaces, gemPair *bfaces)
{
  int     i, n, iface, stat, oclass, mtype, msave, iLen, rLen, *ivec;
  double  data[4], *rvec, *rsave = NULL;
  ego     face, surface, ref, *chldrn;
  gemBRep *brep;

  for (i = 0; i < nFaces; i++) {
    brep  = model->BReps[bfaces[i].BRep-1];
    iface = bfaces[i].index;
    face  = (ego) brep->body->faces[iface-1].handle.ident.ptr;
    stat  = EG_getTopology(face, &surface, &oclass, &mtype, data, &n, &chldrn,
                           &ivec);
    if (stat != EGADS_SUCCESS) {
      if (rsave != NULL) EG_free(rsave);
      return stat;
    }
    stat = EG_getGeometry(surface, &oclass, &mtype, &ref, &ivec, &rvec);
    if (stat != EGADS_SUCCESS) {
      EG_free(rsave);
      return stat;
    }
    if (oclass != SURFACE) {
      if (ivec != NULL) EG_free(ivec);
      EG_free(rvec);
      EG_free(rsave);
      return EGADS_GEOMERR;
    }
    if (mtype == TRIMMED) {
      EG_free(rvec);
      surface = ref;
      stat    = EG_getGeometry(surface, &oclass, &mtype, &ref, &ivec, &rvec);
      if (stat != EGADS_SUCCESS) {
        EG_free(rsave);
        return stat;
      }
    }
    if (ref != NULL) {
      if (ivec != NULL) EG_free(ivec);
      EG_free(rvec);
      EG_free(rsave);
      return EGADS_OUTSIDE;
    }
    if (i == 0) {
      msave = mtype;
      rsave = rvec;
      iLen  = 0;
      rLen  = 9;
      if (mtype == SPHERICAL)   rLen = 10;
      if (mtype == CONICAL)     rLen = 14;
      if (mtype == CYLINDRICAL) rLen = 13;
      if (mtype == TOROIDAL)    rLen = 14;
      if (mtype == REVOLUTION)  rLen =  6;
      if (mtype == EXTRUSION)   rLen =  3;
      if (mtype == BEZIER) {
        iLen = 5;
        rLen = 3*ivec[2]*ivec[4];
        if ((ivec[0]&2) != 0) rLen += ivec[2]*ivec[4];
      }
      if (mtype == BSPLINE) {
        iLen =  7;
        rLen = ivec[3] + ivec[6]  + 3*ivec[2]*ivec[5];
        if ((ivec[0]&2) != 0) rLen += ivec[2]*ivec[5];
      }
      if (ivec != NULL) EG_free(ivec);
    } else {
      if (msave != mtype) {
        if (ivec != NULL) EG_free(ivec);
        EG_free(rvec);
        EG_free(rsave);
        return EGADS_GEOMERR;
      }
      if (mtype == BEZIER) {
        n = 3*ivec[2]*ivec[4];
        if ((ivec[0]&2) != 0) n += ivec[2]*ivec[4];
        if (n != rLen) {
          if (ivec != NULL) EG_free(ivec);
          EG_free(rvec);
          EG_free(rsave);
          return EGADS_GEOMERR;
        }
      }
      if (mtype == BSPLINE) {
        n = ivec[3] + ivec[6]  + 3*ivec[2]*ivec[5];
        if ((ivec[0]&2) != 0) n += ivec[2]*ivec[5];
        if (n != rLen) {
          if (ivec != NULL) EG_free(ivec);
          EG_free(rvec);
          EG_free(rsave);
          return EGADS_GEOMERR;
        }
      }
      if (ivec != NULL) EG_free(ivec);
      
      if ((rvec == NULL) || (rsave == NULL)) {
        EG_free(rvec);
        EG_free(rsave);
        return EGADS_GEOMERR;
      }
      for (n = 0; n < rLen; n++)
        if (rvec[n] != rsave[n]) {
          EG_free(rvec);
          EG_free(rsave);
          return EGADS_GEOMERR;
        }

      EG_free(rvec);
    }
  }

  EG_free(rsave);
  return GEM_SUCCESS;
}
