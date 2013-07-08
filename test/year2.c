/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Year #2 Use Case Test Code (Diamond only)
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "gem.h"
#include "memory.h"
#include "egads.h"                 /* for making quad patches */


//#define DEBUG
//#define UV
#define MAXFACE 15                 /* max number of FaceIDs in Bound */



static void cleanupQuads(gemDRep *drep)
{
  int      i, stat, nIDs, nbound, nattr, type, len, *ints;
  double   *reals;
  char     **IDs, *name;
  gemTri   *quads;
  gemModel *model;
  
  stat = gem_getDRepInfo(drep, &model, &nIDs, &IDs, &nbound, &nattr);
  if (stat != GEM_SUCCESS) return;
  
  for (i = 0; i < nattr; i++) {
    stat = gem_getAttribute(drep, 0, 0, i+1, &name, &type, &len,
                            &ints, &reals, (char **) &quads);
    if (stat != GEM_SUCCESS) continue;
    if (type != GEM_POINTER) continue;
    if (name[0] != 'Q') continue;
    if (name[1] != 'u') continue;
    if (name[2] != 'a') continue;
    if (name[3] != 'd') continue;
    if (name[4] != ':') continue;
    gem_free(quads->xyzs);
    gem_free(quads->tris);
    gem_free(quads->uvs);
    gem_free(quads->vid);
    gem_free(quads);
  }
}


static void quadTess(gemDRep *drep, int bound, int nFace, double *params)
{
  int          stat, i, j, k, n, brep, face, nvs, len, npat, nquad, n1, n2;
  int          *ivec;
  double       uvbox[4], qparam[3] = {0.0, 0.0, 0.0};
  char         quadStr[24];
  const int    *ptype, *pindex, *ipts, *ibnds;
  const double *xyz, *uv;
  gemTri       *quads;
  gemPair      *pairs;
  gemModel     *model;
  ego          body, tess;
  
  stat  = gem_getBoundInfo(drep, bound, &n, &ivec, &pairs, uvbox, &nvs);
  if (stat != GEM_SUCCESS) return;
  model = drep->model;
  
  params[2] = -params[2];
  for (i = 0; i < nFace; i++) {
    brep = pairs[i].BRep;
    face = pairs[i].index;
    body = (ego) model->BReps[brep-1]->body->handle.ident.ptr;
    stat = EG_makeTessBody(body, params, &tess);
    if (stat != EGADS_SUCCESS) continue;

#ifdef DEBUG
    printf(" Quadding BRep %d  Face %d!\n", brep, face);
#endif
    stat = EG_makeQuads(tess, qparam, face);
    if (stat == EGADS_SUCCESS) {
      stat = EG_getQuads(tess, face, &len, &xyz, &uv, &ptype, &pindex, &npat);
      if (stat == EGADS_SUCCESS) {
        for (nquad = j = 0; j < npat; j++) {
          stat = EG_getPatch(tess, face, j+1, &n1, &n2, &ipts, &ibnds);
          if (stat != EGADS_SUCCESS) continue;
          nquad += (n1-1)*(n2-1);
        }
        quads = (gemTri *) gem_allocate(sizeof(gemTri));
        if (quads != NULL) {
          quads->ntris = nquad;
          quads->npts  = len;
          quads->tric  = NULL;
          
          quads->xyzs = (double *) gem_allocate(3*len*sizeof(double));
          quads->uvs  = (double *) gem_allocate(2*len*sizeof(double));
          quads->vid  = (int *)    gem_allocate(2*len*sizeof(int));
          quads->tris = (int *)    gem_allocate(4*nquad*sizeof(int));
          if ((quads->xyzs == NULL) || (quads->uvs == NULL) ||
              (quads->tris == NULL) || (quads->vid == NULL)) {
            if (quads->xyzs != NULL) gem_free(quads->xyzs);
            if (quads->tris != NULL) gem_free(quads->tris);
            if (quads->uvs  != NULL) gem_free(quads->uvs);
            if (quads->vid  != NULL) gem_free(quads->vid);
            gem_free(quads);
          } else {
            for (j = 0; j < 3*len; j++) quads->xyzs[j] = xyz[j];
            for (j = 0; j < 2*len; j++) quads->uvs[j]  = uv[j];
            for (j = 0; j <   len; j++) {
              quads->vid[2*j  ] = ptype[j];
              quads->vid[2*j+1] = pindex[j];
            }
            for (nquad = j = 0; j < npat; j++) {
              stat = EG_getPatch(tess, face, j+1, &n1, &n2, &ipts, &ibnds);
              if (stat != EGADS_SUCCESS) continue;
              for (n = 0; n < n2-1; n++)
                for (k = 0; k < n1-1; k++) {
                  quads->tris[4*nquad  ] = ipts[   n *n1+k  ];
                  quads->tris[4*nquad+1] = ipts[   n *n1+k+1];
                  quads->tris[4*nquad+2] = ipts[(n+1)*n1+k+1];
                  quads->tris[4*nquad+3] = ipts[(n+1)*n1+k  ];
                  nquad++;
                }
            }
            snprintf(quadStr, 24, "Quad:%d:%d", brep, face);
            stat = gem_setAttribute(drep, 0, 0, quadStr,  GEM_POINTER, 1, NULL,
                                    NULL, (char *) quads);
            if (stat != GEM_SUCCESS)
              printf(" quadTess Warning: gem_setAttribute for %s = %d!\n",
                     quadStr, stat);
          }
        }
      }
    }
    EG_deleteObject(tess);
  }
  params[2] = -params[2];
}


