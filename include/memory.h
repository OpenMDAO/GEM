/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Memory Functions Include
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

extern /*@null@*/ /*@out@*/ /*@only@*/ void *gem_allocate(size_t nbytes);

extern /*@null@*/ /*@only@*/ void *gem_callocate(size_t nele, size_t size);

extern /*@null@*/ /*@only@*/ void *gem_reallocate(/*@null@*/ /*@only@*/
                                      /*@returned@*/ void *ptr, size_t nbytes);

extern /*@null@*/ /*@only@*/ char *gem_strdup(/*@null@*/ const char *str);

