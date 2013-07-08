/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Memory Functions
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/*@null@*/ /*@out@*/ /*@only@*/ void *
gem_allocate(size_t nbytes)
{
  return malloc(nbytes);
}


void
gem_free(/*@null@*/ /*@only@*/ void *ptr)
{
  if (ptr != NULL) free(ptr);
}


/*@null@*/ /*@only@*/ void *
gem_callocate(size_t nele, size_t size)
{
  return calloc(nele, size);
}


/*@null@*/ /*@only@*/ void *
gem_reallocate(/*@null@*/ /*@only@*/ /*@returned@*/ void *ptr, size_t nbytes)
{
  return realloc(ptr, nbytes);
}


/*@null@*/ /*@only@*/ char *
gem_strdup(/*@null@*/ const char *str)
{
  int  i, len;
  char *dup;

  if (str == NULL) return NULL;

  len = strlen(str) + 1;
  dup = (char *) gem_allocate(len*sizeof(char));
  if (dup != NULL)
    for (i = 0; i < len; i++) dup[i] = str[i];

  return dup;
}
