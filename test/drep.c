/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             DRep Manager Test Code
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


/* select one for conservative transfer */
#define LINEARCON
//#define LINEARDISCON
//#define CONSTANTDISCON


#ifdef WIN32
#define unlink    _unlink
#endif

#define MAXFACE 15                 /* max number of FaceIDs in Bound */


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


int main(int argc, char *argv[])
{
  int      status, i, j, n, uptodate, nBRep, nParams, nBranch, nattr, bound;
  int      type, nnode, nedge, nloop, nface, nshell, sense, nloops, nb, vs, vx;
  int      ntris, npts, rank, i0, i1, i2, ucvs, in[4], *ivec, *tris;
  double   xform[12], box[6], uvbox[4], size, *xyzs, *pts, *cmp;
  char     *server, *filename, *modeler, *ID, **IDs, *faceIDs[MAXFACE];
  char     command[132], *dset;
  gemCntxt *context;
  gemPair  bface;
  gemModel *model, *auxmdl, *newModel, *tModel;
  gemBRep  **BReps, **auxBReps;
  gemDRep  *DRep;

  if ((argc != 2) && (argc != 3)) {
    printf(" usage: [d/q]drep filename [modeler]!\n");
    return 1;
  }
  for (i = 1; i < 12; i++) xform[i] = 0.0;
  xform[0] = xform[5] = xform[10] = 1.0;
  xform[3] = xform[7] = xform[11] = 1.0;     /* offset */

  server = getenv("GEMserver");
  status = gem_initialize(&context);
  printf(" gem_initialize = %d\n", status);
  if (argc == 2) {
    status = gem_setAttribute(context, 0, 0, "Modeler", GEM_STRING, 7, NULL,
                              NULL, "Parasolid");
  } else {
    status = gem_setAttribute(context, 0, 0, "Modeler", GEM_STRING, 7, NULL, 
                              NULL, argv[2]);
  }
  printf(" gem_setAttribute = %d\n", status);
  status = gem_loadModel(context, server, argv[1], &model);
  printf(" gem_loadModel = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_terminate(context);
    return 1;
  }
#ifdef WIN32
  sprintf(command, "copy %s tmp%s", argv[1], argv[1]);
#else
  sprintf(command, "/bin/cp %s tmp%s", argv[1], argv[1]);
#endif
  system(command);
  sprintf(command, "tmp%s", argv[1]);
  status = gem_loadModel(context, server, command, &auxmdl);
  printf(" gem_getModel = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  printf(" gem_loadModel = %d\n", status);
  status  = gem_getModel(auxmdl, &server, &filename, &modeler, &uptodate,
                         &nBRep, &auxBReps, &nParams, &nBranch, &nattr);
  if (status != GEM_SUCCESS) {
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  unlink(command);
  printf(" \n");
  
  status  = gem_getModel(model, &server, &filename, &modeler, &uptodate,
                         &nBRep, &BReps, &nParams, &nBranch, &nattr);
  printf(" gem_getModel = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  printf("     FileName = %s\n", filename);
  printf("     Modeler  = %s\n", modeler);
  printf("     UpToDate = %d\n", uptodate);
  printf("     nBReps   = %d\n", nBRep);
  printf("\n");

  status = gem_staticModel(context, &newModel);
  printf(" gem_staticModel = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  status = gem_add2Model(newModel, BReps[0], NULL);
  printf(" gem_add2Model = %d\n", status);
  status = gem_add2Model(newModel, auxBReps[0], NULL);
  printf(" gem_add2Model = %d\n\n", status);
/*
  status = gem_add2Model(newModel, BReps[0], xform);
  printf(" gem_add2Model = %d\n\n", status);
*/
  status  = gem_getModel(newModel, &server, &filename, &modeler, &uptodate,
                         &nBRep, &BReps, &nParams, &nBranch, &nattr);
  printf(" gem_getModel = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  printf("     FileName = %s\n", filename);
  printf("     Modeler  = %s\n", modeler);
  printf("     UpToDate = %d\n", uptodate);
  printf("     nBReps   = %d\n", nBRep);
  printf("\n");
  for (i = 0; i < nBRep; i++) {
    status = gem_getBRepInfo(BReps[i], box, &type, &nnode, &nedge, &nloop,
                             &nface, &nshell, &nattr);
    printf(" gem_getBRepInfo %d = %d\n", i, status);
    if (status != GEM_SUCCESS) {
      status = gem_releaseModel(newModel);
      status = gem_releaseModel(auxmdl);
      status = gem_releaseModel(model);
      status = gem_terminate(context);
      return 1;
    }
    printf("     type   = %d\n", type);
    printf("     nnode  = %d\n", nnode);
    printf("     nedge  = %d\n", nedge);
    printf("     nface  = %d\n", nface);
    printf("     nshell = %d\n", nshell);
    printf("     nattr  = %d\n", nattr);
    printf("\n");
    size = sqrt((box[0]-box[3])*(box[0]-box[3]) +
                (box[1]-box[4])*(box[1]-box[4]) +
                (box[2]-box[5])*(box[2]-box[5]));

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
  }

  n = 0;
  bface.BRep  = 0;
  bface.index = 0;
  for (;;) {
    printf(" Enter Face # (done = 0): ");
    scanf("%d", &i);
    if ((i > 0) && (i <= nface)) {
      status = gem_getFace(BReps[0], i, &ID, uvbox, &sense, &nloops, &ivec,
                           &nattr);
      if (status == GEM_SUCCESS) {
        if (n == MAXFACE) {
          printf(" Error: Too many Faces!\n");
          break;
        }
        faceIDs[n] = ID;
        if (bface.BRep == 0) {
          bface.BRep  = 1;
          bface.index = i;
        }
        n++;
      } else {
        printf("       Face %3d: status = %d!\n", i, status);
        break;
      }
      status = gem_getFace(auxBReps[0], i, &ID, uvbox, &sense, &nloops, &ivec,
                           &nattr);
      if (status == GEM_SUCCESS) {
        if (n == MAXFACE) {
          printf(" Error: Too many Faces!\n");
          break;
        }
        faceIDs[n] = ID;
        n++;
      } else {
        printf("       Face %3d: status = %d!\n", i, status);
        break;
      }
    } else {
      break;
    }
  } 
  printf("\n");
  if (n == 0) {
    printf(" Error: No Faces for Bound!\n");
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }

  /* make the DRep and tessellate for our discretization method */
  status = gem_newDRep(newModel, &DRep);
  printf(" gem_newDRep = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  status = gem_tesselDRep(DRep, 1, 15.0, 0.025*size, 0.001*size);
  printf(" gem_tesselDRep = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  status = gem_tesselDRep(DRep, 2, 12.0, 0.033333333*size, 0.001*size);
  printf(" gem_tesselDRep = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  
  /* make the bound */
  status = gem_createBound(DRep, n, faceIDs, &bound);
  printf(" gem_createBound = %d  bound = %d\n", status, bound);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  status = printBoundInfo(DRep, bound);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }

  status = gem_getDRepInfo(DRep, &tModel, &n, &IDs, &nb, &nattr);
  printf(" gem_getDRepInfo = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  for (i = 0; i < n; i++)
    printf("        %4d:  ID = %s\n", i+1, IDs[i]);
  printf("     nBounds = %d\n", nb);
  printf("\n");

  /* create a vertex set */
  in[0]  = 1;
  in[1]  = 0;
  in[2]  = 2;
  in[3]  = 0;
  status = gem_setAttribute(DRep, 0, 0, "BRep", GEM_INTEGER, 4, in, NULL, NULL);
  printf(" gem_setAttribute = %d\n", status);
  status = gem_createVset(DRep, bound, "triLinearContinuous", &vs);
  printf(" gem_createVset = %d   VertexSet = %d\n", status, vs);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  /* create another vertex set */

#ifdef LINEARCON
  status = gem_createVset(DRep, bound, "triLinearContinuous",      &vx);
  dset   = "xyz";
#endif
#ifdef LINEARDISCON
  status = gem_createVset(DRep, bound, "triLinearDiscontinuous",   &vx);
  dset   = "xyzd";
#endif
#ifdef CONSTANTDISCON
  status = gem_createVset(DRep, bound, "triConstantDiscontinuous", &vx);
  dset   = "xyzd";
#endif
  printf(" gem_createVset = %d   VertexSet = %d\n", status, vx);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }

  /* use hidden flag -- do not reparameterize
  status = gem_paramBound(DRep, -bound);  */
  status = gem_paramBound(DRep,  bound);
  printf(" gem_paramBound = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  status = printBoundInfo(DRep, bound);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  
  /* make an unconnected vertex set from a triangulation */
  status = gem_getTessel(DRep, bface, &ntris, &npts, &tris, &pts);
  printf(" gem_getTessel = %d   ntris = %d   npts = %d\n", status, ntris, npts);
  if ((status != GEM_SUCCESS) || (ntris == 0)) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  xyzs = (double *) malloc(3*ntris*sizeof(double));
  if (xyzs == NULL) {
    printf(" Malloc Error on %d Points!\n", ntris);
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  for (i = 0; i < ntris; i++) {
    i0 = tris[3*i  ] - 1;
    i1 = tris[3*i+1] - 1;
    i2 = tris[3*i+2] - 1;
    xyzs[3*i  ] = (pts[3*i0  ] + pts[3*i1  ] + pts[3*i2  ])/3.0;
    xyzs[3*i+1] = (pts[3*i0+1] + pts[3*i1+1] + pts[3*i2+1])/3.0;
    xyzs[3*i+2] = (pts[3*i0+2] + pts[3*i1+2] + pts[3*i2+2])/3.0;
  }
  status = gem_makeVset(DRep, bound, ntris, xyzs, &ucvs);
  free(xyzs);
  printf(" gem_makeVset = %d   VertexSet = %d\n\n", status, ucvs);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  status = printBoundInfo(DRep, bound);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }  
  
  /* set some data on our connected vertex sets */
  status = gem_getData(DRep, bound, vs, "xyz", GEM_INTERP, &npts, &rank, &xyzs);
  printf(" gem_getData = %d    npts = %d  rank = %d\n", status, npts, rank);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  status = gem_putData(DRep, bound, vs, "data", npts, rank, xyzs);
  printf(" gem_putData = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  status = gem_getData(DRep, bound, vx, "xyz", GEM_INTERP, &npts, &rank, &xyzs);
  printf(" gem_getData = %d    npts = %d  rank = %d\n", status, npts, rank);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  status = gem_putData(DRep, bound, vx, "data3", npts, rank, xyzs);
  printf(" gem_putData = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  status = printBoundInfo(DRep, bound);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  
  /* interpolate to our unconnected set */
  status = gem_getData(DRep, bound, ucvs, "data", GEM_INTERP,
                       &npts, &rank, &pts);
  printf(" gem_getData = %d    npts = %d  rank = %d\n", status, npts, rank);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }
  status = printBoundInfo(DRep, bound);
  if (status != GEM_SUCCESS) {
    status = gem_destroyDRep(DRep);
    status = gem_releaseModel(newModel);
    status = gem_releaseModel(auxmdl);
    status = gem_releaseModel(model);
    status = gem_terminate(context);
    return 1;
  }

  status = gem_getData(DRep, bound, ucvs, "xyz", GEM_INTERP,
                       &npts, &rank, &xyzs);
  printf(" gem_getData = %d    npts = %d  rank = %d\n", status, npts, rank);
  cmp      = xyzs;
  uvbox[0] = uvbox[1] = uvbox[2] = uvbox[3] = 0.0;
  for (i = 0; i < npts; i++) {
/*  printf(" %d:  %lf %lf %lf   %lf %lf %lf\n", i+1, cmp[3*i  ], cmp[3*i+1],
           cmp[3*i+2], pts[3*i  ], pts[3*i+1], pts[3*i+2]); */
    uvbox[0] +=  fabs(cmp[3*i  ] - pts[3*i  ]);
    uvbox[1] +=  fabs(cmp[3*i+1] - pts[3*i+1]);
    uvbox[2] +=  fabs(cmp[3*i+2] - pts[3*i+2]);
    uvbox[3] += sqrt((cmp[3*i  ] - pts[3*i  ])*(cmp[3*i  ] - pts[3*i  ]) +
                     (cmp[3*i+1] - pts[3*i+1])*(cmp[3*i+1] - pts[3*i+1]) +
                     (cmp[3*i+2] - pts[3*i+2])*(cmp[3*i+2] - pts[3*i+2]));
  }
  printf("   dev = %le %le %le    mag = %le\n",
         uvbox[0]/npts, uvbox[1]/npts, uvbox[2]/npts, uvbox[3]/npts);
  printf("\n");

  /* interpolate to our connected set */
  status = gem_getData(DRep, bound, vx, dset, GEM_INTERP, &npts, &rank, &xyzs);
  printf(" gem_getData = %d    npts = %d  rank = %d\n", status, npts, rank);
  if (status == GEM_SUCCESS) {
    cmp    = xyzs;
    status = gem_getData(DRep, bound, vx, "data", GEM_CONSERVE,
                         &npts, &rank, &pts);
    printf(" gem_getData = %d    npts = %d  rank = %d\n", status, npts, rank);
    if (status != GEM_SUCCESS) {
      status = gem_destroyDRep(DRep);
      status = gem_releaseModel(newModel);
      status = gem_releaseModel(auxmdl);
      status = gem_releaseModel(model);
      status = gem_terminate(context);
      return 1;
    }
    uvbox[0] = uvbox[1] = uvbox[2] = uvbox[3] = 0.0;
    for (i = 0; i < npts; i++) {
/*    printf(" %d:  %lf %lf %lf   %lf %lf %lf\n", i+1, cmp[3*i  ], cmp[3*i+1],
             cmp[3*i+2], pts[3*i  ], pts[3*i+1], pts[3*i+2]);  */
      uvbox[0] +=  fabs(cmp[3*i  ] - pts[3*i  ]);
      uvbox[1] +=  fabs(cmp[3*i+1] - pts[3*i+1]);
      uvbox[2] +=  fabs(cmp[3*i+2] - pts[3*i+2]);
      uvbox[3] += sqrt((cmp[3*i  ] - pts[3*i  ])*(cmp[3*i  ] - pts[3*i  ]) +
                       (cmp[3*i+1] - pts[3*i+1])*(cmp[3*i+1] - pts[3*i+1]) +
                       (cmp[3*i+2] - pts[3*i+2])*(cmp[3*i+2] - pts[3*i+2]));
    }
    printf("   dev = %le %le %le    mag = %le\n",
           uvbox[0]/npts, uvbox[1]/npts, uvbox[2]/npts, uvbox[3]/npts);
    printf("\n");
  }

  status = gem_destroyDRep(DRep);
  printf(" gem_destroyDRep = %d\n", status);
  status = gem_releaseModel(newModel);
  printf(" gem_releaseModel = %d\n", status);
  status = gem_releaseModel(auxmdl);
  printf(" gem_releaseModel = %d\n", status);
  status = gem_releaseModel(model);
  printf(" gem_releaseModel = %d\n", status);
  status = gem_terminate(context);
  printf(" gem_terminate = %d\n", status);

  return 0;
}
