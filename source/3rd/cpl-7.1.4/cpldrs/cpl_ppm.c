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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>

#include <cpl_memory.h>
#include <cpl_errorstate.h>
#include <cpl_msg.h>
#include <cpl_math_const.h>
#include <cpl_table.h>
#include <cpl_polynomial.h>
#include <cpl_ppm.h>

/**
 * @defgroup cpl_ppm Point pattern matching module
 *
 * @par Synopsis:
 * @code
 *   #include "cpl_ppm.h"
 * @endcode
 */

/* The "triangle" is actually the element 2D pattern made of 3 points.
   It carries its coordinates in the parameter space of similar
   triangles, and an identifier of the three points composing it.
   The identifiers are the matrix column sequence number in the
   matrix containing the points (in the main program). */

typedef struct _triangle
{
    double ratsq;           /* (Rmin/Rmax)^2                      */
    double dratsq;          /* Error                              */

    double theta;           /* Angle min - Angle max  in [0; 2pi[ */
    double dtheta;          /* Error                              */

    double xref, yref;      /* Reference point                    */
    double xmin, ymin;      /* Nearest point                      */
    double xmax, ymax;      /* Farthest point                     */

    int    id_ref;          /* Identifier of reference point      */  
    int    id_min;          /* Identifier of nearest point        */  
    int    id_max;          /* Identifier of farthest point       */  

} triangle;

/* Here we have a set of triangles... */

typedef struct _triangles
{
    int        n;
    triangle **t;
} triangles;

static cpl_array *
find_all_matches(const cpl_matrix *, const cpl_matrix *, 
                 double, const cpl_polynomial *, 
                 const cpl_polynomial *,
                 cpl_matrix **, cpl_matrix **) CPL_ATTR_ALLOC;

static cpl_array *
find_matches(const triangles *, const triangles *, int) CPL_ATTR_ALLOC;

static triangles *triangles_new(int) CPL_ATTR_ALLOC;

static triangles *
triangles_from_points(const cpl_matrix *, int, double) CPL_ATTR_ALLOC;

static triangle *
triangle_new(double, double, double, double, double, double,
             double, int, int, int) CPL_ATTR_ALLOC;

/**@{*/

/**
 * @brief
 *   Match 1-D patterns.
 *   
 * @param peaks     List of observed positions (e.g., of emission peaks)
 * @param lines     List of positions in searched pattern (e.g., wavelengths)
 * @param min_disp  Min expected scale (e.g., spectral dispersion in A/pixel)
 * @param max_disp  Max expected scale (e.g., spectral dispersion in A/pixel)
 * @param tolerance Tolerance for interval ratio comparison
 * @param seq_peaks Returned: index of identified peaks in input @em peaks
 * @param seq_lines Returned: index of identified lines in input @em lines
 *  
 * @return List of all matching points positions
 * 
 * This function attempts to find the reference pattern @em lines in a 
 * list of observed positions @em peaks. In the following documentation 
 * a terminology drawn from the context of arc lamp spectra calibration
 * is used for simplicity: the reference pattern is then a list of
 * wavelengths corresponding to a set of reference arc lamp emission
 * lines - the so-called line catalog; while the observed positions are 
 * the positions (in pixel) on the CCD, measured along the dispersion 
 * direction, of any significant peak of the signal. To identify the
 * observed peaks means to associate them with the right reference
 * wavelength. This is attempted here with a point-pattern matching 
 * technique, where the pattern is contained in the vector @em lines, 
 * and is searched in the vector @em peak. 
 *
 * In order to work, this method just requires a rough expectation
 * value of the spectral dispersion (in Angstrom/pixel), and a line
 * catalog. The line catalog @em lines should just include lines that 
 * are expected somewhere in the CCD exposure of the calibration lamp
 * (note, however, that a catalog including extra lines at its blue 
 * and/or red ends is still allowed).
 * 
 * Typically, the arc lamp lines candidates @em peak will include 
 * light contaminations, hot pixels, and other unwanted signal, 
 * but only in extreme cases does this prevent the pattern-recognition 
 * algorithm from identifying all the spectral lines. The pattern 
 * is detected even in the case @em peak contained more arc lamp 
 * lines than actually listed in the input line catalog.
 * 
 * This method is based on the assumption that the relation between
 * wavelengths and CCD positions is with good approximation @em locally
 * linear (this is always true, for any modern spectrograph).
 * 
 * The ratio between consecutive intervals pairs in wavelength and in 
 * pixel is invariant to linear transformations, and therefore this 
 * quantity can be used in the recognition of local portions of the 
 * searched pattern. All the examined sub-patterns will overlap, leading 
 * to the final identification of the whole pattern, notwithstanding the 
 * overall non-linearity of the relation between pixels and wavelengths.
 *
 * Ambiguous cases, caused by exceptional regularities in the pattern,
 * or by a number of undetected (but expected) peaks that disrupt the
 * pattern on the data, are recovered by linear interpolation and 
 * extrapolation of the safely identified peaks. 
 *
 * More details about the applied algorithm can be found in the comments
 * to the function code.
 *
 * The @em seq_peaks and @em seq_lines are array reporting the positions
 * of matching peaks and wavelengths in the input @em peaks and @em lines
 * vectors. This functionality is not yet supported: this arguments
 * should always be set to NULL or a CPL_ERROR_UNSUPPORTED_MODE would
 * be set.
 */

