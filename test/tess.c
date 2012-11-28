/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Tessellation Test Code
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "gem.h"
#include "gv.h"


  static float     scalex;
  static int       nBRep, ng;
  static gemBRep   **BReps;
  static gemDRep   *DRep;
  static GvGraphic **list;



int main(int argc, char *argv[])
{
  int      i, status, uptodate, nParams, nBranch, mtflag = 0;
  int      type, nnode, nedge, nloop, nface, nshell, nattr;
  char     *filename, *server, *modeler;
  float    arg, focus[4], box[6];
  double   angle = 15.0, relSide = 0.02, relSag = 0.001;
  double   bx[6];
  int      keys[2] = {117, 118}, types[2] = {GV_SURF, GV_SURF};
  float    lims[4] = {0.,2.,0.,2.};
  char     titles[32] = {"U Parameter     V Parameter     "};
  gemCntxt *context;
  gemModel *model;

  status = gem_initialize(&context);
  printf(" gem_initialize = %d\n", status);
  if (status != GEM_SUCCESS) return 1;
#ifdef QUARTZ
  if ((argc != 3) && (argc != 6)) {
    printf(" Usage: qtess Modeler Model [angle relSide relSag]\n\n");
    gem_terminate(context);
    return 1;
  }
  filename = argv[2];
  status = gem_setAttribute(context, 0, 0, "Modeler", GEM_STRING, 7, NULL,
                            NULL, argv[1]);
  printf(" gem_setAttribute = %d\n", status);
#else
  if ((argc != 2) && (argc != 5)) {
    printf(" Usage: dtess Model [angle relSide relSag]\n\n");
    gem_terminate(context);
    return 1;
  }
  filename = argv[1];
#endif

  status = gem_loadModel(context, NULL, filename, &model);
  printf(" gem_loadModel = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_terminate(context);
    printf(" gem_terminate = %d\n", status);
    return 1;
  }
  printf("\n");

  if (argc == 6) {
    sscanf(argv[3], "%f", &arg);
    angle = arg;
    sscanf(argv[4], "%f", &arg);
    relSide = arg;
    sscanf(argv[5], "%f", &arg);
    relSag = arg;
  }
  if (argc == 5) {
    sscanf(argv[2], "%f", &arg);
    angle = arg;
    sscanf(argv[3], "%f", &arg);
    relSide = arg;
    sscanf(argv[4], "%f", &arg);
    relSag = arg;
  }

  printf(" Using angle = %lf,  relSide = %lf,  relSag = %lf\n",
         angle, relSide, relSag);

  status = gem_getModel(model, &server, &filename, &modeler, &uptodate,
                        &nBRep, &BReps, &nParams, &nBranch, &nattr);
  printf(" gem_getModel = %d,  nBreps = %d\n", status, nBRep);
  if (status != GEM_SUCCESS) {
    status = gem_releaseModel(model); 
    printf(" gem_releaseModel = %d\n", status);
    status = gem_terminate(context);
    printf(" gem_terminate = %d\n", status);
    return 1;
  }

  status = gem_newDRep(model, &DRep);
  printf(" gem_newDRep = %d\n", status);
  if (status != GEM_SUCCESS) {
    status = gem_releaseModel(model); 
    printf(" gem_releaseModel = %d\n", status);
    status = gem_terminate(context);
    printf(" gem_terminate = %d\n", status);
    return 1;
  }

  for (ng = i = 0; i < nBRep; i++) {
    status = gem_getBRepInfo(BReps[i], bx, &type, &nnode, &nedge,
                             &nloop, &nface, &nshell, &nattr);
    if (status != GEM_SUCCESS) {
      printf(" gem_getBRepInfo(%d) = %d\n", i+1, status);
      continue;
    }
    if (i == 0) {
      box[0] = bx[0];
      box[1] = bx[1];
      box[2] = bx[2];
      box[3] = bx[3];
      box[4] = bx[4];
      box[5] = bx[5];
    } else {
      if (bx[0] < box[0]) box[0] = bx[0];
      if (bx[1] < box[1]) box[1] = bx[1];
      if (bx[2] < box[2]) box[2] = bx[2];
      if (bx[3] > box[3]) box[3] = bx[3];
      if (bx[4] > box[4]) box[4] = bx[4];
      if (bx[5] > box[5]) box[5] = bx[5];
    }
    ng += nface;
  }

  focus[0] = 0.5*(box[0]+box[3]);
  focus[1] = 0.5*(box[1]+box[4]);
  focus[2] = 0.5*(box[2]+box[5]);
  focus[3] = sqrt( (box[0]-box[3])*(box[0]-box[3]) +
                   (box[1]-box[4])*(box[1]-box[4]) +
                   (box[2]-box[5])*(box[2]-box[5]) );
  scalex = 0.1*focus[3];

  status = gem_tesselDRep(DRep, 0, angle, relSide*focus[3], relSag*focus[3]);
  printf(" gem_tesselDRep = %d\n", status);
  
  if (status == GEM_SUCCESS) {

    status = gv_init("                Test Code", mtflag, 0, keys, types, lims,
                     titles, focus);
    printf("return from gv_init = %d\n", status);
  }
  
  for (i = 0; i < ng; i++) gv_free(list[i], 1);

  /* cleanup */

  status = gem_destroyDRep(DRep);
  printf(" gem_destroyDRep = %d\n", status);  
  status = gem_releaseModel(model); 
  printf(" gem_releaseModel = %d\n", status);
  status = gem_terminate(context);
  printf(" gem_terminate = %d\n\n", status);
  
  return 0;
}


