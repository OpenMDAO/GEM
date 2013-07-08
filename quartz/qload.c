/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Load Function -- CAPRI
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gem.h"
#include "memory.h"
#include "attribute.h"
#include "capri.h"

  extern int *CAPRI_MdlRefs;
  extern int mCAPRI_MdlRefs;


  /* use undocumented CAPRI function */
  extern int  gi_orientFace(int vol, int face, int *norm);
  
  extern void gem_releaseBRep(/*@only@*/ gemBRep *brep);
  extern void gem_clrModel(/*@only@*/ gemModel *model);
  extern int  gem_invertXform(double *xform, double *invXform);


static void
gem_fillEntAttrs(int vol, int type, int index, gemAttrs **attrx)
{
  int    i, stat, nattr, len, atype, *ctype, *ivec;
  char   **aname, *str;
  double *rvec;

  aname = NULL;
  stat  = gi_dEntAttributes(vol, type, index, &aname, &ctype, &nattr);
  if (stat != CAPRI_SUCCESS) return;
  if (nattr == 0) return;
  
  if ((aname != NULL) && (ctype != NULL))
    for (i = 0; i < nattr; i++) {
      stat = gi_dGetEntAttribute(vol, type, index, aname[i], ctype[i],
                                 &atype, &len, &ivec, &rvec, &str);
      if (stat != CAPRI_SUCCESS) continue;
      gem_setAttrib(attrx, aname[i], atype, len, ivec, rvec, str);
    }

  if (ctype != NULL) gi_free(ctype);
  if (aname != NULL) {
    for (i = 0; i < nattr; i++) gi_free(aname[i]);
    gi_free(aname);
  }
}