cpl_bivector *cpl_ppm_match_positions(const cpl_vector *peaks,
                                      const cpl_vector *lines,
                                      double min_disp, double max_disp,
                                      double tolerance, cpl_array **seq_peaks, 
                                      cpl_array **seq_lines)
{

  int      i, j, k, l;
  cpl_size nlint, npint;
  int      minpos;
  double   min;
  double   lratio, pratio;
  double   lo_start, lo_end, hi_start, hi_end, denom;
  double   disp = 0.0;
  double   variation, prev_variation;
  int      max, maxpos, minl, mink;
  int      ambiguous;
  int      npeaks_lo, npeaks_hi;
  int     *peak_lo;
  int     *peak_hi;
  int    **ident;
  int     *nident;
  int     *lident;

  const double  *peak;
  const double  *line;
  cpl_size       npeaks, nlines;

  double  *xpos;
  double  *lambda;
  int     *ilambda;
  double  *tmp_xpos;
  double  *tmp_lambda;
  int     *tmp_ilambda;
  int     *flag;
  int      n = 0;
  int      nn;
  int      nseq = 0;
  int      gap;
  int     *seq_length;
  int      found;


  if (seq_lines || seq_peaks) {
      cpl_error_set(cpl_func, CPL_ERROR_UNSUPPORTED_MODE);
      return NULL;
  }

  peak        = cpl_vector_get_data_const(peaks);
  npeaks      = cpl_vector_get_size(peaks);
  line        = cpl_vector_get_data_const(lines);
  nlines      = cpl_vector_get_size(lines);

  if (npeaks < 4)
      return NULL;

  peak_lo     = cpl_malloc((size_t)npeaks * sizeof(int));
  peak_hi     = cpl_malloc((size_t)npeaks * sizeof(int));
  nident      = cpl_calloc((size_t)npeaks, sizeof(int));
  lident      = cpl_calloc((size_t)nlines, sizeof(int));
  xpos        = cpl_calloc((size_t)npeaks, sizeof(double));
  lambda      = cpl_calloc((size_t)npeaks, sizeof(double));
  ilambda     = cpl_calloc((size_t)npeaks, sizeof(int));
  tmp_xpos    = cpl_calloc((size_t)npeaks, sizeof(double));
  tmp_lambda  = cpl_calloc((size_t)npeaks, sizeof(double));
  tmp_ilambda = cpl_calloc((size_t)npeaks, sizeof(int));
  flag        = cpl_calloc((size_t)npeaks, sizeof(int));
  seq_length  = cpl_calloc((size_t)npeaks, sizeof(int));
  ident       = cpl_malloc((size_t)npeaks * sizeof(int *));
  for (i = 0; i < npeaks; i++)
      ident[i]  = cpl_malloc(3 * (size_t)npeaks * sizeof(int));

  /*
   * This is just the number of intervals - one less than the number
   * of points (catalog wavelengths, or detected peaks).
   */

  nlint = nlines - 1;
  npint = npeaks - 1;


  /*
   * Here the big loops on catalog lines begins.
   */

  for (i = 1; i < nlint; i++) {


    /*
     * For each catalog wavelength I take the previous and the next one, 
     * and compute the ratio of the corresponding wavelength intervals.
     * This ratio will be compared to all the ratios obtained doing the
     * same with all the detected peaks positions.
     */

    lratio = (line[i+1] - line[i]) / (line[i] - line[i-1]);


    /*
     * Here the loop on detected peaks positions begins.
     */

    for (j = 1; j < npint; j++) {

      /*
       * Not all peaks are used for computing ratios: just the ones
       * that are compatible with the expected spectral dispersion
       * are taken into consideration. Therefore, I define the pixel
       * intervals before and after any peak that are compatible with
       * the specified dispersion interval, and select just the peaks
       * within such intervals. If either of the two intervals doesn't
       * contain any peak, then I skip the current peak and continue
       * with the next.
       */

      lo_start = peak[j] - (line[i] - line[i-1]) / min_disp;
      lo_end   = peak[j] - (line[i] - line[i-1]) / max_disp;
      hi_start = peak[j] + (line[i+1] - line[i]) / max_disp;
      hi_end   = peak[j] + (line[i+1] - line[i]) / min_disp;

      for (npeaks_lo = 0, k = 0; k < npeaks; k++) {
        if (peak[k] > lo_end)
          break;
        if (peak[k] > lo_start) {
          peak_lo[npeaks_lo] = k;
          ++npeaks_lo;
        }
      }

      if (npeaks_lo == 0)
        continue;

      for (npeaks_hi = 0, k = 0; k < npeaks; k++) {
        if (peak[k] > hi_end)
          break;
        if (peak[k] > hi_start) {
          peak_hi[npeaks_hi] = k;
          ++npeaks_hi;
        }
      }

      if (npeaks_hi == 0)
        continue;


      /*
       * Now I have all peaks that may help for a local identification.
       * peak_lo[k] is the sequence number of the k-th peak of the lower
       * interval; peak_hi[l] is the sequence number of the l-th peak of
       * the higher interval. j is, of course, the sequence number of the
       * current peak (second big loop).
       */

      prev_variation = 1000.0;
      minl = mink = 0;

      for (k = 0; k < npeaks_lo; k++) {
        denom = peak[j] - peak[peak_lo[k]];
        for (l = 0; l < npeaks_hi; l++) {

          /*
           * For any pair of peaks - one from the lower and the other
           * from the higher interval - I compute the same ratio that
           * was computed with the current line catalog wavelength.
           */

          pratio = (peak[peak_hi[l]] - peak[j]) / denom;

          /*
           * If the two ratios are compatible within the specified
           * tolerance, we have a preliminary identification. This
           * will be marked in the matrix ident[][], where the first
           * index corresponds to a peak sequence number, and the second
           * index is the counter of the identifications made during
           * this whole process. The array of counters is nident[].
           * If more than one interval pair fulfills the specified
           * tolerance, the closest to the expected ratio is selected.
           */

          variation = fabs(lratio-pratio) / pratio;

          if (variation < tolerance) {
            if (variation < prev_variation) {
              prev_variation = variation;
              minl = l;
              mink = k;
            }
          }
        }
      }
      if (prev_variation < tolerance) {
        ident[j][nident[j]]                         = i;
        ident[peak_hi[minl]][nident[peak_hi[minl]]] = i + 1;
        ident[peak_lo[mink]][nident[peak_lo[mink]]] = i - 1;
        ++nident[j];
        ++nident[peak_hi[minl]];
        ++nident[peak_lo[mink]];
      }
    }   /* End loop on positions */
  }    /* End loop on lines     */


  /*
   * At this point I have filled the ident matrix with all my preliminary
   * identifications. Ambiguous identifications must be eliminated.
   */


  for (i = 0; i < npeaks; i++) {


    /*
     * I don't take into consideration peaks that were never identified.
     * They are likely contaminations, or emission lines that were not
     * listed in the input wavelength catalog.
     */

    if (nident[i] > 1) {


      /*
       * Initialise the histogram of wavelengths assigned to the i-th peak.
       */

      for (j = 0; j < nlines; j++)
        lident[j] = 0;


      /*
       * Count how many times each catalog wavelength was assigned
       * to the i-th peak.
       */

      for (j = 0; j < nident[i]; j++)
        ++lident[ident[i][j]];


      /*
       * What wavelength was most frequently assigned to the i-th peak?
       */

      max = 0;
      maxpos = 0;
      for (j = 0; j < nlines; j++) {
        if (max < lident[j]) {
          max = lident[j];
          maxpos = j;
        }
      }


      /*
       * Were there other wavelengths assigned with the same frequency?
       * This would be the case of an ambiguous identification. It is
       * safer to reject this peak...
       */

      ambiguous = 0;

      for (k = maxpos + 1; k < nlines; k++) {
        if (lident[k] == max) {
          ambiguous = 1;
          break;
        }
      }

      if (ambiguous)
        continue;


      /*
       * Otherwise, I assign to the i-th peak the wavelength that was
       * most often assigned to it.
       */

      tmp_xpos[n]   = peak[i];
      tmp_lambda[n] = line[maxpos];
      tmp_ilambda[n] = maxpos;

      ++n;

    }

  }


  /*
   * Check on identified peaks. Contaminations from other spectra might 
   * be present and should be excluded: this type of contamination 
   * consists of peaks that have been _correctly_ identified! The non-
   * spectral type of light contamination should have been almost all 
   * removed already in the previous steps, but it may still be present.
   * Here, the self-consistent sequences of identified peaks are
   * separated one from the other. At the moment, just the longest of
   * such sequences is selected (in other words, spectral multiplexing
   * is ignored).
   */

  if (n > 1) {
    nn = 0;                  /* Number of peaks in the list of sequences */
    nseq = 0;                /* Current sequence */
    for (k = 0; k < n; k++) {
      if (flag[k] == 0) {    /* Was peak k already assigned to a sequence? */
        flag[k] = 1;
        xpos[nn] = tmp_xpos[k];       /* Begin the nseq-th sequence */
        lambda[nn] = tmp_lambda[k];
        ilambda[nn] = tmp_ilambda[k];
        ++seq_length[nseq];
        ++nn;

        /*
         * Now look for all the following peaks that are compatible
         * with the expected spectral dispersion, and add them in 
         * sequence to xpos. Note that missing peaks are not a problem...
         */
         
        i = k;
        while (i < n - 1) {
          found = 0;
          for (j = i + 1; j < n; j++) {
            if (flag[j] == 0) {
              disp = (tmp_lambda[j] - tmp_lambda[i])
                   / (tmp_xpos[j] - tmp_xpos[i]);
              if (disp >= min_disp && disp <= max_disp) {
                flag[j] = 1;
                xpos[nn] = tmp_xpos[j];
                lambda[nn] = tmp_lambda[j];
                ilambda[nn] = tmp_ilambda[j];
                ++seq_length[nseq];
                ++nn;
                i = j;
                found = 1;
                break;
              }
            }
          }
          if (!found)
            break;
        }

        /*
         * Current sequence is completed: begin new sequence on the
         * excluded peaks, starting the loop on peaks again.
         */

        ++nseq;
        k = 0;
      }
    }


    /*
     * Find the longest sequence of self-consistent peaks.
     */

    max = 0;
    maxpos = 0;
    for (i = 0; i < nseq; i++) {
      if (seq_length[i] > max) {
        max = seq_length[i];
        maxpos = i;
      }
    }

    /*
     * Find where this sequence starts in the whole peak position
     * storage.
     */

    nn = 0;
    for (i = 0; i < maxpos; i++)
      nn += seq_length[i];

    /*
     * Move the longest sequence at the beginning of the returned lists
     */

    n = max;
    for (i = 0; i < n; i++, nn++) {
      xpos[i] = xpos[nn];
      lambda[i] = lambda[nn];
      ilambda[i] = ilambda[nn];
    }


    /*
     * Are some wavelengths missing? Recover them.
     */

    for (i = 1; i < n; i++) {
      gap = ilambda[i] - ilambda[i-1];
      for (j = 1; j < gap; j++) {

        if (j == 1) {

          /*
           * Determine the local dispersion from the current pair of peaks
           */
  
          disp = (lambda[i] - lambda[i-1]) / (xpos[i] - xpos[i-1]);
        }

        /*
         * With this, find the expected position of the missing
         * peak by linear interpolation.
         */

        hi_start = xpos[i-1] + (line[ilambda[i-1] + j] - lambda[i-1]) / disp;

        /*
         * Is there a peak at that position? Here a peak from the
         * original list is searched, that is closer than 2 pixels
         * to the expected position. If it is found, insert it at
         * the current position on the list of identified peaks,
         * and leave immediately the loop (taking the new position
         * for the following linear interpolation, in case more
         * than one peak is missing in the current interval).
         * If it is not found, stay in the loop, looking for 
         * the following missing peaks in this interval.
         */

        found = 0;
        for (k = 0; k < npeaks; k++) {
          if (fabs(peak[k] - hi_start) < 2) {
            for (l = n; l > i; l--) {
              xpos[l] = xpos[l-1];
              lambda[l] = lambda[l-1];
              ilambda[l] = ilambda[l-1];
            }
            xpos[i] = peak[k];
            lambda[i] = line[ilambda[i-1] + j];
            ilambda[i] = ilambda[i-1] + j;
            ++n;
            found = 1;
            break;
          }
        }
        if (found)
          break;
      }
    }


    /*
     * Try to extrapolate forward
     */

    found = 1;

    if (n > 0) {
        while (ilambda[n-1] < nlines - 1 && found) {

          /*
           * Determine the local dispersion from the last pair of 
           * identified peaks
           */

          if (n > 1)
              disp = (lambda[n-1] - lambda[n-2]) / (xpos[n-1] - xpos[n-2]);
          else
              disp = 0.0;

          if (disp > max_disp || disp < min_disp)
            break;


          /*
           * With this, find the expected position of the missing
           * peak by linear interpolation.
           */

          hi_start = xpos[n-1] + (line[ilambda[n-1] + 1] - lambda[n-1]) / disp;

          /*
           * Is there a peak at that position? Here a peak from the
           * original list is searched, that is closer than 6 pixels
           * to the expected position. If it is found, insert it at
           * the end of the list of identified peaks. If it is not
           * found, leave the loop.
           */

          found = 0;
          min = fabs(peak[0] - hi_start);
          minpos = 0;
          for (k = 1; k < npeaks; k++) {
            if (min > fabs(peak[k] - hi_start)) {
                min = fabs(peak[k] - hi_start);
                minpos = k;
            }
          }
          if (min < 6.0 && fabs(peak[minpos] - xpos[n-1]) > 1.0) {
            xpos[n] = peak[minpos];
            lambda[n] = line[ilambda[n-1] + 1];
            ilambda[n] = ilambda[n-1] + 1;
            ++n;
            found = 1;
          }
        }
    }

    /*
     * Try to extrapolate backward
     */

    found = 1;
    while (ilambda[0] > 0 && found) {

      /*
       * Determine the local dispersion from the first pair of
       * identified peaks
       */

      disp = (lambda[1] - lambda[0]) / (xpos[1] - xpos[0]);

      if (disp > max_disp || disp < min_disp)
        break;


      /*
       * With this, find the expected position of the missing
       * peak by linear interpolation.
       */

      hi_start = xpos[0] - (lambda[0] - line[ilambda[0] - 1]) / disp;


      /*
       * Is there a peak at that position? Here a peak from the
       * original list is searched, that is closer than 6 pixels
       * to the expected position. If it is found, insert it at
       * the beginning of the list of identified peaks. If it is not
       * found, leave the loop.
       */

      found = 0;
      min = fabs(peak[0] - hi_start);
      minpos = 0;
      for (k = 1; k < npeaks; k++) {
        if (min > fabs(peak[k] - hi_start)) {
            min = fabs(peak[k] - hi_start);
            minpos = k;
        }
      }
      if (min < 6.0 && fabs(peak[minpos] - xpos[0]) > 1.0) {
        for (j = n; j > 0; j--) {
          xpos[j] = xpos[j-1];
          lambda[j] = lambda[j-1];
          ilambda[j] = ilambda[j-1];
        }
        xpos[0] = peak[minpos];
        lambda[0] = line[ilambda[0] - 1];
        ilambda[0] = ilambda[0] - 1;
        ++n;
        found = 1;
      }
    }
  }


  /*
   * At this point all peaks are processed. Free memory, and return
   * the result.
   */

/************************************************+
  for (i = 0; i < npeaks; i++) {
    printf("Peak %d:\n   ", i);
    for (j = 0; j < nident[i]; j++)
      printf("%.2f, ", line[ident[i][j]]);
    printf("\n");
  }

  printf("\n");

  for (i = 0; i < n; i++)
    printf("%.2f, %.2f\n", xpos[i], lambda[i]);
+************************************************/
  for (i = 0; i < npeaks; i++)
    cpl_free(ident[i]);
  cpl_free(ident);
  cpl_free(nident);
  cpl_free(lident);
  cpl_free(ilambda);
  cpl_free(tmp_xpos);
  cpl_free(tmp_lambda);
  cpl_free(tmp_ilambda);
  cpl_free(peak_lo);
  cpl_free(flag);
  cpl_free(seq_length);
  cpl_free(peak_hi);

  if (n == 0) {
    cpl_free(xpos);
    cpl_free(lambda);
    return NULL;
  }

  return cpl_bivector_wrap_vectors(cpl_vector_wrap(n, xpos), 
                                   cpl_vector_wrap(n, lambda));
}

