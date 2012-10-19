/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Load Function -- OpenCSM & EGADS
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <strings.h>
#else
#define snprintf   _snprintf
#define strcasecmp  stricmp
#endif

#include "gem.h"
#include "memory.h"
#include "attribute.h"
#include "egads.h"

/* OpenCSM Defines & Includes */
#include "common.h"
#include "OpenCSM.h"

#define MAX_BODYS     999
  static int  nbody =   0;		/* number of built Bodys */
  static int  bodyList[MAX_BODYS];	/* array of built Bodys */


  extern ego  dia_context;
  
  extern void gem_releaseBRep(/*@only@*/ gemBRep *brep);
  extern void gem_clrModel(/*@only@*/ gemModel *model);


static void
gem_matIdent(double *xform)
{
  int i;

  for (i = 0; i < 12; i++) xform[i] = 0.0;
  xform[0] = xform[5] = xform[10] = 1.0;
}


static int
gem_lookup(ego object, int n, /*@null@*/ ego *objs)
{
  int i, j;
  
  if (objs == NULL) return 0;

  for (j = i = 0; i < n; i++) {
    if (objs[i] == NULL) continue;
    if (objs[i] == object) return j+1;
    j++;
  }
    
  return 0;
}


static void
gem_fillAttrs(ego object, gemAttrs **attrx)
{
  int    i, stat, len, atype, nattr, *ivec;
  char   *name, *str;
  double *rvec;
  
  stat = EG_attributeNum(object, &nattr);
  if (stat != EGADS_SUCCESS) return;
  if (nattr == 0) return;
  
  for (i = 1; i <= nattr; i++) {
    stat = EG_attributeGet(object, i, (const char **) &name, &atype, &len, 
                           (const int **)  &ivec, (const double **) &rvec,
                           (const char **) &str);
    if (stat != EGADS_SUCCESS) continue;
    gem_setAttrib(attrx, name, atype, len, ivec, rvec, str);
  }
  
}


