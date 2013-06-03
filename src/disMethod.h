/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Discretization Method Dynamic Loading Include
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#define MAXMETHOD 32

typedef int  (*DLLFunc) (void);
typedef void (*fQuilt)  (gemQuilt *);
typedef int  (*dQuilt)  (const gemDRep *, int, gemPair *, gemQuilt *);
typedef void (*invEval) (gemQuilt *, double *, double *, int *, double *);
typedef int  (*gInterp) (gemQuilt *, int, int, double *, int, double *,
                         double *);
typedef int  (*bInterp) (gemQuilt *, int, int, double *, int, double *,
                         double *);
typedef int  (*gIntegr) (gemQuilt *, int, int, int, double *, double *);
typedef int  (*bIntegr) (gemQuilt *, int, int, int, double *, double *);