#ifdef DEBUG
static int printBoundInfo(gemDRep *DRep, int bound)
{
  int     status, i, j, n, ns, nvs, type, nGeom, nData, *ivec, *irank;
  char    **IDs;
  double  uvbox[4];
  gemPair *pairs;

  status = gem_getBoundInfo(DRep, bound, &n, &ivec, &pairs, uvbox, &nvs);
  printf(" gem_getBoundInfo = %d\n", status);
  if (status != GEM_SUCCESS) return status;

  for (i = 0; i < n; i++)
    printf("        %3d:  BRep = %2d  Face = %3d   iID = %d\n",
           i+1, pairs[i].BRep, pairs[i].index, ivec[i]);
  printf("     uvbox  = %lf %lf  %lf %lf\n",
         uvbox[0], uvbox[1], uvbox[2], uvbox[3]);
  printf("     nVSets = %d\n", nvs);
  printf("\n");
  for (j = 1; j <= nvs; j++) {
    status = gem_getVsetInfo(DRep, bound, j, &type, &nGeom, &nData,
                             &ns, &IDs, &ivec, &irank);
    printf(" %d: gem_getVsetInfo = %d\n", j, status);
    if (status != GEM_SUCCESS) return status;
    printf("        type   = %d\n", type);
    printf("        nGeom  = %d\n", nGeom);
    printf("        nData  = %d\n", nData);
    printf("        nSets  = %d\n", ns);
    for (i = 0; i < ns; i++)
      printf("           %4d:  DSname = %s,  rank = %d,   ivsrc = %d\n",
             i+1, IDs[i], irank[i], ivec[i]);
    printf("\n");
    if (IDs   != NULL) gem_free(IDs);
    if (ivec  != NULL) gem_free(ivec);
    if (irank != NULL) gem_free(irank);
  }
  
  return GEM_SUCCESS;
}
#endif


