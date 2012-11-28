/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Master Model Test Code
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "gem.h"



static void
printBattr(gemModel *model, int bindex, int aindex)
{
  int    i, status, atype, alen, *integers;
  double *reals;
  char   *aname, *string;
  
  status = gem_getAttribute(model, GEM_BRANCH, bindex, aindex, &aname, &atype,
                            &alen, &integers, &reals, &string);
  if (status != GEM_SUCCESS) {
    printf("gem_getAttribute = %d for Branch %d %d\n", status, bindex, aindex);
    return;
  }
  if ((atype == GEM_BOOL)  || (atype == GEM_INTEGER)) {
    printf("%s = ", aname);
    for (i = 0; i < alen; i++) printf("%d ", integers[i]);
  } else if (atype == GEM_REAL) {
    printf("%s = ", aname);
    for (i = 0; i < alen; i++) printf("%lf ", reals[i]);
  } else if (atype == GEM_STRING) {
    printf("%s = %s", aname, string); 
  } else {
    printf("GEM Attribute %s with unknown type = %d", aname, atype);
  }
  printf("\n");
}


static void
outModel(gemModel *model, int *nparam)
{
  int     i, j, k, status, uptodate, nBRep, nParams, nBranch, type;
  int     nattr, n, ind, *indices, *child;
  char    *server, *filename, *modeler, *ID, *string;
  double  *reals;
  gemSpl  *spline;
  gemBRep **BReps; 

  *nparam = 0;
  status  = gem_getModel(model, &server, &filename, &modeler, &uptodate,
                         &nBRep, &BReps, &nParams, &nBranch, &nattr);
  printf(" gem_getModel = %d\n", status);
  if (status == GEM_SUCCESS) {
    printf("     Server   = %s\n", server);
    printf("     FileName = %s\n", filename);
    printf("     Modeler  = %s\n", modeler);
    printf("     UpToDate = %d\n", uptodate);
    printf("     nBReps   = %d\n", nBRep);
    printf("     nParams  = %d\n", nParams);
    printf("     nBranch  = %d\n", nBranch);
    *nparam = nParams;

    if (nBranch != 0) printf("\n");
    for (i = 0; i < nBranch; i++) {
      printf("     Branch #%d:\n", i+1);
      status = gem_getBranch(model, i+1, &ID, &string, &ind, &n, &indices, 
                             &k, &child, &nattr);
      if (status == GEM_SUCCESS) {
        printf("       bName   = %s\n", ID);
        printf("       bType   = %s\n", string);
        printf("       supprs  = %d\n", ind);
        if (n > 0) {
          printf("       parents = ");
          for (j = 0; j < n; j++) printf("%d ", indices[j]);
          printf("\n");
        }
        if (k > 0) {
          printf("       childrn = ");
          for (j = 0; j < k; j++) printf("%d ", child[j]);
          printf("\n");
        }
        if (nattr != 0) {
          printf("       #attr   = %d\n", nattr);
          for (j = 0; j < nattr; j++) {
            printf("         ");
            printBattr(model, i+1, j+1);
          }
        }

      } else {
        printf("       gem_getBranch %d = %d\n", i+1, status);
      }
    }
      
    if (nParams != 0) printf("\n");
    for (i = 0; i < nParams; i++) {
      printf("     Parameter #%d:\n", i+1);
      status = gem_getParam(model, i+1, &ID, &ind, &n, &type, &k,
                            &indices, &reals, &string, &spline, &nattr);
      if (status == GEM_SUCCESS) {
        printf("       pName  = %s\n", ID);
        printf("       bitflg = %d\n", ind);
        printf("       order  = %d\n", n);
        printf("       pType  = %d\n", type);
        printf("       pLen   = %d\n", k);
        printf("       val(s) = ");
        if (type == GEM_STRING) {
          printf("%s", string);
        } else {
          for (j = 0; j < k; j++) 
            if ((type == GEM_INTEGER) || (type == GEM_BOOL)) {
              printf("%d ", indices[j]);
            } else if (type == GEM_REAL) {
              printf("%lf ", reals[j]);
            }
        }
        printf("\n");
      } else {
        printf("       gem_getParam %d = %d\n", i+1, status);
      }
    }
    
  }

}


int main(int argc, char *argv[])
{
  int      status, nparam, iparam;
  double   real;
  char     *server;
  gemCntxt *context;
  gemModel *model, *newModel;

  if ((argc != 2) && (argc != 3)) {
    printf(" usage: [d/q]mmdl filename [modeler]!\n");
    return 1;
  }

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
  
  if (status == GEM_SUCCESS) {
    outModel(model, &nparam);
    printf(" \n");
    status = gem_copyModel(model, &newModel);
    printf(" gem_copyModel = %d\n", status);
    if (status == GEM_SUCCESS) {
      status = gem_releaseModel(model);
      printf(" gem_releaseModel = %d\n", status);
      model  = newModel;
      /* set new parameter values */
      do {
        printf(" Enter Parameter Index [0-done]: ");
        scanf("%d", &iparam);
        while (getchar() != 10);
        if ((iparam < 1) || (iparam > nparam)) continue;
        printf(" Enter Parameter Value: ");
        scanf("%lf", &real);
        while (getchar() != 10);
        status = gem_setParam(model, iparam, 1, NULL, &real, NULL, NULL);
        if (status != GEM_SUCCESS)
          printf(" gem_setParam &d = %d\n", iparam, status);
      } while ((iparam > 0) && (iparam <= nparam));
      status = gem_regenModel(model);
      printf(" gem_regenModel = %d\n", status);
      printf(" \n");
      outModel(model, &nparam);
      printf(" \n");
#ifdef QUARTZ
      status = gem_saveModel(model, "newModel");
      printf(" gem_saveModel = %d\n", status);
#else
      status = gem_saveModel(model, "newModel.csm");
      printf(" gem_saveModel csm   = %d\n", status);
      status = gem_saveModel(model, "newModel.egads");
      printf(" gem_saveModel egads = %d\n", status);
#endif
    }
    status = gem_releaseModel(model);
    printf(" gem_releaseModel = %d\n", status);
  }
  status = gem_terminate(context);
  printf(" gem_terminate = %d\n", status);

  return 0;
}
