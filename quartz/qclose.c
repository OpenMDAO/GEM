/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Close Function -- CAPRI
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "capri.h"
#include "memory.h"


  extern int *CAPRI_MdlRefs;
  extern int mCAPRI_MdlRefs;


int
gem_kernelClose()
{
  gem_free(CAPRI_MdlRefs);
   CAPRI_MdlRefs = NULL;
  mCAPRI_MdlRefs = 0;

  return gi_uStop(0);
}