int gvupdate( )

/*
 *  used for single process operation to allow the changing of data
 *
 *  returns: 0 no data has changed
 *           non-zero (positive) data has changed
 *           new number of graphics objects - forces a call to gvdata
 *
 */
{ 
  return ng;
}

void gvdata( int ngraphics, GvGraphic* graphic[] )

/*
 *  used to (re)set the graphics objects to be used in plotting
 *
 *  ngraphics is the number of objects to be filled
 *                           (from gvupdate or gv_handshake)
 *
 */
{
  GvColor  color;
  GvObject *object;
  int      status, nnode, nedge, nloop, nface, nshell, nattr;
  int      i, j, k, m, type, npts, ntris, *tris;
  double   box[6], *points;
  char     title[16];
  gemPair  bface;

  printf("in  gvdata - n = %d\n", ngraphics);
  list = graphic;

  for (i = k = 0; k < nBRep; k++) {
    bface.BRep = k+1;
    status = gem_getBRepInfo(BReps[k], box, &type, &nnode, &nedge,
                             &nloop, &nface, &nshell, &nattr);
    if (status != GEM_SUCCESS) continue;

    for (j = 0; j < nface; j++) {
      bface.index = j+1;
      status = gem_getTessel(DRep, bface, &ntris, &npts, &tris, &points);
      if (status != GEM_SUCCESS)
        printf(" BRep #%d: gem_getTessel status = %d\n", k+1, status);
      color.red   = 1.0;
      color.green = 0.0;
      if (nBRep > 1) color.green = k/(nBRep-1.0);
      color.blue  = 0.0;
      sprintf(title, "face #%d ", j+1);
      graphic[i] = gv_alloc(GV_INDEXED, GV_DISJOINTTRIANGLES,
                            GV_FOREGROUND|GV_ORIENTATION|GV_FACETLIGHT|GV_MESH|GV_FORWARD,
                            color, title, bface.BRep, bface.index);
      if (graphic[i] == NULL) {
        printf("gv_alloc ERROR on graphic[%d]\n", i);
      } else {
        graphic[i]->back.red   = 0.5;
        graphic[i]->back.green = 0.5;
        graphic[i]->back.blue  = 0.5;
        graphic[i]->number     = 1;
        object = graphic[i]->object;
        if ((npts <= 0) || (ntris <= 0)) {
          object->length = 0;
        } else {
          graphic[i]->ddata = points;
          object->length = ntris;
          object->type.distris.index = (int *) malloc(3*ntris*sizeof(int));
          if (object->type.distris.index != NULL)
            for (m = 0; m < 3*ntris; m++)
              object->type.distris.index[m] = tris[m]-1;
        }
      }
      i++;
    }
  }
  printf("out gvdata - n = %d\n", i);
}


