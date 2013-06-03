/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Equivalence Function -- CAPRI
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "gem.h"


int
gem_kernelEquivalent(int etype, gemID handle1, gemID handle2)
{
  if ((etype < GEM_NODE) || (etype > GEM_SHELL)) return GEM_BADTYPE;

  if ((handle1.index     == handle2.index) &&
      (handle1.ident.tag == handle2.ident.tag)) return GEM_SUCCESS;

  return GEM_OUTSIDE;
}
