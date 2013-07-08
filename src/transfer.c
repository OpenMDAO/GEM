/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             DRep Data Transfer Functions
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "gem.h"
#include "memory.h"
#include "disMethod.h"
#include "connect.h"


//#define DEBUG


  typedef struct {
    int      irank;
    int      nrank;
    int      sindx;             /* source DLL index */
    int      tindx;             /* target DLL index */
    int      geomFs;            /* geomFlag for source */
    int      geomFt;            /* geomFlag for target */
    double   afact;             /* area penalty function weight */
    double   area_src;          /* area associated with source (output) */
    double   area_tgt;          /* area associated with target (output) */
    gemQuilt *src;
    double   *data_src;
    gemQuilt *tgt;
    /*@exposed@*/
    double   *data_tgt;
    int      nmat;              /* number of MatchPoints */
    gemMatch *mat;              /* array  of MatchPoints */
    gInterp  *Interpolate;
    bInterp  *Interpol_bar;
    gIntegr  *Integrate;
    bIntegr  *Integr_bar;
  } gemCFit;


  extern double gem_orienTri(double *t0, double *t1, double *t2);
  extern int    gem_metDLoaded(const char *name);
  extern int    gem_conjGrad(int (*objFn)(int n, double x[], void *data,
                                          double *obj, /*@null@*/ double grad[]),
                             void *data, int n, double x[], double ftol,
                             /*@null@*/ FILE *fp, double *fopt);



void
gem_fillNeighbor(int k1, int k2, int *tri, int *kside, int *vtable,
                 gemNeigh *stable)
{
  int kn1, kn2, iside, oside, look;
  
  if (k1 > k2) {
    kn1 = k2-1;
    kn2 = k1-1;
  } else {
    kn1 = k1-1;
    kn2 = k2-1;
  }
  
  /* add to side table */
  
  if (vtable[kn1] == NOTFILLED) {
    
    /* virgin vert */
    *kside               += 1;
    vtable[kn1]           = *kside;
    stable[*kside].vert1  = kn1;
    stable[*kside].vert2  = kn2;
    stable[*kside].tri    = tri;
    stable[*kside].thread = NOTFILLED;
    return;
    
  } else {
    
    /* old vert */
    iside = vtable[kn1];
    
  again:
    if (stable[iside].vert2 == kn2) {
      
      /* found it! */
      if (stable[iside].tri != NULL) {
        look = *stable[iside].tri;
        *stable[iside].tri = *tri;
        *tri = look;
        stable[iside].tri = NULL;
      } else {
        printf("GEM Internal: Side %d %d complete [but %d] (gem_fillNeighbor)!\n",
               k1+1, k2+1, *tri);
      }
      return;

    }
    oside = iside;
    iside = stable[oside].thread;
    
    /* try next position in thread */
    if (iside == NOTFILLED) {
      *kside               += 1;
      stable[oside].thread  = *kside;
      stable[*kside].vert1  = kn1;
      stable[*kside].vert2  = kn2;
      stable[*kside].tri    = tri;
      stable[*kside].thread = NOTFILLED;
      return;
    }
    
    goto again;
  }
}


static int
gem_sign(double s)
{
  if (s > 0.0) return  1;
  if (s < 0.0) return -1;
  return  0;
}


/* 
 * returns the intersection status between a point and a tri
 *         vertex weights returned in w
 */
static int
gem_inTriExact(double *t1, double *t2, double *t3, double *p, double *w)
{
  int    d1, d2, d3;
  double sum;
  
  w[0] = gem_orienTri(t2, t3, p);
  w[1] = gem_orienTri(t1, p,  t3);
  w[2] = gem_orienTri(t1, t2, p);
  d1   = gem_sign(w[0]);
  d2   = gem_sign(w[1]);
  d3   = gem_sign(w[2]);
  sum  = w[0] + w[1] + w[2];
  if (sum != 0.0) {
    w[0] /= sum;
    w[1] /= sum;
    w[2] /= sum;
  }
  
  if (d1*d2*d3 == 0)
    if (d1 == 0) {
      if ((d2 == 0) && (d3 == 0)) return GEM_DEGENERATE;
      if (d2 == d3) return GEM_SUCCESS;
      if (d2 ==  0) return GEM_SUCCESS;
      if (d3 ==  0) return GEM_SUCCESS;
    } else if (d2 == 0) {
      if (d1 == d3) return GEM_SUCCESS;
      if (d3 ==  0) return GEM_SUCCESS;
    } else {
      if (d1 == d2) return GEM_SUCCESS;
    }
  
  /* all resultant tris have the same sign -> intersection */
  if ((d1 == d2) && (d2 == d3)) return GEM_SUCCESS;
  
  /* otherwise then no intersection */
  return GEM_OUTSIDE;
}


