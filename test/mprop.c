/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Mass Properties Test Code
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define RAD    0.017453292

#include "gem.h"
#ifdef QUARTZ
#include "capri.h"
#else
#include "egads.h"
  extern ego dia_context;
#endif


static void outMProps(gemBRep *brep)
{
  int    status;
  double props[14];
  
  printf("\n");
  status = gem_getMassProps(brep, GEM_BREP, 0, props);
  if (status != GEM_SUCCESS) {
    printf(" gem_getMassProps = %d\n", status);
    return;
  }
  printf("   Volume       = %lf\n", props[0]);
  printf("   Surface Area = %lf\n", props[1]);
  printf("   CoG          = %lf %lf %lf\n", props[ 2], props[ 3], props[ 4]);
  printf("   ImassMatrix  = %lf %lf %lf\n", props[ 5], props[ 6], props[ 7]);
  printf("                  %lf %lf %lf\n", props[ 8], props[ 9], props[10]);
  printf("                  %lf %lf %lf\n", props[11], props[12], props[13]);
}


int main(int argc, /*@unused@*/ char *argv[])
{
  int      status, i, uptodate, nBRep, nParams, nBranch, nattr;
  double   xform[12], data[6], sinr, cosr, angle;
  char     *server, *filename, *modeler;
  gemCntxt *context;
  gemModel *model, *newModel;
  gemBRep  **BReps;
#ifdef QUARTZ
  int      mdl;
#else
  ego      body, mdl;
#endif

  if (argc != 1) {
    printf(" usage: [d/q]mprop!\n");
    return 1;
  }

  for (i = 1; i < 12; i++) xform[i] = 0.0;
  xform[0] = xform[5] = xform[10] = 2.0;
  xform[3] = 1.0;
  angle = 30.0;
  sinr  = sin(angle*RAD);
  cosr  = cos(angle*RAD);
  xform[0] =  2.0*cosr;
  xform[1] =  2.0*sinr;
  xform[4] = -2.0*sinr;
  xform[5] =  2.0*cosr;

  server = getenv("GEMserver");
  status = gem_initialize(&context);
  printf(" gem_initialize = %d\n", status);
  status = gem_setAttribute(context, 0, 0, "Modeler", GEM_STRING, 7, NULL,
                            NULL, "Parasolid");
  printf(" gem_setAttribute = %d\n", status);
  data[0] = data[1] = data[2] = 0.0;
  data[3] = 1.0;
  data[4] = 2.0;
  data[5] = 3.0;
#ifdef QUARTZ
  mdl = gi_gCreateVolume(server, "Parasolid", 1, data);
  if (mdl <= CAPRI_SUCCESS) {
    printf(" gi_gCreateVolume = %d!\n", mdl);
    status = gem_terminate(context);
    printf(" gem_terminate = %d\n", status);
    return 1;
  }
  status = gi_uSaveModel(mdl, "mprop");
  if (status != CAPRI_SUCCESS) {
    printf(" gi_uSaveModel = %d!\n", status);
    status = gi_uRelModel(mdl);
    printf(" gi_uRelModel = %d\n", status);
    status = gem_terminate(context);
    printf(" gem_terminate = %d\n", status);
    return 1;
  }
  status = gi_uRelModel(mdl);
  printf(" gi_uRelModel = %d\n", status);
  status = gem_loadModel(context, server, "mprop", &model);
#else
  status = EG_makeSolidBody(dia_context, BOX, data, &body);
  if (status != EGADS_SUCCESS) {
    printf(" EG_makeSolidBody = %d!\n", status);
    status = gem_terminate(context);
    printf(" gem_terminate = %d\n", status);
    return 1;
  }
  status = EG_makeTopology(dia_context, NULL, MODEL, 0,
                           NULL, 1, &body, NULL, &mdl);
  if (status != EGADS_SUCCESS) {
    printf(" EG_makeTopology = %d!\n", status);
    status = EG_deleteObject(body);
    printf(" EG_deleteObject = %d\n", status);
    status = gem_terminate(context);
    printf(" gem_terminate = %d\n", status);
    return 1;
  }
  status = EG_saveModel(mdl, "mprop.egads");
  printf(" EG_saveModel = %d!\n", status);
  status = EG_deleteObject(mdl);
  printf(" EG_deleteObject = %d\n", status);
  status = gem_loadModel(context, server, "mprop.egads", &model);
#endif
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
  outMProps(BReps[0]);
  outMProps(BReps[1]);

  status = gem_releaseModel(newModel);
  printf(" gem_releaseModel = %d\n", status);
  status = gem_releaseModel(model);
  printf(" gem_releaseModel = %d\n", status);
  status = gem_terminate(context);
  printf(" gem_terminate = %d\n", status);

  return 0;
}