/* End of function for 1D point pattern matching */

/***************************************************************************
 * Definitions for 2D point-pattern-matching
 ***************************************************************************/

/* This one subtracts y from x, with error propagation. */

static double 
double_subtract(double x, double dx, double y, double dy, double *error)
{
    *error = hypot(dx, dy);
    return x - y;
}

/* This one divides x by y, with error propagation. */

static double 
double_divide(double x, double dx, double y, double dy, double *error)
{   
    const double y2 = y*y;


    if (y2 > 0.0) {
        *error = sqrt(( dx*dx + dy*dy * x*x / (y2) ) / (y2));
        return x/y;
    }

    *error = 0.0;
    return 0.0;
}

/* This one finds angle of the direction defined by point (x,y), with 
   error propagation. */

static double 
double_atan2(double y, double dy, double x, double dx, double *error)
{
    const double x2 = x*x;
    const double y2 = y*y;


    if (x2 > 0.0 || y2 > 0.0) {

        /* 
         * Using error propagation formula and d(atan(u))/du = 1/(1+u^2) 
         */

        *error = sqrt((dy*dy*x2 + dx*dx*y2) / ((x2 + y2)*(x2 + y2)));
        return atan2(y, x);

    }

    *error = 0.0;
    return 0.0;
}

/* This one returns the square of the distance between two points */