int
gem_diamondBody(ego object, /*@null@*/ char *bID, gemBRep *brep)
{
  int          i, j, k, m, n, len, stat, oclass, mtype, nobjs, *senses;
  int          nshell, nface, nloop, nedge, nnode, atype, alen;
  const int    *ints;
  double       limits[4];
  const double *reals;
  char         buffer[1025];
  const char   *string;
  ego          geom, *objs;
  ego          *shells, *faces, *loops, *edges, *nodes;
  gemID        gid;
  gemBody      *body;

  stat = EG_getTopology(object, &geom, &oclass, &mtype, limits, &nobjs, &objs,
                        &senses);
  if (stat   != EGADS_SUCCESS) return stat;
  if (oclass != BODY) return EGADS_NOTBODY;
  gid.index     = 0;
  gid.ident.ptr = object;
  
  body = (gemBody *) gem_allocate(sizeof(gemBody));
  if (body == NULL) return GEM_ALLOC;
  body->handle = gid;
  body->nnode  = 0;
  body->nodes  = NULL;
  body->nedge  = 0;
  body->edges  = NULL;
  body->nloop  = 0;
  body->loops  = NULL;
  body->nface  = 0;
  body->faces  = NULL;
  body->nshell = 0;
  body->shells = NULL;
  body->attr   = NULL;
  body->type   = GEM_SOLID;
  if ((mtype == SHEETBODY) || (mtype == FACEBODY)) body->type = GEM_SHEET;
  if  (mtype == WIREBODY) body->type = GEM_WIRE;
  brep->body = body;
  gem_matIdent(brep->xform);
  gem_matIdent(brep->invXform);
  stat = EG_getBoundingBox(object, body->box);
  if (stat != EGADS_SUCCESS) return stat;
  
  nodes  = NULL;
  edges  = NULL;
  loops  = NULL;
  faces  = NULL;
  shells = NULL;
  stat   = EG_getBodyTopos(object, NULL, NODE,  &nnode,  &nodes);
  if (stat != EGADS_SUCCESS) goto ret;
  stat   = EG_getBodyTopos(object, NULL, EDGE,  &nedge,  &edges);
  if (stat != EGADS_SUCCESS) goto ret;
  stat   = EG_getBodyTopos(object, NULL, LOOP,  &nloop,  &loops);
  if (stat != EGADS_SUCCESS) goto ret;
  stat   = EG_getBodyTopos(object, NULL, FACE,  &nface,  &faces);
  if (stat != EGADS_SUCCESS) goto ret;
  stat   = EG_getBodyTopos(object, NULL, SHELL, &nshell, &shells);
  if (stat != EGADS_SUCCESS) goto ret;

  /* make the nodes */
  if ((nnode != 0) && (nodes != NULL)) {
    stat = GEM_ALLOC;
    body->nodes = (gemNode *) gem_allocate(nnode*sizeof(gemNode));
    if (body->nodes == NULL) goto ret;
    for (i = 0; i < nnode; i++) body->nodes[i].attr = NULL;
    body->nnode = nnode;
    for (i = 0; i < nnode; i++) {
      stat = EG_getTopology(nodes[i], &geom, &oclass, &mtype, limits, &nobjs,
                            &objs, &senses);
      if (stat != EGADS_SUCCESS) goto ret;
      gid.ident.ptr         = nodes[i];
      body->nodes[i].handle = gid;
      body->nodes[i].xyz[0] = limits[0];
      body->nodes[i].xyz[1] = limits[1];
      body->nodes[i].xyz[2] = limits[2];
      gem_fillAttrs(nodes[i], &body->nodes[i].attr);
    }
  }

  /* make the edges */
  if ((nedge != 0) && (edges != NULL)) {
    stat = GEM_ALLOC;
    body->edges = (gemEdge *) gem_allocate(nedge*sizeof(gemEdge));
    if (body->edges == NULL) goto ret;
    for (i = 0; i < nedge; i++) {
      body->edges[i].faces[0] = body->edges[i].faces[1] = 0;
      body->edges[i].attr     = NULL;
    }
    body->nedge = nedge;
    for (n = i = 0; i < nedge; i++) {
      stat = EG_getTopology(edges[i], &geom, &oclass, &mtype, limits, &nobjs,
                            &objs, &senses);
      if (stat != EGADS_SUCCESS) goto ret;
      if (mtype == DEGENERATE) {
        edges[i] = NULL;
        continue;
      }
      gid.ident.ptr             = edges[i];
      body->edges[n].handle     = gid;
      body->edges[n].tlimit[0]  = limits[0];
      body->edges[n].tlimit[1]  = limits[1];
      body->edges[n].nodes[0]   = gem_lookup(objs[0], nnode, nodes);
      if (nobjs == 1) {
        body->edges[n].nodes[1] = body->edges[n].nodes[0];
      } else {
        body->edges[n].nodes[1] = gem_lookup(objs[1], nnode, nodes);
      }
      gem_fillAttrs(edges[i], &body->edges[n].attr);
      n++;
    }
    body->nedge = n;
  }
  
  /* make the loops */
  if ((nloop != 0) && (loops != NULL)) {
    stat = GEM_ALLOC;
    body->loops = (gemLoop *) gem_allocate(nloop*sizeof(gemLoop));
    if (body->loops == NULL) goto ret;
    for (i = 0; i < nloop; i++) {
      body->loops[i].edges = NULL;
      body->loops[i].attr  = NULL;
    }
    body->nloop = nloop;
    for (i = 0; i < nloop; i++) {
      stat = EG_getTopology(loops[i], &geom, &oclass, &mtype, limits, &nobjs,
                            &objs, &senses);
      if (stat != EGADS_SUCCESS) goto ret;
      gid.ident.ptr         = loops[i];
      body->loops[i].handle = gid;
      body->loops[i].type   = 0;
      body->loops[i].face   = 0;
      body->loops[i].edges  = (int *) gem_allocate(nobjs*sizeof(int));
      stat = GEM_ALLOC;
      if (body->loops[i].edges == NULL) goto ret;
      for (n = j = 0; j < nobjs; j++) {
        k = gem_lookup(objs[j], nedge, edges);
        if (k == 0) continue;
        body->loops[i].edges[n] = senses[j]*k;
        n++;
      }
      body->loops[i].nedges = n;
      gem_fillAttrs(loops[i], &body->loops[i].attr);
    }
  }
  
  /* make the faces */
  if ((nface != 0) && (faces != NULL)) {
    len = 0;
    if (bID != NULL) len = strlen(bID);
    stat = GEM_ALLOC;
    body->faces = (gemFace *) gem_allocate(nface*sizeof(gemFace));
    if (body->faces == NULL) goto ret;
    for (i = 0; i < nface; i++) {
      body->faces[i].loops = NULL;
      body->faces[i].ID    = NULL;
      body->faces[i].attr  = NULL;
    }
    body->nface = nface;
    for (i = 0; i < nface; i++) {
      stat = EG_getTopology(faces[i], &geom, &oclass, &mtype, limits, &nobjs,
                            &objs, &senses);
      if (stat != EGADS_SUCCESS) goto ret;
      gid.ident.ptr           = faces[i];
      body->faces[i].handle   = gid;
      body->faces[i].uvbox[0] = limits[0];
      body->faces[i].uvbox[1] = limits[1];
      body->faces[i].uvbox[2] = limits[2];
      body->faces[i].uvbox[3] = limits[3];
      body->faces[i].norm     = mtype;
      body->faces[i].loops    = (int *) gem_allocate(nobjs*sizeof(int));
      stat = GEM_ALLOC;
      if (body->faces[i].loops == NULL) goto ret;
      body->faces[i].nloops   = nobjs;
      for (j = 0; j < nobjs; j++) {
        k = gem_lookup(objs[j], nloop, loops);
        body->faces[i].loops[j] = k;
        if (k == 0) continue;
        if (body->loops == NULL) continue;
        body->loops[k-1].face = i+1;
        if (senses[j] == -1) body->loops[k-1].type = 1;
        /* set face markers in edge struct */
        if (body->edges == NULL) continue;
        for (n = 0; n < body->loops[k-1].nedges; n++) {
          m = body->loops[k-1].edges[n];
          if (m < 0) {
            if (body->edges[-m-1].faces[0] != 0)
              printf(" Diamond Internal: -Face for Edge %d = %d %d\n",
                     -m, body->edges[-m-1].faces[0], i+1);
            body->edges[-m-1].faces[0] = i+1;
          } else {
            if (body->edges[m-1].faces[1]  != 0)
              printf(" Diamond Internal: +Face for Edge %d = %d %d\n",
                      m, body->edges[m-1].faces[1],  i+1);
            body->edges[m-1].faces[1]  = i+1;
          }
        }
      }
      if (bID == NULL) {
        stat = EG_attributeRet(faces[i], "body", &atype, &alen, &ints,
                               &reals, &string);
        if ((stat != EGADS_SUCCESS) || (atype != ATTRINT)) {
          printf(" Diamond Internal: OpenCSM FaceID for %d = %d,  atype = %d\n",
                 i+1, stat, atype);
        } else {
          snprintf(buffer, 1024, "%d:%d", ints[0], ints[1]);
          for (j = 2; j < alen; j+=2) {
            m = strlen(buffer);
            if (m >= 1024) break;
            snprintf(&buffer[m], 1024-m, "::%d:%d", ints[j], ints[j+1]);
          }
          buffer[1024]      = 0;
          body->faces[i].ID = gem_strdup(buffer);
        }
      } else {
        body->faces[i].ID = (char *) gem_allocate((len+10)*sizeof(char));
        if (body->faces[i].ID == NULL) goto ret;
        snprintf(body->faces[i].ID, len+10, "%s:%d", bID, i+1);
      }
      gem_fillAttrs(faces[i], &body->faces[i].attr);
    }
  }
  
  /* make the shells */
  if ((nshell != 0) && (shells != NULL)) {
    stat = GEM_ALLOC;
    body->shells = (gemShell *) gem_allocate(nshell*sizeof(gemShell));
    if (body->shells == NULL) goto ret;
    for (i = 0; i < nshell; i++) {
      body->shells[i].faces = NULL;
      body->shells[i].attr  = NULL;
    }
    body->nshell = nshell;
    for (i = 0; i < nshell; i++) {
      stat = EG_getTopology(shells[i], &geom, &oclass, &mtype, limits, &nobjs,
                            &objs, &senses);
      if (stat != EGADS_SUCCESS) goto ret;
      gid.ident.ptr          = shells[i];
      body->shells[i].handle = gid;
      body->shells[i].type   = 0;
      body->shells[i].faces  = (int *) gem_allocate(nobjs*sizeof(int));
      stat = GEM_ALLOC;
      if (body->shells[i].faces == NULL) goto ret;
      body->shells[i].nfaces = nobjs;
      for (j = 0; j < nobjs; j++)
        body->shells[i].faces[j] = gem_lookup(objs[j], nface, faces);
      gem_fillAttrs(shells[i], &body->shells[i].attr);
    }
    /* set the type for solids */
    if ((body->type == GEM_SOLID) && (nshell > 1)) {
      EG_getTopology(object, &geom, &oclass, &mtype, limits, &nobjs, &objs,
                     &senses);
      for (j = 0; j < nobjs; j++) {
        if (senses[j] == 1) continue;
        k = gem_lookup(objs[j], nshell, shells);
        if (k == 0) continue;
        body->shells[k-1].type = 1;
      }
    }
  }

  gem_fillAttrs(object, &body->attr);
  stat = EGADS_SUCCESS;
  
ret:
  if (shells != NULL) EG_free(shells);
  if (faces  != NULL) EG_free(faces);
  if (loops  != NULL) EG_free(loops);
  if (edges  != NULL) EG_free(edges);
  if (nodes  != NULL) EG_free(nodes);
  return stat;
}