static void
gem_inElem(gemQuilt *quilt, invEval iEval, double *uvq, int npts,
           gemTarget *target, double *uvs)
{
  int    i, j, k, i0, i1, i2, n, stat, type;
  double w[3], *st0, *st1, *st2;

  /* find closest triangle for all positions */
  for (i = 0; i < quilt->nElems; i++) {
    type = quilt->elems[i].tIndex - 1;
    for (j = 0; j < quilt->types[type].ntri; j++) {
      n   =  quilt->types[type].tris[3*j  ] - 1;
      i0  =  quilt->elems[i].gIndices[n] - 1;
      st0 = &quilt->types[type].gst[2*n];
      n   =  quilt->types[type].tris[3*j+1] - 1;
      i1  =  quilt->elems[i].gIndices[n] - 1;
      st1 = &quilt->types[type].gst[2*n];
      n   =  quilt->types[type].tris[3*j+2] - 1;
      i2  =  quilt->elems[i].gIndices[n] - 1;
      st2 = &quilt->types[type].gst[2*n];
      if (gem_sign(gem_orienTri(&uvq[2*i0], &uvq[2*i1],
                                &uvq[2*i2])) == 0) continue;
      for (k = 0; k < npts; k++) {
        if (target[k].eIndex > 0) continue;
        stat = gem_inTriExact(&uvq[2*i0], &uvq[2*i1], &uvq[2*i2], &uvs[2*k], w);
        if (stat == GEM_SUCCESS) {
          /* inside -- set the position */
          target[k].eIndex = i+1;
          target[k].st[0]  = w[0]*st0[0] + w[1]*st1[0] + w[2]*st2[0];
          target[k].st[1]  = w[0]*st0[1] + w[1]*st1[1] + w[2]*st2[1];
/*        printf("  %d/%d:  %lf %lf  ", i+1, j, 
                 target[k].st[0], target[k].st[1]);  */
          iEval(quilt, uvq, &uvs[2*k], &target[k].eIndex, target[k].st);
/*        printf(" %lf %lf\n", target[k].st[0], target[k].st[1]);  */
          break;
        }
        if (w[1] < w[0]) w[0] = w[1];
        if (w[2] < w[0]) w[0] = w[2];
        if (target[k].eIndex == 0) {
          target[k].eIndex = -i-1;
          target[k].st[0]  = w[0];
          target[k].st[1]  = j;
        } else {
          if (w[0] > target[k].st[0]) {
            target[k].eIndex = -i-1;
            target[k].st[0]  = w[0];
            target[k].st[1]  = j;
          }
        }
      }
    }
  }
  
  /* fix up points from extrapolated triangles */
  for (i = 0; i < npts; i++) {
    if (target[i].eIndex >= 0) continue;
    k    = -target[i].eIndex - 1;
    j    =  target[i].st[1] + 0.00001;
    type =  quilt->elems[k].tIndex - 1;
    n    =  quilt->types[type].tris[3*j  ] - 1;
    i0   =  quilt->elems[k].gIndices[n] - 1;
    st0  = &quilt->types[type].gst[2*n];
    n    =  quilt->types[type].tris[3*j+1] - 1;
    i1   =  quilt->elems[k].gIndices[n] - 1;
    st1  = &quilt->types[type].gst[2*n];
    n    =  quilt->types[type].tris[3*j+2] - 1;
    i2   =  quilt->elems[k].gIndices[n] - 1;
    st2  = &quilt->types[type].gst[2*n];
    gem_inTriExact(&uvq[2*i0], &uvq[2*i1], &uvq[2*i2], &uvs[2*i], w);
    target[i].eIndex = k+1;
    target[i].st[0]  = w[0]*st0[0] + w[1]*st1[0] + w[2]*st2[0];
    target[i].st[1]  = w[0]*st0[1] + w[1]*st1[1] + w[2]*st2[1];
/*  printf("  %d/%d:  %lf %lf  ", i+1, j,
           target[i].st[0], target[i].st[1]);  */
    iEval(quilt, uvq, &uvs[2*i], &target[i].eIndex, target[i].st);
/*  printf(" %lf %lf\n", target[i].st[0], target[i].st[1]);  */
  }

}