static double 
point_distsq(double xa, double ya, double xb, double yb)
{
    return ((xa - xb)*(xa - xb) + (ya - yb)*(ya - yb));
}

/* This constructs a triangle from three points, propagates errors,
   and keeps the points identifiers. */

static triangle *
triangle_new(double xa, double ya, double xb, double yb, double xc, double yc,
             double error, int ida, int idb, int idc)
{
    const double r1 = point_distsq(xa, ya, xb, yb);
    const double r2 = point_distsq(xa, ya, xc, yc);
    const double dr1 = sqrt(8*error*error*r1);
    const double dr2 = sqrt(8*error*error*r2);

    double  dt1, dt2;

    const double t1 = double_atan2(ya - yb, CPL_MATH_SQRT2 * error,
                                   xa - xb, CPL_MATH_SQRT2 * error, &dt1);
    const double t2 = double_atan2(ya - yc, CPL_MATH_SQRT2 * error,
                                   xa - xc, CPL_MATH_SQRT2 * error, &dt2);

    triangle *t = cpl_calloc(1, sizeof(triangle));

    t->xref = xa;
    t->yref = ya;
    t->id_ref = ida;

    if (r1 < r2) {
        t->ratsq  = double_divide(r1, dr1, r2, dr2, &t->dratsq);
        t->theta  = double_subtract(t1, dt1, t2, dt2, &t->dtheta);
        t->xmin   = xb;
        t->ymin   = yb;
        t->id_min = idb;
        t->xmax   = xc;
        t->ymax   = yc;
        t->id_max = idc;
    } else {
        t->ratsq  = double_divide(r2, dr2, r1, dr1, &t->dratsq);
        t->theta  = double_subtract(t2, dt2, t1, dt1, &t->dtheta);
        t->xmin   = xc;
        t->ymin   = yc;
        t->id_min = idc;
        t->xmax   = xb;
        t->ymax   = yb;
        t->id_max = idb;
    }

    while (t->theta < 0.0) 
        t->theta += CPL_MATH_2PI;

    while (t->theta >= CPL_MATH_2PI) 
        t->theta -= CPL_MATH_2PI;

    return t;
}

/* Triangle destructor */

static void 
triangle_delete(triangle **t)
{
    cpl_free(*t);
    *t = NULL;
}

/* A constructor of arrays of triangles... */

static triangles *triangles_new(int n)
{
    triangles *t = cpl_calloc(1, sizeof(triangles));

    t->t = cpl_calloc((size_t)n, sizeof(triangle *));
    t->n = n;
 
    return t;
}

/* ... and this one destroys sets of triangles. */

static void triangles_delete(triangles **t)
{
    while ((*t)->n--)
        triangle_delete((*t)->t + (*t)->n);
    cpl_free((*t)->t);
    cpl_free(*t);
    *t = NULL;
}

static void triangles_delete_holder(triangles **t)
{
    cpl_free((*t)->t);
    cpl_free(*t);
    *t = NULL;
}

/* Given one matrix 2xN of points, generates all the possible
   triangles from the points listed in its first n columns, 
   propagating to the triangles the error on point positions. */

static triangles *
triangles_from_points(const cpl_matrix *points, int n, double err)
{
    triangles     *pattern;
    const double  *m = cpl_matrix_get_data_const(points);
    const cpl_size nc = cpl_matrix_get_ncol(points);
    const int      nt = n*(n-1)*(n-2)/2;
    int            count;
    int            i, j, k;


    cpl_msg_debug(cpl_func, "Evaluate %d triangles...", nt);

    pattern = triangles_new(nt);

    count = 0;
    for (i = 0; i < n-2; i++) {
        for (j = i+1; j < n-1; j++) {
            for (k = j+1; k < n; k++) {
                double         x1, x2, x3, y1, y2, y3;

                /*
                 * i, j, and k, are the matrix columns where the
                 * three points are. Three triangles are created
                 * using each one of these points as reference.
                 */

                x1 = m[i];
                x2 = m[j];
                x3 = m[k];
                y1 = m[i + nc];
                y2 = m[j + nc];
                y3 = m[k + nc];

                pattern->t[count] = triangle_new(x1, y1, x2, y2, x3, y3, err,
                                                 i, j, k);
                count++;
                pattern->t[count] = triangle_new(x2, y2, x1, y1, x3, y3, err,
                                                 j, i, k);
                count++;
                pattern->t[count] = triangle_new(x3, y3, x2, y2, x1, y1, err,
                                                 k, j, i);
                count++;
            }
        }
    }

    return pattern;
}

/*
 * Difference between angles (in radians) in [0;pi]
 */

static double 
difference_of_angles(double a1, double a2)
{

    double d;

    /* NB: Handle a1 = 2pi - epsilon, a2 = epsilon => d = 2 epsilon */
    while (a1 < -CPL_MATH_PI) a1 += CPL_MATH_2PI;
    while (a1 >  CPL_MATH_PI) a1 -= CPL_MATH_2PI;
    while (a2 < -CPL_MATH_PI) a2 += CPL_MATH_2PI;
    while (a2 >  CPL_MATH_PI) a2 -= CPL_MATH_2PI;

    d = a1 - a2;

   /* NB: Handle a1 = pi - epsilon, a2 = epsilon - pi => d = 2 epsilon */
    while (d < -CPL_MATH_PI) d += CPL_MATH_2PI;
    while (d >  CPL_MATH_PI) d -= CPL_MATH_2PI;

    return fabs(d);
}

/*
 * The distance is calculated in normalized parameter space [0;1]x[0;1]
 * as well as its error (if error is not a null pointer). This is to 
 * give equal weight to differences in radii and differences in theta.
 */

static double
distance_of_triangles(const triangle *t1, const triangle *t2, double *error)
{
    const double dtheta =
        difference_of_angles(t1->theta, t2->theta); /* in [0;pi] */
    const double dr = t1->ratsq - t2->ratsq;
    const double rr = t1->dratsq*t1->dratsq + t2->dratsq*t2->dratsq;
    const double tt = (t1->dtheta*t1->dtheta + t2->dtheta*t2->dtheta) / 4;

    const double dist = hypot(dr, dtheta / CPL_MATH_PI);

    if (error) {
        if (dist != 0.0) {
            const double dist_in_errs =
                sqrt((dr * dr)/rr + (dtheta * dtheta)/tt);
            *error = dist / dist_in_errs;
        } else {
            *error = 1.0;
        }
    }

    return dist;
}

static triangle *
nearest_triangle(triangle *t, triangles *list)
{
    triangle **pattern = list->t;
    const int  n       = list->n;
    triangle  *nearest;

    double     dist, mindist;
    int        i;

    
    mindist = distance_of_triangles(t, pattern[0], NULL);
    nearest = pattern[0];
    for (i = 1; i < n; i++) {
        dist = distance_of_triangles(t, pattern[i], NULL);
        if (mindist > dist) {
            mindist = dist;
            nearest = pattern[i];
        }
    }

    return nearest;
}


/*
 * Get scale ratio of matching triangles (t1 / t2)
 */

static double 
scaling_factor_of_triangles(const triangle *t1, const triangle *t2)
{
    const double s1 = point_distsq(t1->xref, t1->yref, t1->xmax, t1->ymax);
    const double s2 = point_distsq(t2->xref, t2->yref, t2->xmax, t2->ymax);

    return (s2 != 0.0) ? sqrt(s1/s2) : 0.0;
}


/*
 * Get angle of orientation between matching patterns
 */