int
gem_quartzBody(int vol, gemBRep *brep)
{
  int     i, j, k, m, n, nshell, nface, nloop, nedge, nnode;
  int     stat, type, nloops, sID, nsface, *outer, *loops, *edgsen;
  int     *lshell, *faces, *sfaces;
  char    *name, *ID;
  double  uvlims[4];
  gemID   gid;
  gemBody *body;

  type = gi_dVolumeType(vol);
  if (type <  CAPRI_SUCCESS) return type;
  stat = gi_iGetDisplace(vol, brep->xform);
  if (stat != CAPRI_SUCCESS) return stat;
  stat = gem_invertXform(brep->xform, brep->invXform);
  if (stat != GEM_SUCCESS) return stat;
  stat = gi_dGetVolume(vol, &nnode, &nedge, &nface, &nshell, &name);
  if (stat != CAPRI_SUCCESS) return stat;
  if (type == 2) {
    stat = gi_dGetWire(vol, &nloop, &loops, &edgsen);
    if (stat != CAPRI_SUCCESS) return stat;
    nshell = 0;
  } else {
    nloop = 0;
    for (i = 1; i <= nface; i++) {
      stat = gi_dGetFace(vol, i, uvlims, &nloops, &loops, &edgsen);
      if (stat != CAPRI_SUCCESS) return stat;
      nloop += nloops;
    }
    nshell = 1;
  }

  gid.index     = vol;
  gid.ident.tag = 0;
  
  body = (gemBody *) gem_allocate(sizeof(gemBody));
  if (body == NULL) return GEM_ALLOC;
  body->handle = gid;
  body->nnode  = 0;
  body->nodes  = NULL;
  body->nedge  = 0;
  body->edges  = NULL;
  body->nloop  = 0;
  body->loops  = NULL;
  body->nface  = 0;
  body->faces  = NULL;
  body->nshell = 0;
  body->shells = NULL;
  body->attr   = NULL;
  body->type   = GEM_SOLID;
  if (type == 1) body->type = GEM_SHEET;
  if (type == 2) body->type = GEM_WIRE;
  brep->body = body;
  gi_dBox(vol, body->box);
  
  /* make the nodes */
  if (nnode != 0) {
    body->nodes = (gemNode *) gem_allocate(nnode*sizeof(gemNode));
    if (body->nodes == NULL) return GEM_ALLOC;
    for (i = 0; i < nnode; i++) body->nodes[i].attr = NULL;
    body->nnode = nnode;
    for (i = 0; i < nnode; i++) {
      stat = gi_dGetNode(vol, i+1, body->nodes[i].xyz);
      if (stat != CAPRI_SUCCESS) return stat;
      gid.ident.tag         = i+1;
      body->nodes[i].handle = gid;
      gem_fillEntAttrs(vol, CAPRI_NODE, i+1, &body->nodes[i].attr);
    }
  }
  
  /* make the edges */
  if (nedge != 0) {
    body->edges = (gemEdge *) gem_allocate(nedge*sizeof(gemEdge));
    if (body->edges == NULL) return GEM_ALLOC;
    for (i = 0; i < nedge; i++) body->edges[i].attr = NULL;
    body->nedge = nedge;
    for (i = 0; i < nedge; i++) {
      stat = gi_dGetEdge(vol, i+1, body->edges[i].tlimit, 
                                   body->edges[i].nodes);
      if (stat != CAPRI_SUCCESS) return stat;
      gid.ident.tag           = i+1;
      body->edges[i].handle   = gid;
      body->edges[i].faces[0] = body->edges[i].faces[1] = 0;
      gem_fillEntAttrs(vol, CAPRI_EDGE, i+1, &body->edges[i].attr);
    }
  }
  
  /* make loops */
  if (nloop != 0) {
    body->loops = (gemLoop *) gem_allocate(nloop*sizeof(gemLoop));
    if (body->loops == NULL) return GEM_ALLOC;
    for (i = 0; i < nloop; i++) {
      body->loops[i].edges = NULL;
      body->loops[i].attr  = NULL;
    }
    body->nloop = nloop;

    if (type == 2) {

      stat = gi_dGetWire(vol, &nloops, &loops, &edgsen);
      if (stat != CAPRI_SUCCESS) return stat;
      for (n = i = 0; i < nloop; i++) {
        gid.ident.tag         = i+1;
        body->loops[i].handle = gid;
        body->loops[i].type   = 0;
        body->loops[i].face   = 0;
        body->loops[i].edges  = (int *) gem_allocate(loops[i]*sizeof(int));
        stat = GEM_ALLOC;
        if (body->loops[i].edges == NULL) return GEM_ALLOC;
        for (j = 0; j < loops[i]; j++, n++)
          body->loops[i].edges[j] = edgsen[2*j]*edgsen[2*j+1];
        body->loops[i].nedges = loops[i];
      }

    } else {

      m = 0;
      for (i = 1; i <= nface; i++) {
        stat = gi_dGetFace(vol, i, uvlims, &nloops, &loops, &edgsen);
        if (stat != CAPRI_SUCCESS) return stat;
        outer = NULL;
        if (nloops > 1) {
          stat = gi_cOrderLoops(vol, i, &nsface, &nloops, &outer);
          if (stat != CAPRI_SUCCESS) return stat;
        }
        for (n = k = 0; k < nloops; k++, m++) {
          gid.ident.tag         = k+1;
          body->loops[m].handle = gid;
          body->loops[m].type   = 0;
          body->loops[m].face   = i;
          body->loops[m].edges  = (int *) gem_allocate(loops[k]*sizeof(int));
          if (body->loops[m].edges == NULL) {
            if (outer != NULL) gi_free(outer);
            return GEM_ALLOC;
          }
          for (j = 0; j < loops[k]; j++, n++)
            body->loops[m].edges[j] = edgsen[2*j]*edgsen[2*j+1];
          body->loops[m].nedges = loops[k];
          if (outer != NULL) 
            if (outer[k] < 0) body->loops[m].type = 1;
        }
        if (outer != NULL) gi_free(outer);
      }

    }
  }

  /* make the faces */
  if (nface != 0) {
    body->faces = (gemFace *) gem_allocate(nface*sizeof(gemFace));
    if (body->faces == NULL) return GEM_ALLOC;
    for (i = 0; i < nface; i++) {
      body->faces[i].loops = NULL;
      body->faces[i].ID    = NULL;
      body->faces[i].attr  = NULL;
    }
    body->nface = nface;
    for (n = i = 0; i < nface; i++) {
      stat = gi_dEntityID(vol, CAPRI_FACE, i+1, &sID, &ID);
      if (stat != CAPRI_SUCCESS) return stat;
      stat = gi_dGetFace(vol, i+1, body->faces[i].uvbox, &nloops, 
                         &loops, &edgsen);
      if (stat != CAPRI_SUCCESS) return stat;
      stat = gi_orientFace(vol, i+1, &body->faces[i].norm);
      if (stat != CAPRI_SUCCESS) return stat;
      gid.ident.tag         = i+1;
      body->faces[i].handle = gid;
      body->faces[i].ID     = gem_strdup(ID);
      body->faces[i].nloops = nloops;
      body->faces[i].loops  = (int *) gem_allocate(nloops*sizeof(int));
      gi_free(ID);
      if (body->faces[i].loops == NULL) return GEM_ALLOC;
      for (m = j = 0; j < nloops; j++) {
        n++;
        body->faces[i].loops[j] = n;
        m += loops[j];
      }
      if (body->edges != NULL)
        for (j = 0; j < m; j++) {
          k = edgsen[2*j];
          if (edgsen[2*j+1] == 1) {
            body->edges[k-1].faces[1] = i+1;
          } else {
            body->edges[k-1].faces[0] = i+1;
          }
        }
      
      gem_fillEntAttrs(vol, CAPRI_FACE, i+1, &body->faces[i].attr);
    }
  }
  
  /* make shells */
  if (nshell != 0) {
    if (type == 1) {

      body->shells = (gemShell *) gem_allocate(nshell*sizeof(gemShell));
      if (body->shells == NULL) return GEM_ALLOC;
      for (i = 0; i < nshell; i++) {
        body->shells[i].faces = NULL;
        body->shells[i].attr  = NULL;
      }
      body->nshell = nshell;
      for (i = 0; i < nshell; i++) {
        gid.ident.tag          = i+1;
        body->shells[i].handle = gid;
        body->shells[i].type   = 0;
        body->shells[i].faces  = (int *) gem_allocate(nface*sizeof(int));
        if (body->shells[i].faces == NULL) return GEM_ALLOC;
        body->shells[i].nfaces = nface;
        for (j = 0; j < nface; j++)
          body->shells[i].faces[j] = j+1;
      }

    } else {

      stat = gi_cGetShells(vol, &nshell, &lshell, &faces, &sfaces);
      if (stat != CAPRI_SUCCESS) return stat;
      body->shells = (gemShell *) gem_allocate(nshell*sizeof(gemShell));
      if (body->shells == NULL) return GEM_ALLOC;
      for (i = 0; i < nshell; i++) {
        body->shells[i].faces = NULL;
        body->shells[i].attr  = NULL;
      }
      body->nshell = nshell;
      if (nshell == 1) {
        for (i = 0; i < nshell; i++) {
          gid.ident.tag          = i+1;
          body->shells[i].handle = gid;
          body->shells[i].type   = 0;
          body->shells[i].faces  = (int *) gem_allocate(nface*sizeof(int));
          if (body->shells[i].faces == NULL) return GEM_ALLOC;
          body->shells[i].nfaces = nface;
          for (j = 0; j < nface; j++)
            body->shells[i].faces[j] = j+1;
        }    
      } else {
        for (m = i = 0; i < nshell; i++) {
          gid.ident.tag          = i+1;
          body->shells[i].handle = gid;
          body->shells[i].type   = 0;
          body->shells[i].faces  = (int *) 
                                   gem_allocate(lshell[2*i]*sizeof(int));
          if (body->shells[i].faces == NULL) return GEM_ALLOC;
          body->shells[i].nfaces = lshell[2*i];
          if (lshell[2*i+1] < 0) body->shells[i].type = 1;
          for (j = 0; j < lshell[2*i]; j++, m++)
            body->shells[i].faces[j] = faces[m];
        }  
      }
      if (lshell != NULL) gi_free(lshell);
      if (faces  != NULL) gi_free(faces);
      if (sfaces != NULL) gi_free(sfaces);
      
    }
  }

  gem_fillEntAttrs(vol, CAPRI_VOLUME, 0, &body->attr);
  return GEM_SUCCESS;
}


