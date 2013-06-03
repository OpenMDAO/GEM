/*
 *      GEM: Geometry Environment for MDAO frameworks
 *
 *             2D Initial Triangulation Functions
 *
 *      Copyright 2011-2013, Massachusetts Institute of Technology
 *      Licensed under The GNU Lesser General Public License, version 2.1
 *      See http://www.opensource.org/licenses/lgpl-2.1.php
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>	/* Needed in some systems for DBL_MAX definition */
#include <limits.h>	/* Needed in other systems for DBL_EPSILON */

#include "fillArea.h"
#include "memory.h"


#define NOTFILLED	-1
#define TOL		 1.e-7
#define CHUNK            256


#define AREA2D(a,b,c)   ((a[0]-c[0])*(b[1]-c[1]) - (a[1]-c[1])*(b[0]-c[0]))
#define DIST2(a,b)      ((a[0]-b[0])*(a[0]-b[0]) + (a[1]-b[1])*(a[1]-b[1]))
#define DOT2(a,b)       (((a)[0]*(b)[0]) + ((a)[1]*(b)[1]))
#define VSUB2(a,b,c)    (c)[0] = (a)[0] - (b)[0]; (c)[1] = (a)[1] - (b)[1];
#define MAX(a,b)        (((a) > (b)) ?  (a) : (b))
#define MIN(a,b)        (((a) < (b)) ?  (a) : (b))
#define ABS(a)          (((a) <   0) ? -(a) : (a))



/* 
 * determine if this line segment crosses any active segments 
 * pass:      0 - first pass; conservative algorithm
 * 	      1 - second pass; use dirty tricks
 */