int main(int argc, char *argv[])
{
  int      status, i, j, n, uptodate, nBRep, nParams, nBranch, nattr, bound;
  int      type, nnode, nedge, nloop, nface, nshell, sense, nloops, nb, inx[3];
  int      npts, rank, in[6], vs[3], *ivec;
  double   box[6], uvbox[4], tessSize[3], size, *xyzs, *pts, *cmp;
  char     *filename, *server, *modeler, *ID, **IDs, *dset, *faceIDs[MAXFACE];
  char     *types[4] = {"triConstantDiscontinuous", "triLinearDiscontinuous",
                        "triLinearContinuous", "quadLinearContinuous"};
  gemCntxt *context;
  gemModel *models[4], *newModel, *tModel;
  gemBRep  **BReps;
  gemDRep  *DRep;

  if (argc != 4) {
    printf(" usage: year2 vs1 vs2 vs3 (0-3)!\n\n");
    printf("        0 - ConstantDiscontinuous\n");
    printf("        1 - LinearDiscontinuous\n");
    printf("        2 - LinearContinuous\n");
    printf("        3 - LinearContinuous for Quads\n\n");
    return 1;
  }
  for (i = 1; i <= 3; i++) {
    inx[i-1] = atoi(argv[i]);
    if ((inx[i-1] < 0) || (inx[i-1] > 3)) {
      printf(" year2 error: type for vs%d = %d [0-3]!\n", i, inx[i-1]);
      return 1;
    }
  }
  
  status = gem_initialize(&context);
  if (status != GEM_SUCCESS) {
    printf(" gem_initialize = %d\n", status);
    return 1;
  }
  status = gem_loadModel(context, NULL, "GEM_year2_base.egads", &models[0]);
  if (status != GEM_SUCCESS) {
    printf(" gem_loadModel GEM_year2_base.egads = %d\n", status);
    status = gem_terminate(context);
    return 1;
  }
  status = gem_loadModel(context, NULL, "GEM_year2_ibd.egads",  &models[1]);
  if (status != GEM_SUCCESS) {
    printf(" gem_loadModel GEM_year2_ibd.egads  = %d\n", status);
    status = gem_releaseModel(models[0]);
    status = gem_terminate(context);
    return 1;
  }
  status = gem_loadModel(context, NULL, "GEM_year2_obd.egads",  &models[2]);
  if (status != GEM_SUCCESS) {
    printf(" gem_loadModel GEM_year2_obd.egads  = %d\n", status);
    status = gem_releaseModel(models[1]);
    status = gem_releaseModel(models[0]);
    status = gem_terminate(context);
    return 1;
  }
  status = gem_loadModel(context, NULL, "GEM_year2_cut.egads",  &models[3]);
  if (status != GEM_SUCCESS) {
    printf(" gem_loadModel GEM_year2_cut.egads  = %d\n", status);
    status = gem_releaseModel(models[2]);
    status = gem_releaseModel(models[1]);
    status = gem_releaseModel(models[0]);
    status = gem_terminate(context);
    return 1;
  }
  
  status = gem_staticModel(context, &newModel);
  if (status != GEM_SUCCESS) {
    printf(" gem_staticModel = %d\n", status);
    for (i = 0; i < 4; i++) gem_releaseModel(models[i]);
    status = gem_terminate(context);
    return 1;
  }
  
  for (j = 0; j < 4; j++) {
    status  = gem_getModel(models[j], &server, &filename, &modeler, &uptodate,
                           &nBRep, &BReps, &nParams, &nBranch, &nattr);
    if (status != GEM_SUCCESS) {
      printf(" gem_getModel = %d\n", status);
      for (i = 0; i < 4; i++) gem_releaseModel(models[i]);
      status = gem_terminate(context);
      return 1;
    }
#ifdef DEBUG
    printf("     FileName = %s\n", filename);
    printf("     Modeler  = %s\n", modeler);
    printf("     UpToDate = %d\n", uptodate);
    printf("     nBReps   = %d\n", nBRep);
#endif
    status = gem_add2Model(newModel, BReps[0], NULL);
    if (status != GEM_SUCCESS) {
      printf(" gem_add2Model = %d\n", status);
      status = gem_releaseModel(newModel);
      for (i = 0; i < 4; i++) gem_releaseModel(models[i]);
      status = gem_terminate(context);
      return 1;
    }
#ifdef DEBUG
    printf("\n");
#endif
  }

  status  = gem_getModel(newModel, &server, &filename, &modeler, &uptodate,
                         &nBRep, &BReps, &nParams, &nBranch, &nattr);
  if (status != GEM_SUCCESS) {
    printf(" gem_getModel = %d\n", status);
    status = gem_releaseModel(newModel);
    for (i = 0; i < 4; i++) gem_releaseModel(models[i]);
    status = gem_terminate(context);
    return 1;
  }
#ifdef DEBUG
  printf("     FileName = %s\n", filename);
  printf("     Modeler  = %s\n", modeler);
  printf("     UpToDate = %d\n", uptodate);
  printf("     nBReps   = %d\n", nBRep);
  printf("\n");
#endif
  for (n = i = 0; i < nBRep; i++) {
    status = gem_getBRepInfo(BReps[i], box, &type, &nnode, &nedge, &nloop,
                             &nface, &nshell, &nattr);
    if (status != GEM_SUCCESS) {
      printf(" gem_getBRepInfo %d = %d\n", i, status);
      status = gem_releaseModel(newModel);
      for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
      status = gem_terminate(context);
      return 1;
    }
#ifdef DEBUG
    printf("     type   = %d\n", type);
    printf("     nnode  = %d\n", nnode);
    printf("     nedge  = %d\n", nedge);
    printf("     nface  = %d\n", nface);
    printf("     nshell = %d\n", nshell);
    printf("     nattr  = %d\n", nattr);
    printf("\n");
#endif
    size = sqrt((box[0]-box[3])*(box[0]-box[3]) +
                (box[1]-box[4])*(box[1]-box[4]) +
                (box[2]-box[5])*(box[2]-box[5]));
#ifdef DEBUG
    for (j = 1; j <= nface; j++) {
      status = gem_getFace(BReps[i], j, &ID, uvbox, &sense, &nloops, &ivec,
                           &nattr);
      if (status == GEM_SUCCESS) {
        printf("       Face %3d: ID = %s   %d %d %d\n",
               j, ID, sense, nloops, nattr);
      } else {
        printf("       Face %3d: status = %d!\n", j, status);
      }
    }
    printf("\n");
#endif
    if (i == 3) {
      gem_getFace(BReps[i],  2, &ID, uvbox, &sense, &nloops, &ivec,
                  &nattr);
      faceIDs[n] = ID;
      n++;
      gem_getFace(BReps[i],  5, &ID, uvbox, &sense, &nloops, &ivec,
                  &nattr);
      faceIDs[n] = ID;
      n++;
      gem_getFace(BReps[i],  6, &ID, uvbox, &sense, &nloops, &ivec,
                  &nattr);
      faceIDs[n] = ID;
      n++;
      gem_getFace(BReps[i],  7, &ID, uvbox, &sense, &nloops, &ivec,
                  &nattr);
      faceIDs[n] = ID;
      n++;
      gem_getFace(BReps[i], 10, &ID, uvbox, &sense, &nloops, &ivec,
                  &nattr);
      faceIDs[n] = ID;
      n++;
    } else {
      gem_getFace(BReps[i],  1, &ID, uvbox, &sense, &nloops, &ivec,
                  &nattr);
      faceIDs[n] = ID;
      n++;
    }
  }
  
  /* make the DRep and tessellate for our discretization method */
  status = gem_newDRep(newModel, &DRep);
  if (status != GEM_SUCCESS) {
    printf(" gem_newDRep = %d\n", status);
    status = gem_releaseModel(newModel);
    for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
    status = gem_terminate(context);
    return 1;
  }
  tessSize[2] = 15.0;
  tessSize[0] =  0.050*size;
  tessSize[1] =  0.001*size;
  status      = gem_tesselDRep(DRep, 0, tessSize[2], tessSize[0], tessSize[1]);
  if (status != GEM_SUCCESS) {
    printf(" gem_tesselDRep = %d\n", status);
    status = gem_releaseModel(newModel);
    for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
    status = gem_terminate(context);
    return 1;
  }
  
  /* make the bound */
  status = gem_createBound(DRep, n, faceIDs, &bound);
  if (status != GEM_SUCCESS) {
    printf(" gem_createBound = %d  bound = %d\n", status, bound);
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
    status = gem_terminate(context);
    return 1;
  }
  quadTess(DRep, bound, 3, tessSize);
#ifdef DEBUG
  status = printBoundInfo(DRep, bound);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
    status = gem_terminate(context);
    return 1;
  }
