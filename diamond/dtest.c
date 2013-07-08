/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Diamond Model Test Code
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "gem.h"
#include "egads.h"

  /* the EGADS context for diamond */
  extern ego dia_context;



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
printattr(gemBRep *brep, int etype, int eindex, int aindex)
{
  int    i, status, atype, alen, *integers;
  double *reals;
  char   *aname, *string;
  
  status = gem_getAttribute(brep, etype, eindex, aindex, &aname, &atype,
                            &alen, &integers, &reals, &string);
  if (status != GEM_SUCCESS) {
    printf("gem_getAttribute = %d for %d %d %d\n", status, etype, eindex, 
           aindex);
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


int main(int argc, char *argv[])
{
  int      i, j, k, status, uptodate, nBRep, nParams, nBranch, type;
  int      nnode, nedge, nloop, nface, nshell, nattr, n, ind;
  int      nds[2], fcs[2], *indices, *child;
  char     *server, *filename, *modeler, *ID, *string;
  double   box[6], uvbox[4], *reals;
  gemSpl   *spline;
  gemBRep  **BReps; 
  gemCntxt *context;
  gemModel *model;

  if (argc != 2) {
    printf(" usage: test filename!\n");
    return 1;
  }

  status = gem_initialize(&context);
  printf(" gem_initialize = %d\n", status);
  status = gem_loadModel(context, NULL, argv[1], &model);
  printf(" gem_loadModel = %d\n", status);
  printf(" \n");

  if (status == GEM_SUCCESS) {
    status = gem_getModel(model, &server, &filename, &modeler, &uptodate,
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
      
      for (i = 0; i < nBRep; i++) {
        printf("\n");
        j = i+1;
        printf("     BRep #%d:\n", j);
        status = gem_setAttribute(BReps[i], GEM_BREP, 0, "BRepIndex",
                                  GEM_INTEGER, 1, &j, NULL, NULL);
        printf("     gem_setAttribute = %d\n", status);
        status = gem_getBRepInfo(BReps[i], box, &type, &nnode, &nedge,
                                 &nloop, &nface, &nshell, &nattr);
        if (status == GEM_SUCCESS) {
          printf("       box    = %lf %lf %lf\n", box[0], box[1], box[2]);
          printf("                %lf %lf %lf\n", box[3], box[4], box[5]);
          printf("       type   = %d\n", type);
          printf("       #node  = %d\n", nnode);
          printf("       #edge  = %d\n", nedge);
          printf("       #loop  = %d\n", nloop);
          printf("       #face  = %d\n", nface);
          printf("       #shell = %d\n", nshell);
          if (nattr != 0) {
            printf("       #attr  = %d\n", nattr);
            for (j = 0; j < nattr; j++) {
              printf("         ");
              printattr(BReps[i], GEM_BREP, 0, j+1);
            }
          }
          
          printf("\n");
          for (k = 0; k < nshell; k++) {
            printf("       Shell #%d:\n", k+1);
            status = gem_getShell(BReps[i], k+1, &type, &n, &indices, &nattr);
            if (status == GEM_SUCCESS) {
              printf("         type  = %d\n", type);
              printf("         faces = ");
              for (j = 0; j < n; j++) printf("%d ", indices[j]);
              printf("\n");
              if (nattr != 0) {
                printf("         #attr = %d\n", nattr);
                for (j = 0; j < nattr; j++) {
                  printf("           ");
                  printattr(BReps[i], GEM_SHELL, k+1, j+1);
                }
              }          
            } else {
              printf("       gem_getShell = %d\n", status);
            }
          }
          
          printf("\n");
          for (k = 0; k < nface; k++) {
            printf("       Face #%d:\n", k+1);
            status = gem_getFace(BReps[i], k+1, &ID, uvbox, &type, &n, 
                                 &indices, &nattr);
            if (status == GEM_SUCCESS) {
              printf("         uvbox = %lf %lf\n", uvbox[0], uvbox[1]);
              printf("                 %lf %lf\n", uvbox[2], uvbox[3]);
              printf("         ID    = %s\n", ID);
              printf("         sense = %d\n", type);
              printf("         loops = ");
              for (j = 0; j < n; j++) printf("%d ", indices[j]);
              printf("\n");
              if (nattr != 0) {
                printf("         #attr = %d\n", nattr);
                for (j = 0; j < nattr; j++) {
                  printf("           ");
                  printattr(BReps[i], GEM_FACE, k+1, j+1);
                }
              }          
            } else {
              printf("       gem_getFace = %d\n", status);
            }
          }
          
          printf("\n");
          for (k = 0; k < nloop; k++) {
            printf("       Loop #%d:\n", k+1);
            status = gem_getLoop(BReps[i], k+1, &ind, &type, &n, 
                                 &indices, &nattr);
            if (status == GEM_SUCCESS) {
              printf("         Face  = %d\n", ind);
              printf("         type  = %d\n", type);
              printf("         Edges = ");
              for (j = 0; j < n; j++) printf("%d ", indices[j]);
              printf("\n");
              if (nattr != 0) {
                printf("         #attr = %d\n", nattr);
                for (j = 0; j < nattr; j++) {
                  printf("           ");
                  printattr(BReps[i], GEM_LOOP, k+1, j+1);
                }
              }          
            } else {
              printf("       gem_getLoop = %d\n", status);
            }
          }
          
          printf("\n");
          for (k = 0; k < nedge; k++) {
            printf("       Edge #%d:\n", k+1);
            status = gem_getEdge(BReps[i], k+1, uvbox, nds, fcs, &nattr);
            if (status == GEM_SUCCESS) {
              printf("         tlims = %lf %lf\n", uvbox[0], uvbox[1]);
              printf("         Nodes = %d %d\n", nds[0], nds[1]);
              printf("         Faces = %d %d\n", fcs[0], fcs[1]);
              if (nattr != 0) {
                printf("         #attr = %d\n", nattr);
                for (j = 0; j < nattr; j++) {
                  printf("           ");
                  printattr(BReps[i], GEM_EDGE, k+1, j+1);
                }
              }
            } else {
              printf("       gem_getEdge = %d\n", status);
            }
          }
          
          printf("\n");
          for (k = 0; k < nnode; k++) {
            printf("       Node #%d:\n", k+1);
            status = gem_getNode(BReps[i], k+1, uvbox, &nattr);
            if (status == GEM_SUCCESS) {
              printf("         xyz   = %lf %lf %lf\n", 
                     uvbox[0], uvbox[1], uvbox[2]);
              if (nattr != 0) {
                printf("         #attr = %d\n", nattr);
                for (j = 0; j < nattr; j++) {
                  printf("           ");
                  printattr(BReps[i], GEM_NODE, k+1, j+1);
                }
              }          
            } else {
              printf("       gem_getNode = %d\n", status);
            }
          }

        } else {
          printf("       gem_getBRepInfo = %d\n", status);
        }
      }
    }
    printf(" \n");
    status = gem_releaseModel(model);
    printf(" gem_releaseModel = %d\n", status);
  }
  status = gem_terminate(context);
  printf(" gem_terminate = %d\n", status);
  status = gem_initialize(&context);
  printf(" gem_(re)initialize = %d\n", status);
  status = gem_terminate(context);
  printf(" gem_terminate = %d\n", status);

  return 0;
}
