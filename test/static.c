/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Static Model Test Code
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
  double   xform[12];
  char     *server, *filename, *modeler;
  gemCntxt *context;
  gemModel *model, *newModel;
  gemBRep  **BReps; 

  if ((argc != 2) && (argc != 3)) {
    printf(" usage: [d/q]static filename [modeler]!\n");
    return 1;
  }
  for (i = 1; i < 12; i++) xform[i] = 0.0;
  xform[0] = xform[5] = xform[10] = 1.5;

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
  printf(" \n");
  if (status != GEM_SUCCESS) {
    status = gem_terminate(context);
    printf(" gem_terminate = %d\n", status);
    return 1;
  }
  status  = gem_getModel(model, &server, &filename, &modeler, &uptodate,
                         &nBRep, &BReps, &nParams, &nBranch, &nattr);
  printf(" gem_getModel = %d\n", status);

  status = gem_staticModel(context, &newModel);
  printf(" gem_staticModel = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_releaseModel(model);
    printf(" gem_releaseModel = %d\n", status);
    status = gem_terminate(context);
    printf(" gem_terminate = %d\n", status);
    return 1;
  }
  status = gem_add2Model(newModel, BReps[0], NULL);
  printf(" gem_add2Model = %d\n", status);
  status = gem_add2Model(newModel, BReps[0], xform);
  printf(" gem_add2Model = %d\n\n", status);
  status  = gem_getModel(newModel, &server, &filename, &modeler, &uptodate,
                         &nBRep, &BReps, &nParams, &nBranch, &nattr);
  printf(" gem_getModel = %d\n", status);
  printf("     FileName = %s\n", filename);
  printf("     Modeler  = %s\n", modeler);
  printf("     UpToDate = %d\n", uptodate);
  printf("     nBReps   = %d\n", nBRep);
  printf("\n");

#ifndef QUARTZ
  status = gem_saveModel(newModel, "newModel.egads");
  printf(" gem_saveModel egads = %d\n", status);
#endif
  status = gem_releaseModel(newModel);
  printf(" gem_releaseModel = %d\n", status);
  status = gem_releaseModel(model);
  printf(" gem_releaseModel = %d\n", status);
  status = gem_terminate(context);
  printf(" gem_terminate = %d\n", status);

  return 0;
}