#endif

  status = gem_getDRepInfo(DRep, &tModel, &n, &IDs, &nb, &nattr);
  if (status != GEM_SUCCESS) {
    printf(" gem_getDRepInfo = %d\n", status);
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
    status = gem_terminate(context);
    return 1;
  }
#ifdef DEBUG
  for (i = 0; i < n; i++)
    printf("        %4d:  ID = %s\n", i+1, IDs[i]);
  printf("     nBounds = %d\n", nb);
  printf("\n");
#endif

  /* specify which VSets get which BReps */
  in[0]  = 1;
  in[1]  = 0;
  in[2]  = 2;
  in[3]  = 3;
  in[4]  = 4;
  in[5]  = 0;
  status = gem_setAttribute(DRep, 0, 0, "BRep",     GEM_INTEGER, 6, in,
                            NULL, NULL);
  if (status != GEM_SUCCESS)
    printf(" gem_setAttribute = %d  BRep\n", status);
  /* set the first to be the source of the parameterization 
  status = gem_setAttribute(DRep, 0, 0, "reParam",  GEM_INTEGER, 1, in,
                            NULL, NULL);
  if (status != GEM_SUCCESS)
    printf(" gem_setAttribute = %d  reParam\n", status);  */
  
  /* create the Vertex Sets */
  for (i = 0; i < 3; i++) {
    status = gem_createVset(DRep, bound, types[inx[i]], &vs[i]);
    printf(" gem_createVset %d = %d  %s  VertexSet = %d\n",
           i+1, status, types[inx[i]], vs[i]);
    if (status != GEM_SUCCESS) {
      status = gem_destroyDRep(DRep);
      status = gem_releaseModel(newModel);
      for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
      status = gem_terminate(context);
      return 1;
    }
  }

  status = gem_paramBound(DRep, bound);
  if (status != GEM_SUCCESS) {
    printf(" gem_paramBound = %d\n", status);
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
    status = gem_terminate(context);
    return 1;
  }
