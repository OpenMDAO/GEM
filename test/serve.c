/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Tessellation Test Code
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <unistd.h>		// usleep

#ifdef WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#endif

#define MAX_EXPR_LEN     128
#define MAX_STR_LEN    32767

#include "gem.h"
#include "wsserver.h"


  extern void  wv_destroyContext(wvContext **context);

  static int       nBRep, nParams, nBranch;
  static double    angle = 15.0, relSide = 0.02, relSag = 0.001;
  static gemBRep   **BReps;
  static gemModel  *model;
  static wvContext *cntxt;


static int makeTess(int flag)
{
  int     i, j, k, status, type, nnode, nedge, nloop, nface, nshell, ntris;
  int     head, nattr, npts, attrs, itri, nseg, *segs, igprim, nitem, *tris;
  float   box[6], focus[4], color[3], *xyzs;
  double  bx[6], *points, size;
  char    gpname[33];
  gemDRep *DRep;
  gemPair bface;
  wvData  items[5];

  if (flag != 0) wv_removeAll(cntxt);
  
  status = gem_newDRep(model, &DRep);
  printf(" gem_newDRep = %d\n", status);
  if (status != GEM_SUCCESS)  return status;

  for (i = 0; i < nBRep; i++) {
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
  }

                              size = box[3] - box[0];
    if (size < box[4]-box[1]) size = box[4] - box[1];
    if (size < box[5]-box[2]) size = box[5] - box[2];

  focus[0] = 0.5*(box[0] + box[3]);
  focus[1] = 0.5*(box[1] + box[4]);
  focus[2] = 0.5*(box[2] + box[5]);
  focus[3] = size;

  status = gem_tesselDRep(DRep, 0, angle, relSide*focus[3], relSag*focus[3]);
  printf(" gem_tesselDRep = %d\n", status);
  
  for (i = 0; i < nBRep; i++) {
    bface.BRep = i+1;
    status = gem_getBRepInfo(BReps[i], bx, &type, &nnode, &nedge,
                             &nloop, &nface, &nshell, &nattr);
    if (status != GEM_SUCCESS) continue;

    for (j = 0; j < nface; j++) {
      bface.index = j+1;
      status = gem_getTessel(DRep, bface, &ntris, &npts, &tris, &points);
      if (status != GEM_SUCCESS)
        printf(" BRep #%d: gem_getTessel status = %d\n", i+1, status);
      sprintf(gpname, "Body %d Face %d", bface.BRep, bface.index);
      attrs = WV_ON | WV_ORIENTATION; 
      /* vertices */
      wv_setData(WV_REAL64, npts, points, WV_VERTICES, &items[0]);
      wv_adjustVerts(&items[0], focus);
      /* triangles */
      wv_setData(WV_INT32, 3*ntris, tris, WV_INDICES, &items[1]);
      /* triangle colors */
      color[0] = 1.0;
      color[1] = 0.0;
      color[2] = 0.0;
      if (nBRep > 1) color[1] = i/(nBRep-1.0);
      wv_setData(WV_REAL32, 1, color, WV_COLORS, &items[2]);
      nitem = 3;
      /* triangle sides (segments) */
      segs = (int *) malloc(6*ntris*sizeof(int));
      if (segs != NULL) {
        nitem = 5;
        for (nseg = itri = 0; itri < ntris; itri++)
          for (k = 0; k < 3; k++) {
            segs[2*nseg  ] = tris[3*itri+(k+1)%3];
            segs[2*nseg+1] = tris[3*itri+(k+2)%3];
            nseg++;
          }
        wv_setData(WV_INT32, 2*nseg, segs, WV_LINDICES, &items[3]);
        free(segs);
        /* segment colors */
        color[0] = 0.0;
        color[1] = 0.0;
        color[2] = 0.0;
        wv_setData(WV_REAL32, 1, color, WV_LCOLOR, &items[4]);
      }

      /* make graphic primitive */
      if (cntxt != NULL) {
        igprim = wv_addGPrim(cntxt, gpname, WV_TRIANGLE, attrs, nitem, items);
        if (igprim >= 0) {
          /* make line width 1 */
          if (cntxt->gPrims != NULL) cntxt->gPrims[igprim].lWidth = 1.0;
        } else {
          printf(" addGPrim for %s = %d\n", gpname, igprim);
        }
      }
    }

    for (j = 0; j < nedge; j++) {
      /* name and attributes */
      bface.index = j+1;
      sprintf(gpname, "Body %d Edge %d", bface.BRep, j+1);
      attrs  = WV_ON;
      status = gem_getDiscrete(DRep, bface, &npts, &points);
      if (status != GEM_SUCCESS) continue;
      if (npts < 2) continue;
      head = npts - 1;
      xyzs = (float *) malloc(6*head*sizeof(float));
      if (xyzs == NULL) continue;
      for (nseg = 0; nseg < head; nseg++) {
        xyzs[6*nseg  ] = points[3*nseg  ];
        xyzs[6*nseg+1] = points[3*nseg+1];
        xyzs[6*nseg+2] = points[3*nseg+2];
        xyzs[6*nseg+3] = points[3*nseg+3];
        xyzs[6*nseg+4] = points[3*nseg+4];
        xyzs[6*nseg+5] = points[3*nseg+5];
      }
      /* vertices */
      wv_setData(WV_REAL32, 2*nseg, xyzs, WV_VERTICES, &items[0]);
      wv_adjustVerts(&items[0], focus);
      free(xyzs);
      /* line colors */
      color[0] = 0.0;
      color[1] = 0.0;
      color[2] = 1.0;
      wv_setData(WV_REAL32, 1, color, WV_COLORS, &items[1]);
      /* make graphic primitive */
      if (cntxt != NULL) {
        igprim = wv_addGPrim(cntxt, gpname, WV_LINE, attrs, 2, items);
        if (igprim >= 0) {
          /* make line width 1.5 */
          if (cntxt->gPrims != NULL) cntxt->gPrims[igprim].lWidth = 1.5;
          if (head != 0) wv_addArrowHeads(cntxt, igprim, 0.05, 1, &head);
        } else {
          printf(" addGPrim for %s = %d\n", gpname, igprim);
        }
      }
    }

  }  

  status = gem_destroyDRep(DRep);
  printf(" gem_destroyDRep = %d\n", status);
  
  return GEM_SUCCESS;
}