int
gem_fillMM(int mmdl, gemModel *mdl)
{
  int      i, j, k, n, stat, cmdl, vol, nparam, nbranch, bflag, type, len;
  int      *ivec, irange[2], units, ipar, nchild, suppres, ibranch;
  char     *name, *str, *CADtype;
  double   *rvec, rrange[2];
  gemParam *params;
  gemFeat  *branches;

  stat = gi_fGetMModel(mmdl, &cmdl, &vol, &nparam, &nbranch);
  if (stat != CAPRI_SUCCESS) return stat;
  if ((nparam == 0) && (nbranch == 0)) return CAPRI_SUCCESS;

  /* fill up the parameters */
  if (nparam > 0) {
    params = (gemParam *) gem_allocate(nparam*sizeof(gemParam));
    if (params == NULL) return GEM_ALLOC;
    for (i = 0; i < nparam; i++) {
      params[i].name             = NULL;
      params[i].handle.index     = mmdl;
      params[i].handle.ident.tag = i+1;
      params[i].type             = GEM_INTEGER;
      params[i].order            = 0;
      params[i].len              = 1;
      params[i].vals.integer     = 0;
      params[i].bnds.ilims[0]    = 0;
      params[i].bnds.ilims[1]    = 0;
      params[i].attr             = NULL;
      params[i].changed          = 0;
    }
    mdl->nParams = nparam;
    mdl->Params  = params;

    for (i = 0; i < nparam; i++) {
      stat = gi_fGetParam(mmdl, i+1, &name, &type, &bflag, &len, &ibranch);
      if (stat != CAPRI_SUCCESS) return stat;
      params[i].name    = gem_strdup(name);
      params[i].len     = len;
      params[i].bitflag = bflag&1;
      if (type == CAPRI_BOOL) {
        stat = gi_fGetBool(mmdl, i+1, &ivec);
        if (stat != CAPRI_SUCCESS) return stat;
        if (len == 1) {
          params[i].vals.bool1 = ivec[0];
        } else {
          params[i].vals.bools = (int *) gem_allocate(len*sizeof(int));
          if (params[i].vals.bools == NULL) return GEM_ALLOC;
          for (j = 0; j < len; j++)
            params[i].vals.bools[j] = ivec[j];
        }
        params[i].type = GEM_BOOL;
      } else if (type == CAPRI_INTEGER) {
        stat = gi_fGetInteger(mmdl, i+1, &ivec, irange);
        if (stat != CAPRI_SUCCESS) return stat;
        if (len == 1) {
          params[i].vals.integer = ivec[0];
        } else {
          params[i].vals.integers = (int *) gem_allocate(len*sizeof(int));
          if (params[i].vals.integers == NULL) return GEM_ALLOC;
          for (j = 0; j < len; j++)
            params[i].vals.integers[j] = ivec[j];
        }
        params[i].type = GEM_INTEGER;
        if (bflag&2 != 0) {
          params[i].bitflag       |= 8;
          params[i].bnds.ilims[0]  = irange[0];
          params[i].bnds.ilims[1]  = irange[1];
        }
      } else if (type == CAPRI_REAL) {
        stat = gi_fGetReal(mmdl, i+1, &rvec, rrange, &units);
        if (stat != CAPRI_SUCCESS) return stat;
        if (len == 1) {
          params[i].vals.real = rvec[0];
        } else {
          params[i].vals.reals = (double *) gem_allocate(len*sizeof(double));
          if (params[i].vals.reals == NULL) return GEM_ALLOC;
          for (j = 0; j < len; j++)
            params[i].vals.reals[j] = rvec[j];
        }
        params[i].type = GEM_REAL;
        if (bflag&2 != 0) {
          params[i].bitflag       |= 8;
          params[i].bnds.rlims[0]  = rrange[0];
          params[i].bnds.rlims[1]  = rrange[1];
        }
      } else if (type == CAPRI_STRING) {
        stat = gi_fGetString(mmdl, i+1, &str);
        if (stat != CAPRI_SUCCESS) return stat;
        params[i].vals.string = gem_strdup(str);
        params[i].type = GEM_STRING;
      } else {
        stat = gi_fGetSpline(mmdl, i+1, &params[i].order, &units, &rvec);
        if (stat != CAPRI_SUCCESS) return stat;
        if (units == 1) params[i].bitflag |= 2;
        if (type >= CAPRI_3DSPLINE) params[i].bitflag |= 4;
        params[i].vals.splpnts = (gemSpl *) gem_allocate(len*sizeof(gemSpl));
        if (params[i].vals.splpnts == NULL) return GEM_ALLOC;
        switch (type) {
          case CAPRI_SPLINE:
            for (j = 0; j < len; j++) {
              params[i].vals.splpnts[j].pntype  = GEM_POSITION;
              params[i].vals.splpnts[j].pos[0]  = rvec[2*j  ];
              params[i].vals.splpnts[j].pos[1]  = rvec[2*j+1];
              params[i].vals.splpnts[j].pos[2]  = 0.0;
              params[i].vals.splpnts[j].tan[0]  = 0.0;
              params[i].vals.splpnts[j].tan[1]  = 0.0;
              params[i].vals.splpnts[j].tan[2]  = 0.0;
              params[i].vals.splpnts[j].radc[0] = 0.0;
              params[i].vals.splpnts[j].radc[1] = 0.0;
              params[i].vals.splpnts[j].radc[2] = 0.0;
            }
            params[i].vals.splpnts[    0].pntype = 0;
            params[i].vals.splpnts[len-1].pntype = 0;
            break;
          case CAPRI_SPEND:
            for (j = 0; j < len; j++) {
              params[i].vals.splpnts[j].pntype  = GEM_POSITION;
              params[i].vals.splpnts[j].pos[0]  = rvec[2*j+2];
              params[i].vals.splpnts[j].pos[1]  = rvec[2*j+3];
              params[i].vals.splpnts[j].pos[2]  = 0.0;
              params[i].vals.splpnts[j].tan[0]  = 0.0;
              params[i].vals.splpnts[j].tan[1]  = 0.0;
              params[i].vals.splpnts[j].tan[2]  = 0.0;
              params[i].vals.splpnts[j].radc[0] = 0.0;
              params[i].vals.splpnts[j].radc[1] = 0.0;
              params[i].vals.splpnts[j].radc[2] = 0.0;
            }
            params[i].vals.splpnts[    0].pntype = GEM_TANGENT;
            params[i].vals.splpnts[    0].tan[0] = rvec[0];
            params[i].vals.splpnts[    0].tan[1] = rvec[1];
            params[i].vals.splpnts[len-1].pntype = GEM_TANGENT;
            params[i].vals.splpnts[len-1].tan[0] = rvec[2*len+2];
            params[i].vals.splpnts[len-1].tan[1] = rvec[2*len+3];
            break;
          case CAPRI_SPTAN:
            for (j = 0; j < len; j++) {
              params[i].vals.splpnts[j].pntype  = GEM_POSITION | GEM_TANGENT;
              params[i].vals.splpnts[j].pos[0]  = rvec[4*j  ];
              params[i].vals.splpnts[j].pos[1]  = rvec[4*j+1];
              params[i].vals.splpnts[j].pos[2]  = 0.0;
              params[i].vals.splpnts[j].tan[0]  = rvec[4*j+2];
              params[i].vals.splpnts[j].tan[1]  = rvec[4*j+3];
              params[i].vals.splpnts[j].tan[2]  = 0.0;
              params[i].vals.splpnts[j].radc[0] = 0.0;
              params[i].vals.splpnts[j].radc[1] = 0.0;
              params[i].vals.splpnts[j].radc[2] = 0.0;
            }
            params[i].vals.splpnts[    0].pntype = GEM_TANGENT;
            params[i].vals.splpnts[len-1].pntype = GEM_TANGENT;
            break;
          case CAPRI_SPCURV:
            for (j = 0; j < len; j++) {
              params[i].vals.splpnts[j].pntype  = GEM_POSITION | GEM_TANGENT |
                                                  GEM_RADCURV;
              params[i].vals.splpnts[j].pos[0]  = rvec[6*j  ];
              params[i].vals.splpnts[j].pos[1]  = rvec[6*j+1];
              params[i].vals.splpnts[j].pos[2]  = 0.0;
              params[i].vals.splpnts[j].tan[0]  = rvec[6*j+2];
              params[i].vals.splpnts[j].tan[1]  = rvec[6*j+3];
              params[i].vals.splpnts[j].tan[2]  = 0.0;
              params[i].vals.splpnts[j].radc[0] = rvec[6*j+4];
              params[i].vals.splpnts[j].radc[1] = rvec[6*j+5];
              params[i].vals.splpnts[j].radc[2] = 0.0;
            }
            params[i].vals.splpnts[    0].pntype = GEM_TANGENT | GEM_RADCURV;
            params[i].vals.splpnts[len-1].pntype = GEM_TANGENT | GEM_RADCURV;
            break;
          case CAPRI_3DSPLINE:
            for (j = 0; j < len; j++) {
              params[i].vals.splpnts[j].pntype  = GEM_POSITION;
              params[i].vals.splpnts[j].pos[0]  = rvec[3*j  ];
              params[i].vals.splpnts[j].pos[1]  = rvec[3*j+1];
              params[i].vals.splpnts[j].pos[2]  = rvec[3*j+2];
              params[i].vals.splpnts[j].tan[0]  = 0.0;
              params[i].vals.splpnts[j].tan[1]  = 0.0;
              params[i].vals.splpnts[j].tan[2]  = 0.0;
              params[i].vals.splpnts[j].radc[0] = 0.0;
              params[i].vals.splpnts[j].radc[1] = 0.0;
              params[i].vals.splpnts[j].radc[2] = 0.0;
            }
            break;
          case CAPRI_3DSPEND:
            for (j = 0; j < len; j++) {
              params[i].vals.splpnts[j].pntype  = GEM_POSITION;
              params[i].vals.splpnts[j].pos[0]  = rvec[3*j+3];
              params[i].vals.splpnts[j].pos[1]  = rvec[3*j+4];
              params[i].vals.splpnts[j].pos[2]  = rvec[3*j+5];
              params[i].vals.splpnts[j].tan[0]  = 0.0;
              params[i].vals.splpnts[j].tan[1]  = 0.0;
              params[i].vals.splpnts[j].tan[2]  = 0.0;
              params[i].vals.splpnts[j].radc[0] = 0.0;
              params[i].vals.splpnts[j].radc[1] = 0.0;
              params[i].vals.splpnts[j].radc[2] = 0.0;
            }
            params[i].vals.splpnts[    0].pntype |= GEM_TANGENT;
            params[i].vals.splpnts[    0].tan[0]  = rvec[0];
            params[i].vals.splpnts[    0].tan[1]  = rvec[1];
            params[i].vals.splpnts[    0].tan[2]  = rvec[2];
            params[i].vals.splpnts[len-1].pntype |= GEM_TANGENT;
            params[i].vals.splpnts[len-1].tan[0]  = rvec[3*len+3];
            params[i].vals.splpnts[len-1].tan[1]  = rvec[3*len+4];
            params[i].vals.splpnts[len-1].tan[2]  = rvec[3*len+5];
            break;
          case CAPRI_3DSPTAN:
            for (j = 0; j < len; j++) {
              params[i].vals.splpnts[j].pntype  = GEM_POSITION | GEM_TANGENT;
              params[i].vals.splpnts[j].pos[0]  = rvec[6*j  ];
              params[i].vals.splpnts[j].pos[1]  = rvec[6*j+1];
              params[i].vals.splpnts[j].pos[2]  = rvec[6*j+2];
              params[i].vals.splpnts[j].tan[0]  = rvec[6*j+3];
              params[i].vals.splpnts[j].tan[1]  = rvec[6*j+4];
              params[i].vals.splpnts[j].tan[2]  = rvec[6*j+5];
              params[i].vals.splpnts[j].radc[0] = 0.0;
              params[i].vals.splpnts[j].radc[1] = 0.0;
              params[i].vals.splpnts[j].radc[2] = 0.0;
            }
            break;
          case CAPRI_3DSPCURV:
            for (j = 0; j < len; j++) {
              params[i].vals.splpnts[j].pntype  = GEM_POSITION | GEM_TANGENT |
                                                  GEM_RADCURV;
              params[i].vals.splpnts[j].pos[0]  = rvec[9*j  ];
              params[i].vals.splpnts[j].pos[1]  = rvec[9*j+1];
              params[i].vals.splpnts[j].pos[2]  = rvec[9*j+2];
              params[i].vals.splpnts[j].tan[0]  = rvec[9*j+3];
              params[i].vals.splpnts[j].tan[1]  = rvec[9*j+4];
              params[i].vals.splpnts[j].tan[2]  = rvec[9*j+5];
              params[i].vals.splpnts[j].radc[0] = rvec[9*j+6];
              params[i].vals.splpnts[j].radc[1] = rvec[9*j+7];
              params[i].vals.splpnts[j].radc[2] = rvec[9*j+8];
            }
            break;
        }
        params[i].type = GEM_SPLINE;
      }
    }
  }

  /* get the branches */
  if (nbranch > 0) {
    nbranch++;
    branches = (gemFeat *) gem_allocate(nbranch*sizeof(gemFeat));
    if (branches == NULL) return GEM_ALLOC;
    for (i = 0; i < nbranch; i++) {
      branches[i].name             = NULL;
      branches[i].handle.index     = mmdl;
      branches[i].handle.ident.tag = i;
      branches[i].branchType       = NULL;
      branches[i].nParents         = 1;
      branches[i].parents.pnode    = 0;
      branches[i].nChildren        = 0;
      branches[i].children.node    = 0;
      branches[i].attr             = NULL;
      branches[i].changed          = 0;
    }
    mdl->nBranches = nbranch;
    mdl->Branches  = branches;

    for (i = 0; i < nbranch; i++) {
      stat = gi_fGetBranch(mmdl, i, &name, &CADtype, &suppres, &ipar, &nchild,
                           &ivec);
      if (stat != CAPRI_SUCCESS) return stat;
      branches[i].name          = gem_strdup(name);
      branches[i].sflag         = suppres;
      branches[i].parents.pnode = ipar+1;
      if (nchild == 1) {
        branches[i].children.node = ivec[0]+1;
        branches[i].nChildren     = 1;
      } else if (nchild > 1) {
        branches[i].children.nodes = (int *) gem_allocate(nchild*sizeof(int));
        if (branches[i].children.nodes == NULL) return GEM_ALLOC;
        for (j = 0; j < nchild; j++)
          branches[i].children.nodes[j] = ivec[j]+1;
        branches[i].nChildren = nchild;
      }
      if (CADtype != NULL)
        if (strcmp(CADtype, "COMPONENT") == 0) {
          branches[i].branchType = gem_strdup("INSTANCE");
        } else {
          branches[i].branchType = gem_strdup(CADtype);
        }
      gem_fillEntAttrs(mmdl, CAPRI_BRANCH, i, &branches[i].attr);
    }

    /* find multiple parents */
    branches[0].nParents      = 0;
    branches[0].parents.pnode = 0;
    for (i = 1; i < nbranch; i++) {
      if (branches[i].parents.pnode != 0) continue;
      for (n = j = 0; j < nbranch; j++) {
        if (i == j) continue;
        if (branches[j].nChildren == 1) {
          if (branches[j].children.node == i+1) n++;
        } else if (branches[j].nChildren > 1) {
          for (k = 0; k < branches[j].nChildren; k++)
            if (branches[j].children.nodes[k] == i+1) {
              n++;
              break;
            }
        }
      }
      if (n == 0) {
        branches[i].nParents = 0;
      } else if (n == 1) {
        branches[i].parents.pnode = 0;
        for (j = 0; j < nbranch; j++) {
          if (i == j) continue;
          if (branches[j].nChildren == 1) {
            if (branches[j].children.node == i+1) 
              branches[i].parents.pnode = j+1;
          } else if (branches[j].nChildren > 1) {
            for (k = 0; k < branches[j].nChildren; k++)
              if (branches[j].children.nodes[k] == i+1) {
                branches[i].parents.pnode = j+1;
                break;
              }
          }
          if (branches[i].parents.pnode != 0) break;
        }
        branches[i].nParents = 1;
      } else {
        branches[i].parents.pnodes = (int *) gem_allocate(n*sizeof(int));
        if (branches[i].parents.pnodes == NULL) return GEM_ALLOC;
        for (n = j = 0; j < nbranch; j++) {
          if (i == j) continue;
          if (branches[j].nChildren == 1) {
            if (branches[j].children.node == i+1) {
              branches[i].parents.pnodes[n] = j+1;
              n++;
            }
          } else if (branches[j].nChildren > 1) {
            for (k = 0; k < branches[j].nChildren; k++)
              if (branches[j].children.nodes[k] == i+1) {
                branches[i].parents.pnodes[n] = j+1;
                n++;
                break;
              }
          }
        }
        branches[i].nParents = n;
      }
    }

    /* mark suppression state of "inactive" */

  }

  return CAPRI_SUCCESS;
}