#ifdef DEBUG
  status = printBoundInfo(DRep, bound);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
    status = gem_terminate(context);
    return 1;
  }
#endif
  
  /* set some data on our connected vertex sets */
#ifdef UV
  dset = "uv";
  if (inx[0] < 2) dset = "uvd";
#else
  dset = "xyz";
  if (inx[0] < 2) dset = "xyzd";
#endif
  status = gem_getData(DRep, bound, vs[0], dset, GEM_INTERP,
                       &npts, &rank, &xyzs);
  printf(" gem_getData = %d    npts = %d  rank = %d\n", status, npts, rank);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
    status = gem_terminate(context);
    return 1;
  }
  status = gem_putData(DRep, bound, vs[0], "data", npts, rank, xyzs);
  if (status != GEM_SUCCESS) {
    printf(" gem_putData = %d\n", status);
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
    status = gem_terminate(context);
    return 1;
  }
  status = gem_putData(DRep, bound, vs[0], "cons", npts, rank, xyzs);
  if (status != GEM_SUCCESS) {
    printf(" gem_putData = %d\n", status);
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
    status = gem_terminate(context);
    return 1;
  }
#ifdef DEBUG
  status = printBoundInfo(DRep, bound);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
    status = gem_terminate(context);
    return 1;
  }