static int
gem_getPositions(gemDRep *drep, int bound, invEval iEval, gemXfer *xfer)
{
  int       i, npts, ivsrc, vs;
  double    *uvq, *uvs;
  gemQuilt  *quilt;
  gemTarget *target;

  ivsrc  = xfer->ivss;
  vs     = xfer->ivst;
  quilt  = drep->bound[bound-1].VSet[ivsrc-1].quilt;
  uvq    = drep->bound[bound-1].VSet[ivsrc-1].sets[1].dset.data;
  npts   = drep->bound[bound-1].VSet[vs-1].nonconn->npts;
  if (drep->bound[bound-1].VSet[vs-1].nSets < 2) return GEM_BADVSETINDEX;
  uvs    = drep->bound[bound-1].VSet[vs-1].sets[1].dset.data;

  target = (gemTarget *) gem_allocate(npts*sizeof(gemTarget));
  if (target == NULL) return GEM_ALLOC;
  for (i = 0; i < npts; i++) {
    target[i].eIndex = 0;
    target[i].st[0]  = 0.0;
    target[i].st[1]  = 0.0;
  }
  gem_inElem(quilt, iEval, uvq, npts, target, uvs);

  xfer->position   = target;
  xfer->nPositions = npts;
  return GEM_SUCCESS;
}


static int
gem_getConnPos(gemDRep *drep, int bound, invEval iEval, gemXfer *xfer)
{
  int       i, npts, ivsrc, vs, gflgt;
  double    *uvq, *uvs;
  gemQuilt  *quilt;
  gemTarget *target;
  
  ivsrc  = xfer->ivss;
  vs     = xfer->ivst;
  gflgt  = xfer->gflgt;
  quilt  = drep->bound[bound-1].VSet[ivsrc-1].quilt;
  uvq    = drep->bound[bound-1].VSet[ivsrc-1].sets[1].dset.data;
  npts   = drep->bound[bound-1].VSet[vs-1].quilt->nVerts;
  if (drep->bound[bound-1].VSet[vs-1].nSets < 2) return GEM_BADVSETINDEX;
  uvs    = drep->bound[bound-1].VSet[vs-1].sets[1].dset.data;
  if (gflgt == 0)
    uvs  = drep->bound[bound-1].VSet[vs-1].sets[3].dset.data;
  
  target = (gemTarget *) gem_allocate(npts*sizeof(gemTarget));
  if (target == NULL) return GEM_ALLOC;
  for (i = 0; i < npts; i++) {
    target[i].eIndex = 0;
    target[i].st[0]  = 0.0;
    target[i].st[1]  = 0.0;
  }
  gem_inElem(quilt, iEval, uvq, npts, target, uvs);
  
  xfer->position   = target;
  xfer->nPositions = npts;
  return GEM_SUCCESS;
}