int
gem_kernelLoad(gemCntxt *gem_cntxt, /*@null@*/ char *server, char *name, 
               gemModel **model)
{
  int      i, j, stat, atype, alen, aindex, cmdl, mmdl, v1, vn, mfile, *ivec;
  char     *modeler;
  double   *rvec;
  gemID    gid;
  gemModel *mdl, *prev;
  gemBRep  **BReps;

  *model = NULL;
  if (gem_cntxt == NULL) return GEM_NULLOBJ;
  if (gem_cntxt->magic != GEM_MCONTEXT) return GEM_BADCONTEXT;
  if (name == NULL) return CAPRI_INVALID;

  stat = gem_retAttribute(gem_cntxt, 0, 0, "Modeler", &aindex, &atype, &alen,
                          &ivec, &rvec, &modeler);
  if (stat != GEM_SUCCESS) {
    if (stat == GEM_NOTFOUND) 
      printf(" Quartz Error: Attribute 'Modeler' not set for CAPRI Load!\n");
    return stat;
  }
  if (atype != GEM_STRING) return GEM_BADTYPE;

  cmdl = gi_uLoadModel(server, modeler, name);
  if (cmdl < CAPRI_SUCCESS) return cmdl;
  mmdl = gi_fMasterModel(cmdl, 0);
  if ((mmdl <= CAPRI_SUCCESS) && (mmdl != CAPRI_UNSUPPORT)) return stat;
  gid.index     = cmdl;
  gid.ident.tag = mmdl;

  stat = gi_dGetModel(cmdl, &v1, &vn, &mfile, &modeler);
  if (stat != CAPRI_SUCCESS) {
    gi_uRelModel(cmdl);
    return stat;
  }

  BReps = (gemBRep **) gem_allocate((vn-v1+1)*sizeof(gemBRep *));
  if (BReps == NULL) {
    gi_uRelModel(cmdl);
    return GEM_ALLOC;
  }
  for (i = 0; i < vn-v1+1; i++) {
    BReps[i] = (gemBRep *) gem_allocate(sizeof(gemBRep));
    if (BReps[i] == NULL) {
      for (j = 0; j < i; j++) gem_free(BReps[j]);
      gem_free(BReps);
      gi_uRelModel(cmdl);
      return GEM_ALLOC;
    }
    BReps[i]->magic   = GEM_MBREP;
    BReps[i]->omodel  = NULL;
    BReps[i]->phandle = gid;
    BReps[i]->ibranch = 0;
    BReps[i]->inumber = 0;
    BReps[i]->body    = NULL;
  }
  for (i = 0; i < vn-v1+1; i++) {
    stat = gem_quartzBody(v1+i, BReps[i]);
    if (stat != CAPRI_SUCCESS) {
      for (j = 0; j <= i; j++)
        gem_releaseBRep(BReps[j]);
      gem_free(BReps);
      gi_uRelModel(cmdl);
      return stat;
    }
  }

  mdl = (gemModel *) gem_allocate(sizeof(gemModel));
  if (mdl == NULL) {
    for (i = 0; i < vn-v1+1; i++)
      gem_releaseBRep(BReps[i]);
    gem_free(BReps);
    gi_uRelModel(cmdl);
    return GEM_ALLOC;
  }

  mdl->magic     = GEM_MMODEL;
  mdl->handle    = gid;
  mdl->nonparam  = 1;
  mdl->server    = NULL;
  mdl->location  = gem_strdup(name);
  mdl->modeler   = gem_strdup(modeler);
  mdl->nBRep     = vn - v1 + 1;
  mdl->BReps     = BReps;
  mdl->nParams   = 0;
  mdl->Params    = NULL;
  mdl->nBranches = 0;
  mdl->Branches  = NULL;
  mdl->attr      = NULL;
  mdl->prev      = (gemModel *) gem_cntxt;
  mdl->next      = NULL;
  for (i = 0; i < vn-v1+1; i++) BReps[i]->omodel = mdl;

  if (mmdl > CAPRI_SUCCESS) {
    stat = gem_fillMM(mmdl, mdl);
    if (stat != GEM_SUCCESS) {
      gem_clrModel(mdl);
      gi_uRelModel(cmdl);
      return stat;
    }
    mdl->nonparam = 0;
  }
  
  /* set the new model in our linked list*/
  prev = gem_cntxt->model;
  gem_cntxt->model = mdl;
  if (prev != NULL) {
    mdl->next  = prev;
    prev->prev = gem_cntxt->model;
  }

  /* set our reference */
  if (CAPRI_MdlRefs != NULL) {
    if (cmdl >= mCAPRI_MdlRefs) {
      i    = cmdl+256;
      ivec = (int *) gem_reallocate(CAPRI_MdlRefs, i*sizeof(int));
      if (ivec == NULL) {
        printf(" GEM/CAPRI Internal: Model References Disabled!\n");
        gem_free(CAPRI_MdlRefs);
         CAPRI_MdlRefs = NULL;
        mCAPRI_MdlRefs = 0;
      }
       CAPRI_MdlRefs = ivec;
      mCAPRI_MdlRefs = i;
    }
    CAPRI_MdlRefs[cmdl-1] = 1;
  }

  *model = mdl;
  return GEM_SUCCESS;
}