int
gem_fillModelE(ego object, char *name, int ext, 
               int *nBRep, gemBRep ***BReps)
{
  int     i, j, stat, beg, len, oclass, mtype, nobjs, *senses;
  double  limits[4];
  char    *bID;
  ego     geom, *objs;
  gemID   handle;
  gemBRep **breps;
  
  *nBRep = 0;
  *BReps = NULL;
  stat   = EG_getTopology(object, &geom, &oclass, &mtype, limits, &nobjs,
                          &objs, &senses);
  if (stat   != EGADS_SUCCESS) return stat;
  if (oclass != MODEL) return EGADS_NOTMODEL;
  if (nobjs == 0) return EGADS_NODATA;

  *nBRep = nobjs;
  *BReps = breps = (gemBRep **) gem_allocate(nobjs*sizeof(gemBRep *));
  if (breps == NULL) return GEM_ALLOC;

  handle.index     = 0;
  handle.ident.ptr = object;
  for (i = 0; i < nobjs; i++) {
    breps[i] = (gemBRep *) gem_allocate(sizeof(gemBRep));
    if (breps[i] == NULL) {
      for (j = 0; j < i; j++) gem_free(breps[j]);
      gem_free(*BReps);
      return GEM_ALLOC;
    }
    breps[i]->magic   = GEM_MBREP;
    breps[i]->omodel  = NULL;
    breps[i]->phandle = handle;
    breps[i]->ibranch = 0;
    breps[i]->inumber = 0;
    breps[i]->body    = NULL;
  }
  /* find root name */
  for (beg = ext-1; beg >= 0; beg--)
    if ((name[beg] == '/') || (name[beg] == '\\') ||
        (name[beg] == ':')) break;
  beg++;
  len = ext - beg + 10;
  bID = (char *) gem_allocate(len*sizeof(char));
  if (bID == NULL) {
    for (j = 0; j < nobjs; j++)
      gem_releaseBRep(breps[j]);
    gem_free(*BReps);
    return GEM_ALLOC;
  }
  
  for (i = 0; i < nobjs; i++) {
    snprintf(bID, len, "%s:%d", &name[beg], i+1);
    stat = gem_diamondBody(objs[i], bID, breps[i]);
    if (stat != EGADS_SUCCESS) {
      for (j = 0; j < nobjs; j++)
        gem_releaseBRep(breps[j]);
      gem_free(*BReps);
      gem_free(bID);
      return GEM_ALLOC;
    }
  }
  
  gem_free(bID);
  return GEM_SUCCESS;
}


