/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Save Function -- EGADS & OpenCSM
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
#define strcasecmp  stricmp
#endif


#include "egads.h"
#include "gem.h"
#include "memory.h"

/* OpenCSM Includes */
#include "common.h"
#include "OpenCSM.h"


  extern ego dia_context;


int
gem_kernelSave(gemModel *model, /*@null@*/ char *filename)
{
  int     i, j, len, stat;
  void    *modl;
  char    *name;
  ego     *bodies, body, emdl;
  gemBody *gbody;

  if  (model == NULL) return GEM_NULLOBJ;

  /* parametric model (OpenCSM) save */
  
  if (model->handle.index == 1) {
    modl = model->handle.ident.ptr;
    i    = 1;
    if (filename == NULL) {
      name = model->location;
    } else {
      name = filename;
      len  = strlen(name);
      for (i = len-1; i > 0; i--)
        if (name[i] == '.') break;
      if (i == 0) return GEM_BADNAME;
      if (strcasecmp(&name[i],".csm") != 0) i = 0;
    }
    if (i != 0) return ocsmSave(modl, name);
  }

  /* save EGADS model */

  if  (model->nBRep == 0) return GEM_BADVALUE;
  if ((model->handle.index == 0) &&
      (model->handle.ident.ptr != NULL)) {
    emdl = (ego) model->handle.ident.ptr;
    name = filename;
    if (filename == NULL) name = model->location;
    if (name == NULL) return GEM_NULLNAME;
    return EG_saveModel(emdl, name);
  }
  if (name == NULL) return GEM_NULLNAME;

  /* from static -- make EGADS model, save and release */
  
  bodies = (ego *) gem_allocate(model->nBRep*sizeof(ego));
  if (bodies == NULL) return GEM_ALLOC;
  for (i = 0; i < model->nBRep; i++) {
    gbody = model->BReps[i]->body;
    body  = (ego) gbody->handle.ident.ptr;
    stat  = EG_copyObject(body, NULL, &bodies[i]);
    if (stat != EGADS_SUCCESS) {
      for (j = 0; j < i; j++) EG_deleteObject(bodies[j]);
      gem_free(bodies);
      return stat;
    }
  }
  stat = EG_makeTopology(dia_context, NULL, MODEL, 0, NULL, 
                         model->nBRep, bodies, NULL, &emdl);
  gem_free(bodies);
  if (stat != EGADS_SUCCESS) return stat;
  stat = EG_saveModel(emdl, filename);
  EG_deleteObject(emdl);

  return stat;
}
