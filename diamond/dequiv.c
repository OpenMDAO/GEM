/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Equivalency Function -- EGADS
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "gem.h"
#include "egads.h"


int
gem_kernelEquivalent(int etype, gemID handle1, gemID handle2)
{
  if ((etype < GEM_NODE) || (etype > GEM_SHELL)) return GEM_BADTYPE;
  if (handle1.ident.ptr == handle2.ident.ptr) return GEM_SUCCESS;

  return EG_isEquivalent((ego) handle1.ident.ptr, (ego) handle2.ident.ptr);
}
