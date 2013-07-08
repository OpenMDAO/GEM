/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Internal Attribute Functions
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gem.h"
#include "memory.h"


int
gem_getAttrib(/*@null@*/ gemAttrs *attr, int aindex, char **name, int *atype,
              int *alen, int **integers, double **reals, char **string)
{
  *name     = NULL;
  *alen     =  0;
  *atype    = -1;
  *integers = NULL;
  *reals    = NULL;
  *string   = NULL;
  if (attr == NULL) return GEM_BADINDEX;
  if ((aindex <= 0) || (aindex > attr->nattrs)) return GEM_BADINDEX;

  *name     = attr->attrs[aindex-1].name;
  *atype    = attr->attrs[aindex-1].type;
  *alen     = attr->attrs[aindex-1].length;
  *integers = attr->attrs[aindex-1].integers;
  *reals    = attr->attrs[aindex-1].reals;
  *string   = attr->attrs[aindex-1].string;
  return GEM_SUCCESS;
}


int
gem_retAttrib(/*@null@*/ gemAttrs *attr, /*@null@*/ char *name, int *aindex,
              int *atype, int *alen, int **integers, double **reals, 
              char **string)
{
  int i;

  *aindex   = -1;
  *alen     =  0;
  *atype    = -1;
  *integers = NULL;
  *reals    = NULL;
  *string   = NULL;
  if (attr == NULL) return GEM_NOTFOUND;
  if (name == NULL) return GEM_NULLNAME;

  for (i = 0; i < attr->nattrs; i++) {
    if (strcmp(name, attr->attrs[i].name) != 0) continue;
    *aindex   = i+1;
    *atype    = attr->attrs[i].type;
    *alen     = attr->attrs[i].length;
    *integers = attr->attrs[i].integers;
    *reals    = attr->attrs[i].reals;
    *string   = attr->attrs[i].string;
    return GEM_SUCCESS;
  }

  return GEM_NOTFOUND;
}