static double 
angle_between_triangles(const triangle *t1, const triangle *t2)
{
    const double a1 = atan2(t1->yref - t1->ymax, t1->xref - t1->xmax);
    const double a2 = atan2(t2->yref - t2->ymax, t2->xref - t2->xmax);
    double a = a1 - a2;

    while (a >= CPL_MATH_2PI) 
        a -= CPL_MATH_2PI;
    while (a < 0) 
        a += CPL_MATH_2PI;

    return a;
}


static cpl_array *
find_matches(const triangles *p_triangle, const triangles *n_triangle, int np)
{
    int        nt = p_triangle->n;
    int        i;
    int       *data;
    cpl_array *matches = cpl_array_new(np, CPL_TYPE_INT);


    cpl_array_fill_window_int(matches, 0, np, -1);
    data = cpl_array_get_data_int(matches);

    for (i = 0; i < nt; i++) {
        if (n_triangle->t[i]) {
            data[p_triangle->t[i]->id_ref] = n_triangle->t[i]->id_ref;
            data[p_triangle->t[i]->id_min] = n_triangle->t[i]->id_min;
            data[p_triangle->t[i]->id_max] = n_triangle->t[i]->id_max;
        }
    }

    for (i = 0; i < np; i++)
        if (data[i] < 0)
            cpl_array_set_invalid(matches, i);

    return matches;
    
}

/* Returns 0 on success */

static int
find_transform(const cpl_array *matches, const cpl_matrix *pattern, 
               const cpl_matrix *data, cpl_size degree, 
               cpl_polynomial *trans_x, cpl_polynomial *trans_y) 
{

    cpl_errorstate  prev_state = cpl_errorstate_get();
    cpl_error_code  error;
    cpl_vector     *xdpos;    /* x positions on data      */
    cpl_vector     *ydpos;    /* y positions on data      */
    cpl_matrix     *ppos;     /* x,y positions on pattern */
    double         *xd;
    double         *yd;
    double         *xp;
    double         *yp;
    const cpl_size  np     = cpl_array_get_size(matches);
    const cpl_size  nvalid = np - cpl_array_count_invalid(matches);
    int             null;
    int             count  = 0;
    int             i, j;


    xdpos = cpl_vector_new(nvalid);
    ydpos = cpl_vector_new(nvalid);
    ppos  = cpl_matrix_new(2, nvalid);

    xd = cpl_vector_get_data(xdpos);
    yd = cpl_vector_get_data(ydpos);
    xp = cpl_matrix_get_data(ppos);
    yp = cpl_matrix_get_data(ppos) + nvalid;

    count = 0;
    for (i = 0; i < np; i++) {
        j = cpl_array_get_int(matches, i, &null);
        if (null)
            continue;
        xd[count] = cpl_matrix_get(data, 0, j);
        yd[count] = cpl_matrix_get(data, 1, j);
        xp[count] = cpl_matrix_get(pattern, 0, i);
        yp[count] = cpl_matrix_get(pattern, 1, i);
        count++;
    }

    error = cpl_polynomial_fit(trans_x, ppos, NULL, xdpos, NULL, 
                               CPL_FALSE, NULL, &degree);

    if (error == CPL_ERROR_SINGULAR_MATRIX) {

        /*
         * Try a 1-D fit
         */

        const cpl_size degrees[] = {degree, 0};

        cpl_errorstate_set(prev_state);

        error = cpl_polynomial_fit(trans_x, ppos, NULL, xdpos, NULL,
                                   CPL_TRUE, NULL, degrees);

        if (error == CPL_ERROR_SINGULAR_MATRIX) {
            cpl_errorstate_set(prev_state);
            error = CPL_ERROR_NONE;
        }
    }

    if (!error) {
        error = cpl_polynomial_fit(trans_y, ppos, NULL, ydpos, NULL, 
                                   CPL_FALSE, NULL, &degree);

        if (error == CPL_ERROR_SINGULAR_MATRIX) {
    
            /*
             * Try a 1-D fit
             */

            const cpl_size degrees[] = {0, degree};

            cpl_errorstate_set(prev_state);
    
            error = cpl_polynomial_fit(trans_y, ppos, NULL, ydpos, NULL,
                                       CPL_TRUE, NULL, degrees);

            if (error == CPL_ERROR_SINGULAR_MATRIX) {
                cpl_errorstate_set(prev_state);
                error = CPL_ERROR_NONE;
            }
        }
    }

    cpl_matrix_delete(ppos);
    cpl_vector_delete(xdpos);
    cpl_vector_delete(ydpos);

    if (error) {
        cpl_error_set(cpl_func, error);
        return 1;
    }

    return 0;

}


static int
nearest_point(double x, double y, const cpl_matrix *matrix)
{
    const cpl_size nc     = cpl_matrix_get_ncol(matrix);
    const double  *data   = cpl_matrix_get_data_const(matrix);
    int            minpos, i;
    double         min, value;


    minpos = 0;
    min    = point_distsq(x, y, data[0], data[nc]);
    for (i = 1; i < nc; i++) {
        value = point_distsq(x, y, data[i], data[i+nc]);
        if (min > value) {
            min = value;
            minpos = i;
        }
    }

    return minpos;

}

static cpl_array *
find_all_matches(const cpl_matrix *pattern, const cpl_matrix *data, 
                 double radius, const cpl_polynomial *trans_x, 
                 const cpl_polynomial *trans_y,
                 cpl_matrix **mpattern, cpl_matrix **mdata)
{
    cpl_vector    *point   = cpl_vector_new(2);
    double        *p       = cpl_vector_get_data(point);
    const double  *ddata   = cpl_matrix_get_data_const(data);
    const cpl_size dnc     = cpl_matrix_get_ncol(data);
    const double  *dpatt   = cpl_matrix_get_data_const(pattern);
    const cpl_size pnc     = cpl_matrix_get_ncol(pattern);
    cpl_array     *matches = cpl_array_new(pnc, CPL_TYPE_INT);
    double         exp_x, exp_y;
    double        *md;
    double        *mp;
    cpl_size       nvalid, count;
    int            null;
    int            i, j;


    radius *= radius;

    for (i = 0; i < pnc; i++) {
        p[0] = dpatt[i];
        p[1] = dpatt[i + pnc];
        exp_x = cpl_polynomial_eval(trans_x, point);
        exp_y = cpl_polynomial_eval(trans_y, point);
        j = nearest_point(exp_x, exp_y, data);
        if (radius > point_distsq(exp_x, exp_y, ddata[j], ddata[j + dnc])) {
            cpl_array_set_int(matches, i, j);
        }
    }

    cpl_vector_delete(point);

    nvalid = pnc - cpl_array_count_invalid(matches);

    if (nvalid) {
        if (mpattern && mdata) {
            *mdata = cpl_matrix_new(2, nvalid);
            md = cpl_matrix_get_data(*mdata);
            *mpattern = cpl_matrix_new(2, nvalid);
            mp = cpl_matrix_get_data(*mpattern);
            count = 0;
            for (i = 0; i < pnc; i++) {
                j = cpl_array_get_int(matches, i, &null);
                if (null)
                    continue;
                md[count] = ddata[j];
                md[count + nvalid] = ddata[j + dnc];
                mp[count] = dpatt[i];
                mp[count + nvalid] = dpatt[i + pnc];
                count++;
            }
        }
    }
    else {
        cpl_array_delete(matches);
        return NULL;
    }

    return matches;
}


