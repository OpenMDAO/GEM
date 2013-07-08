#include <stdio.h>
#include <stdlib.h>

#include "gem.h"

#define FUZZ  -1.e-8              /* used for inTriCross test */

  extern double gem_orienTet(double *, double *, double *, double *);
  extern double gem_orienTri(double *, double *, double *);
  extern void   gem_exactInit();


/* returns the sign of the double */

static int
gem_sign(double s)
{
  if (s > 0.0) return  1;
  if (s < 0.0) return -1;
               return  0;
}


/* returns the intersection status between a line and tri */

static int
gem_intersecTet(double *t1, double *t2, double *t3, double *p1, double *p2,
                double *w)
{ 
  int d1, d2, d3;
	      
  /* 1. test if p1-p2 spans the triangle 123 */
    
  d1 = gem_sign( gem_orienTet(p1,t1,t2,t3) );
  d2 = gem_sign( gem_orienTet(p2,t1,t2,t3) );
        
  if (d1*d2 > 0) return GEM_OUTSIDE;
  if (d2   == 0) return GEM_DEGENERATE;

  /* 2. Yes it spans the plane, then does it pierce the triangle?  */

  w[0] = gem_orienTet(t2,p1,p2,t3);
  w[1] = gem_orienTet(t3,p1,p2,t1);
  w[2] = gem_orienTet(t1,p1,p2,t2);
  d1   = gem_sign(w[0]);
  d2   = gem_sign(w[1]);
  d3   = gem_sign(w[2]);

  if (d1*d2*d3 == 0) 
    if (d1 == 0) {
      if ((d2 == 0) && (d3 == 0)) return GEM_OUTSIDE;
      if (d2 == d3) return GEM_SUCCESS;
      if (d2 ==  0) return GEM_SUCCESS;
      if (d3 ==  0) return GEM_SUCCESS;
    } else if (d2 == 0) {
      if (d1 == d3) return GEM_SUCCESS;
      if (d3 ==  0) return GEM_SUCCESS;
    } else {
      if (d1 == d2) return GEM_SUCCESS;
    }
  
  /* all resultant tets have the same sign -> intersection */

  if ((d1 == d2) && (d2 == d3)) return GEM_SUCCESS;
      
  /* otherwise then no intersection */

  return GEM_OUTSIDE;
}


static int
inTri3D(double *t1, double *t2, double *t3, double *p, double *w)
{
  int    stat;
  double t31[3], t32[3], t33[3], p1[3], p2[3], sum;

  /* cast this as a 3D line/tet intersection problem */

  w[0]   = w[1] = w[2] = 0.0;
  t31[0] = t1[0];
  t31[1] = t1[1];
  t31[2] = 0.0;
  t32[0] = t2[0];
  t32[1] = t2[1];
  t32[2] = 0.0;
  t33[0] = t3[0];
  t33[1] = t3[1];
  t33[2] = 0.0;
  p1[0]  = p2[0] = p[0];
  p1[1]  = p2[1] = p[1];
  p1[2]  = -0.5;
  p2[2]  =  0.5;
  
  stat   = gem_intersecTet(t31, t32, t33, p1, p2, w);
  sum    = w[0] + w[1] + w[2];
  w[0]  /= sum;
  w[1]  /= sum;
  w[2]  /= sum;
  
  return stat;
}


/* returns the intersection status between a point and a tri */

static int
inTri(double *t1, double *t2, double *t3, double *p, double *w)
{
  int    d1, d2, d3;
  double sum;
  
  w[0] = w[1] = w[2] = 0.0;
  if (gem_sign(gem_orienTri(t1,t2,t3)) == 0) return GEM_DEGENERATE;

  w[0]  = gem_orienTri(t2,t3,p);
  w[1]  = gem_orienTri(t1,p,t3);
  w[2]  = gem_orienTri(t1,t2,p);
  d1    = gem_sign(w[0]);
  d2    = gem_sign(w[1]);
  d3    = gem_sign(w[2]);
  sum   = w[0] + w[1] + w[2];
  w[0] /= sum;
  w[1] /= sum;
  w[2] /= sum;
  
  if (d1*d2*d3 == 0)
    if (d1 == 0) {
      if ((d2 == 0) && (d3 == 0)) return GEM_OUTSIDE;
      if (d2 == d3) return GEM_SUCCESS;
      if (d2 ==  0) return GEM_SUCCESS;
      if (d3 ==  0) return GEM_SUCCESS;
    } else if (d2 == 0) {
      if (d1 == d3) return GEM_SUCCESS;
      if (d3 ==  0) return GEM_SUCCESS;
    } else {
      if (d1 == d2) return GEM_SUCCESS;
    }
  
  /* all resultant tris have the same sign -> intersection */
  
  if ((d1 == d2) && (d2 == d3)) return GEM_SUCCESS;
  
  /* otherwise then no intersection */
  
  return GEM_OUTSIDE;
}