static int
crossSeg(int index, const double *mid, int i2, const double *vertices, 
         int pass, fillArea *fa)
{
  int    i, i0, i1, iF0, iF1;
  double angle, cosan, sinan, dist2, distF, eps, ty0, ty1, frac;
  double uv0[2], uv1[2], uv2[2], x[2];
  double uvF0[2], uvF1[2];

  uv2[0] = vertices[2*i2  ];
  uv2[1] = vertices[2*i2+1];

  /*  Store away coordinates of front */
  iF0 	  = fa->front[index].i0;
  iF1 	  = fa->front[index].i1;
  uvF0[0] = vertices[2*iF0  ];
  uvF0[1] = vertices[2*iF0+1];
  uvF1[0] = vertices[2*iF1  ];
  uvF1[1] = vertices[2*iF1+1];

  dist2  = DIST2( mid,  uv2);
  distF  = DIST2(uvF0, uvF1);
  eps    = (dist2 + distF) * DBL_EPSILON;

  /* transform so that we are in mid-uv2 coordinate frame */
  angle = atan2(uv2[1]-mid[1], uv2[0]-mid[0]);
  cosan = cos(angle);
  sinan = sin(angle);

  /* look at the current front */

  for (i = 0; i < fa->nfront; i++) {
    if ((i == index) || (fa->front[i].sright == NOTFILLED)) continue;
    if (fa->front[i].snew == 0) continue;
    i0 = fa->front[i].i0;
    i1 = fa->front[i].i1;
    if ((i0 == i2) || (i1 == i2)) continue;
    uv0[0] = vertices[2*i0  ];
    uv0[1] = vertices[2*i0+1];
    uv1[0] = vertices[2*i1  ];
    uv1[1] = vertices[2*i1+1];

    /* look to see if the transformed y's from uv2-mid cross 0.0 */
    ty0 = (uv0[1]-mid[1])*cosan - (uv0[0]-mid[0])*sinan;
    ty1 = (uv1[1]-mid[1])*cosan - (uv1[0]-mid[0])*sinan;
    if ((ty0 == 0.0) && (ty1 == 0.0)) return 1;
    if  (ty0*ty1 >= 0.0) continue;

    /* get fraction of line for crossing */
    frac = -ty0/(ty1-ty0);
    if ((frac < 0.0) || (frac > 1.0)) continue;

    /* get the actual coordinates */
    x[0] = uv0[0] + frac*(uv1[0]-uv0[0]);
    x[1] = uv0[1] + frac*(uv1[1]-uv0[1]);

    /* are we in the range for the line seg? */
    frac = (x[0]-mid[0])*cosan + (x[1]-mid[1])*sinan;
    if ((frac > 0.0) && (frac*frac < dist2*(1.0+TOL))) return 2;
  }

  /* look at our original loops */

  for (i = 0; i < fa->nsegs; i++) {
    double area10, area01, area11, area00;

    i0 = fa->segs[2*i  ];
    i1 = fa->segs[2*i+1];

    if (((i0 == fa->front[index].i0) && (i1 == fa->front[index].i1)) ||
        ((i0 == fa->front[index].i1) && (i1 == fa->front[index].i0))) 
      continue;

    uv0[0] = vertices[2*i0  ];
    uv0[1] = vertices[2*i0+1];
    uv1[0] = vertices[2*i1  ];
    uv1[1] = vertices[2*i1+1];

    if (pass != 0) {
      area10 = AREA2D(uv2, uv1, uvF0);
      area00 = AREA2D(uv2, uv0, uvF0);
      area10 = ABS(area10);
      area00 = ABS(area00);
    }
    if ((pass != 0) && area10 < eps && area00 < eps) {
      /* I2 and Boundary Segment are collinear with IF0 (Front.I0) */
      double del0[2], del1[2], del2[2];

      VSUB2(uv2, uvF0, del2);
      VSUB2(uv1, uvF0, del1);
      VSUB2(uv0, uvF0, del0);
      /*  See if I1 is between IF0 and I2 */
      if (i1 != iF0 && DOT2(del2, del1) > 0 &&
	  DOT2(del2, del2) > DOT2(del1, del1)) return 5;
      /*  See if I0 is between IF0 and I2 */
      if (i0 != iF0 && DOT2(del2, del0) > 0 && 
	  DOT2(del2, del2) > DOT2(del0, del0)) return 6;
    }
    if (pass != 0) {
      area11 = AREA2D(uv2, uv1, uvF1);
      area01 = AREA2D(uv2, uv0, uvF1);
      area11 = ABS(area11);
      area01 = ABS(area01);
    }
    if ((pass != 0) && area11 < eps && area01 < eps) {
      /* I2 and Boundary Segment are collinear with IF1 (Front.I1) */
      double del0[2], del1[2], del2[2];

      VSUB2(uv2, uvF1, del2);
      VSUB2(uv1, uvF1, del1);
      VSUB2(uv0, uvF1, del0);
      /*  See if I1 is between IF1 and I2 */
      if (i1 != iF1 && DOT2(del2, del1) > 0 &&
	  DOT2(del2, del2) > DOT2(del1, del1)) return 7;
      /*  See if I0 is between IF1 and I2 */
      if (i0 != iF1 && DOT2(del2, del0) > 0 && 
	  DOT2(del2, del2) > DOT2(del0, del0)) return 8;
    }

    if ((i1 == i2) || (i0 == i2)) continue;
    /* look to see if the transformed y's from uv2-mid cross 0.0 */
    ty0 = (uv0[1]-mid[1])*cosan - (uv0[0]-mid[0])*sinan;
    ty1 = (uv1[1]-mid[1])*cosan - (uv1[0]-mid[0])*sinan;
    if ((ty0 == 0.0) && (ty1 == 0.0)) return 3;
    if  (ty0*ty1 >= 0.0) continue;

    /* get fraction of line for crossing */
    frac = -ty0/(ty1-ty0);
    if ((frac < 0.0) || (frac > 1.0)) continue;

    /* get the actual coordinates */
    x[0] = uv0[0] + frac*(uv1[0]-uv0[0]);
    x[1] = uv0[1] + frac*(uv1[1]-uv0[1]);

    /* are we in the range for the line seg? */
    frac = (x[0]-mid[0])*cosan + (x[1]-mid[1])*sinan;
    if ((frac > 0.0) && (frac*frac < dist2*(1.0+TOL))) return 4;
  }

  return 0;
}