/**
 * @brief
 *   Match 2-D distributions of points.
 *
 * @param data        List of data points (e.g., detected stars positions).
 * @param use_data    Number of @em data points used for preliminary match.
 * @param err_data    Error on @em data points positions.
 * @param pattern     List of pattern points (e.g., expected stars positions).
 * @param use_pattern Number of @em pattern points used for preliminary match.
 * @param err_pattern Error on @em pattern points positions.
 * @param tolerance   Max relative difference of angles and scales from
 *                    their median value for match acceptance.
 * @param radius      Search radius applied in final matching (@em data units).
 * @param mdata       List of identified @em data points.
 * @param mpattern    List of matching @em pattern points.
 * @param lin_scale   Linear transformation scale factor.
 * @param lin_angle   Linear transformation rotation angle.
 *
 * @return Indexes of identified data points (pattern-to-data).
 *
 * A point is described here by its coordinates on a cartesian plane.
 * The input matrices @em data and @em pattern must have 2 rows, as 
 * their column vectors are the points coordinates.
 *
 * This function attemps to associate points in @em data to points in
 * @em pattern, under the assumption that a transformation limited to
 * scaling, rotation, and translation, would convert positions in
 * @em pattern into positions in @em data. Association between points 
 * is also indicated in the following as "match", or "identification". 
 *
 * Point identification is performed in two steps. In the first step 
 * only a subset of the points is identified (preliminary match). In 
 * the second step the identified points are used to define a first-guess 
 * transformation from @em pattern points to @em data points, that is 
 * applied to identify all the remaining points as well. The second 
 * step would be avoided if a @em use_pattern equal to the number of 
 * points in @em pattern is given, and exactly @em use_pattern points 
 * would be identified already in the first step.
 *
 * First step:
 *
 * All possible triangles (sub-patterns) are built using the first 
 * @em use_data points from @em data and the first @em use_pattern 
 * points from @em pattern. The values of @em use_data and @em use_pattern
 * must always be at least 3 (however, see the note at the end),
 * and should not be greater than the length of the corresponding
 * lists of points. The point-matching algorithm goes as follow:
 *
 * @code
 * For every triplet of points:
 *    Select one point as the reference. The triangle coordinates 
 *    are defined by
 *
 *                ((Rmin/Rmax)^2, theta_min - theta_max)
 *
 *    where Rmin (Rmax) is the shortest (longest) distance from the 
 *    reference point to one of the two other points, and theta_min
 *    (theta_max) is the view angle in [0; 2pi[ to the nearest 
 *    (farthest) point.
 *
 *    Triangles are computed by using each point in the triplet 
 *    as reference, thereby computing 3 times as many triangles 
 *    as needed.
 *
 *    The accuracy of triangle patterns is robust against distortions
 *    (i.e., systematic inaccuracies in the points positions) of the 
 *    second order. This is because, if the points positions had 
 *    constant statistical uncertainty, the relative uncertainty in 
 *    the triangle coordinates would be inversely proportional to 
 *    the triangle size, while if second order distortions are 
 *    present the systematic error on points position would be 
 *    directly proportional to the triangle size.
 *
 * For every triangle derived from the @em pattern points:
 *    Match with nearest triangle derived from @em data points 
 *    if their distance in the parameter space is less than their
 *    uncertainties (propagated from the points positions uncertainties
 *    @em err_data and @em err_pattern). For every matched pair of 
 *    triangles, record their scale ratio, and their orientation 
 *    difference. Note that if both @em err_data and @em err_pattern
 *    are zero, the tolerance in triangle comparison will also be
 *    zero, and therefore no match will be found.
 *
 * Get median scale ratio and median angle of rotation, and reject 
 * matches with a relative variation greater than @em tolerance from 
 * the median of either quantities. The estimator of all the rotation 
 * angles a_i is computed as 
 *
 *             atan( med sin(a_i) / med cos(a_i) )
 *
 * @endcode
 *
 * Second step: 
 * 
 * From the safely matched triangles, a list of identified points is 
 * derived, and the best transformation from @em pattern points to 
 * @em data points (in terms of best rotation angle, best scaling 
 * factor, and best shift) is applied to attempt the identification of 
 * all the points that are still without match. This matching is made 
 * by selecting for each @em pattern point the @em data point which is 
 * closest to its transformed position, and at a distance less than 
 * @em radius.
 * 
 * The returned array of integers is as long as the number of points in
 * @em pattern, and each element reports the position of the matching
 * point in @em data (counted starting from zero), or is invalid if no
 * match was found for the @em pattern point. For instance, if element
 * N of the array has value M, it means that the Nth point in @em pattern 
 * matches the Mth point in @em data. A NULL pointer is returned in case 
 * no point was identified.
 *
 * If @em mdata and @em mpattern are both valid pointers, two more
 * matrices will be returned with the coordinates of the identified
 * points. These two matrices will both have the same size: 2 rows, 
 * and as many columns as successfully identified points. Matching 
 * points will be in the same column of both matrices. Those matrix
 * should in the end be destroyed using cpl_matrix_delete().
 *
 * If @em lin_scale is a valid pointer, it is returned with a good estimate 
 * of the scale (distance_in_data = lin_scale * distance_in_pattern).
 * This makes sense only in case the transformation between @em pattern
 * and @em data is an affine transformation. In case of failure, 
 * @em lin_scale is set to zero.
 * 
 * If @em lin_angle is a valid pointer, it is returned with a good 
 * estimate of the rotation angle between @em pattern and @em data
 * in degrees (counted counterclockwise, from -180 to +180, and with
 * data_orientation = pattern_orientation + lin_angle). This makes 
 * sense only in case the transformation between @em pattern and 
 * @em data is an affine transformation. In case of failure, 
 * @em lin_angle is set to zero.
 *
 * The returned values for @em lin_scale and @em lin_angle have the only
 * purpose of providing a hint on the relation between @em pattern points 
 * and @em data points. This function doesn't attempt in any way to
 * determine or even suggest a possible transformation between @em pattern
 * points and @em data points: this function just matches points, and it
 * is entriely a responsibility of the caller to fit the appropriate
 * transformation between one coordinate system and the other.
 * A polynomial transformation of degree 2 from @em pattern to @em data
 * may be fit in the following way (assuming that @em mpattern and 
 * @em mdata are available):
 *
 * @code
 *
 * int             degree  = 2;
 * int             npoints = cpl_matrix_get_ncol(mdata);
 * double         *dpoints = cpl_matrix_get_data(mdata);
 * cpl_vector     *data_x  = cpl_vector_wrap(npoints, dpoints);
 * cpl_vector     *data_y  = cpl_vector_wrap(npoints, dpoints + npoints);
 * cpl_polynomial *x_trans = cpl_polynomial_new(degree);
 * cpl_polynomial *y_trans = cpl_polynomial_new(degree);
 *
 * cpl_polynomial_fit(x_trans, mpattern, NULL, data_x, NULL, CPL_FALSE,
 *                    NULL, degree);
 * cpl_polynomial_fit(y_trans, mpattern, NULL, data_y, NULL, CPL_FALSE,
 *                    NULL, degree);
 *
 * @endcode
 *
 * @note
 * The basic requirement for using this function is that the searched
 * point pattern (or at least most of it) is contained in the data. 
 * As an indirect consequence of this, it would generally be appropriate 
 * to have more points in @em data than in @em pattern (and analogously, 
 * to have @em use_data greater than @em use_pattern), even if this is 
 * not strictly necessary.
 *
 * Also, @em pattern and @em data should not contain too few points
 * (say, less than 5 or 4) or the identification may risk to be incorrect:
 * more points enable the construction of many more triangles, reducing
 * the risk of ambiguity (multiple valid solutions). Special situations, 
 * involving regularities in patterns (as, for instance, input @em data 
 * containing just three equidistant points, or the case of a regular 
 * grid of points) would certainly provide an answer, and this answer 
 * would very likely be wrong (the human brain would fail as well, 
 * and for exactly the same reasons).
 *
 * The reason why a two steps approach is encouraged here is mainly to 
 * enable an efficient use of this function: in principle, constructing 
 * all possible triangles using @em all of the available points is never 
 * wrong, but it could become very slow: a list of N points implies the 
 * evaluation of N*(N-1)*(N-2)/2 triangles, and an even greater number
 * of comparisons between triangles. The possibility of evaluating 
 * first a rough transformation based on a limited number of identified
 * points, and then using this transformation for recovering all the 
 * remaining points, may significantly speed up the whole identification
 * process. However it should again be ensured that the main requirement
 * (i.e., that the searched point pattern must be contained in the data)
 * would still be valid for the selected subsets of points: a random
 * choice would likely lead to a matching failure (due to too few, or 
 * no, common points).
 *
 * A secondary reason for the two steps approach is to limit the effect 
 * of another class of ambiguities, happening when either or both of
 * the input matrices contains a very large number of uniformely
 * distributed points. The likelihood to find several triangles that 
 * are similar by chance, and at all scales and orientations, may 
 * increase to unacceptable levels.
 *
 * A real example may clarify a possible way of using this function:
 * let @em data contain the positions (in pixel) of detected stars 
 * on a CCD. Typically hundreds of star positions would be available, 
 * but only the brightest ones may be used for preliminary identification. 
 * The input @em data positions will therefore be opportunely ordered 
 * from the brightest to the dimmest star positions. In order to 
 * identify stars, a star catalogue is needed. From a rough knowledge 
 * of the pointing position of the telescope and of the size of the 
 * field of view, a subset of stars can be selected from the catalogue: 
 * they will be stored in the @em pattern list, ordered as well by their 
 * brightness, and with their RA and Dec coordinates converted into 
 * standard coordinates (a gnomonic coordinate system centered on the 
 * telescope pointing, i.e., a cartesian coordinate system), no matter 
 * in what units of arc, and no matter what orientation of the field.
 * For the first matching step, the 10 brightest catalogue stars may 
 * be selected (selecting less stars would perhaps be unsafe, selecting 
 * more would likely make the program slower without producing any 
 * better result). Therefore @em use_pattern would be set to 10. 
 * From the data side, it would generally be appropriate to select 
 * twice as many stars positions, just to ensure that the searched 
 * pattern is present. Therefore @em use_data would be set to 20.
 * A reasonable value for @em tolerance and for @em radius would be
 * respectively 0.1 (a 10% variation of scales and angles) and 20 
 * (pixels).
 */