int main(int argc, char *argv[])
{
  int      status, uptodate, nattr, port = 7681;
  char     *filename, *server, *modeler, *wv_start;
  float    arg;
  gemCntxt *context;
  float    eye[3]    = {0.0, 0.0, 7.0};
  float    center[3] = {0.0, 0.0, 0.0};
  float    up[3]     = {0.0, 1.0, 0.0};

  status = gem_initialize(&context);
  printf(" gem_initialize = %d\n", status);
  if (status != GEM_SUCCESS) return 1;
#ifdef QUARTZ
  if ((argc != 3) && (argc != 6)) {
    printf(" Usage: qserve Modeler Model [angle relSide relSag]\n\n");
    gem_terminate(context);
    return 1;
  }
  filename = argv[2];
  status = gem_setAttribute(context, 0, 0, "Modeler", GEM_STRING, 7, NULL,
                            NULL, argv[1]);
  printf(" gem_setAttribute = %d\n", status);
#else
  if ((argc != 2) && (argc != 5)) {
    printf(" Usage: d/gserve Model [angle relSide relSag]\n\n");
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
  status = gem_getModel(model, &server, &filename, &modeler, &uptodate,
                        &nBRep, &BReps, &nParams, &nBranch, &nattr);
  printf(" gem_getModel = %d,  nBreps = %d\n", status, nBRep);
  if ((status != GEM_SUCCESS) || (nBRep == 0)) {
    status = gem_releaseModel(model); 
    printf(" gem_releaseModel = %d\n", status);
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
         
  /* create the WebViewer context */
  cntxt = wv_createContext(1, 30.0, 1.0, 10.0, eye, center, up);
  if (cntxt == NULL) {
    printf(" failed to create wvContext!\n");
    status = gem_releaseModel(model); 
    printf(" gem_releaseModel = %d\n", status);
    status = gem_terminate(context);
    printf(" gem_terminate = %d\n", status);
    return 1;
  }

  status = makeTess(0);
  if (status != GEM_SUCCESS) {
    wv_destroyContext(&cntxt);
    printf(" makeTess = %d!\n", status);
    status = gem_releaseModel(model); 
    printf(" gem_releaseModel = %d\n", status);
    status = gem_terminate(context);
    printf(" gem_terminate = %d\n", status);
    return 1;
  }

  /* get the command to start the client (if any) */
  wv_start = getenv("WV_START");

  /* start the server */
  status = 0;
  if (wv_startServer(port, NULL, NULL, NULL, 0, cntxt) == 0) {

    /* stay alive a long as we have a client */
    while (wv_statusServer(0)) {
      usleep(500000);

      /* start the browser if the first time through this loop */
      if (status == 0) {
        if (wv_start != NULL) system(wv_start);
        status++;
      }
    }
  }

  wv_cleanupServers();
  
  status = gem_releaseModel(model); 
  printf(" gem_releaseModel = %d\n", status);
  status = gem_terminate(context);
  printf(" gem_terminate = %d\n\n", status);
  return 0;
}


static int
getToken(char *text, int nskip, char *token)
{
  int lentok, i, count, iskip;

  token[0] = '\0';
  lentok   = 0;

  /* count the number of semi-colons */
  for (count = i = 0; i < strlen(text); i++)
    if (text[i] == ';') count++;

  if (count < nskip+1) return 0;

  /* skip over nskip tokens */
  for (i = iskip = 0; iskip < nskip; iskip++) {
    while (text[i] != ';') i++;
    i++;
  }

  /* extract the token we are looking for */
  while (text[i] != ';') {
    token[lentok++] = text[i++];
    token[lentok  ] = '\0';
  }

  return strlen(token);
}


void
browserMessage(void *wsi, char *text, /*@unused@*/ int lena)
{
  int    i, j, k, status, ipmtr, bflg, order, type, len, index, nattr, err;
  int    irow, icol, *ints, *nints, ibrch, sup, np, *par, nc, *chld, iattr;
  char   response[MAX_STR_LEN], entry[MAX_STR_LEN], *name, *string, *pEnd;
  char   arg1[MAX_EXPR_LEN], arg2[MAX_EXPR_LEN], arg3[MAX_EXPR_LEN];
  char   arg4[MAX_EXPR_LEN], *aname;
  double *reals, *nreals;
  gemSpl *spline;
  
  /* NO-OP */
  if (strlen(text) == 0) {

  /* "identify;" */
  } else if (strncmp(text, "identify;", 9) == 0) {

    /* build the response */
    sprintf(response, "identify;serveGEM;");

    /* send the response */
    wv_sendText(wsi, response);

  /* "getPmtrs;" */
  } else if (strncmp(text, "getPmtrs;", 9) == 0) {

    /* build the response in JSON format */
    sprintf(response, "getPmtrs;[");
    j = 0;
    for (ipmtr = 1; ipmtr <= nParams; ipmtr++) {
      status = gem_getParam(model, ipmtr, &name, &bflg, &order, &type, &len,
                            &ints, &reals, &string, &spline, &nattr);
      if (status != GEM_SUCCESS) continue;
      if ((type == GEM_STRING) || (type == GEM_SPLINE)) continue;
      if (j != 0) strcat(response, ",");
      j++;
      k = 500;
      if ((bflg&2) != 0) k = 501;
      sprintf(entry, "{\"name\":\"%s\",\"type\":%d,\"nrow\":%d,\"ncol\":%d,\"value\":[",
              name, k, 1, len);
      strcat(response, entry);

      index = 0;
      for (i = 1; i <= len; i++) {
        if (type == GEM_REAL) {
          if (i == len) {
            sprintf(entry, "%lg]", reals[index++]);
          } else {
            sprintf(entry, "%lg,", reals[index++]);
          }
        } else {
          if (i == len) {
            sprintf(entry, "%d]", ints[index++]);
          } else {
            sprintf(entry, "%d,", ints[index++]);
          }        
        }
        strcat(response, entry);
      }
      strcat(response, "}");
    }

    /* send the response */
    strcat(response, "]");
    wv_sendText(wsi, response);

  /* "setPmtr;ipmtr;irow;icol;value; " */
  } else if (strncmp(text, "setPmtr", 7) == 0) {

    /* extract arguments */
    ipmtr = 0;
    irow  = 0;
    icol  = 0;
    if (getToken(text, 1, arg1)) ipmtr = strtol(arg1, &pEnd, 10);
    if (getToken(text, 2, arg2)) irow  = strtol(arg2, &pEnd, 10);
    if (getToken(text, 3, arg3)) icol  = strtol(arg3, &pEnd, 10);

    err = -999;
    j   =  0;
    for (i = 1; i <= nParams; i++) {
      status = gem_getParam(model, i, &name, &bflg, &order, &type, &len,
                            &ints, &reals, &string, &spline, &nattr);
      if (status != GEM_SUCCESS) continue;
      if ((type == GEM_STRING) || (type == GEM_SPLINE)) continue;
      j++;
      if (j == ipmtr) {
        if (getToken(text, 4, arg4) == 0) break;
        nints  = NULL;
        nreals = NULL;
        if (type == GEM_REAL) {
          nreals = (double *) malloc(len*sizeof(double));
          if (nreals != NULL) {
            for (k = 0; k < len; k++) nreals[k] = reals[k];
            nreals[icol-1] = strtod(arg4, &pEnd);
          }
        } else {
          nints = (int *) malloc(len*sizeof(int));
          if (nints != NULL) {
            for (k = 0; k < len; k++) nints[k] = ints[k];
            nints[icol-1] = strtol(arg4, &pEnd, 10);
          }
        }
        err = gem_setParam(model, i, len, nints, nreals, string, spline);
        if (nints  != NULL) free(nints);
        if (nreals != NULL) free(nreals);
        break;
      }
    }

    /* build the response */
    if (err == GEM_SUCCESS) {
      sprintf(response, "setPmtr;");
    } else {
      sprintf(response, "ERROR:: setPmtr(%d, %d, %d) -> %d", ipmtr, irow, icol, err);
    }

    /* send the response */
    wv_sendText(wsi, response);

  /* "getBrchs;" */
  } else if (strncmp(text, "getBrchs;", 9) == 0) {

    /* build the response in JSON format */
    sprintf(response, "getBrchs;[");

    for (ibrch = 1; ibrch <= nBranch; ibrch++) {
      status = gem_getBranch(model, ibrch, &name, &string, &sup, &np, &par,
                             &nc, &chld, &nattr);
      sprintf(entry, "{\"name\":\"%s\",\"type\":\"%s\",\"actv\":%d,\"attrs\":[",
              name, string, sup);
      strcat(response, entry);

      for (j = iattr = 0; iattr < nattr; iattr++) {
        status = gem_getAttribute(model, GEM_BRANCH, ibrch, iattr+1, &aname, &type,
                                  &len, &ints, &reals, &string);
        if (type != GEM_STRING) continue;
        if (j != 0) strcat(response, ",");
        j++;
        sprintf(entry, "[\"%s\",\"%s\"]", aname, string);
        strcat(response, entry);
      }
        
      sprintf(entry, "],\"ileft\":%d,\"irite\":%d,\"ichld\":%d,\"args\":[",
              0, 0, 0); 
      strcat(response, entry);

      if (ibrch < nBranch) {
        sprintf(entry, "]},");
      } else {
        sprintf(entry, "]}]");
      }
      strcat(response, entry);
    }
    if (nBranch < 1) {
      sprintf(entry, "]");
      strcat(response, entry);
    }

    /* send the response */
    wv_sendText(wsi, response);
   
  /* "toglBrch;ibrch;" */
  } else if (strncmp(text, "toglBrch;", 9) == 0) {

    /* extract arguments */
    ibrch = 0;

    if (getToken(text, 1, arg1)) ibrch = strtol(arg1, &pEnd, 10);

    /* build the response */

      status = gem_getBranch(model, ibrch, &name, &string, &sup, &np, &par,
                             &nc, &chld, &nattr);
      if (status != GEM_SUCCESS) {
        sprintf(response, "ERROR:: toglBrch(%d) -> %d", ibrch, status);
      } else {
        if (sup == GEM_ACTIVE) {
          sup    = GEM_SUPPRESSED;
          status = gem_setSuppress(model, ibrch, sup);
          if (status == GEM_SUCCESS) {
            sprintf(response, "toglBrch;");
          } else {
            sprintf(response, "ERROR:: toglBrch(%d) --> %d", ibrch, status);
          }
        } else if (sup == GEM_SUPPRESSED) {
          sup    = GEM_ACTIVE;
          status = gem_setSuppress(model, ibrch, sup);
          if (status == GEM_SUCCESS) {
            sprintf(response, "toglBrch;");
          } else {
            sprintf(response, "ERROR:: toglBrch(%d) --> %d", ibrch, status);
          }
        } else {
          sprintf(response, "ERROR:: toglBrch(%d) = %d", ibrch, sup);
        } 
        
      }

    /* send the response */
    wv_sendText(wsi, response);

  /* "save;filename" */
  } else if (strncmp(text, "save;", 5) == 0) {

    /* extract argument */
    getToken(text, 1, arg1);

    /* save the file */
    status = gem_saveModel(model, arg1);

    /* build the response */
    if (status == GEM_SUCCESS) {
      sprintf(response, "save;");
    } else {
      sprintf(response, "ERROR:: save(%s) -> %d", arg1, status);
    }

    /* send the response */
    wv_sendText(wsi, response);

  /* "build;"  */
  } else if (strncmp(text, "build;", 6) == 0) {

    /* build the response */
    status = gem_regenModel(model);
    if (status == GEM_SUCCESS) {
      status = gem_getModel(model, &string, &name, &aname, &k,
                            &nBRep, &BReps, &nParams, &nBranch, &nattr);
      makeTess(1);
    }

    if (status == GEM_SUCCESS) {
      sprintf(response, "build;0;%d", nBRep);
    } else {
      sprintf(response, "ERROR:: build() -> %d", status);
    }

    /* send the response  */
    wv_sendText(wsi, response);

  }
    
}
