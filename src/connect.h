/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             Triangle Neighbor Fill Include
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */


#define NOTFILLED	-1


  typedef struct {
    int vert1;                  /* 1nd vert number for side */
    int vert2;                  /* 2nd vert number for side */
    int *tri;                   /* 1st triangle storage or NULL for match */
    int thread;                 /* thread to next side with 1st vert number */
  } gemNeigh;


  extern void gem_fillNeighbor( int k1, int k2, int *tri, int *kside, 
                                int *vtable, gemNeigh *stable );
