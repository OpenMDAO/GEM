/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             2D Initial Triangulation Functions Include
 *
 *      Copyright 2011-2012, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */


  /* structures */

  typedef struct {
    int   sleft;                /* left  segment in front */
    int   i0;                   /* left  vertex index */
    int   i1;                   /* right vertex index */
    int   sright;               /* right segment in front */
    short snew;                 /* is this a new segment? */
    short mark;                 /* is this segment marked? */
  } Front;

  typedef struct {
    int    mfront;
    int    nfront;
    int    mpts;
    int    npts;
    int   *pts;
    int    nsegs;
    int   *segs;
    Front *front;
  } fillArea;


  extern int gem_fillArea( int nconts, const int *cntr, const double *vertices,
                           int *tris, int *nfig8, int pass, fillArea *fa );

