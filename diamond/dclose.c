/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Kernel Close Function -- OpenCSM & EGADS
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "egads.h"

  extern ego dia_context;

int
gem_kernelClose()
{
  int stat;

  stat = EG_close(dia_context);
  dia_context = NULL;
  return stat;
}
