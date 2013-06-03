/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Branch Attribute Function -- OpenCSM
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "egads.h"
#include "gem.h"

/* OpenCSM Includes */
#include "common.h"
#include "OpenCSM.h"


int
gem_kernelBranchAttr(gemID handle, int branch, char *name, int atype, 
                     int alen, /*@null@*/ int *ints, /*@null@*/ double *reals,
                     /*@null@*/ char *string)
{
  int  stat, nbrch, npmtr, nbody;
  char buffer[33];
  void *modl;

  if (alen != 1) return GEM_BADRANK;
  if ((atype < GEM_BOOL) || (atype > GEM_STRING)) return GEM_BADTYPE;
  if (handle.index == 0) return GEM_NOTPARMTRIC;

  modl = handle.ident.ptr;
  stat = ocsmInfo(modl, &nbrch, &npmtr, &nbody);
  if (stat != SUCCESS) return stat;
  if ((branch < 2) || (branch > nbrch+1)) return GEM_BADINDEX;

  if (atype == GEM_BOOL) {
    if (ints == NULL) return GEM_NULLVALUE;
    if (ints[0] == 0) {
      stat = ocsmSetAttr(modl, branch-1, name, "F");
    } else {
      stat = ocsmSetAttr(modl, branch-1, name, "T");
    }
  } else if (atype == GEM_INTEGER) {
    if (ints == NULL) return GEM_NULLVALUE;
    sprintf(buffer, "%d", ints[0]);
    stat = ocsmSetAttr(modl, branch-1, name, buffer);
  } else if (atype == GEM_REAL) {
    if (reals == NULL) return GEM_NULLVALUE;
    sprintf(buffer, "%lf", reals[0]);
    stat = ocsmSetAttr(modl, branch-1, name, buffer);
  } else {
    if (string == NULL) return GEM_NULLVALUE;
    stat = ocsmSetAttr(modl, branch-1, name, string);
  }
  return stat;
}