static int
inTriCross(double *t1, double *t2, double *t3, double *p, double *w)
{
  double dx1, dy1, dx2, dy2, a3, dxx, dyy;

  w[0] = w[1] = w[2] = 0.0;
  dx1  = t2[0] - t1[0];
  dy1  = t2[1] - t1[1];
  dx2  = t3[0] - t1[0];
  dy2  = t3[1] - t1[1];
  a3   = dx1*dy2-dy1*dx2;
  if (a3 == 0.0) return GEM_DEGENERATE;
  a3   = 1.0/a3;
  dxx  = p[0] - t1[0];
  dyy  = p[1] - t1[1];
  w[1] =  a3*(dxx*dy2-dyy*dx2);
  w[2] = -a3*(dxx*dy1-dyy*dx1);
  w[0] =  1.0 - w[1] - w[2];

  if ((w[0] > FUZZ) && (w[1] > FUZZ) && (w[2] > FUZZ)) return GEM_SUCCESS;
  return GEM_OUTSIDE;
}


/* returns the intersection status between a line from t1 and the tri */

static int
lineTri(double *t1, double *t2, double *t3, double *p, double *w)
{
  int    d1, d2;
  double sum;
  
  w[0] = w[1] = 0.0;
  if (gem_sign(gem_orienTri(t1,t2,t3)) == 0) return GEM_DEGENERATE;
  
  w[0]  = gem_orienTri(t1,p,t3);
  w[1]  = gem_orienTri(t1,t2,p);
  d1    = gem_sign(w[0]);
  d2    = gem_sign(w[1]);
  
  if ((d1 == 0) && (d2 == 0)) return GEM_OUTSIDE;
  if  (d1 == 0) return GEM_SUCCESS;
  if  (d2 == 0) return GEM_SUCCESS;
  
  /* all resultant tris have the same sign -> intersection */
  
  if (d1 == d2) {
    sum   = w[0] + w[1];
    w[0] /= sum;
    w[1] /= sum;
    return GEM_SUCCESS;
  }
  
  /* otherwise then no intersection */
  
  return GEM_OUTSIDE;
}


int main(int argc, char *argv[])
{
  int    stat;
  double uv[2], w[3];
  double triangle[6] = {1.0/3.0, 1.0/3.0,  2.0/3.0, 1.0/3.0,  1.0/3.0, 1.5/3.0};

  if (argc != 3) {
    printf(" usage: inTest u v!\n");
    return 1;
  }
  gem_exactInit();

  uv[0] = atof(argv[1]);
  uv[1] = atof(argv[2]);

  stat  = inTriCross(&triangle[0], &triangle[2], &triangle[4], uv, w);
  printf("  inTriCross = %d  %lf %lf %lf\n", stat, w[0], w[1], w[2]);
  stat  = inTri3D(&triangle[0], &triangle[2], &triangle[4], uv, w);
  printf("  inTri3D    = %d  %lf %lf %lf\n", stat, w[0], w[1], w[2]);
  stat  = inTri(&triangle[0], &triangle[2], &triangle[4], uv, w);
  printf("  inTri      = %d  %lf %lf %lf\n", stat, w[0], w[1], w[2]);

  stat  = lineTri(&triangle[0], &triangle[2], &triangle[4], uv, w);
  printf("  lineTri    = %d  %lf %lf\n", stat, w[0], w[1]);
  stat  = lineTri(uv, &triangle[2], &triangle[4], &triangle[0], w);
  printf("  lineTriOpp = %d  %lf %lf\n", stat, w[0], w[1]);
  
  return 0;
}
