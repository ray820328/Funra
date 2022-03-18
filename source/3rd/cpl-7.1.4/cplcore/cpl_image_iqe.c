/*
 * This file is part of the ESO Common Pipeline Library
 * Copyright (C) 2001-2017 European Southern Observatory
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

/*-----------------------------------------------------------------------------
                                   Includes
 -----------------------------------------------------------------------------*/

#include "cpl_image_iqe.h"

#include "cpl_error_impl.h"
#include "cpl_memory.h"
#include "cpl_tools.h"
#include "cpl_bivector.h"
#include "cpl_image_basic.h"
#include "cpl_image_defs.h"
#include "cpl_math_const.h"
#include "cpl_mpfit.h"

#include <math.h>
#include <string.h>
#include <assert.h>

/*-----------------------------------------------------------------------------
                            Private Function prototypes
                            - more are declared below
 -----------------------------------------------------------------------------*/

static int cpl_iqe(float *, int, int, float *, float *);

/*-----------------------------------------------------------------------------
                            Function codes
 -----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/**
  @ingroup cpl_image
  @brief    Compute an image quality estimation for an object
  @param    in      the input image
  @param    llx
  @param    lly     the zone to analyse ((1, 1) for the first pixel)
  @param    urx     The zone must be at least 4 by 4 pixels
  @param    ury
  @return   a newly allocated cpl_bivector containing the results or
              NULL in error case.

  This function makes internal use of the iqe() MIDAS function (called
  here cpl_iqe()) written by P. Grosbol. Refer to the MIDAS documentation
  for more details. This function has proven to give good results over
  the years when called from RTD. The goal is to provide the exact same
  functionality in CPL as the one provided in RTD. The code is simply
  copied from the MIDAS package, it is not maintained by the CPL team.

  The returned object must be deallocated with cpl_bivector_delete().
  It contains in the first vector the computed values, and in the second
  one, the associated errors.
  The computed values are:
  - x position of the object
  - y position of the object
  - FWHM along the major axis
  - FWHM along the minor axis
  - the angle of the major axis with the horizontal in degrees
  - the peak value of the object
  - the background computed

  The bad pixels map of the image is not taken into account.
  The input image must be of type float.

  Possible #_cpl_error_code_ set in this function:
  - CPL_ERROR_NULL_INPUT if (one of) the input pointer(s) is NULL
  - CPL_ERROR_ILLEGAL_INPUT if the specified zone is not valid or if the
  computation fails on this zone
  - CPL_ERROR_INVALID_TYPE if the input image has the wrong type

  @note
  This function may lead to a strong underestimation
  of the sigmas and FWHMs (up to 25% underestimation in the case of
  noiseless data with a box 4 times the sigma, 1% underestimation in the
  case of noiseless data with a box 7 times the sigma).
 */