/* Input specified as contours.
 * Outer contour must be counterclockwise.
 * All inner contours must be clockwise.
 *  
 * Every contour is specified by giving all its points in order. No
 * point shoud be repeated. i.e. if the outer contour is a square,
 * only the four distinct endpoints should be specified in order.
 *  
 * ncontours: #contours
 * cntr: An array describing the number of points in each
 *	 contour. Thus, cntr[i] = #points in the i'th contour.
 * vertices: Input array of vertices. Vertices for each contour
 *           immediately follow those for previous one. Array location
 *           vertices[0] must NOT be used (i.e. i/p starts from
 *           vertices[1] instead. The output triangles are
 *	     specified  WRT the indices of these vertices.
 * triangles: Output array to hold triangles (allocated before the call)
 * pass:      0 - first pass; conservative algorithm
 * 	      1 - second pass; use dirty tricks
 *  
 * The number of output triangles produced for a polygon with n points is:
 *    (n - 2) + 2*(#holes)
 *
 * returns: -1 degenerate contour (zero length segment)
 *           0 allocation error
 *           + number of triangles
 */
int
gem_fillArea(int ncontours, const int *cntr, const double *vertices,
             int *triangles, int *n_fig8, int pass, fillArea *fa)
{
  int    i, j, i0, i1, i2, index, indx2, k, l, npts, neg;
  int    start, next, left, right, ntri, mtri;
  double side2, dist, d, area, uv0[2], uv1[2], uv2[2], mid[2];
  Front  *tmp;
  int    *itmp;

  *n_fig8 = 0;
  for (i = 0; i < ncontours; i++) if (cntr[i] < 3) return -1;
  for (fa->nfront = i = 0; i < ncontours; i++) fa->nfront += cntr[i];
  if (fa->nfront == 0) return -1;
  fa->npts = fa->nsegs = fa->nfront;

  mtri = fa->nfront - 2 + 2*(ncontours-1);
  ntri = 0;

  /* allocate the memory for the front */

  if (fa->front == NULL) {
    fa->mfront = CHUNK;
    while (fa->mfront < fa->nfront) fa->mfront += CHUNK;
    fa->front = (Front *) gem_allocate(fa->mfront*sizeof(Front));
    if (fa->front == NULL) return 0;
    fa->segs  = (int *) gem_allocate(2*fa->mfront*sizeof(int));
    if (fa->segs  == NULL) {
      gem_free(fa->front);
      fa->front = NULL;
      return 0;
    }
  } else {
    if (fa->mfront < fa->nfront) {
      i = fa->mfront;
      while (i < fa->nfront) i += CHUNK;
      tmp = (Front *) gem_reallocate(fa->front, i*sizeof(Front));
      if (tmp == NULL) return 0;
      itmp = (int *) gem_reallocate(fa->segs, 2*i*sizeof(int));
      if (itmp == NULL) {
        gem_free(tmp);
        return 0;
      }
      fa->mfront = i;
      fa->front  =  tmp;
      fa->segs   = itmp;
    }
  }

  /* allocate the memory for our point markers */
  npts = fa->nfront+1;
  if (fa->pts == NULL) {
    fa->mpts = CHUNK;
    while (fa->mpts < npts) fa->mpts += CHUNK;
    fa->pts = (int *) gem_allocate(fa->mpts*sizeof(int));
    if (fa->pts == NULL) return 0;
  } else {
    if (fa->mpts < npts) {
      i = fa->mpts;
      while (i < npts) i += CHUNK;
      itmp = (int *) gem_reallocate(fa->pts, i*sizeof(int));
      if (itmp == NULL) return 0;
      fa->mpts = i;
      fa->pts  = itmp;
    }
  }

  /* initialize the front */
  for (start = index = i = 0; i < ncontours; i++) {
    left = start + cntr[i] - 1;
    for (j = 0; j < cntr[i]; j++, index++) {
      fa->segs[2*index  ]     = left      + 1;
      fa->segs[2*index+1]     = start + j + 1;
      fa->front[index].sleft  = left;
      fa->front[index].i0     = left      + 1;
      fa->front[index].i1     = start + j + 1;
      fa->front[index].sright = start + j + 1;
      fa->front[index].snew   = 0;
      left = start + j;
    }
    fa->front[index-1].sright = start;

    /* look for fig 8 nodes in the contour */
    for (j = 0; j < cntr[i]-1; j++) {
      i0 = start + j + 1;
      for (k = j+1; k < cntr[i]; k++) {
        i1 = start + k + 1;
        if ((vertices[2*i0  ] == vertices[2*i1  ]) &&
            (vertices[2*i0+1] == vertices[2*i1+1])) {
          if (i0+1 == i1) {
            printf(" GEM Internal: Null in loop %d -> %d %d\n", i, i0, i1);
            continue;
          }
          printf(" GEM Internal: Fig 8 in loop %d (%d) -> %d %d (removed)\n",
                 i, ncontours, i0, i1);
	  /* figure 8's in the external loop decrease the triangle count */
          if (i == 0) (*n_fig8)++;  /* . . . . sometimes                 */
          for (l = 0; l < index; l++) {
            if (fa->front[l].i0 == i1) fa->front[l].i0 = i0;
            if (fa->front[l].i1 == i1) fa->front[l].i1 = i0;
          }
        }
      }
    }
    start += cntr[i];
  }

  /* collapse the front while building the triangle list*/

  neg = 0;
  do {

    /* count the number of vertex hits (right-hand links) */

    for (i = 0; i < npts; i++) fa->pts[i] = 0;
    for (i = 0; i < fa->nfront; i++) 
      if (fa->front[i].sright != NOTFILLED) fa->pts[fa->front[i].i1]++;

    /* remove any simple isolated triangles */

    for (j = i = 0; i < fa->nfront; i++) {
      if (fa->front[i].sright == NOTFILLED) continue;
      i0    = fa->front[i].i0;
      i1    = fa->front[i].i1;
      right = fa->front[i].sright;
      left  = fa->front[right].sright;
      if (fa->front[left].i1 == i0) {
        i2     = fa->front[right].i1;
        uv0[0] = vertices[2*i0  ];
        uv0[1] = vertices[2*i0+1];
        uv1[0] = vertices[2*i1  ];
        uv1[1] = vertices[2*i1+1];
        uv2[0] = vertices[2*i2  ];
        uv2[1] = vertices[2*i2+1];
        area   = AREA2D(uv0, uv1, uv2);
        if ((neg == 0) && (area <= 0.0)) continue;
        if (fa->front[left].sright != i) {
          start = fa->front[left].sright;
          fa->front[start].sleft  = fa->front[i].sleft;
          start = fa->front[i].sleft;
          fa->front[start].sright = fa->front[left].sright;
        }
        triangles[3*ntri  ]    = i0;
        triangles[3*ntri+1]    = i1;
        triangles[3*ntri+2]    = i2;
        fa->front[i].sleft     = fa->front[i].sright     = NOTFILLED;
        fa->front[right].sleft = fa->front[right].sright = NOTFILLED;
        fa->front[left].sleft  = fa->front[left].sright  = NOTFILLED;
        ntri++;
        j++;
        if (ntri >= mtri) break;
        neg = 0;
      }
    }
    if (j != 0) continue;

    /* look for triangles hidden by "figure 8" vetrices */

    for (j = i = 0; i < fa->nfront; i++) {
      if (fa->front[i].sright == NOTFILLED) continue;
      i0 = fa->front[i].i0;
      i1 = fa->front[i].i1;
      if (fa->pts[i1] == 1) continue;
      for (k = 0; k < fa->nfront; k++) {
        if (fa->front[k].sright == NOTFILLED) continue;
        if (k == fa->front[i].sright) continue;
        if (fa->front[k].i0 != i1) continue;
        i2 = fa->front[k].i1;
        uv0[0] = vertices[2*i0  ];
        uv0[1] = vertices[2*i0+1];
        uv1[0] = vertices[2*i1  ];
        uv1[1] = vertices[2*i1+1];
        uv2[0] = vertices[2*i2  ];
        uv2[1] = vertices[2*i2+1];
        area   = AREA2D(uv0, uv1, uv2);
        if ((neg == 0) && (area <= 0.0)) continue;
        for (l = 0; l < fa->nfront; l++) {
          if (fa->front[l].sright == NOTFILLED) continue;
          if (fa->front[l].sleft  == NOTFILLED) continue;
          if ((fa->front[l].i0 == i2) && (fa->front[l].i1 == i0)) {
            if (fa->front[i].sleft != l) {
              index = fa->front[i].sleft;
              indx2 = fa->front[l].sright;
              fa->front[i].sleft      = l;
              fa->front[l].sright     = i;
              fa->front[index].sright = indx2;
              fa->front[indx2].sleft  = index;
            }
            if (fa->front[i].sright != k) {
              index = fa->front[i].sright;
              indx2 = fa->front[k].sleft;
              fa->front[i].sright     = k;
              fa->front[k].sleft      = i;
              fa->front[index].sleft  = indx2;
              fa->front[indx2].sright = index;
            }
            if (fa->front[k].sright != l) {
              index = fa->front[k].sright;
              indx2 = fa->front[l].sleft;
              fa->front[k].sright     = l;
              fa->front[l].sleft      = k;
              fa->front[index].sleft  = indx2;
              fa->front[indx2].sright = index;
            }

            left   = fa->front[i].sleft;
            right  = fa->front[i].sright;
            triangles[3*ntri  ]    = i0;
            triangles[3*ntri+1]    = i1;
            triangles[3*ntri+2]    = i2;
            fa->front[i].sleft     = fa->front[i].sright     = NOTFILLED;
            fa->front[right].sleft = fa->front[right].sright = NOTFILLED;
            fa->front[left].sleft  = fa->front[left].sright  = NOTFILLED;
            ntri++;
            j++;
            if (ntri >= mtri) break;
            neg = 0;
          }
        }
        if (ntri >= mtri) break;
      }
      if (ntri >= mtri) break;
    }
    if (j != 0) continue;

    /* get smallest segment left */

    for (i = 0; i < fa->nfront; i++) fa->front[i].mark = 0;
small:
    index = -1;
    side2 = DBL_MAX;
    for (i = 0; i < fa->nfront; i++) {
      if (fa->front[i].sright == NOTFILLED) continue;
      if (fa->front[i].mark == 1) continue;
      i0     = fa->front[i].i0;
      i1     = fa->front[i].i1;
      uv0[0] = vertices[2*i0  ];
      uv0[1] = vertices[2*i0+1];
      uv1[0] = vertices[2*i1  ];
      uv1[1] = vertices[2*i1+1];
      d      = DIST2(uv0, uv1);
      if (d < side2) {
        side2 = d;
        index = i;
      }
    }
    if (index == -1) {
      for (k = 0; k < *n_fig8; k++) 
        if (ntri + 2*k == mtri) break;
      if (neg == 0) {
        neg = 1;
        continue;
      }
      printf(" GEM Internal: can't find segment!\n");
      goto error;
    }

    /* find the best candidate -- closest to midpoint and correct area */

    i0     = fa->front[index].i0;
    i1     = fa->front[index].i1;
    uv0[0] = vertices[2*i0  ];
    uv0[1] = vertices[2*i0+1];
    uv1[0] = vertices[2*i1  ];
    uv1[1] = vertices[2*i1+1];
    mid[0] = 0.5*(uv0[0] + uv1[0]);
    mid[1] = 0.5*(uv0[1] + uv1[1]);

    indx2 = -1;
    dist  = DBL_MAX;
    for (i = 0; i < fa->nfront; i++) {
      if ((i == index) || (fa->front[i].sright == NOTFILLED)) continue;
      i2 = fa->front[i].i1;
      if ((i2 == i0) || (i2 == i1)) continue;
      uv2[0] = vertices[2*i2  ];
      uv2[1] = vertices[2*i2+1];
      area   = AREA2D(uv0, uv1, uv2);
      if (area > 0.0) {
        d = DIST2(mid, uv2)/area;
        if (d < dist) {
          if (crossSeg(index, mid, i2, vertices, pass, fa)) continue;
          dist  = d;
          indx2 = i;
        }
      }
    }
    /* may not find a candidate for segments that are too small
               retry with next largest (and hope for closure later) */
    if (indx2 == -1) {
      fa->front[index].mark = 1;
      goto small;
    }

    /* construct the triangle */

    i2 = fa->front[indx2].i1;
    triangles[3*ntri  ] = i0;
    triangles[3*ntri+1] = i1;
    triangles[3*ntri+2] = i2;
    ntri++;
    neg = 0;

    /* patch up the front */

    left  = fa->front[index].sleft;
    right = fa->front[index].sright;

    if (i2 == fa->front[left].i0) {
      /* 1) candate is in the left segment */

      fa->front[left].sright = right;
      fa->front[left].i1     = i1;
      fa->front[left].snew   = 1;
      fa->front[right].sleft = left;
      fa->front[index].sleft = fa->front[index].sright = NOTFILLED;

    } else if (i2 == fa->front[right].i1) {
      /* 2) candate is in the right segment */

      fa->front[left].sright = right;
      fa->front[right].sleft = left;
      fa->front[right].i0    = i0;
      fa->front[right].snew  = 1;
      fa->front[index].sleft = fa->front[index].sright = NOTFILLED;

    } else {
      /* 3) some other situation */

      start = 0;

      /* "figure 8" vertices? */

      if (fa->pts[i0] != 1) 
        for (i = 0; i < fa->nfront; i++) {
          if (fa->front[i].sright == NOTFILLED) continue;
          if (fa->front[i].i0 != i2) continue;
          if (fa->front[i].i1 != i0) continue;
          j = fa->front[i].sright;
          fa->front[left].sright = j;
          fa->front[j].sleft     = left;
          fa->front[index].sleft = i;
          fa->front[i].sright    = index;
          left = i;
          fa->front[left].sright = right;
          fa->front[left].i1     = i1;
          fa->front[left].snew   = 1;
          fa->front[right].sleft = left;
          fa->front[index].sleft = fa->front[index].sright = NOTFILLED;
          start = 1;
          break;
        }

      if ((fa->pts[i1] != 1) && (start == 0))
        for (i = 0; i < fa->nfront; i++) {
          if (fa->front[i].sright == NOTFILLED) continue;
          if (fa->front[i].i0 != i1) continue;
          if (fa->front[i].i1 != i2) continue;
          j = fa->front[i].sleft;
          fa->front[right].sleft  = j;
          fa->front[j].sright     = right;
          fa->front[index].sright = i;
          fa->front[i].sleft      = index;
          right = i;
          fa->front[left].sright = right;
          fa->front[right].sleft = left;
          fa->front[right].i0    = i0;
          fa->front[right].snew  = 1;
          fa->front[index].sleft = fa->front[index].sright = NOTFILLED;
          start = 1;
          break;
        }

      /* no, add a segment */

      if (start == 0) {

        next = -1;
        for (i = 0; i < fa->nfront; i++)
          if (fa->front[i].sright == NOTFILLED) {
            next = i;
            break;
          }

        if (next == -1) {
          if (fa->nfront >= fa->mfront) {
            i = fa->mfront + CHUNK;
            tmp = (Front *) gem_reallocate(fa->front, i*sizeof(Front));
            if (tmp == NULL) return 0;
            itmp = (int *) gem_reallocate(fa->segs, 2*i*sizeof(int));
            if (itmp == NULL) {
              gem_free(tmp);
              return 0;
            }
            fa->mfront = i;
            fa->front  =  tmp;
            fa->segs   = itmp;
          }
          next = fa->nfront;
          fa->nfront++;
        }

        start = fa->front[indx2].sright;
        fa->front[index].i1     = i2;
        fa->front[index].sright = start;
        fa->front[index].snew   = 1;
        fa->front[start].sleft  = index;
        fa->front[indx2].sright = next;
        fa->front[right].sleft  = next;
        fa->front[next].sleft   = indx2;
        fa->front[next].i0      = i2;
        fa->front[next].i1      = i1;
        fa->front[next].sright  = right;
        fa->front[next].snew    = 1;
      }
    }

  } while (ntri < mtri);

error:
  for (j = i = 0; i < fa->nfront; i++) 
    if (fa->front[i].sright != NOTFILLED) j++;

  if (j != 0) {
#ifdef DEBUG
    printf(" GEM Internal: # unused segments = %d\n", j);
#endif
    ntri = 0;
  }

  return ntri;
}