static int
gem_makeMatch(gemDRep *drep, int bound, gInterp *Interpolate, invEval iEval,
              gemXfer *xfer)
{
  int       i, j, k, n, npts, ivsrc, vs, gflgt, t, nmat, mindx, *indices;
  double    *uvq, *uvs, *uvt, *st;
  gemQuilt  *quilt, *quiltt;
  gemTarget *target;
  gemMatch  *match;
  
  ivsrc  = xfer->ivss;
  vs     = xfer->ivst;
  gflgt  = xfer->gflgt;
  quilt  = drep->bound[bound-1].VSet[ivsrc-1].quilt;
  uvq    = drep->bound[bound-1].VSet[ivsrc-1].sets[1].dset.data;
  quiltt = drep->bound[bound-1].VSet[vs-1].quilt;
  if (drep->bound[bound-1].VSet[vs-1].nSets < 2) return GEM_BADVSETINDEX;
  uvt    = drep->bound[bound-1].VSet[vs-1].sets[1].dset.data;
  mindx  = gem_metDLoaded(drep->bound[bound-1].VSet[vs-1].disMethod);
  if (mindx < 0) return mindx;
  
  /* do we have specific match point positions? */
  for (nmat = i = 0; i < quiltt->nElems; i++) {
    t     = quiltt->elems[i].tIndex - 1;
    nmat += quiltt->types[t].nmat;
  }

  if (nmat == 0) {
    /* all reference -- use what we already have */
    npts  = xfer->nPositions;
    match = (gemMatch *) gem_allocate(npts*sizeof(gemMatch));
    if (match == NULL) return GEM_ALLOC;
    target = xfer->position;
    for (i = 0; i < npts; i++) {
      match[i].source.eIndex = target[i].eIndex;
      match[i].source.st[0]  = target[i].st[0];
      match[i].source.st[1]  = target[i].st[1];
      match[i].target.eIndex = 0;
      match[i].target.st[0]  = 0.0;
      match[i].target.st[1]  = 0.0;
    }

    /* fill in the target from the quilt directly */
    for (i = 0; i < quiltt->nElems; i++) {
      t = quiltt->elems[i].tIndex - 1;
      if (quiltt->types[t].ndata == 0) {
        n       = quiltt->types[t].nref;
        st      = quiltt->types[t].gst;
        indices = quiltt->elems[i].gIndices;
      } else {
        n       = quiltt->types[t].ndata;
        st      = quiltt->types[t].dst;
        indices = quiltt->elems[i].dIndices;
      }
      for (j = 0; j < n; j++) {
        k = indices[j] - 1;
        if (match[k].target.eIndex != 0) continue;
        match[k].target.eIndex = i+1;
        match[k].target.st[0]  = st[2*j  ];
        match[k].target.st[1]  = st[2*j+1];
      }
    }
/*
    for (i = 0; i < npts; i++) {
      int    i0, i1, i2;
      double w[3], tpar[2], spar[2];
    
      k       = match[i].source.eIndex-1;
      w[0]    = 1.0 - match[i].source.st[0] - match[i].source.st[1];
      w[1]    = match[i].source.st[0];
      w[2]    = match[i].source.st[1];
      i0      = quilt->elems[k].gIndices[0] - 1;
      i1      = quilt->elems[k].gIndices[1] - 1;
      i2      = quilt->elems[k].gIndices[2] - 1;
      spar[0] = w[0]*uvq[2*i0  ] + w[1]*uvq[2*i1  ] + w[2]*uvq[2*i2  ];
      spar[1] = w[0]*uvq[2*i0+1] + w[1]*uvq[2*i1+1] + w[2]*uvq[2*i2+1];
      k       = match[i].target.eIndex-1;
      w[0]    = 1.0 - match[i].target.st[0] - match[i].target.st[1];
      w[1]    = match[i].target.st[0];
      w[2]    = match[i].target.st[1];
      i0      = quiltt->elems[k].gIndices[0] - 1;
      i1      = quiltt->elems[k].gIndices[1] - 1;
      i2      = quiltt->elems[k].gIndices[2] - 1;
      tpar[0] = w[0]*uvt[2*i0  ] + w[1]*uvt[2*i1  ] + w[2]*uvt[2*i2  ];
      tpar[1] = w[0]*uvt[2*i0+1] + w[1]*uvt[2*i1+1] + w[2]*uvt[2*i2+1];
      printf(" %d: %d %lf %lf    %d %lf %lf\n", i+1, match[i].source.eIndex,
             spar[0], spar[1], match[i].target.eIndex, tpar[0], tpar[1]);
    }
 */
    xfer->match  = match;
    xfer->nMatch = npts;
    return GEM_SUCCESS;
  }

  /* count the positions */
  for (npts = i = 0; i < quiltt->nElems; i++) {
    t = quiltt->elems[i].tIndex - 1;
    if (quiltt->types[t].nmat == 0) {
      npts += quiltt->types[t].nref;
    } else {
      npts += quiltt->types[t].nmat;
    }
  }
  /* set the match locations */
  match = (gemMatch *) gem_allocate(npts*sizeof(gemMatch));
  if (match == NULL) return GEM_ALLOC;
  for (i = 0; i < npts; i++) {
    match[i].target.eIndex = 0;
    match[i].target.st[0]  = 0.0;
    match[i].target.st[1]  = 0.0;
  }

  uvs = (double *) gem_allocate(2*npts*sizeof(double));
  if (uvs == NULL) {
    gem_free(match);
    return GEM_ALLOC;
  }
  
  /* set things up in parameter space */
  for (k = i = 0; i < quiltt->nElems; i++) {
    t = quiltt->elems[i].tIndex - 1;
    if (quiltt->types[t].nmat == 0) {
      n  = quiltt->types[t].nref;
      st = quiltt->types[t].gst;
    } else {
      n  = quiltt->types[t].nmat;
      st = quiltt->types[t].matst;
    }
    for (j = 0; j < n; j++, k++)
      Interpolate[mindx](quiltt, 1, i+1, &st[2*j], 2, uvt, &uvs[2*k]);
  }
  
  target = (gemTarget *) gem_allocate(npts*sizeof(gemTarget));
  if (target == NULL) {
    gem_free(uvs);
    gem_free(match);
    return GEM_ALLOC;
  }
  for (i = 0; i < npts; i++) {
    target[i].eIndex = 0;
    target[i].st[0]  = 0.0;
    target[i].st[1]  = 0.0;
  }
  gem_inElem(quilt, iEval, uvq, npts, target, uvs);
  gem_free(uvs);

  for (i = 0; i < npts; i++) {
    match[i].source.eIndex = target[i].eIndex;
    match[i].source.st[0]  = target[i].st[0];
    match[i].source.st[1]  = target[i].st[1];
  }
  gem_free(target);
  
  /* fill in the target from the quilt */
  for (k = i = 0; i < quiltt->nElems; i++) {
    t = quiltt->elems[i].tIndex - 1;
    if (quiltt->types[t].nmat == 0) {
      n  = quiltt->types[t].nref;
      st = quiltt->types[t].gst;
    } else {
      n  = quiltt->types[t].nmat;
      st = quiltt->types[t].matst;
    }
    for (j = 0; j < n; j++, k++) {
      match[k].target.eIndex = i+1;
      match[k].target.st[0]  = st[2*j  ];
      match[k].target.st[1]  = st[2*j+1];
    }
  }
  
  xfer->match  = match;
  xfer->nMatch = npts;
  return GEM_SUCCESS;
}


