/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Solid Boolean Operator Test Code
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "gem.h"


int main(int argc, char *argv[])
{
  int      status, i, uptodate, nBRep, nParams, nBranch, nattr;
  int      type, nnode, nedge, nloop, nface, nshell;
  double   xform[12], box[6];
  char     *server, *filename, *modeler;
  gemCntxt *context;
  gemModel *model1, *model2, *newModel;
  gemBRep  **BReps, *BRep; 

  if ((argc != 2) && (argc != 3)) {
    printf(" usage: [d/q]sbo filename [modeler]!\n");
    return 1;
  }
  for (i = 1; i < 12; i++) xform[i] = 0.0;
  xform[0] = xform[5] = xform[10] = 1.0;

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
  status = gem_loadModel(context, server, argv[1], &model1);
  printf(" gem_loadModel 1 = %d\n", status);
#ifdef QUARTZ
  status = gem_saveModel(model1, "qsbo");
  printf(" gem_saveModel = %d\n", status);
  status = gem_loadModel(context, server, "qsbo", &model2);
#else
  status = gem_loadModel(context, server, argv[1], &model2);
#endif
  printf(" gem_loadModel 2 = %d\n", status);
  printf(" \n");
  if (status != GEM_SUCCESS) {
    status = gem_terminate(context);
    printf(" gem_terminate = %d\n", status);
    return 1;
  }
  status  = gem_getModel(model1, &server, &filename, &modeler, &uptodate,
                         &nBRep, &BReps, &nParams, &nBranch, &nattr);
  printf(" gem_getModel 1 = %d  nBRep = %d\n", status, nBRep);
  BRep = BReps[0];
  status  = gem_getModel(model2, &server, &filename, &modeler, &uptodate,
                         &nBRep, &BReps, &nParams, &nBranch, &nattr);
  printf(" gem_getModel 2 = %d  nBRep = %d\n", status, nBRep);
  
  status = gem_getBRepInfo(BRep, box, &type, &nnode, &nedge, &nloop,
                           &nface, &nshell, &nattr);
  printf(" gem_getBRepInfo = %d\n", status);
  xform[11] = 0.5*(box[5]-box[2]);
  status = gem_solidBoolean(BRep, BReps[0], xform, GEM_UNION, &newModel);
  printf(" gem_solidBoolean = %d\n", status);

  status  = gem_getModel(newModel, &server, &filename, &modeler, &uptodate,
                         &nBRep, &BReps, &nParams, &nBranch, &nattr);
  printf(" gem_getModel = %d\n", status);
  printf("     FileName = %s\n", filename);
  printf("     Modeler  = %s\n", modeler);
  printf("     UpToDate = %d\n", uptodate);
  printf("     nBReps   = %d\n", nBRep);
  printf("\n");

#ifdef QUARTZ
  status = gem_saveModel(newModel, "newModel");
#else
  status = gem_saveModel(newModel, "newModel.egads");
#endif
  printf(" gem_saveModel egads = %d\n", status);
  status = gem_releaseModel(newModel);
  printf(" gem_releaseModel = %d\n", status);
  status = gem_releaseModel(model2);
  printf(" gem_releaseModel 2 = %d\n", status);
  status = gem_releaseModel(model1);
  printf(" gem_releaseModel 1 = %d\n", status);
  status = gem_terminate(context);
  printf(" gem_terminate = %d\n", status);

  return 0;
}