static int
gem_fillModelO(void *modl, gemModel *mdl)
{
  int      i, j, k, n, stat, nbrch, npmtr, type, class, actv;
  int      ichld, ileft, irite, narg, nattr, nroot, nrow, ncol;
  char     name[129], value[129];
  double   real;
  gemAttr  *attr;
  gemAttrs *attrs;
  gemParam *params;
  gemFeat  *branches;

  stat = ocsmInfo(modl, &nbrch, &npmtr, &nbody);
  if (stat != SUCCESS) return stat;
  
  /* do the branches */
  
  if (nbrch > 0) {
    branches = (gemFeat *) gem_allocate((nbrch+1)*sizeof(gemFeat));
    if (branches == NULL) return GEM_ALLOC;
    for (i = 0; i <= nbrch; i++) {
      branches[i].name             = NULL;
      branches[i].handle.index     = i;
      branches[i].handle.ident.ptr = modl;
      branches[i].branchType       = NULL;
      branches[i].nParents         = 2;
      branches[i].parents.pnodes   = NULL;
      branches[i].nChildren        = 1;
      branches[i].children.node    = 0;
      branches[i].attr             = NULL;
      branches[i].changed          = 0;
    }
    mdl->nBranches            = nbrch+1;
    mdl->Branches             = branches;
    branches[0].name          = gem_strdup("OpenCSM");
    branches[0].branchType    = gem_strdup("PART");
    branches[0].nParents      = 0;
    branches[0].sflag         = GEM_READONLY;
    
    nroot  = 0;
    for (i = 1; i <= nbrch; i++) {
      stat = ocsmGetName(modl, i, name);
      if (stat != SUCCESS) return stat;
      branches[i].name = gem_strdup(name);
      stat = ocsmGetBrch(modl, i, &type, &class, &actv, &ichld,
                         &ileft, &irite, &narg, &nattr);
      if (stat != SUCCESS) return stat;
      if ((irite == -1) || (ileft == -1)) {
        if ((irite == -1) && (ileft == -1)) {
          branches[i].nParents      = 1;
          branches[i].parents.pnode = 1;
          nroot++;
        } else {
          branches[i].nParents      = 1;
          if (ileft == -1) ileft    = irite;
          branches[i].parents.pnode = ileft+1;
          if (ileft == 0) nroot++;
        }
      } else {
        if (ileft == irite) {
          branches[i].nParents      = 1;
          branches[i].parents.pnode = ileft+1;
          if (ileft == 0) nroot++;
        } else {
          branches[i].parents.pnodes = (int *) gem_allocate(2*sizeof(int));
          if (branches[i].parents.pnodes == NULL) return GEM_ALLOC;
          branches[i].parents.pnodes[0] = ileft+1;
          branches[i].parents.pnodes[1] = irite+1;
        }
      }
      branches[i].branchType    = gem_strdup(ocsmGetText(type));
      branches[i].children.node = ichld+1;
      branches[i].sflag         = GEM_ACTIVE;
      if (actv == OCSM_INACTIVE)   branches[i].sflag = GEM_INACTIVE;
      if (actv == OCSM_SUPPRESSED) branches[i].sflag = GEM_SUPPRESSED;
      if (branches[i].children.node <= 1) branches[i].nChildren = 0;
      if (nattr <= 0) continue;
      attrs = (gemAttrs *) gem_allocate(sizeof(gemAttrs));
      if (attrs == NULL) continue;
      attr = (gemAttr *) gem_allocate(nattr*sizeof(gemAttr));
      if (attr == NULL) {
        gem_free(attrs);
        continue;
      }
      attrs->nattrs    = 0;
      attrs->mattrs    = nattr;
      attrs->attrs     = attr;
      branches[i].attr = attrs;
      for (j = 0; j < nattr; j++) {
        stat = ocsmRetAttr(modl, i, j+1, name, value);
        if (stat != SUCCESS) continue;
        attr[attrs->nattrs].name     = gem_strdup(name);
        if (attr[attrs->nattrs].name == NULL) continue;
        attr[attrs->nattrs].length   = strlen(value);
        attr[attrs->nattrs].type     = GEM_STRING;
        attr[attrs->nattrs].integers = NULL;
        attr[attrs->nattrs].reals    = NULL;
        attr[attrs->nattrs].string   = gem_strdup(value);
        attrs->nattrs += 1;
      }
    }
    
    /* set the root's children */
    branches[0].nChildren = nroot;
    if (nroot == 1) {
      branches[0].children.node = 0;
      for (i = 1; i <= nbrch; i++) {
        stat = ocsmGetBrch(modl, i, &type, &class, &actv, &ichld,
                           &ileft, &irite, &narg, &nattr);
        if (stat != SUCCESS) return stat;
        if ((irite == 0) && (ileft == 0)) {
          branches[0].children.node = i+1;
          break;
        }
        if ((irite == -1) || (ileft == -1)) {
          if (ileft == -1) ileft = irite;
          if (ileft == 0) {
            branches[0].children.node = i+1;
            break;
          }         
        }
        if ((irite != -1) || (ileft != -1)) continue;
        branches[0].children.node = i+1;
        break;
      }
    } else {
      branches[0].children.nodes = (int *) gem_allocate(nroot*sizeof(int));
      if (branches[0].children.nodes == NULL) return GEM_ALLOC;
      nroot  = 0;
      for (i = 1; i <= nbrch; i++) {
        stat = ocsmGetBrch(modl, i, &type, &class, &actv, &ichld,
                           &ileft, &irite, &narg, &nattr);
        if (stat != SUCCESS) return stat;
        if ((irite == 0) && (ileft == 0)) {
          branches[0].children.nodes[nroot] = i+1;
          nroot++;
        }
        if ((irite == -1) || (ileft == -1)) {
          if (ileft == -1) ileft = irite;
          if (ileft == 0) {
            branches[0].children.nodes[nroot] = i+1;
            nroot++;
          }         
        }
        if ((irite != -1) || (ileft != -1)) continue;
        branches[0].children.nodes[nroot] = i+1;
        nroot++;
      }
    }
  }
  
  /* do the parameters */

  if (npmtr <= 0)  return GEM_SUCCESS;
  params = (gemParam *) gem_allocate(npmtr*sizeof(gemParam));
  if (params == NULL) return GEM_ALLOC;
  for (i = 0; i < npmtr; i++) {
    params[i].name             = NULL;
    params[i].handle.index     = i+1;
    params[i].handle.ident.ptr = modl;
    params[i].type             = GEM_REAL;
    params[i].order            = 0;
    params[i].bitflag          = 0;
    params[i].len              = 1;
    params[i].vals.real        = 0.0;
    params[i].bnds.rlims[0]    = 0.0;
    params[i].bnds.rlims[1]    = 0.0;
    params[i].attr             = NULL;
    params[i].changed          = 0;
  }
  mdl->nParams = npmtr;
  mdl->Params  = params;

  for (i = 0; i < npmtr; i++) {
    stat = ocsmGetPmtr(modl, i+1, &type, &nrow, &ncol, name);
    if (stat != SUCCESS) return stat;
    params[i].name = gem_strdup(name);
    if ((nrow == 1) && (ncol == 1)) {
      stat = ocsmGetValu(modl, i+1, 1, 1, &real);
      if (stat != SUCCESS) return stat;
      params[i].vals.real = real;
    } else {
      params[i].len        = nrow*ncol;
      params[i].vals.reals = (double *) 
                             gem_allocate(params[i].len*sizeof(double));
      if (params[i].vals.reals == NULL) return GEM_ALLOC;
      for (n = j = 0; j < ncol; j++)
        for (k = 0; k < nrow; k++, n++) {
          stat = ocsmGetValu(modl, i+1, k+1, j+1, &real);
          if (stat != SUCCESS) return stat;
          params[i].vals.reals[n] = real;
        }
    }
    if (type == OCSM_INTERNAL) params[i].bitflag = 2;
  }

  return GEM_SUCCESS;
}