int
gem_setAttrib(gemAttrs **attrx, /*@null@*/ char *name, int atype, int alen,
              /*@null@*/ int *integers, /*@null@*/ double *reals, 
              /*@null@*/ char *string)
{
  int      i, j;
  gemAttr  *temp;
  gemAttrs *attr;

  if (name == NULL) return GEM_NULLNAME;
  if ((atype != GEM_BOOL)   && (atype != GEM_INTEGER) && (atype != GEM_REAL) &&
      (atype != GEM_STRING) && (atype != GEM_POINTER)) return GEM_BADTYPE;
  if (alen > 0)
    if ((atype == GEM_STRING) || (atype == GEM_POINTER)) {
      if (string   == NULL) return GEM_NULLVALUE;
    } else if (atype == GEM_REAL) {
      if (reals    == NULL) return GEM_NULLVALUE;
    } else {
      if (integers == NULL) return GEM_NULLVALUE;
    }

  attr = *attrx;
  if (name == NULL) return GEM_NOTFOUND;
  if (attr == NULL) {
    attr = (gemAttrs *) gem_allocate(sizeof(gemAttrs));
    if (attr == NULL) return GEM_ALLOC;
    *attrx = attr;
    attr->nattrs = 0;
    attr->mattrs = 0;
    attr->attrs  = NULL;
  }

  for (i = 0; i < attr->nattrs; i++) {
    if (attr->attrs == NULL) continue;
    if (strcmp(name, attr->attrs[i].name) != 0) continue;
    if (alen <= 0) {
      for (j = i; j < attr->nattrs-1; j++)
        attr->attrs[j] = attr->attrs[j+1];
      attr->nattrs--;
    } else {
      gem_free(attr->attrs[i].integers);
      gem_free(attr->attrs[i].reals);
      if (attr->attrs[i].type != GEM_POINTER) gem_free(attr->attrs[i].string);
      attr->attrs[i].integers = NULL;
      attr->attrs[i].reals    = NULL;
      attr->attrs[i].string   = NULL;
      attr->attrs[i].type     = atype;
      if (atype == GEM_STRING) {
        if (string == NULL) return GEM_NULLVALUE;
        attr->attrs[i].length = strlen(string);
        attr->attrs[i].string = gem_strdup(string);
        if (attr->attrs[i].string == NULL) attr->attrs[i].length = 0;
      } else if (atype == GEM_POINTER) {
        if (string == NULL) return GEM_NULLVALUE;
        attr->attrs[i].string = string;
        attr->attrs[i].length = alen;
      } else if (atype == GEM_REAL) {
        if (reals == NULL) return GEM_NULLVALUE;
        attr->attrs[i].length = 0;
        attr->attrs[i].reals  = (double *) gem_allocate(alen*sizeof(double));
        if (attr->attrs[i].reals != NULL) {
          for (j = 0; j < alen; j++)
            attr->attrs[i].reals[j] = reals[j];
          attr->attrs[i].length = alen;
        }
      } else {
        if (integers == NULL) return GEM_NULLVALUE;
        attr->attrs[i].length   = 0;
        attr->attrs[i].integers = (int *) gem_allocate(alen*sizeof(int));
        if (attr->attrs[i].integers != NULL) {
          for (j = 0; j < alen; j++)
            attr->attrs[i].integers[j] = integers[j];
          attr->attrs[i].length = alen;
        }
      }
      if (attr->attrs[i].length == 0) return GEM_ALLOC;
    }
    return GEM_SUCCESS;
  }
  if (alen <= 0) return GEM_NOTFOUND;

  /* not found -- create */

  i = attr->nattrs;
  if (i == attr->mattrs) {
    if (attr->attrs == NULL) {
      attr->attrs = (gemAttr *) gem_allocate(sizeof(gemAttr));
      if (attr->attrs == NULL) return GEM_ALLOC;
      attr->mattrs = attr->nattrs = 1;
    } else {
      temp = (gemAttr *) gem_reallocate(attr->attrs, (i+1)*sizeof(gemAttr));
      if (temp == NULL) return GEM_ALLOC;
      attr->attrs  = temp;
      attr->mattrs = attr->nattrs = i+1;
    }
  }
  if (attr->attrs == NULL) return GEM_ALLOC;
  attr->attrs[i].integers = NULL;
  attr->attrs[i].reals    = NULL;
  attr->attrs[i].string   = NULL;
  attr->attrs[i].type     = atype;
  if (atype == GEM_STRING) {
    if (string == NULL) return GEM_NULLVALUE;
    attr->attrs[i].length = strlen(string);
    attr->attrs[i].string = gem_strdup(string);
    if (attr->attrs[i].string == NULL) attr->attrs[i].length = 0;
  } else if (atype == GEM_POINTER) {
    if (string == NULL) return GEM_NULLVALUE;
    attr->attrs[i].string = string;
    attr->attrs[i].length = alen;
  } else if (atype == GEM_REAL) {
    if (reals == NULL) return GEM_NULLVALUE;
    attr->attrs[i].length = 0;
    attr->attrs[i].reals  = (double *) gem_allocate(alen*sizeof(double));
    if (attr->attrs[i].reals != NULL) {
      for (j = 0; j < alen; j++)
        attr->attrs[i].reals[j] = reals[j];
      attr->attrs[i].length = alen;
    }
  } else {
    if (integers == NULL) return GEM_NULLVALUE;
    attr->attrs[i].length   = 0;
    attr->attrs[i].integers = (int *) gem_allocate(alen*sizeof(int));
    if (attr->attrs[i].integers != NULL) {
      for (j = 0; j < alen; j++)
        attr->attrs[i].integers[j] = integers[j];
      attr->attrs[i].length = alen;
    }
  }
  if (attr->attrs[i].length == 0) {
    attr->nattrs--;
    return GEM_ALLOC;
  }
  attr->attrs[i].name = gem_strdup(name);
  if (attr->attrs[i].name == NULL) {
    gem_free(attr->attrs[i].integers);
    gem_free(attr->attrs[i].reals);
    if (attr->attrs[i].type != GEM_POINTER) gem_free(attr->attrs[i].string);
    attr->nattrs--;
    return GEM_ALLOC;
  }
  
  return GEM_SUCCESS;
}


void
gem_clrAttribs(gemAttrs **attrx)
{
  int      i;
  gemAttrs *attr;
  
  attr = *attrx;
  if (attr == NULL) return;
  
  for (i = 0; i < attr->nattrs; i++) {
    gem_free(attr->attrs[i].name);
    gem_free(attr->attrs[i].integers);
    gem_free(attr->attrs[i].reals);
    if (attr->attrs[i].type != GEM_POINTER) gem_free(attr->attrs[i].string);
  }
  gem_free(attr->attrs);
  gem_free(attr);
  *attrx = NULL;
  
}


void
gem_cpyAttribs(gemAttrs *attrs, gemAttrs **attrx)
{
  int i;
  
  gem_clrAttribs(attrx);
  if (attrs == NULL) return;
  
  for (i = 0; i < attrs->nattrs; i++)
    gem_setAttrib(attrx, attrs->attrs[i].name, attrs->attrs[i].type, 
                  attrs->attrs[i].length, attrs->attrs[i].integers,
                  attrs->attrs[i].reals,  attrs->attrs[i].string);
}
