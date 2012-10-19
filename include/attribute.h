/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Internal Attribute Functions Include
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

extern int
gem_getAttrib(/*@null@*/ gemAttrs *attr, int aindex, char **name, int *atype,
              int *alen, int **integers, double **reals, char **string);

extern int
gem_retAttrib(/*@null@*/ gemAttrs *attr, /*@null@*/ char *name, int *aindex,
              int *atype, int *alen, int **integers, double **reals, 
              char **string);

extern int
gem_setAttrib(gemAttrs **attrx, /*@null@*/ char *name, int atype, int alen,
              /*@null@*/ int *integers, /*@null@*/ double *reals, 
              /*@null@*/ char *string);

extern void
gem_clrAttribs(gemAttrs **attrx);

extern void
gem_cpyAttribs(gemAttrs *attrs, gemAttrs **attrx);
