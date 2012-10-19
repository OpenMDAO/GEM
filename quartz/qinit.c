/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Initialize Function -- CAPRI
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "capri.h"
#include "memory.h"


  int *CAPRI_MdlRefs = NULL;
  int mCAPRI_MdlRefs = 0;


int
gem_kernelInit()
{
  int stat;

  gi_putenv("CAPRIshell=On");
  gi_putenv("CAPRIwire=On");
  gi_putenv("CAPRIspline=New");
  stat = gi_uStart();
  if (stat != CAPRI_SUCCESS) return stat;

  CAPRI_MdlRefs = (int *) gem_allocate(256*sizeof(int));
  if (CAPRI_MdlRefs != NULL) {
    mCAPRI_MdlRefs = 256;
  } else {
    printf(" GEM/CAPRI Internal: Model References Disabled!\n");
  }

  return CAPRI_SUCCESS;
}