/*----------------------------------------------------------------------------*/
cpl_bivector * cpl_image_iqe(const cpl_image * in,
                             cpl_size          llx,
                             cpl_size          lly,
                             cpl_size          urx,
                             cpl_size          ury)
{
    cpl_image       *   extracted;
    float               parm[8],
                        sdev[8];
    float           *   pextracted;
    cpl_bivector    *   res;
    double          *   res_x;
    double          *   res_y;
    int                 iqe_error;
    cpl_size            nx, ny;
    int                 inx, iny;

    /* Check entries */
    cpl_ensure(in,                         CPL_ERROR_NULL_INPUT,    NULL);
    cpl_ensure(in->type == CPL_TYPE_FLOAT, CPL_ERROR_INVALID_TYPE,  NULL);
    cpl_ensure(urx >= llx + 3,             CPL_ERROR_ILLEGAL_INPUT, NULL);
    cpl_ensure(ury >= lly + 3,             CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Extract the image */
    extracted = cpl_image_extract(in, llx, lly, urx, ury);
    cpl_ensure(extracted, CPL_ERROR_ILLEGAL_INPUT, NULL);

    nx = cpl_image_get_size_x(extracted);
    ny = cpl_image_get_size_y(extracted);

    inx = (int)nx;
    iny = (int)ny;

    if ((cpl_size)inx != nx || (cpl_size)iny != ny) {
        cpl_image_delete(extracted);
        (void)cpl_error_set_(CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    /* Get the pointer to the data */
    pextracted = cpl_image_get_data_float(extracted);

    iqe_error = cpl_iqe(pextracted, inx, iny, parm, sdev);
    cpl_image_delete(extracted);
    cpl_ensure(!iqe_error, CPL_ERROR_ILLEGAL_INPUT, NULL);

    /* Create the result cpl_bivector */
    res = cpl_bivector_new(7);
    res_x = cpl_bivector_get_x_data(res);
    res_y = cpl_bivector_get_y_data(res);
    res_x[0] = (double)llx + parm[0];
    res_y[0] = sdev[0];
    res_x[1] = (double)lly + parm[2];
    res_y[1] = sdev[2];
    res_x[2] = parm[1];
    res_y[2] = sdev[1];
    res_x[3] = parm[3];
    res_y[3] = sdev[3];
    res_x[4] = parm[4];
    res_y[4] = sdev[4];
    res_x[5] = parm[5];
    res_y[5] = sdev[5];
    res_x[6] = parm[6];
    res_y[6] = sdev[6];
    return res;
}

/* Declare local functions private to avoid namespace conflicts */
/* Also add function declaration, to prevent problems in case the ieq() code
   is copied once again from RTD. */
static int estm9p(float *, int, int, int, int, float *, float *, float *);
static int iqebgv(float *, int, int, float *, float *, int *);
static int iqemnt(float *, int, int, float, float, float *, int);
static int iqesec(float *, int, int, float, float *, float *, int);
static int iqefit(float *, int, int, float, float *, float *, float *, int);
static int g2efit(float *, float *, int, int, float [], float [], double *);
static int g2einit(float *, float *, int, int);
static int g2efunc(int, float *, float *, float *, float *, float *);

/******************************************************************************/
/* iqe() COPIED FROM RTD 20-06-2003                                           */
/*       - unused function definition indexd() removed                        */
/*       - static modifier added to private functions                         */
/*       - old-style function definitions replaced                            */
/******************************************************************************/

/* define constants */

#define hsq2 CPL_MATH_SQRT1_2           /* constant 0.5*sqrt(2) */

#define    MA                   6    /* No. of variables                */
#define    MITER               64    /* Max. no. of iterations          */
#define    MMA                 16    /* Max. size of matrix             */
#define    SWAP(a,b) {double temp=(a);(a)=(b);(b)=temp;}

/* end define */


static     float     *pval;
static     float    *pwght;
static     int       mxx, mp;
static     double     w[9];
static     double    xi[9];
static     double    yi[9];

#ifdef _OPENMP
#pragma omp threadprivate(pval,pwght,mxx,mp,w,xi,yi)
#endif


static int cpl_iqe(
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
.PURPOSE   Estimate parameters for the Image Quality using a small
           frame around the object. The following parameters are
           estimated and given in the array 'parm':
                parm[0] = mean X position within array, first pixel = 0
                parm[1] = FWHM in X
                parm[2] = mean Y position within array, first pixel = 0
                parm[3] = FWHM in Y
                parm[4] = angle of major axis, degrees, along X = 0
                parm[5] = peak value of object above background
                parm[6] = mean background level
           Further, estimates of the standard deviation of the parameters
           are given in 'sdev'. The routine is just a sequence of calls
           to 'iqebgv', 'iqemnt', 'iqesec' and 'iqefit'.
.RETURN    status,  0: OK, <0: estimate failed,
------------------------------------------------------------------------*/
float      *pfm,
int        mx,
int        my,
float      *parm,
float      *sdev
)
{
int      n, err, nbg;
int      winsize;

float    bgv, bgs;
float    ap[6], cv[6], est[6], sec[6];

for (n=0; n<7; n++) parm[n] = sdev[n] = 0.0;

winsize = (mx * my) - 1;            /* size of sub window */

if ((err=iqebgv(pfm, mx, my, &bgv, &bgs, &nbg))) return -1;
parm[6] = bgv;
sdev[6] = bgs;

if ((err=iqemnt(pfm, mx, my, bgv, bgs, est, winsize))) return -2;
parm[0] = est[1];
parm[1] = CPL_MATH_FWHM_SIG*est[2];  /* Sigma to FWHM */
parm[2] = est[3];
parm[3] = CPL_MATH_FWHM_SIG*est[4];  /* Sigma to FWHM */
parm[5] = est[0];

if ((err=iqesec(pfm, mx, my, bgv, est, sec, winsize))) return -3;
parm[4] = CPL_MATH_DEG_RAD*sec[5];  /* Radian to Degrees      */

if ((err=iqefit(pfm, mx, my, bgv, sec, ap, cv, winsize))<0) return -4;
parm[0] = ap[1];
sdev[0] = cv[1];
parm[1] = CPL_MATH_FWHM_SIG*ap[2];  /* Sigma to FWHM */
sdev[1] = CPL_MATH_FWHM_SIG*cv[2];  /* Sigma to FWHM */
parm[2] = ap[3];
sdev[2] = cv[3];
parm[3] = CPL_MATH_FWHM_SIG*ap[4];  /* Sigma to FWHM */
sdev[3] = CPL_MATH_FWHM_SIG*cv[4];  /* Sigma to FWHM */
parm[4] = fmod(CPL_MATH_DEG_RAD*ap[5]+180.0, 180.0);
sdev[4] = CPL_MATH_DEG_RAD*cv[5];  /* Radian to Degrees      */
if (sdev[4] > 180.) sdev[4] = 180.0;        /* max is: Pi */
parm[5] = ap[0];
sdev[5] = cv[0];

return 0;
}

static int iqebgv(
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
.PURPOSE   Estimate background level for subimage
.RETURN    status,  0: OK, -1:no buffer space, -2: no points left
------------------------------------------------------------------------*/
float      *pfm,
int        mx,
int        my,
float      *bgm,
float      *bgs,
int        *nbg
)
{
  int      n, m, ns, ms, nt, mt;
  float    *pfb, *pwb, *pf, *pw;
  float    *pf0, *pf1, *pf2, *pf3, *pfs0, *pfs1, *pfs2, *pfs3;
  double   val, fks, ba, bm, bs;

  *bgm = 0.0;
  *bgs = 0.0;
  *nbg = 0;

  pfs0 = pfm;
  pfs1 = pfm + mx - 1;
  pfs2 = pfm + mx*(my-1);
  pfs3 = pfm + mx*my - 1;

  ns = (mx<my) ? mx - 1 : my - 1;
  ms  = (mx<my) ? mx/4 : my/4;
  pfb = (float *) cpl_calloc(8*ns*ms, sizeof(float));
  if (!pfb) return -1;
  pwb = pfb + 4*ns*ms;

/* extrat edge of matrix from each corner  */

  nt = 0;
  pf = pfb;
  for (m=0; m<ms; m++) {
     pf0 = pfs0; pf1 = pfs1; pf2 = pfs2; pf3 = pfs3;
     for (n=0; n<ns; n++) {
    *pf++ = *pf0++;
    *pf++ = *pf1; pf1 += mx;
    *pf++ = *pf2; pf2 -= mx;
    *pf++ = *pf3--;
      }
     nt += 4*ns;
     ns -= 2;
     pfs0 += mx + 1;
     pfs1 += mx - 1;
     pfs2 -= mx - 1;
     pfs3 -= mx + 1;
   }

/*  skip all elements with zero weight and sort clean array */

  pf0 = pfb; pw = pwb;
  n = nt;
     mt = nt;
     while (n--) *pw++ = 1.0;
  cpl_tools_sort_ascn_float(pfb, mt);
  nt = mt;

/* first estimate of mean and rms   */

  m = mt/2; n = mt/20;
  ba = pfb[m];
  bs = 0.606*(ba-pfb[n]);                     /*  5% point at 1.650 sigma */
  if (bs<=0.0) bs = sqrt(fabs(ba));      /* assume sigma of Poisson dist. */
  *bgm = ba;

/* then do 5 loops kappa sigma clipping  */

  for (m=0; m<5; m++) {
     pf = pfb; pw = pwb;
     fks = 5.0 * bs;
     bm = bs = 0.0; mt = 0;
     for (n=0; n<nt; n++, pw++) {
    val = *pf++;
    if (0.0<*pw && fabs(val-ba)<fks) {
       bm += val; bs += val*val; mt++;
     }
    else *pw = 0.0;
      }
     if (mt<1) { cpl_free (pfb); return -2; }
     ba = bm/mt; bs = bs/mt - ba*ba;
     bs = (0.0<bs) ? sqrt(bs) : 0.0;
   }

/* set return values and clean up     */

  *bgm = ba;
  *bgs = bs;
  *nbg = mt;
  cpl_free(pfb);

  return 0;
}

static int iqemnt(
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
.PURPOSE   Find center of object and do simple moment analysis
.COMMEMTS  The parameter array 'amm' is as follows:
              amm[0] = amplitude over background
              amm[1] = X center,   amm[2] = X sigma;
              amm[3] = Y center,   amm[4] = Y sigma;
              amm[5] = angle of major axis
.RETURN    status,  0: OK,  -1: mean<0.0
------------------------------------------------------------------------*/
float      *pfm,
int        mx,
int        my,
float      bgv,
float      bgs,
float      *amm,
int        winsize
)
{
int      n, nx, ny, nt, nxc, nyc, ndx, ndy, ioff;
int      k, ki, ks, kn, psize;
float    av, dx, dy;
float    *pf;
double   val, x, y, dv, xm, ym;
double   am, ax, ay, axx, ayy, axy;

/* Missing initialization fixed */
ndx = ndy = 0;
av = 0.0;

dv = 5.0*bgs;
xm = mx - 1.0;
ym = my - 1.0;
for (nx=0; nx<6; nx++) amm[nx] = 0.0;

/* get approx. center of object by going up along the gradient    */

n = nx = ny = 1;
nxc = mx/2; nyc = my/2;
nt = (nxc<nyc) ? nxc : nyc;
while (nt--)
   {
   if (estm9p(pfm, mx, my, nxc, nyc, &av, &dx, &dy)) break;

   if (n)
      n = 0;
   else
      {
      if (dx*ndx<0.0) nx = 0;
      if (dy*ndy<0.0) ny = 0;
      }
   if (!nx && !ny) break;

   ndx = (0.0<dx) ? nx : -nx;
   ndy = (0.0<dy) ? ny : -ny;
   nxc += ndx;
   nyc += ndy;
   }

/* then try a simple moment of pixels above 5 sigma  */

y = 0.0;
nt = 0; ny = my;
pf = pfm;
ax = ay = 0.0;
while (ny--)
   {
   x = 0.0;
   nx = mx;                /* ojo! */
   while (nx--)
      {
      val = *pf++ - bgv;
      if (dv<val)
         {
         ax  += x;
         ay  += y;
         nt++;
         }
      x += 1.0;
      }
   y += 1.0;
   }
if (nt<1) return -1;
nx = floor(ax/nt); ny = floor(ay/nt);
val = pfm[nx+mx*ny];
if (av<val) { nxc = nx; nyc = ny; }         /* the higher peak wins  */

/* finally, compute moments just around this position  */

nt = 0; nx = 1;
x = nxc; y = nyc;
ioff = nxc+mx*nyc;
n = (mx<my) ? mx-1 : my-1;
pf = pfm + ioff;
psize = pf - pfm;
if ((psize < 0) || (psize > winsize)) return -99;

   val = *pf - bgv;
   am  = val;
   ax  = val*x;
   ay  = val*y;
   axx = val*x*x;
   ayy = val*y*y;
   axy = val*x*y;
   nt++;

ki = ks = kn = 1;
while (n--)
   {
   k = kn;
   if (!ki && ks==-1)
      {
      if (nx)
         nx = 0;
      else
         break;
      }
   ioff = (ki) ? ks : ks*mx;
   while (k--)
      {
      if (ki) x += ks; else y += ks;
      if (x<0.0 || y<0.0 || xm<x || ym<y) break;

      pf += ioff;
      psize = pf - pfm;
      if ((psize < 0) || (psize > winsize)) break;

      val = *pf - bgv;
      if (dv<val)
         {
         am  += val;
         ax  += val*x;
         ay  += val*y;
         axx += val*x*x;
         ayy += val*y*y;
         axy += val*x*y;
         nt++; nx++;
         }
      }
   if ((ki=(!ki))) { ks = -ks; kn++; }
   }
if (am<=0.0) return -1;

/* normalize the moments and put them in to the output array   */

amm[1] = ax/am;
amm[3] = ay/am;
axx = axx/am - amm[1]*amm[1];
amm[2] = (0.0<axx) ? sqrt(axx) : 0.0;
ayy = ayy/am - amm[3]*amm[3];
amm[4] = (0.0<ayy) ? sqrt(ayy) : 0.0;
axy = (axy/am-amm[1]*amm[3])/axx;
amm[5] = fmod(atan(axy)+CPL_MATH_PI, CPL_MATH_PI);
nx = amm[1]; ny = amm[3];
amm[0] = pfm[nx+ny*mx] - bgv;

return 0;
}

static int estm9p(
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
.PURPOSE   Estimate parameters for 3x3 pixel region
.RETURN    status, 0:OK, -1:out of range,
------------------------------------------------------------------------*/
float      *pfm,
int        mx,
int        my,
int        nx,
int        ny,
float      *rm,
float      *dx,
float      *dy
)
{
int      n, nt, ix, iy;
float    a, am;
float    *pfb, *pwb, fb[9], _fb[9], wb[9];

cpl_size  idx[9];

/* check if 3x3 region is fully within frame   */

if (nx<1 || mx<nx-2 || ny<1 || my<ny-2) return -1;


/* extract region into local array and generate a rank index for it */

iy = 3;
pfb = fb; pwb = wb;
pfm += nx-1 + mx*(ny-1);
   while(iy--)
      {
      ix = 3;
      while (ix--)
         {
         *pfb++ = *pfm++;
         *pwb++ = 1.0;
         }
      pfm += mx - 3;
      }
memcpy(_fb, fb, 9 * sizeof(float));
cpl_tools_sort_stable_pattern_float(_fb, 9, 0, 0, idx);
/* omit largest value and estimate mean     */

wb[idx[8]] = 0.0;

nt = 0;
am = 0.0;
for (n=0; n<9; n++) {
   if (0.0<wb[n]) { am += fb[n]; nt++; }
   }
*rm = am/nt;;


/* calculate mean gradient in X and Y */

a = am = 0.0; ix = iy = 0;
for (n=0; n<9; n +=3) {
   if (0.0<wb[n]) a += fb[n], ix++;
   if (0.0<wb[n+2]) am +=fb[n+2], iy++;
   }
*dx = 0.5*(am/iy - a/ix);

a = am = 0.0; ix = iy = 0;
for (n=0; n<3; n++) {
   if (0.0<wb[n]) a += fb[n], ix++;
   if (0.0<wb[n+6]) am +=fb[n+6], iy++;
   }

*dy = 0.5*(am/iy - a/ix);

return 0;
}

static int iqesec(
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
.PURPOSE   Perform a sector analysis of object. Estimates for center and
           size are given in 'est' which is used for bootstrap.
.COMMEMTS  The parameter arrays 'est' and 'sec' are as follows
              sec[0] = amplitude over background
              sec[1] = X center,   sec[2] = X sigma;
              sec[3] = Y center,   sec[4] = Y sigma;
              sec[5] = angle of major axis
.RETURN    status, 0:OK, -1: no buffer,
------------------------------------------------------------------------*/
float      *pfm,
int        mx,
int        my,
float      bgv,
float      *est,
float      *sec,
int        winsize
)
{
int      n, k, ki, ks, kn, nxc, nyc, ioff, idx;
int      ns[8], psize;
float    *pf, f;
double   x, y, xm, ym, xc, yc, dx, dy;
double    r, rl, rh, sb[8], a2r, a2i;

/* initiate basic variables    */

  for (n=0; n<6; n++) sec[n] = 0.0;
  for (n=0; n<8; n++) sb[n] = 0.0, ns[n] = 0;
  xc = x = est[1]; xm = mx - 1.0;
  yc = y = est[3]; ym = my - 1.0;
  if (est[2]<est[4]) {
     rl = 2.0*est[2]; rh = 4.0*est[4]; n = ceil(16.0*est[4]);
   }
  else {
     rl = 2.0*est[4]; rh = 4.0*est[2]; n = ceil(16.0*est[2]);
   }

/* extract the sectors around the center of the object  */

  nxc = floor(x+0.5); nyc = floor(y+0.5);
  ioff = nxc+mx*nyc;
  pf = pfm + ioff;

  ki = ks = kn = 1;
  while (n--) {
     k = kn;
     ioff = (ki) ? ks : ks*mx;
     while (k--) {
    if (ki) x += ks; else y += ks;
    if (x<0.0 || y<0.0 || xm<x || ym<y) break;

    pf += ioff;
        psize = pf - pfm;
        if ((psize < 0) || (psize > winsize)) break;

    dx = x - xc; dy = y - yc;
    r = sqrt(dx*dx + dy*dy);
    if (rl<r && r<rh) {
       f = *pf - bgv;
       idx = ((int) (CPL_MATH_4_PI*atan2(y-yc,x-xc)+8.5))%8;
       sb[idx] += (0.0<f) ? f : 0.0;
       ns[idx]++;
     }
      }
     if ((ki=(!ki))) { ks = -ks; kn++; }
   }

/* normalize the sector array and do explicit FFT for k=1,2  */

  for (n=0; n<8; n++) {
     if (ns[n]<1) ns[n] = 1;
     sb[n] /= ns[n];
   }

  a2r = sb[0]-sb[2]+sb[4]-sb[6];
  a2i = sb[1]-sb[3]+sb[5]-sb[7];

  for (n=0; n<6; n++) sec[n] = est[n];        /* copy estimates over  */
  if (a2r==0.0 && a2i==0.0) return -2;
  sec[5] = fmod(0.5*atan2(a2i,a2r), CPL_MATH_PI);

  return 0;
}

static int iqefit(
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
.PURPOSE   Fit 2D Gaussian function to PSF
.COMMEMTS  The parameter arrays 'est' and 'ap' are as follows
              ap[0] = amplitude over background
              ap[1] = X center,   ap[2] = X sigma;
              ap[3] = Y center,   ap[4] = Y sigma;
              ap[5] = angle of major axis
.RETURN    no. of iterations, <0: error, -10: no buffer
------------------------------------------------------------------------*/
float      *pfm,
int        mx,
int        my,
float      bgv,
float      *est,
float      *ap,
float      *cm,
int        winsize
)
{
int      n, ix, iy, nx, ny, nxs, nys, psize;
float    *pfb, *pwb, *pf, *pw, *pfmo;
double   chi;


/* initialize basic variables    */

for (n=0; n<6; n++) ap[n] = cm[n] = 0.0;


/* allocate buffer for a 4 sigma region around the object */

pfmo = pfm;                /* keep original start of array */

nxs = floor(est[1] - 4.0*est[2]);
if (nxs<0) nxs = 0;
nys = floor(est[3] - 4.0*est[4]);
if (nys<0) nys = 0;
nx = ceil(8.0*est[2]);
if (mx<nxs+nx) nx = mx - nxs;
ny = ceil(8.0*est[4]);
if (my<nys+ny) ny = my - nys;

pfb = (float *) cpl_calloc(2*nx*ny, sizeof(float));
if (!pfb) return -10;
pwb = pfb + nx*ny;


/* extract region from external buffer */

pfm += nxs + mx*nys;
psize = pfm - pfmo;
if ((psize < 0) || (psize > winsize)) { cpl_free (pfb); return -99; }


pf = pfb; pw = pwb;
iy = ny;
   while (iy--)
      {
      ix = nx;
      while (ix--)
         {
         *pf++ = *pfm++ - bgv;
         psize = pfm - pfmo;
         if (psize > winsize) { cpl_free (pfb); return -99; }

         *pw++ = 1.0;
         }
      pfm += mx - nx;
      psize = pfm - pfmo;
      if ((psize < 0) || (psize > winsize)) { cpl_free (pfb); return -99; }
      }

/* initialize parameters for fitting    */

ap[0] = est[0];
ap[1] = est[1] - nxs;
ap[2] = est[2];
ap[3] = est[3] - nys;
ap[4] = est[4];
ap[5] = est[5];

/* perform actual 2D Gauss fit on small subimage  */

n = g2efit(pfb, pwb, nx, ny, ap, cm, &chi);

/* normalize parameters and uncertainties, and exit   */

ap[1] += nxs;
ap[3] += nys;

cpl_free(pfb);
return n;
}

static int g2einit(
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
.PURPOSE   Initiate gauss fit function, set pointers to data and weights
.RETURN    status,  0: OK, -1: error - bad pixel no.
------------------------------------------------------------------------*/
float      *val,
float      *wght,
int        nx,
int        ny
)
{
double   fh, w1, w2, w3;

  if (nx<1) {                     /* if NO x-pixel set to NULL          */
     pval  = (float *) 0;
     pwght = (float *) 0;
     mxx = mp = 0;
     return -1;
   }

  pval = val;                     /* otherwise initiate static varables */
  pwght = wght;
  mxx = nx;
  mp = (0<ny) ? ny*nx : nx;

  fh = 0.5*sqrt(3.0/5.0);      /* positions and weights for integration */
  w1 = 16.0/81.0;
  w2 = 10.0/81.0;
  w3 = 25.0/324.0;

  xi[0] = 0.0; yi[0] = 0.0; w[0] = w1;
  xi[1] = 0.0; yi[1] =  fh; w[1] = w2;
  xi[2] = 0.0; yi[2] = -fh; w[2] = w2;
  xi[3] =  fh; yi[3] = 0.0; w[3] = w2;
  xi[4] = -fh; yi[4] = 0.0; w[4] = w2;
  xi[5] =  fh; yi[5] =  fh; w[5] = w3;
  xi[6] = -fh; yi[6] =  fh; w[6] = w3;
  xi[7] =  fh; yi[7] = -fh; w[7] = w3;
  xi[8] = -fh; yi[8] = -fh; w[8] = w3;

  return 0;
}

static int g2efunc(
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
.PURPOSE   evaluate function value for given index
.RETURN    status,  0: OK, 1: error - bad pixel no.
------------------------------------------------------------------------*/
int        idx,
float      *val,
float      *fval,
float      *psig,
float      *a,
float      *dyda
)
{
  int      n;
  double   ff, sum, ci, si;
  double   xc, yc, xx, yy, x, y;

  if (idx<0 || mp<=idx) return -1;              /* check index          */
  if (pwght && pwght[idx]<0.0) return 1;        /* check if valid pixel */
  if (a[2]<=0.0 || a[4]<=0.0) return -2;        /* negative sigmas      */

  xc = (double) (idx%mxx) - a[1];
  yc = (double) (idx/mxx) - a[3];

  *val = pval[idx];
  *psig = (pwght) ? pwght[idx] : 1.0;
  si = sin(a[5]);
  ci = cos(a[5]);

  sum = 0.0;
  for (n=0; n<9; n++) {
     x  = xc + xi[n];
     y  = yc + yi[n];
     xx = (ci*x + si*y)/a[2];
     yy = (-si*x + ci*y)/a[4];
     sum += w[n]*exp(-0.5*(xx*xx+yy*yy));
   }
  xx = (ci*xc + si*yc)/a[2];
  yy = (-si*xc + ci*yc)/a[4];

  ff    = a[0]*sum;
  *fval = ff;

  dyda[0] = sum;
  dyda[1] = ff*(ci*xx/a[2] - si*yy/a[4]);
  dyda[2] = ff*xx*xx/a[2];
  dyda[3] = ff*(si*xx/a[2] + ci*yy/a[4]);
  dyda[4] = ff*yy*yy/a[4];
  dyda[5] = ff*((si*xc-ci*yc)*xx/a[2] + (ci*xc+si*yc)*yy/a[4]);

  return 0;
}

static int g2efunc2(int ndata, int npar, double *p, double *deviates,
                    double **derivs, void *d)
{
    int i, j;
    float * dyda = cpl_malloc(npar * sizeof(*dyda));
    float fp[MA];

    assert(d == NULL);

    for (j = 0; j < MA; j++) {
        fp[j] = p[j];
    }

    /* Compute function deviates */
    for (i=0; i<ndata; i++) {
        float z, err, val;
        int st = g2efunc(i, &val, &z, &err, fp, dyda);
        if (st < 0) {
            /* error */
            cpl_free(dyda);
            return st;
        }
        if (st > 0 || err == 0.) {
            /* bad pixel */
            deviates[i] = 0.;
            if (derivs) {
                int k;
                for (k=0; k<npar; k++) {
                    if (derivs[k]) {
                        derivs[k][i] = 0.;
                    }
                }
            }
        }
        else {
            deviates[i] = (val - z) / err;
            if (derivs) {
                int k;
                for (k=0; k<npar; k++) {
                    if (derivs[k]) {
                        derivs[k][i] = -dyda[k] / err;
                    }
                }
            }
        }
    }
    cpl_free(dyda);
    return 0;
}

static int g2efit(
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
.PURPOSE   Perform 2D Gauss fit
.RETURN    status,  no. of iterations, else  -1: error - bad pixel no,
                                             -2: error - iteration.
------------------------------------------------------------------------*/
float      *val,
float      *wght,
int        nx,
int        ny,
float      ap[MA],
float      cv[MA],
double     *pchi
)
{
    int i;
    int status;
    double * a = cpl_malloc((MA) * sizeof(*a));
    mp_par * pars = cpl_calloc(MA, sizeof(*pars));
    mp_result result;

    if (g2einit(val, wght, nx, ny)) return -1;

    memset(&result, 0, sizeof(result));
    for (i = 0; i < MA; i++) {
        a[i] = ap[i];
        pars[i].side = 3;
    }
    /* no negative sigma */
    pars[2].limited[0] = 1;
    pars[2].limits[0] = 0.;
    pars[4].limited[0] = 1;
    pars[4].limits[0] = 0.;

    result.xerror = cpl_malloc(MA * sizeof(result.xerror[0]));

    status = cpl_mpfit((mp_func)&g2efunc2, nx * ny, MA, a, pars,
                       NULL, NULL, &result);


    for (i = 0; i < MA; i++) {
        ap[i] = a[i];
        cv[i] = result.xerror[i];
    }
    ap[5] = fmod(ap[5], 4.0*atan(1.0));
    *pchi = result.bestnorm;

    cpl_free(a);
    cpl_free(result.xerror);
    cpl_free(pars);
    
    if (status <= 0) return -2;
    if (ap[1]<0.0 || nx<ap[1] || ap[3]<0.0 || ny<ap[3]) return -3;
    if (result.niter > MITER) return -4;
    return result.niter;
}