cpl_array *
cpl_ppm_match_points(const cpl_matrix *data, cpl_size use_data, double err_data,
                     const cpl_matrix *pattern, cpl_size use_pattern, double
                     err_pattern, double tolerance, double radius, 
                     cpl_matrix **mdata, cpl_matrix **mpattern,
                     double *lin_scale, double *lin_angle)
{

    cpl_size        nd, np;       /* Number of points in data and pattern   */
    triangles      *d_triangle;   /* Triangles from data points             */
    triangles      *p_triangle;   /* Triangles from pattern points          */
    triangles      *n_triangle;   /* Nearest d_triangles to each p_triangle */
    cpl_array      *matches = NULL;
    cpl_table      *table = NULL;
    cpl_polynomial *trans_x;
    cpl_polynomial *trans_y;
    double          err;
    double         *distance;
    double         *scale, median_scale;
    double         *angle, median_angle;
    double         *angle_s;
    double         *angle_c;
    double          scale_tolerance, angle_tolerance;
    int             i;


    if (mpattern)
        *mpattern = NULL;

    if (mdata)
        *mdata = NULL;

    if (lin_scale)
        *lin_scale = 0.0;

    if (lin_angle)
        *lin_angle = 0.0;

    if (data == NULL || pattern == NULL) {
        cpl_error_set(cpl_func, CPL_ERROR_NULL_INPUT);
        return NULL;
    }

    nd = cpl_matrix_get_ncol(data);
    np = cpl_matrix_get_ncol(pattern);

    if (nd < 3 || np < 3 || use_data < 3 || use_pattern < 3) {
        cpl_error_set(cpl_func, CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    if (use_data > nd || use_pattern > np) {
        cpl_error_set(cpl_func, CPL_ERROR_ACCESS_OUT_OF_RANGE);
        return NULL;
    }

    // FIXME: Remove these checks! Currently only the interface is changed
    //        to cpl_size. Once the module has been ported completely the
    //        following tests must be removed.

    if (((int)use_data != use_data) || ((int)use_pattern != use_pattern)) {
        cpl_error_set(cpl_func, CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

/*
    if (use_data < use_pattern) {
        cpl_error_set_message(cpl_func, CPL_ERROR_ILLEGAL_INPUT, 
                "A pattern larger than data is not yet supported");
        return NULL;
    }
*/

    if (err_data <= 0.0 && err_pattern <= 0.0) {
        cpl_error_set(cpl_func, CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    if (tolerance < 0.0 || radius < 0.0) {
        cpl_error_set(cpl_func, CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    if ((mdata == NULL && mpattern) || (mpattern == NULL && mdata)) {
        cpl_error_set(cpl_func, CPL_ERROR_ILLEGAL_INPUT);
        return NULL;
    }

    /*
     * Looking for matches between data-triangles and pattern-triangles...
     *
     * The n_triangle list of triangles is as long as the p_triangle
     * list of triangles from pattern points, and just points to
     * triangles in d_triangle made from data points, whenever an
     * association is found.
     */

    // FIXME: Remove casts to int when the module is ported to cpl_size
    cpl_msg_debug(cpl_func, "Pattern:");
    cpl_msg_indent_more();
    p_triangle = triangles_from_points(pattern, (int)use_pattern, err_pattern);
    cpl_msg_indent_less();
    cpl_msg_debug(cpl_func, "Data:");
    cpl_msg_indent_more();
    d_triangle = triangles_from_points(data, (int)use_data, err_data);
    cpl_msg_indent_less();
    n_triangle = triangles_new(p_triangle->n);

    /*
     * The table keeps track of relations between matching triangles,
     * such as their distance in the parameter space, their scale, 
     * their relative orientation...
     */

    table = cpl_table_new(p_triangle->n);

    cpl_table_new_column(table, "distance", CPL_TYPE_DOUBLE);
    cpl_table_fill_column_window_double(table, "distance", 
                                        0, p_triangle->n, 0.0);
    distance = cpl_table_get_data_double(table, "distance");

    cpl_table_new_column(table, "scale", CPL_TYPE_DOUBLE);
    cpl_table_fill_column_window_double(table, "scale", 
                                        0, p_triangle->n, 0.0);
    scale = cpl_table_get_data_double(table, "scale");

    cpl_table_new_column(table, "angle", CPL_TYPE_DOUBLE);
    cpl_table_fill_column_window_double(table, "angle", 
                                        0, p_triangle->n, 0.0);
    angle = cpl_table_get_data_double(table, "angle");

    cpl_table_new_column(table, "angle_s", CPL_TYPE_DOUBLE);
    cpl_table_fill_column_window_double(table, "angle_s", 
                                        0, p_triangle->n, 0.0);
    angle_s = cpl_table_get_data_double(table, "angle_s");

    cpl_table_new_column(table, "angle_c", CPL_TYPE_DOUBLE);
    cpl_table_fill_column_window_double(table, "angle_c", 
                                        0, p_triangle->n, 0.0);
    angle_c = cpl_table_get_data_double(table, "angle_c");

    /*
     * For each pattern triangle find the most similar data triangle.
     * The triangles are associated if their distance in the parameter
     * space is less than their uncertainties.
     */

    for (i = 0; i < p_triangle->n; i++) {
        triangle  *nearest =   /* Nearest triangle */
            nearest_triangle(p_triangle->t[i], d_triangle);
        distance[i] = distance_of_triangles(p_triangle->t[i], nearest, &err);
        if (distance[i] / err < 1.0) {
            scale[i] = scaling_factor_of_triangles(nearest, p_triangle->t[i]);
            angle[i] = angle_between_triangles(nearest, p_triangle->t[i]);
            angle_s[i] = sin(angle[i]);
            angle_c[i] = cos(angle[i]);
            n_triangle->t[i] = nearest;
/*
            cpl_msg_debug(cpl_func, 
                          "distance, error, scale, angle = %f, %f, %f, %f",
                          distance[i], err, scale[i], angle[i]);
*/
        }
        else {
            cpl_table_set_invalid(table, "distance", i);
            cpl_table_set_invalid(table, "scale", i);
            cpl_table_set_invalid(table, "angle", i);
            cpl_table_set_invalid(table, "angle_s", i);
            cpl_table_set_invalid(table, "angle_c", i);
/*
            cpl_msg_debug(cpl_func, "EXCLUDED:\ndistance, error = %f, %f",
                          distance[i], err);
*/
        }
    }

    /*
     * If there are matches, determine the median relations between
     * matching triangles.
     */

    if (cpl_table_has_valid(table, "distance")) {
        const double median_s = cpl_table_get_column_median(table, "angle_s");
#ifdef CPL_PPM_DEBUG
        const double rms_s    = cpl_table_get_column_stdev(table, "angle_s");
        const double rms_c    = cpl_table_get_column_stdev(table, "angle_c");
        const double rms_angle= hypot(rms_s, rms_c);
        double rms_scale;
#endif
        const double median_c = cpl_table_get_column_median(table, "angle_c");

        median_scale = cpl_table_get_column_median(table, "scale");

        /* The circular (i.e. non-Euclidian) nature of the angle makes it
           unsuitable for the median (and mean), so instead take the median
           of the Cartesian coordinates */
        median_angle = atan2(median_s, median_c);

#ifdef CPL_PPM_DEBUG
        rms_scale = cpl_table_get_column_stdev(table, "scale");

        cpl_msg_debug(cpl_func, 
                      "Median scale (first iteration) = %.4f +/- %.4f", 
                      median_scale, rms_scale);
        cpl_msg_debug(cpl_func, 
                      "Median angle (first iteration) = %.4f +/- %.4f degrees", 
                      median_angle*CPL_MATH_DEG_RAD, rms_angle*CPL_MATH_DEG_RAD);
#endif
    }
    else {
        cpl_msg_debug(cpl_func, "No match found among %d = %d row(s)",
                      (int)cpl_table_get_nrow(table), (int)(p_triangle->n));
        cpl_table_delete(table);
        triangles_delete(&p_triangle);
        triangles_delete(&d_triangle);
        triangles_delete_holder(&n_triangle);
        return matches;
    }


    /*
     * Now eliminate outliers: matches implying scales where 
     *
     *        | scale - median_scale | > tolerance * median_scale 
     *
     * are rejected. The same is done with angles (using the same 
     * tolerance).
     */

    scale_tolerance = tolerance * median_scale;
    angle_tolerance = tolerance * fabs(median_angle);

    for (i = 0; i < p_triangle->n; i++) {
        if (n_triangle->t[i]) {
            if (fabs(scale[i] - median_scale) > scale_tolerance
            || difference_of_angles(angle[i], median_angle) > angle_tolerance) {
#ifdef CPL_PPM_DEBUG
                cpl_msg_debug(cpl_func, "Discarding %d/%d: |%g - %g| > %g * %g or "
                              "diff_angle(%g, %g) > %g * |%g|", i+1, p_triangle->n,
                              scale[i], median_scale, tolerance, median_scale,
                              angle[i], median_angle, tolerance, median_angle);
#endif
                n_triangle->t[i] = NULL;
                cpl_table_set_invalid(table, "distance", i);
                cpl_table_set_invalid(table, "scale", i);
                cpl_table_set_invalid(table, "angle", i);
                cpl_table_set_invalid(table, "angle_s", i);
                cpl_table_set_invalid(table, "angle_c", i);
            }
        }
    }

    /*
     * There should by definition be survivors: however, for safety
     * the "if" check is done anyway...
     */

    if (cpl_table_has_valid(table, "distance")) {
        const double median_s = cpl_table_get_column_median(table, "angle_s");
#ifdef CPL_PPM_DEBUG
        const double rms_s    = cpl_table_get_column_stdev(table, "angle_s");
        const double rms_c    = cpl_table_get_column_stdev(table, "angle_c");
        const double rms_angle= hypot(rms_s, rms_c);
        double rms_scale;
#endif
        const double median_c = cpl_table_get_column_median(table, "angle_c");

        median_scale = cpl_table_get_column_median(table, "scale");
        median_angle = atan2(median_s, median_c);

#ifdef CPL_PPM_DEBUG
        rms_scale = cpl_table_get_column_stdev(table, "scale");

        cpl_msg_debug(cpl_func, 
                      "Median scale (second iteration) = %.4f +/- %.4f",
                      median_scale, rms_scale);
        cpl_msg_debug(cpl_func, 
                      "Median angle (second iteration) = %.4f +/- %.4f degrees", 
                      median_angle*CPL_MATH_DEG_RAD, rms_angle*CPL_MATH_DEG_RAD);
#endif
    }
    else {
/*
        cpl_msg_warning(cpl_func, "No match found (impossible!)");
        cpl_error_set_message(cpl_func, CPL_ERROR_UNSPECIFIED, 
                              "No match found (impossible!)");
*/
        cpl_msg_debug(cpl_func, "No match found among %d row(s)",
                      (int)cpl_table_get_nrow(table));
        cpl_table_delete(table);
        triangles_delete(&p_triangle);
        triangles_delete(&d_triangle);
        triangles_delete_holder(&n_triangle);
        return matches;
    }

    cpl_table_delete(table);

    /*
     * Reaching this point means to have a good estimate of the scale 
     * [data_size = median_scale * pattern_size] and of the angle
     * [data_orientation = median_angle + pattern_orientation]. 
     */
 
    if (lin_scale) {
        *lin_scale = median_scale;
    }
 
    if (lin_angle) {
        *lin_angle = median_angle * CPL_MATH_DEG_RAD;
    }

    /*
     * Build the list of safely matched points from the surviving
     * triangles, and find the best transformation from points in 
     * pattern to points in data. The safely matched points are
     * "listed" in an integer array, whose index i corresponds to
     * column i in the pattern matrix, and whose value j corresponds
     * to column j in the data matrix. In case of no match for 
     * column i, element i is left invalid.
     */

    // FIXME: Remove casts to int when the module is ported to cpl_size
    matches = find_matches(p_triangle, n_triangle, (int)use_pattern);

    triangles_delete(&p_triangle);
    triangles_delete(&d_triangle);
    triangles_delete_holder(&n_triangle);

    /*
     * Using the safely matched points, find the best linear
     * transformation from pattern points to data poins...
     */

    trans_x = cpl_polynomial_new(2);
    trans_y = cpl_polynomial_new(2);
    find_transform(matches, pattern, data, 1, trans_x, trans_y);

    cpl_array_delete(matches);

    /*
     * Now the transformation is used to find a match to all points
     * in pattern. Even the points who already have found a match, 
     * are put again into discussion. If mpattern and mdata are
     * given, the coordinate of the matching points in pattern
     * and in data are returned.
     */

    matches = find_all_matches(pattern, data, radius, trans_x, trans_y,
                               mpattern, mdata);

    /*
     * Fare thee well, transformation!
     */

    cpl_polynomial_delete(trans_x);
    cpl_polynomial_delete(trans_y);

    return matches;
   
}

/**@}*/