/*
 * obj_bar: compute objective function and gradient via backward differentiation
 */
static int
obj_bar(int    n,                     /* (in)  number of design variables */
        double ftgt[],                /* (in)  design variables */
        void   *blind,                /* (in)  blind pointer to structure */
        double *obj,                  /* (out) objective function */
        double ftgt_bar[])            /* (out) gradient of objective function */
{
  int     status = GEM_SUCCESS;       /* (out) return status */
  
  int     idat, ielms, ielmt, imat, irank, jrank, nrank, sindx, tindx, gfs, gft;
  double  f_src, f_tgt;
  double  area_src, area_tgt, area_tgt_bar, obj_bar1;
  double  *result, *result_bar = NULL, *data_bar = NULL;
  gemCFit *cfit = (gemCFit *) blind;
  
  irank  = cfit->irank;
  nrank  = cfit->nrank;
  sindx  = cfit->sindx;
  tindx  = cfit->tindx;
  gfs    = cfit->geomFs;
  gft    = cfit->geomFt;

  result = (double *) gem_allocate(nrank*sizeof(double));
  if (result == NULL) return GEM_ALLOC;
  
  /* store ftgt into tgt structure */
  for (idat = 0; idat < n; idat++)
    cfit->data_tgt[nrank*idat+irank] = ftgt[idat];
  
  /* compute the area for src */
  area_src = 0.0;
  
  for (ielms = 0; ielms < cfit->src->nElems; ielms++) {
    status = cfit->Integrate[sindx](cfit->src, gfs, ielms+1, nrank,
                                    cfit->data_src, result);
    if (status != GEM_SUCCESS) goto cleanup;
    
    area_src += result[irank];
  }
  cfit->area_src = area_src;
  
  /* compute the area for tgt */
  area_tgt = 0.0;
  
  for (ielmt = 0; ielmt < cfit->tgt->nElems; ielmt++) {
    status = cfit->Integrate[tindx](cfit->tgt, gft, ielmt+1, nrank,
                                    cfit->data_tgt, result);
    if (status != GEM_SUCCESS) goto cleanup;
    
    area_tgt += result[irank];
  }
  cfit->area_tgt = area_tgt;
  
  /* penalty function part of objective function */
  *obj = cfit->afact * pow(area_tgt-area_src, 2);
  
  /* minimize the difference between source and target at the Match points */
  for (imat = 0; imat < cfit->nmat; imat++) {
    ielms  = cfit->mat[imat].source.eIndex;
    ielmt  = cfit->mat[imat].target.eIndex;
    if ((ielms == 0) || (ielmt == 0)) continue;
    status = cfit->Interpolate[sindx](cfit->src, gfs, ielms,
                                      cfit->mat[imat].source.st,
                                      nrank, cfit->data_src, result);
    if (status != GEM_SUCCESS) goto cleanup;
    f_src  = result[irank];
    
    status = cfit->Interpolate[tindx](cfit->tgt, gft, ielmt,
                                      cfit->mat[imat].target.st,
                                      nrank, cfit->data_tgt, result);
    if (status != GEM_SUCCESS) goto cleanup;
    f_tgt  = result[irank];
    
    *obj  += pow(f_tgt-f_src, 2);
  }
  
  /* if we do not need gradient, return now */
  if (ftgt_bar == NULL) goto cleanup;
  
  result_bar = (double *) gem_allocate(nrank*sizeof(double));
  if (result_bar == NULL) {
    status = GEM_ALLOC;
    goto cleanup;
  }
  data_bar = (double *) gem_allocate(nrank*n*sizeof(double));
  if (data_bar == NULL) {
    status = GEM_ALLOC;
    goto cleanup;
  }

  /* initialize the derivatives */
  obj_bar1  = 1.0;
  for (jrank = 0; jrank < nrank; jrank++) {
    for (idat = 0; idat < n; idat++)
      data_bar[nrank*idat+jrank] = 0.0;
    result_bar[jrank] = 0.0;
  }
  
  /* backward: minimize the difference between the source and target
   at the Match points */
  for (imat = cfit->nmat-1; imat >= 0; imat--) {
    ielms  = cfit->mat[imat].source.eIndex;
    ielmt  = cfit->mat[imat].target.eIndex;
    if ((ielms == 0) || (ielmt == 0)) continue;
    status = cfit->Interpolate[sindx](cfit->src, gfs, ielms,
                                      cfit->mat[imat].source.st,
                                      nrank, cfit->data_src, result);
    if (status != GEM_SUCCESS) goto cleanup;
    f_src  = result[irank];
    
    status = cfit->Interpolate[tindx](cfit->tgt, gft, ielmt,
                                      cfit->mat[imat].target.st,
                                      nrank, cfit->data_tgt, result);
    if (status != GEM_SUCCESS) goto cleanup;
    f_tgt  = result[irank];
    
/*  *obj += pow(f_tgt-f_src, 2);  */
    result_bar[irank] = (f_tgt - f_src) * 2 * obj_bar1;
    
    status = cfit->Interpol_bar[tindx](cfit->tgt, gft, ielmt,
                                       cfit->mat[imat].target.st,
                                       nrank, result_bar, data_bar);
    if (status != GEM_SUCCESS) goto cleanup;
  }
  
  /* backward: penalty function part of objective function
     *obj = cfit->afact * pow(area_tgt - area_src, 2);  */
  area_tgt_bar = cfit->afact * 2 * (area_tgt - area_src) * obj_bar1;
  
  result_bar[irank] = area_tgt_bar;
  
  /* backward: compute the area for tgt */
  for (ielmt = cfit->tgt->nElems-1; ielmt >= 0; ielmt--) {
    status = cfit->Integr_bar[tindx](cfit->tgt, gft, ielmt+1, nrank, result_bar,
                                     data_bar);
    if (status != GEM_SUCCESS) goto cleanup;
  }

  for (idat = 0; idat < n; idat++)
    ftgt_bar[idat] = data_bar[nrank*idat+irank];
  
cleanup:
  if (data_bar   != NULL) gem_free(data_bar);
  if (result_bar != NULL) gem_free(result_bar);
  gem_free(result);
  return(status);
}