#endif
  
  /* interpolate to our connected set */
  for (i = 1; i < 3; i++) {
    printf("\n VertexSet %d:\n", i+1);
#ifdef UV
    dset = "uv";
    if (inx[i] < 2) dset = "uvd";
#else
    dset = "xyz";
    if (inx[i] < 2) dset = "xyzd";
#endif
    status = gem_getData(DRep, bound, vs[i], dset, GEM_INTERP,
                         &npts, &rank, &xyzs);
    printf(" gem_getData = %d    npts = %d  rank = %d\n", status, npts, rank);
    if (status == GEM_SUCCESS) {
      cmp    = xyzs;
      status = gem_getData(DRep, bound, vs[i], "data", GEM_INTERP,
                           &npts, &rank, &pts);
      printf(" gem_getData = %d    npts = %d  rank = %d\n", status, npts, rank);
      if (status != GEM_SUCCESS) {
        status = gem_destroyDRep(DRep);
        status = gem_releaseModel(newModel);
        for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
        status = gem_terminate(context);
        return 1;
      }
      uvbox[0] = uvbox[1] = uvbox[2] = uvbox[3] = 0.0;
#ifdef UV
      for (j = 0; j < npts; j++) {
//      printf(" %d:  %lf %lf   %lf %lf\n", j+1, cmp[2*j  ], cmp[2*j+1],
//             pts[2*j  ], pts[2*j+1]);
        uvbox[0] +=  fabs(cmp[2*j  ] - pts[2*j  ]);
        uvbox[1] +=  fabs(cmp[2*j+1] - pts[2*j+1]);
        uvbox[3] += sqrt((cmp[2*j  ] - pts[2*j  ])*(cmp[2*j  ] - pts[2*j  ]) +
                         (cmp[2*j+1] - pts[2*j+1])*(cmp[2*j+1] - pts[2*j+1]));
      }
      printf("   dev = %le %le    mag = %le\n",
             uvbox[0]/npts, uvbox[1]/npts, uvbox[3]/npts);
#else
      for (j = 0; j < npts; j++) {
//      printf(" %d:  %lf %lf %lf   %lf %lf %lf\n", j+1, cmp[3*j  ], cmp[3*j+1],
//             cmp[3*j+2], pts[3*j  ], pts[3*j+1], pts[3*j+2]);
        uvbox[0] +=  fabs(cmp[3*j  ] - pts[3*j  ]);
        uvbox[1] +=  fabs(cmp[3*j+1] - pts[3*j+1]);
        uvbox[2] +=  fabs(cmp[3*j+2] - pts[3*j+2]);
        uvbox[3] += sqrt((cmp[3*j  ] - pts[3*j  ])*(cmp[3*j  ] - pts[3*j  ]) +
                         (cmp[3*j+1] - pts[3*j+1])*(cmp[3*j+1] - pts[3*j+1]) +
                         (cmp[3*j+2] - pts[3*j+2])*(cmp[3*j+2] - pts[3*j+2]));
      }
      printf("   dev = %le %le %le    mag = %le\n",
             uvbox[0]/npts, uvbox[1]/npts, uvbox[2]/npts, uvbox[3]/npts);
#endif
      
      status = gem_getData(DRep, bound, vs[i], "cons", GEM_CONSERVE,
                           &npts, &rank, &pts);
      printf(" gem_getData = %d    npts = %d  rank = %d\n", status, npts, rank);
      if (status != GEM_SUCCESS) {
        status = gem_destroyDRep(DRep);
        status = gem_releaseModel(newModel);
        for (j = 0; j < 4; j++) gem_releaseModel(models[j]);
        status = gem_terminate(context);
        return 1;
      }
      uvbox[0] = uvbox[1] = uvbox[2] = uvbox[3] = 0.0;
#ifdef UV
      for (j = 0; j < npts; j++) {
//      printf(" %d:  %lf %lf   %lf %lf\n", j+1, cmp[2*j  ], cmp[2*j+1],
//             pts[2*j  ], pts[2*j+1]);
        uvbox[0] +=  fabs(cmp[2*j  ] - pts[2*j  ]);
        uvbox[1] +=  fabs(cmp[2*j+1] - pts[2*j+1]);
        uvbox[3] += sqrt((cmp[2*j  ] - pts[2*j  ])*(cmp[2*j  ] - pts[2*j  ]) +
                         (cmp[2*j+1] - pts[2*j+1])*(cmp[2*j+1] - pts[2*j+1]));
      }
      printf("   dev = %le %le    mag = %le\n",
             uvbox[0]/npts, uvbox[1]/npts, uvbox[3]/npts);
#else
      for (j = 0; j < npts; j++) {
//      printf(" %d:  %lf %lf %lf   %lf %lf %lf\n", j+1, cmp[3*j  ], cmp[3*j+1],
//             cmp[3*j+2], pts[3*j  ], pts[3*j+1], pts[3*j+2]);
        uvbox[0] +=  fabs(cmp[3*j  ] - pts[3*j  ]);
        uvbox[1] +=  fabs(cmp[3*j+1] - pts[3*j+1]);
        uvbox[2] +=  fabs(cmp[3*j+2] - pts[3*j+2]);
        uvbox[3] += sqrt((cmp[3*j  ] - pts[3*j  ])*(cmp[3*j  ] - pts[3*j  ]) +
                         (cmp[3*j+1] - pts[3*j+1])*(cmp[3*j+1] - pts[3*j+1]) +
                         (cmp[3*j+2] - pts[3*j+2])*(cmp[3*j+2] - pts[3*j+2]));
      }
      printf("   dev = %le %le %le    mag = %le\n",
             uvbox[0]/npts, uvbox[1]/npts, uvbox[2]/npts, uvbox[3]/npts);
#endif
      printf("\n");
    }
  }
  
  cleanupQuads(DRep);

  status = gem_destroyDRep(DRep);
  if (status != GEM_SUCCESS)
    printf(" gem_destroyDRep    = %d\n", status);
  status = gem_releaseModel(newModel);
  if (status != GEM_SUCCESS)
    printf(" gem_releaseModel   = %d\n", status);
  for (i = 0; i < 4; i++) {
    status = gem_releaseModel(models[i]);
    if (status != GEM_SUCCESS)
      printf(" gem_releaseModel %d = %d\n", i, status);
  }
  status = gem_terminate(context);
  if (status != GEM_SUCCESS)
    printf(" gem_terminate = %d\n", status);

  return 0;
}
