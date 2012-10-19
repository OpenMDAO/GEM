/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Initialization Function -- OpenCSM & EGADS
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "egads.h"
#include "OpenCSM.h"

  /* use a single contest for all operations */
  ego dia_context = NULL;


int
gem_kernelInit()
{
  int major, minor;
  
  if (dia_context != NULL) return EGADS_EMPTY;
  
  ocsmVersion(&major, &minor);
  printf("\n GEM Info: Using OpenCSM %d.%02d\n", major, minor);
  EG_revision(&major, &minor);
  printf(" GEM Info: Using EGADS   %d.%02d\n\n", major, minor);

  return EG_open(&dia_context);
}