int
gem_dataTransfer(gemDRep *drep, int bound, int ivsrc, int issrc, int vs,
                 int method, int *iset, gInterp *Interpolate,
                 bInterp *Interpol_bar, gIntegr *Integrate, bIntegr *Integr_bar,
                 invEval *iEval)
{
  int      i, j, nrank, npts, mindx, stat, eIndex, gflgs, gflgt;
  char     *name;
  double   *data, *sdata, *ftgt, st[2], fopt;
  gemQuilt *quilt;
  gemDSet  *sets;
  gemXfer  *xfer, *last;
  gemCFit  fit;
  FILE     *fp;

  /* get source information*/
  if (drep->bound[bound-1].VSet[ivsrc-1].disMethod == NULL)
    return GEM_NOTCONNECT;
  quilt = drep->bound[bound-1].VSet[ivsrc-1].quilt;
  name  = drep->bound[bound-1].VSet[ivsrc-1].sets[issrc-1].name;
  nrank = drep->bound[bound-1].VSet[ivsrc-1].sets[issrc-1].dset.rank;
  data  = drep->bound[bound-1].VSet[ivsrc-1].sets[issrc-1].dset.data;
  gflgs = 1;
  mindx = gem_metDLoaded(drep->bound[bound-1].VSet[ivsrc-1].disMethod);
  if (mindx < 0) return mindx;
  if (quilt->verts != NULL) gflgs = 0;
  if (drep->bound[bound-1].VSet[vs-1].nonconn != NULL) gflgs = 1;
  
  /* specify target information */
  gflgt = 1;
  if (drep->bound[bound-1].VSet[vs-1].quilt != NULL)
    if (drep->bound[bound-1].VSet[vs-1].quilt->verts != NULL) gflgt = 0;
  
  /* find or create transfer structure */
  xfer = drep->bound[bound-1].xferList;
  last = NULL;
  while (xfer != NULL) {
    if ((xfer->ivss == ivsrc) && (xfer->gflgs == gflgs) &&
        (xfer->ivst == vs)    && (xfer->gflgt == gflgt)) break;
    last = xfer;
    xfer = xfer->next;
  }
  /* not found -- make new transfer structure */
  if (xfer == NULL) {
    xfer = (gemXfer *) gem_allocate(sizeof(gemXfer));
    if (xfer == NULL) return GEM_ALLOC;
    xfer->ivss       = ivsrc;
    xfer->gflgs      = gflgs;
    xfer->ivst       = vs;
    xfer->gflgt      = gflgt;
    xfer->nPositions = 0;
    xfer->nMatch     = 0;
    xfer->position   = NULL;
    xfer->match      = NULL;
    xfer->next       = NULL;
    if (last == NULL) {
      drep->bound[bound-1].xferList = xfer;
    } else {
      last->next = xfer;
    }
  }
  
  /* check for correct structure in xferList */
  stat = GEM_SUCCESS;
  if (drep->bound[bound-1].VSet[vs-1].nonconn == NULL) {
    if (xfer->position == NULL) {
      stat = gem_getConnPos(drep, bound, iEval[mindx], xfer);
      if ((stat != GEM_SUCCESS) && (method == GEM_CONSERVE)) return stat;
    }
    if (method == GEM_CONSERVE) {
      if (xfer->match == NULL)
        stat = gem_makeMatch(drep, bound, Interpolate, iEval[mindx], xfer);
    }
  } else {
    if (xfer->position == NULL)
      stat = gem_getPositions(drep, bound, iEval[mindx], xfer);
  }
  if (stat != GEM_SUCCESS) return stat;

  /* create storage for the new DataSet */
  name = gem_strdup(name);
  if (name == NULL) return GEM_ALLOC;
  if (drep->bound[bound-1].VSet[vs-1].nonconn == NULL) {
    npts = drep->bound[bound-1].VSet[vs-1].quilt->nVerts;
  } else {
    npts = drep->bound[bound-1].VSet[vs-1].nonconn->npts;
  }
  sdata = (double *) gem_allocate(npts*nrank*sizeof(double));
  if (sdata == NULL) {
    gem_free(name);
    return GEM_ALLOC;
  }
  for (i = 0; i < npts*nrank; i++) sdata[i] = 0.0;
  
  /* fill it in */
  if (drep->bound[bound-1].VSet[vs-1].nonconn == NULL) {

    if (method == GEM_INTERP) {
      
      for (i = 0; i < npts; i++) {
        eIndex = xfer->position[i].eIndex;
        if (eIndex == 0) continue;
        st[0]  = xfer->position[i].st[0];
        st[1]  = xfer->position[i].st[1];
        stat   = Interpolate[mindx](quilt, gflgs, eIndex, st, nrank, data,
                                    &sdata[nrank*i]);
        if (stat != GEM_SUCCESS) {
          gem_free(sdata);
          gem_free(name);
          return stat;
        }
      }
      
    } else {
      
      stat = gem_metDLoaded(drep->bound[bound-1].VSet[vs-1].disMethod);
      if (stat < 0) {
        gem_free(sdata);
        gem_free(name);
        return mindx;
      }
      
      /* conservative schemes */
      fit.nrank        = nrank;
      fit.sindx        = mindx;
      fit.tindx        = stat;
      fit.geomFs       = gflgs;
      fit.geomFt       = gflgt;
      fit.afact        = 1.e6;
      fit.area_src     = 0.0;
      fit.area_tgt     = 0.0;
      fit.src          = quilt;
      fit.data_src     = data;
      fit.tgt          = drep->bound[bound-1].VSet[vs-1].quilt;
      fit.data_tgt     = sdata;
      fit.nmat         = xfer->nMatch;
      fit.mat          = xfer->match;
      fit.Interpolate  = Interpolate;
      fit.Interpol_bar = Interpol_bar;
      fit.Integrate    = Integrate;
      fit.Integr_bar   = Integr_bar;
      
      /* set up vectors for optimizer's dependent variables */
      ftgt = (double *) gem_allocate((npts+nrank)*sizeof(double));
      if (ftgt == NULL) {
        gem_free(sdata);
        gem_free(name);
        return stat;
      }
      
      fp = NULL;
#ifdef DEBUG
      fp = stdout;
#endif
      
      /* perform optimization (with area penalty function) */
      for (i = 0; i < nrank; i++) {
        fit.irank = i;
        /* initialize the dependent variables at the target nodes */
        for (j = 0; j < npts; j++) {
          eIndex = xfer->position[j].eIndex;
          if (eIndex == 0) continue;
          st[0]  = xfer->position[j].st[0];
          st[1]  = xfer->position[j].st[1];
          stat   = Interpolate[mindx](quilt, gflgt, eIndex, st, nrank, data,
                                      &ftgt[npts]);
          if (stat != GEM_SUCCESS) {
            gem_free(ftgt);
            gem_free(sdata);
            gem_free(name);
            return stat;
          }
          ftgt[j] = ftgt[npts+i];
        }
        stat = gem_conjGrad(obj_bar, &fit, npts, ftgt, 1e-6, fp, &fopt);
        if (stat != GEM_SUCCESS) break;
//#ifdef DEBUG
        printf("  Rank = %d:  integrated src = %le,  tgt = %le\n", i,
               fit.area_src, fit.area_tgt);
//#endif
      }
      gem_free(ftgt);
      if (stat != GEM_SUCCESS) {
        gem_free(sdata);
        gem_free(name);
        return stat;
      }
      
    }
    
  } else {
    
    /* nonconnected - interpolate only */
    
    for (i = 0; i < npts; i++) {
      eIndex = xfer->position[i].eIndex;
      if (eIndex == 0) continue;
      st[0]  = xfer->position[i].st[0];
      st[1]  = xfer->position[i].st[1];
      if (eIndex <= 0) {
        printf(" dataTransfer: eIndex = %d for point %d %d\n",
               eIndex, i+1, npts);
        continue;
      }
      stat = Interpolate[mindx](quilt, gflgs, eIndex, st, nrank, data,
                                &sdata[nrank*i]);
      if (stat != GEM_SUCCESS) {
        gem_free(sdata);
        gem_free(name);
        return stat;
      }
    }

  }
  
  *iset = drep->bound[bound-1].VSet[vs-1].nSets+1;
  sets  = (gemDSet *) gem_reallocate(drep->bound[bound-1].VSet[vs-1].sets,
                                     *iset*sizeof(gemDSet));
  if (sets == NULL) {
    gem_free(sdata);
    gem_free(name);
    return GEM_ALLOC;
  }
  sets[*iset-1].ivsrc     = ivsrc;
  sets[*iset-1].name      = name;
  sets[*iset-1].dset.npts = npts;
  sets[*iset-1].dset.rank = nrank;
  sets[*iset-1].dset.data = sdata;
  
  drep->bound[bound-1].VSet[vs-1].sets  = sets;
  drep->bound[bound-1].VSet[vs-1].nSets = *iset;

  return GEM_SUCCESS;
}