int
gem_kernelLoad(gemCntxt *gem_cntxt, /*@null@*/ char *server, 
               char *name, gemModel **model)
{
  int      i, j, len, stat, ibody, buildTo, builtTo, nBRep, ierror;
  ego      obj, bobj;
  gemID    gid;
  gemModel *mdl, *prev;
  gemBRep  **BReps;
  FILE     *fp;
  void     *modl;
  modl_T   *MODL;

  *model = NULL;
  if (gem_cntxt == NULL) return GEM_NULLOBJ;
  if (gem_cntxt->magic != GEM_MCONTEXT) return GEM_BADCONTEXT;
  if (name == NULL) return EGADS_NONAME;

  /* does file exist? */

  fp = fopen(name, "r");
  if (fp == NULL) return EGADS_NOTFOUND;
  fclose(fp);

  /* find extension */

  len = strlen(name);
  for (i = len-1; i > 0; i--)
    if (name[i] == '.') break;
  if (i == 0) return EGADS_NODATA;

  if (strcasecmp(&name[i],".csm") == 0) {

    /* do an OpenCSM load */
    stat = ocsmLoad(name, &modl);
    if (stat < SUCCESS) return stat;
    MODL = (modl_T *) modl;
    MODL->context = dia_context;
    
    /* check that Branches are properly ordered */
    stat = ocsmCheck(modl);
    if (stat < SUCCESS) {
      ocsmFree(modl);
      return stat;
    }

    buildTo = 0;	/* all */
    nbody   = MAX_BODYS;
    stat    = ocsmBuild(modl, buildTo, &builtTo, &nbody, bodyList);
    EG_deleteObject(dia_context);	/* clean up after build */
    if (stat != SUCCESS) {
      ocsmFree(modl);
      return stat;
    }
    nBRep = nbody;
    
    /* put away the BReps */
    BReps = (gemBRep **) gem_allocate(nBRep*sizeof(gemBRep *));
    if (BReps == NULL) {
      for (j = 0; j < nbody; j++) {
        ibody = bodyList[j];
        bobj  = MODL->body[ibody].ebody;
        EG_deleteObject(bobj);
      }
      ocsmFree(modl);
      return GEM_ALLOC;
    }

    gid.index     = 1;
    gid.ident.ptr = modl;
    for (i = 0; i < nBRep; i++) {
      BReps[i] = (gemBRep *) gem_allocate(sizeof(gemBRep));
      if (BReps[i] == NULL) {
        for (j = 0; j < i; j++) gem_free(BReps[j]);
        gem_free(BReps);
        for (j = 0; j < nbody; j++) {
          ibody = bodyList[j];
          bobj  = MODL->body[ibody].ebody;
          EG_deleteObject(bobj);
        }
        ocsmFree(modl);
        return GEM_ALLOC;
      }
      BReps[i]->magic   = GEM_MBREP;
      BReps[i]->omodel  = NULL;
      BReps[i]->phandle = gid;
      BReps[i]->ibranch = 0;
      BReps[i]->inumber = 0;
      BReps[i]->body    = NULL;
    }
  
    for (i = 0; i < nbody; i++) {
      ibody = bodyList[i];
      bobj  = MODL->body[ibody].ebody;
      stat  = gem_diamondBody(bobj, NULL, BReps[i]);
      if (stat != EGADS_SUCCESS) {
        for (j = 0; j < nBRep; j++) gem_releaseBRep(BReps[j]);
        gem_free(BReps);
        for (j = 0; j < nbody; j++) {
          ibody = bodyList[j];
          bobj  = MODL->body[ibody].ebody;
          EG_deleteObject(bobj);
        }
        ocsmFree(modl);
        return stat;
      }
    }

    /* make the GEM model */
    mdl = (gemModel *) gem_allocate(sizeof(gemModel));
    if (mdl == NULL) {
      for (j = 0; j < nBRep; j++) gem_releaseBRep(BReps[j]);
      gem_free(BReps);
      for (j = 0; j < nBRep; j++) {
        ibody = bodyList[j];
        bobj = MODL->body[ibody].ebody;
        EG_deleteObject(bobj);
      }
      ocsmFree(modl);
      return GEM_ALLOC;
    }

    mdl->magic     = GEM_MMODEL;
    mdl->handle    = gid;
    mdl->nonparam  = 0;
    mdl->server    = gem_strdup(server);
    mdl->location  = gem_strdup(name);
    mdl->modeler   = gem_strdup("OpenCSM");
    mdl->nBRep     = nBRep;
    mdl->BReps     = BReps;
    mdl->nParams   = 0;
    mdl->Params    = NULL;
    mdl->nBranches = 0;
    mdl->Branches  = NULL;
    mdl->attr      = NULL;
    mdl->prev      = (gemModel *) gem_cntxt;
    mdl->next      = NULL;
    for (i = 0; i < nBRep; i++) BReps[i]->omodel = mdl;

    /* fill in the master-model info */
    stat = gem_fillModelO(modl, mdl);
    if (stat != SUCCESS) {
      gem_clrModel(mdl);
      for (j = 0; j < nBRep; j++) {
        ibody = bodyList[j];
        bobj  = MODL->body[ibody].ebody;
        EG_deleteObject(bobj);
      }
      ocsmFree(modl);
      return stat;
    }
    
    /* set our reference */
    prev = gem_cntxt->model;
    gem_cntxt->model = mdl;
    if (prev != NULL) {
      mdl->next  = prev;
      prev->prev = gem_cntxt->model;
    }

  } else {

    stat = EG_loadModel(dia_context, 0, name, &obj);
    if (stat != EGADS_SUCCESS) return stat;

    /* fill up the BReps */
    
    stat = gem_fillModelE(obj, name, i, &nBRep, &BReps);
    if (stat != EGADS_SUCCESS) return stat;

    mdl = (gemModel *) gem_allocate(sizeof(gemModel));
    if (mdl == NULL) return GEM_ALLOC;

    gid.index      = 0;
    gid.ident.ptr  = obj;

    mdl->magic     = GEM_MMODEL;
    mdl->handle    = gid;
    mdl->nonparam  = 1;
    mdl->server    = gem_strdup(server);
    mdl->location  = gem_strdup(name);
    mdl->modeler   = gem_strdup("EGADS");
    mdl->nBRep     = nBRep;
    mdl->BReps     = BReps;
    mdl->nParams   = 0;
    mdl->Params    = NULL;
    mdl->nBranches = 0;
    mdl->Branches  = NULL;
    mdl->attr      = NULL;
    mdl->prev      = (gemModel *) gem_cntxt;
    mdl->next      = NULL;
    for (i = 0; i < nBRep; i++) BReps[i]->omodel = mdl;

    prev = gem_cntxt->model;
    gem_cntxt->model = mdl;
    if (prev != NULL) {
      mdl->next  = prev;
      prev->prev = gem_cntxt->model;
    }

  }

  *model = mdl;
  return GEM_SUCCESS;
}
