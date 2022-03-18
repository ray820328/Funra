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

/*----------------------------------------------------------------------------
                                   Includes
 ----------------------------------------------------------------------------*/

#include "cpl_error_impl.h"
#include "cpl_wcs.h"
#include "cpl_init.h"
#include "cpl_test.h"
#include "cpl_memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

#ifdef CPL_WCS_INSTALLED
/* Needed for cpl_error_set_wcs() */
#include <wcslib.h>
#endif

#define TESTFILE "nonaxis.fits"

#define NSK 2
#define NDK 13
#define NIK 3

const char *skeys[NSK] = {"CTYPE1", "CTYPE2"};
const char *skeys_tab[NSK] = {"TCTYP3","TCTYP5"};

const char *dkeys[NDK] = {"CRVAL1", "CRVAL2", "CRPIX1", "CRPIX2",
                          "CD1_1", "CD1_2", "CD2_1", "CD2_2", "PV2_1",
                          "PV2_2", "PV2_3", "PV2_4", "PV2_5"};
const char *dkeys_tab[NDK] = {"TCRVL3","TCRVL5","TCRPX3","TCRPX5",
			      "TC3_3","TC3_5","TC5_3","TC5_5","TV5_1",
			      "TV5_2","TV5_3","TV5_4","TV5_5"};

const char *ikeys[NIK] = {"NAXIS","NAXIS1","NAXIS2"};


const char *svals[NSK] = {"RA---ZPN", "DEC--ZPN"};
const double dvals[NDK] = {5.57368333333, -72.0576388889, 5401.6, 6860.8,
                           5.81347849634012E-21, 9.49444444444444E-05,
                           -9.49444444444444E-05, -5.81347849634012E-21,
                           1.0, 0.0, 42.0, 0.0, 0.0};
const int ivals[NIK] = {2, 2048, 2048};

const char *svals2[NSK] = {"RA---ZPN", "DEC--ZPN"};
const double dvals2[NDK] = {136.018529319233, 34.5793698413348,
                            3000.83678222321, -936.601342583387,
                            5.33299465243094E-08, -0.000111500211205622,
                            0.000111494500595739, -1.41761930842942E-08,
                            1.0, 0.0, -45.0, 0.0, 0.0};
const int ivals2[NIK] = {2, 2064, 2064};

#define NP 2
const double physin[2*NP] = {1024.0, 1024.0, 1025.0, 1023.0};
const double worldout[2*NP] = {3.825029720, -71.636524754,
                               3.824722171, -71.636616487};
const double stdcout[2] = {-0.554171733, 0.415628800};


const double worldin[2] = {3.824875946, -71.636570620};
const double physout[2] = {1024.5, 1023.5};

#define NSTDS 94
const double xy[] = {
    382.252,  36.261,
    18.097,  38.428,
    1551.399,  51.774,
    800.372,  85.623,
    296.776,  97.524,
    1688.610, 137.485,
    1137.220, 207.411,
    881.385, 213.913,
    1301.158, 239.626,
    1925.201, 242.518,
    152.270, 252.434,
    493.289, 274.073,
    1027.982, 286.037,
    1123.301, 302.822,
    791.430, 309.277,
    1634.414, 329.992,
    45.455, 334.707,
    224.075, 343.282,
    622.339, 352.331,
    1967.698, 358.488,
    901.068, 409.338,
    953.233, 417.257,
    1289.861, 426.885,
    596.422, 430.980,
    129.039, 446.756,
    1534.073, 463.523,
    1476.210, 464.923,
    749.281, 475.854,
    441.555, 478.242,
    1333.284, 482.094,
    662.150, 536.999,
    1246.223, 585.173,
    475.595, 656.727,
    169.960, 679.461,
    1502.242, 689.464,
    93.412, 694.360,
    545.388, 696.031,
    247.199, 746.899,
    615.903, 757.244,
    1927.821, 864.380,
    1724.554, 889.479,
    1449.032, 895.684,
    1558.846, 906.941,
    777.762, 912.936,
    1327.582, 914.718,
    857.511, 920.258,
    850.675, 936.752,
    317.737, 966.788,
    1954.225, 967.560,
    1334.811,1055.225,
    311.918,1075.083,
    1132.445,1097.327,
    1600.109,1096.887,
    1254.242,1119.658,
    1285.886,1141.102,
    665.050,1149.314,
    259.059,1182.549,
    1511.898,1203.002,
    1041.352,1220.464,
    944.823,1302.967,
    990.619,1303.994,
    1303.476,1320.765,
    1059.565,1334.145,
    1035.701,1340.268,
    441.037,1340.244,
    1149.912,1355.465,
    225.527,1356.680,
    1558.375,1387.388,
    2034.853,1410.099,
    799.400,1429.687,
    1285.559,1491.646,
    218.601,1500.189,
    726.648,1516.822,
    1112.176,1520.933,
    147.153,1531.091,
    904.416,1534.735,
    1736.805,1557.387,
    1496.763,1603.196,
    1478.237,1618.552,
    770.317,1656.818,
    1625.301,1676.042,
    1207.898,1696.559,
    470.996,1727.196,
    1807.232,1732.755,
    1462.826,1735.153,
    1098.744,1753.648,
    67.590,1757.731,
    431.763,1811.486,
    92.574,1834.930,
    833.029,1939.049,
    244.391,1953.157,
    1040.013,1959.332,
    874.378,1980.592,
    1396.658,2014.189
};
const double radec[] = {
    135.88677979, 34.28698730,
    135.88664246, 34.24613953,
    135.88479614, 34.41748047,
    135.88023376, 34.33363724,
    135.87858582, 34.27736664,
    135.87304688, 34.43286514,
    135.86373901, 34.37126541,
    135.86285400, 34.34273529,
    135.85929871, 34.38956833,
    135.85894775, 34.45923233,
    135.85778809, 34.26112366,
    135.85470581, 34.29931259,
    135.85314941, 34.35920334,
    135.85084534, 34.36973190,
    135.84997559, 34.33263397,
    135.84710693, 34.42679977,
    135.84652710, 34.24911499,
    135.84542847, 34.26911163,
    135.84413147, 34.31375504,
    135.84321594, 34.46398163,
    135.83653259, 34.34495163,
    135.83547974, 34.35073853,
    135.83410645, 34.38834763,
    135.83352661, 34.31077576,
    135.83146667, 34.25836945,
    135.82904053, 34.41551590,
    135.82885742, 34.40904236,
    135.82754517, 34.32785416,
    135.82716370, 34.29343414,
    135.82658386, 34.39306641,
    135.81916809, 34.31809998,
    135.81260681, 34.38336945,
    135.80287170, 34.29724884,
    135.79995728, 34.26297760,
    135.79847717, 34.41189957,
    135.79797363, 34.25428772,
    135.79760742, 34.30493546,
    135.79077148, 34.27165985,
    135.78939819, 34.31282425,
    135.77473450, 34.45937729,
    135.77145386, 34.43675232,
    135.77061462, 34.40594864,
    135.76892090, 34.41823959,
    135.76821899, 34.33092880,
    135.76800537, 34.39233398,
    135.76731873, 34.33979416,
    135.76509094, 34.33901596,
    135.76103210, 34.27935028,
    135.76074219, 34.46220398,
    135.74896240, 34.39315033,
    135.74639893, 34.27863312,
    135.74327087, 34.37044907,
    135.74317932, 34.42278671,
    135.74020386, 34.38416672,
    135.73742676, 34.38760376,
    135.73634338, 34.31815338,
    135.73170471, 34.27264023,
    135.72891235, 34.41275406,
    135.72660828, 34.36019516,
    135.71536255, 34.34933853,
    135.71527100, 34.35440445,
    135.71296692, 34.38951492,
    135.71118164, 34.36217117,
    135.71046448, 34.35951996,
    135.71041870, 34.29297638,
    135.70826721, 34.37225342,
    135.70826721, 34.26868439,
    135.70388794, 34.41787720,
    135.70075989, 34.47105789,
    135.69830322, 34.33304977,
    135.68984985, 34.38727188,
    135.68881226, 34.26796341,
    135.68650818, 34.32481003,
    135.68591309, 34.36791611,
    135.68466187, 34.25994110,
    135.68405151, 34.34471130,
    135.68084717, 34.43779755,
    135.67460632, 34.41091919,
    135.67254639, 34.40884781,
    135.66752625, 34.32962036,
    135.66487122, 34.42524338,
    135.66201782, 34.37860870,
    135.65805054, 34.29612732,
    135.65704346, 34.44551849,
    135.65676880, 34.40697479,
    135.65438843, 34.36631012,
    135.65388489, 34.25082397,
    135.64648438, 34.29161072,
    135.64344788, 34.25370026,
    135.62927246, 34.33645630,
    135.62741089, 34.27054977,
    135.62643433, 34.35960388,
    135.62356567, 34.34106827,
    135.61895752, 34.39946365
};

static void cpl_wcs_error_test(int);
static void cpl_wcs_test_propertylist(const cpl_propertylist *);
static void cpl_wcs_test_one(void);
static void cpl_wcs_test_5115(void);

int main (void) {
    int i;
    double crval1, crval2, crpix1, crpix2, cd1, cd2, cd3, cd4;
    cpl_propertylist *pl,*opl;
    cpl_wcs *wcs;
    cpl_matrix      *from,*xymat,*rdmat;
    cpl_matrix      * to     = NULL;
    cpl_array       * status = NULL;
    const cpl_array * nullarray;
    const cpl_array * arrdims;
    const cpl_matrix * nullmatrix;
    const cpl_array *crval, *crpix;
    const cpl_matrix *cd;
    double d1,d2;
    cpl_error_code error;

    /* Initialise */

    cpl_test_init(PACKAGE_BUGREPORT,CPL_MSG_WARNING);

    cpl_wcs_test_5115();
    cpl_wcs_test_one();
    cpl_wcs_error_test(1);

    /* Create a propertylist for the WCS header items */

    pl = cpl_propertylist_new();

    /* Use a non-empty propertylist with no WCS */

    error = cpl_propertylist_append_double(pl, "EXPTIME", 0.0);
    cpl_test_eq_error(error, CPL_ERROR_NONE);

    wcs = cpl_wcs_new_from_propertylist(pl);

    /* Check that the WCS functions are provided */

    if (cpl_error_get_code() == CPL_ERROR_NO_WCS) {
        cpl_test_error(CPL_ERROR_NO_WCS);
        cpl_test_null(wcs);
        cpl_msg_warning(cpl_func, "WCSLIB not found - skip cpl_wcs tests") ;
        cpl_propertylist_delete(pl) ;
        return cpl_test_end(0);
    }

    cpl_test_error(CPL_ERROR_UNSPECIFIED);
    cpl_test_null(wcs);

    /* Some NULL input tests */
    wcs = cpl_wcs_new_from_propertylist(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(wcs);

    i = cpl_wcs_get_image_naxis(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_zero(i);

    nullarray = cpl_wcs_get_image_dims(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullarray);

    nullarray = cpl_wcs_get_crval(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullarray);

    nullarray = cpl_wcs_get_crpix(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullarray);

    nullmatrix = cpl_wcs_get_cd(NULL);
    cpl_test_error(CPL_ERROR_NULL_INPUT);
    cpl_test_null(nullmatrix);

    /* Create a matrix. physin is _not_ modified */
    from = cpl_matrix_wrap(NP, 2, (double*)physin);

    /* Test handling of NULL wcs */
    error = cpl_wcs_convert(NULL, from, &to, &status, CPL_WCS_PHYS2WORLD);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);
    cpl_test_null(to);
    cpl_test_null(status);

    /* Create the WCS header */

    for (i = 0; i < NSK; i++)
        cpl_propertylist_append_string(pl,skeys[i],svals[i]);
    for (i = 0; i < NDK; i++)
        cpl_propertylist_append_double(pl,dkeys[i],dvals[i]);
    for (i = 0; i < NIK; i++)
        cpl_propertylist_append_int(pl,ikeys[i],ivals[i]);

    /* Get a wcs structure */

    wcs = cpl_wcs_new_from_propertylist(pl);
    cpl_test_nonnull(wcs);
    cpl_propertylist_delete(pl);

    cpl_test_eq(cpl_wcs_get_image_naxis(wcs), 2);

    arrdims = cpl_wcs_get_image_dims(wcs);
    cpl_test_nonnull(arrdims);
    cpl_test_eq(cpl_array_get_type(arrdims), CPL_TYPE_INT);
    cpl_test_eq(cpl_array_get_size(arrdims), 2);
    for (i = 1; i < NIK; i++) {
        cpl_test_eq(cpl_array_get_int(arrdims, i-1, NULL), ivals[i]);
    }

    crval = cpl_wcs_get_crval(wcs);
    cpl_test_nonnull(crval);
    cpl_test_eq(cpl_array_get_type(crval), CPL_TYPE_DOUBLE);
    cpl_test_eq(cpl_array_get_size(crval), 2);

    crpix = cpl_wcs_get_crpix(wcs);
    cpl_test_nonnull(crpix);
    cpl_test_eq(cpl_array_get_type(crpix), CPL_TYPE_DOUBLE);
    cpl_test_eq(cpl_array_get_size(crpix), 2);

    cd = cpl_wcs_get_cd(wcs);
    cpl_test_nonnull(cd);
    cpl_test_eq(cpl_matrix_get_nrow(cd), 2);
    cpl_test_eq(cpl_matrix_get_ncol(cd), 2);

    /* Test cpl_wcs_convert to see if we get the correct error messages */

    /* If wcs is NULL */
    error = cpl_wcs_convert(NULL,from,&to,&status,CPL_WCS_PHYS2WORLD);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);
    cpl_test_null(to);
    cpl_test_null(status);

    /* If from is NULL */
    error = cpl_wcs_convert(wcs,NULL,&to,&status,CPL_WCS_PHYS2WORLD);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);
    cpl_test_null(to);
    cpl_test_null(status);

    /* If to is NULL */
    error = cpl_wcs_convert(wcs,from,NULL,&status,CPL_WCS_PHYS2WORLD);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);
    cpl_test_null(to);
    cpl_test_null(status);

    /* If status is NULL */
    error = cpl_wcs_convert(wcs,from,&to,NULL,CPL_WCS_PHYS2WORLD);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);
    cpl_test_null(to);
    cpl_test_null(status);

    /* If the transformation is invalid */
    error = cpl_wcs_convert(wcs, from, &to, &status,
                            CPL_WCS_PHYS2STD + CPL_WCS_WORLD2STD);
    cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);
    cpl_test_null(to);
    cpl_test_null(status);

    /* Ok, do a conversion of physical to world coordinates */

    cpl_msg_info("","Transform physical -> world (2 points)");
    error = cpl_wcs_convert(wcs,from,&to,&status,CPL_WCS_PHYS2WORLD);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_nonnull(to);
    cpl_test_nonnull(status);
    (void)cpl_matrix_unwrap(from);

    /* Test the output values. The status should all be 0. The output matrix
       is compared to predefined world coordinate values */

    for (i = 0; i < NP; i++)
        cpl_test_zero(cpl_array_get_data_int_const(status)[i]);

    cpl_test_abs(worldout[0], cpl_matrix_get(to, 0, 0), FLT_EPSILON);
    cpl_test_abs(worldout[1], cpl_matrix_get(to, 0, 1), FLT_EPSILON);
    cpl_test_abs(worldout[2], cpl_matrix_get(to, 1, 0), FLT_EPSILON);
    cpl_test_abs(worldout[3], cpl_matrix_get(to, 1, 1), FLT_EPSILON);

    d1 = fabs(worldout[0] - cpl_matrix_get(to,0,0));
    d2 = fabs(worldout[1] - cpl_matrix_get(to,0,1));
    cpl_msg_info("","phys1,phys2:   %15.9f %15.9f",physin[0],physin[1]);
    cpl_msg_info("","world1,world2: %15.9f %15.9f",worldout[0],worldout[1]);
    cpl_msg_info("","calc1,calc2:   %15.9f %15.9f",cpl_matrix_get(to,0,0),
            cpl_matrix_get(to,0,1));
    cpl_msg_info("","diff1,diff2:   %15.9f %15.9f",d1,d2);
    cpl_msg_info("","status:        %d",
                 cpl_array_get_data_int_const(status)[0]);

    d1 = fabs(worldout[2] - cpl_matrix_get(to,1,0));
    d2 = fabs(worldout[3] - cpl_matrix_get(to,1,1));
    cpl_msg_info("","phys1,phys2:   %15.9f %15.9f",physin[2],physin[3]);
    cpl_msg_info("","world1,world2: %15.9f %15.9f",worldout[2],worldout[3]);
    cpl_msg_info("","calc1,calc2:   %15.9f %15.9f",cpl_matrix_get(to,1,0),
            cpl_matrix_get(to,1,1));
    cpl_msg_info("","diff1,diff2:   %15.9f %15.9f",d1,d2);
    cpl_msg_info("","status:        %d",
                 (cpl_array_get_data_int_const(status)[1]));
    cpl_matrix_delete(to);
    cpl_array_delete(status);

    /* Do world to physical conversion */

    cpl_msg_info("","Transform world -> physical");
    /* worldin is _not_ modified */
    from = cpl_matrix_wrap(1,2, (double*)worldin);
    error = cpl_wcs_convert(wcs,from,&to,&status,CPL_WCS_WORLD2PHYS);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_nonnull(to);
    cpl_test_nonnull(status);
    (void)cpl_matrix_unwrap(from);

    /* Test the output values again */

    cpl_test_zero(cpl_array_get_data_int_const(status)[0]);
    cpl_test_abs(physout[0], cpl_matrix_get(to, 0, 0), 100.0 * FLT_EPSILON);
    cpl_test_abs(physout[1], cpl_matrix_get(to, 0, 1),  20.0 * FLT_EPSILON);

    d1 = fabs(physout[0] - cpl_matrix_get(to,0,0));
    d2 = fabs(physout[1] - cpl_matrix_get(to,0,1));
    cpl_msg_info("","world1,world2: %15.9f %15.9f",worldin[0],worldin[1]);
    cpl_msg_info("","phys1,phys2:   %15.9f %15.9f",physout[0],physout[1]);
    cpl_msg_info("","calc1,calc2:   %15.9f %15.9f",cpl_matrix_get(to,0,0),
            cpl_matrix_get(to,0,1));
    cpl_msg_info("","diff1,diff2:   %15.9f %15.9f",d1,d2);
    cpl_msg_info("","status:        %d",
                 cpl_array_get_data_int_const(status)[0]);
    cpl_matrix_delete(to);
    cpl_array_delete(status);

    /* Do physical to standard */

    cpl_msg_info("","Transform physical -> standard");
    /* physin is _not_ modified */
    from = cpl_matrix_wrap(1, 2, (double*)physin);
    error = cpl_wcs_convert(wcs,from,&to,&status,CPL_WCS_PHYS2STD);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_nonnull(to);
    cpl_test_nonnull(status);
    (void)cpl_matrix_unwrap(from);

    /* Test the output values again */

    cpl_test_zero(cpl_array_get_data_int_const(status)[0]);
    cpl_test_abs(stdcout[0], cpl_matrix_get(to, 0, 0), 1.7e-9);
    cpl_test_abs(stdcout[1], cpl_matrix_get(to, 0, 1), 1.7e-9);

    d1 = fabs(stdcout[0] - cpl_matrix_get(to,0,0));
    d2 = fabs(stdcout[1] - cpl_matrix_get(to,0,1));
    cpl_msg_info("","phys1,phys2:   %15.9f %15.9f",physin[0],physin[1]);
    cpl_msg_info("","std1,std2:     %15.9f %15.9f",stdcout[0],stdcout[1]);
    cpl_msg_info("","calc1,calc2:   %15.9f %15.9f",cpl_matrix_get(to,0,0),
            cpl_matrix_get(to,0,1));
    cpl_msg_info("","diff1,diff2:   %15.9f %15.9f",d1,d2);
    cpl_msg_info("","status:        %d",
                 cpl_array_get_data_int_const(status)[0]);
    cpl_matrix_delete(to);
    cpl_array_delete(status);

    /* Do world to standard */

    cpl_msg_info("","Transform world -> standard");
    /* worldout is _not_ modified */
    from = cpl_matrix_wrap(1, 2, (double*)worldout);
    error = cpl_wcs_convert(wcs, from, &to, &status, CPL_WCS_WORLD2STD);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_nonnull(to);
    cpl_test_nonnull(status);
    (void)cpl_matrix_unwrap(from);

    /* Test the output values again */

    cpl_test_zero(cpl_array_get_data_int_const(status)[0]);
    cpl_test_abs(stdcout[0], cpl_matrix_get(to,0,0), 1.7e-9);
    cpl_test_abs(stdcout[1], cpl_matrix_get(to,0,1), 1.7e-9);

    d1 = fabs(stdcout[0] - cpl_matrix_get(to,0,0));
    d2 = fabs(stdcout[1] - cpl_matrix_get(to,0,1));
    cpl_msg_info("","world1,world2: %15.9f %15.9f",worldout[0],worldout[1]);
    cpl_msg_info("","std1,std2:     %15.9f %15.9f",stdcout[0],stdcout[1]);
    cpl_msg_info("","calc1,calc2:   %15.9f %15.9f",cpl_matrix_get(to,0,0),
            cpl_matrix_get(to,0,1));
    cpl_msg_info("","diff1,diff2:   %15.9f %15.9f",d1,d2);
    cpl_msg_info("","status:        %d",
                 cpl_array_get_data_int_const(status)[0]);
    cpl_matrix_delete(to);
    cpl_array_delete(status);

    /* Tidy */

    cpl_wcs_delete(wcs);

    /* Do a WCS fit. Start by defining an input WCS */

    pl = cpl_propertylist_new();
    for (i = 0; i < NDK; i++)
        cpl_propertylist_append_double(pl,dkeys[i],dvals2[i]);
    for (i = 0; i < NSK; i++)
        cpl_propertylist_append_string(pl,skeys[i],svals2[i]);
    for (i = 0; i < NIK; i++)
        cpl_propertylist_append_int(pl,ikeys[i],ivals2[i]);

    /* Wrap the coordinates into matrices */

    /* xy and radec are _not_ modified */
    xymat = cpl_matrix_wrap(NSTDS, 2, (double*)xy);
    rdmat = cpl_matrix_wrap(NSTDS, 2, (double*)radec);
    if (cpl_msg_get_level() <= CPL_MSG_DEBUG) {
        cpl_matrix_dump(xymat, stdout);
        cpl_matrix_dump(rdmat, stdout);
    }

    /* Do 6 constant plate solution - first with failure checks */

    error = cpl_wcs_platesol(NULL, rdmat, xymat, 3, 3.0,CPL_WCS_PLATESOL_6,
                             CPL_WCS_MV_CRVAL,&opl);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);
    cpl_test_null(opl);

    error = cpl_wcs_platesol(pl, NULL, xymat, 3, 3.0,CPL_WCS_PLATESOL_6,
                             CPL_WCS_MV_CRVAL,&opl);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);
    cpl_test_null(opl);

    error = cpl_wcs_platesol(pl, rdmat, NULL, 3, 3.0,CPL_WCS_PLATESOL_6,
                             CPL_WCS_MV_CRVAL,&opl);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);
    cpl_test_null(opl);

    error = cpl_wcs_platesol(pl, rdmat, xymat, 3, 3.0,CPL_WCS_PLATESOL_6,
                             CPL_WCS_MV_CRVAL, NULL);
    cpl_test_eq_error(error, CPL_ERROR_NULL_INPUT);

    error = cpl_wcs_platesol(pl, rdmat, xymat, 3, 3.0,
                             2 * CPL_WCS_PLATESOL_4 + 2 * CPL_WCS_PLATESOL_6,
                             CPL_WCS_MV_CRVAL,&opl);
    cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);
    cpl_test_null(opl);

    error = cpl_wcs_platesol(pl, rdmat, xymat, 3, 3.0,CPL_WCS_PLATESOL_6,
                             2 * CPL_WCS_MV_CRVAL + 2 * CPL_WCS_MV_CRPIX,
                             &opl);
    cpl_test_eq_error(error, CPL_ERROR_UNSUPPORTED_MODE);
    cpl_test_null(opl);

    error = cpl_wcs_platesol(pl,rdmat,xymat, 0, 3.0,CPL_WCS_PLATESOL_6,
                             CPL_WCS_MV_CRVAL,&opl);
    cpl_test_eq_error(error, CPL_ERROR_ILLEGAL_INPUT);
    cpl_test_null(opl);

    error = cpl_wcs_platesol(pl, rdmat, xymat, 3, 0.0, CPL_WCS_PLATESOL_6,
                             CPL_WCS_MV_CRVAL, &opl);
    cpl_test_eq_error(error, CPL_ERROR_DATA_NOT_FOUND);
    cpl_test_null(opl);

    /* Do 6 constant plate solution */

    error = cpl_wcs_platesol(pl,rdmat,xymat,3,3.0,CPL_WCS_PLATESOL_6,
                             CPL_WCS_MV_CRVAL,&opl);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_nonnull(opl);
    d1 = cpl_propertylist_get_double(opl,"CSYER1");
    d2 = cpl_propertylist_get_double(opl,"CSYER2");
    cpl_msg_info("","WCS 6 constant fit with errors: %20.12g %20.12g",d1,d2);
    cpl_test_leq(d1, 3.8e-5);
    cpl_test_leq(d2, 3.8e-5);
    cpl_wcs_test_propertylist(opl);
    if (cpl_msg_get_level() <= CPL_MSG_DEBUG)
        cpl_propertylist_dump(opl, stdout);
    cpl_propertylist_delete(opl);

    /* Do 6 constant plate solution - using CPL_WCS_MV_CRPIX */

    error = cpl_wcs_platesol(pl,rdmat,xymat,3,3.0,CPL_WCS_PLATESOL_6,
                             CPL_WCS_MV_CRPIX, &opl);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_nonnull(opl);
    d1 = cpl_propertylist_get_double(opl,"CSYER1");
    d2 = cpl_propertylist_get_double(opl,"CSYER2");
    cpl_msg_info("","WCS 6 constant fit with errors: %20.12g %20.12g",d1,d2);
    cpl_test_leq(d1, 3.8e-5);
    cpl_test_leq(d2, 3.8e-5);
    cpl_wcs_test_propertylist(opl);
    if (cpl_msg_get_level() <= CPL_MSG_DEBUG)
        cpl_propertylist_dump(opl, stdout);
    cpl_propertylist_delete(opl);

    /* Do 4 constant plate solution */

    error = cpl_wcs_platesol(pl, rdmat, xymat, 3, 3.0, CPL_WCS_PLATESOL_4,
                             CPL_WCS_MV_CRVAL, &opl);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_nonnull(opl);
    d1 = cpl_propertylist_get_double(opl,"CSYER1");
    d2 = cpl_propertylist_get_double(opl,"CSYER2");
    cpl_msg_info("","WCS 4 constant fit with errors: %20.12g %20.12g",d1,d2);
    cpl_test_leq(d1, 1.05e-4);
    cpl_test_leq(d2, 7.3e-5);
    cpl_wcs_test_propertylist(opl);
    if (cpl_msg_get_level() <= CPL_MSG_DEBUG)
        cpl_propertylist_dump(opl, stdout);
    cpl_propertylist_delete(opl);

    /* Do 4 constant plate solution - using CPL_WCS_MV_CRPIX */
    error = cpl_wcs_platesol(pl, rdmat, xymat, 3, 3.0, CPL_WCS_PLATESOL_4,
                             CPL_WCS_MV_CRPIX, &opl);
    cpl_test_eq_error(error, CPL_ERROR_NONE);
    cpl_test_nonnull(opl);
    d1 = cpl_propertylist_get_double(opl,"CSYER1");
    d2 = cpl_propertylist_get_double(opl,"CSYER2");
    cpl_msg_info("","WCS 6 constant fit with errors: %20.12g %20.12g",d1,d2);
    cpl_test_leq(d1, 1.05e-4);
    cpl_test_leq(d2, 7.3e-5);
    cpl_wcs_test_propertylist(opl);
    if (cpl_msg_get_level() <= CPL_MSG_DEBUG)
        cpl_propertylist_dump(opl, stdout);
    cpl_propertylist_delete(opl);

    /* Tidy a bit */

    cpl_propertylist_delete(pl);
    (void)cpl_matrix_unwrap(xymat);
    (void)cpl_matrix_unwrap(rdmat);

    /* Test accessor functions */

    pl = cpl_propertylist_new();
    for (i = 0; i < NDK; i++)
        cpl_propertylist_append_double(pl,dkeys[i],dvals2[i]);
    for (i = 0; i < NSK; i++)
        cpl_propertylist_append_string(pl,skeys[i],svals2[i]);
    for (i = 0; i < NIK; i++)
        cpl_propertylist_append_int(pl,ikeys[i],ivals2[i]);

    wcs = cpl_wcs_new_from_propertylist(pl);
    cpl_test_nonnull(wcs);
    cpl_test_eq(cpl_wcs_get_image_naxis(wcs), 2);

    arrdims = cpl_wcs_get_image_dims(wcs);
    cpl_test_nonnull(arrdims);
    cpl_test_eq(cpl_array_get_type(arrdims), CPL_TYPE_INT);
    cpl_test_eq(cpl_array_get_size(arrdims), 2);
    for (i = 1; i < NIK; i++) {
        cpl_test_eq(cpl_array_get_int(arrdims, i-1, NULL), ivals2[i]);
    }

    crval = cpl_wcs_get_crval(wcs);
    cpl_test_nonnull(crval);
    cpl_test_eq(cpl_array_get_type(crval), CPL_TYPE_DOUBLE);
    cpl_test_eq(cpl_array_get_size(crval), 2);

    crpix = cpl_wcs_get_crpix(wcs);
    cpl_test_nonnull(crpix);
    cpl_test_eq(cpl_array_get_type(crpix), CPL_TYPE_DOUBLE);
    cpl_test_eq(cpl_array_get_size(crpix), 2);

    cd = cpl_wcs_get_cd(wcs);
    cpl_test_nonnull(cd);
    cpl_test_eq(cpl_matrix_get_nrow(cd), 2);
    cpl_test_eq(cpl_matrix_get_ncol(cd), 2);

    crval1 = cpl_array_get_double(crval, 0, NULL);
    cpl_test_eq(crval1, dvals2[0]);
    crval2 = cpl_array_get_double(crval, 1, NULL);
    cpl_test_eq(crval2, dvals2[1]);

    crpix1 = cpl_array_get_double(crpix, 0, NULL);
    cpl_test_eq(crpix1, dvals2[2]);
    crpix2 = cpl_array_get_double(crpix, 1, NULL);
    cpl_test_eq(crpix2, dvals2[3]);

    cd1 = cpl_matrix_get(cd, 0, 0);
    cpl_test_eq(cd1, dvals2[4]);
    cd2 = cpl_matrix_get(cd, 0, 1);
    cpl_test_eq(cd2, dvals2[5]);
    cd3 = cpl_matrix_get(cd, 1, 0);
    cpl_test_eq(cd3, dvals2[6]);
    cd4 = cpl_matrix_get(cd, 1, 1);
    cpl_test_eq(cd4, dvals2[7]);

    /* Tidy */

    cpl_propertylist_delete(pl);
    cpl_wcs_delete(wcs);

    /* See if we can parse the table style keywords */

    pl = cpl_propertylist_new();
    for (i = 0; i < NDK; i++)
        cpl_propertylist_append_double(pl,dkeys_tab[i],dvals[i]);
    for (i = 0; i < NSK; i++)
        cpl_propertylist_append_string(pl,skeys_tab[i],svals[i]);
    wcs = cpl_wcs_new_from_propertylist(pl);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(wcs);

    /* Tidy */

    cpl_propertylist_delete(pl);
    cpl_wcs_delete(wcs);

    /* Are there any memory leaks (NB: this only covers CPL. Memory is
       allocated separately by WCSLIB */
    return cpl_test_end(0);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test cpl_wcs_new_from_propertylist() using a specified propertylist
  @param  self The propertylist to test, must be non-NULL and with NAXIS == 0
  @return void
  @see cpl_wcs_new_from_propertylist()

  FIXME: Add more validation

 */
/*----------------------------------------------------------------------------*/
static void cpl_wcs_test_propertylist(const cpl_propertylist * self)
{
        cpl_wcs * wcs =  cpl_wcs_new_from_propertylist(self);
        const int naxis = cpl_wcs_get_image_naxis(wcs);
        const cpl_array  * ardims  = cpl_wcs_get_image_dims(wcs);
        const cpl_array  * arcrval = cpl_wcs_get_crval(wcs);
        const cpl_array  * arcrpix = cpl_wcs_get_crpix(wcs);
        const cpl_matrix * mtcd    = cpl_wcs_get_cd(wcs);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(wcs);

        cpl_test_zero(naxis);

        cpl_test_null(ardims);

        cpl_test_nonnull(arcrval);
        cpl_test_eq(cpl_array_get_size(arcrval), 2);

        cpl_test_nonnull(arcrpix);
        cpl_test_eq(cpl_array_get_size(arcrpix), 2);

        cpl_test_nonnull(mtcd);
        cpl_test_eq(cpl_matrix_get_nrow(mtcd), 2);
        cpl_test_eq(cpl_matrix_get_ncol(mtcd), 2);

        cpl_wcs_delete(wcs);
}

/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test cpl_error_wcs_set()
  @param  The number of WCS error codes to test
  @return void
  @note The number of error messages in WCSLIB 4.4.4 is 14

 */
/*----------------------------------------------------------------------------*/
static void cpl_wcs_error_test(int nerror) {

    int i;

    for (i = -1; i < nerror; i++) {
        cpl_error_code error;

        error = cpl_error_set_wcs(CPL_ERROR_NONE, i, "my_wcs_dummy", "OK");
        cpl_test_eq_error(error, CPL_ERROR_NONE);

        error = cpl_error_set_wcs(CPL_ERROR_EOL,  i, "my_wcs_dummy", "FAIL");
        cpl_test_eq_error(error, CPL_ERROR_EOL);
    }
}


/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test a header from a specific file, nonaxis.fits, w. NAXIS=0
  @return void

 */
/*----------------------------------------------------------------------------*/
static void cpl_wcs_test_one(void)
{

    cpl_propertylist * pl = cpl_propertylist_load(TESTFILE, 0);

    cpl_msg_info(cpl_func, "Testing " TESTFILE ": %p", (const void*)pl);

    if (pl == NULL) {
        /* Ignore errors */
        cpl_test_error(cpl_error_get_code());
    } else {
        cpl_wcs_test_propertylist(pl);
        cpl_propertylist_delete(pl);
    }
}



/*----------------------------------------------------------------------------*/
/**
  @internal
  @brief  Test a given test case
  @return void

 */
/*----------------------------------------------------------------------------*/
static void cpl_wcs_test_5115(void)
{
    cpl_propertylist * header = cpl_propertylist_new();
    cpl_wcs * wcs;
    cpl_error_code code;

    code = cpl_propertylist_append_int(header, "NAXIS", 3);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_int(header, "NAXIS1", 14);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_int(header, "NAXIS2", 14);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_int(header, "NAXIS3", 10);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_double(header, "CRPIX1", 7.5);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_double(header, "CRPIX2", 7.5);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_double(header, "CRPIX3", 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_double(header, "CRVAL1", 6.12595);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_double(header, "CRVAL2", -72.08291);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_double(header, "CRVAL3", 1.43099);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_double(header, "CDELT1", -5.55555e-5);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_double(header, "CDELT2", -5.55555e-5);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_double(header, "CDELT3", 0.0002075);
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(header, "CTYPE1", "RA---TAN");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(header, "CTYPE2", "DEC--TAN");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(header, "CTYPE3", "WAVE");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(header, "CUNIT1", "deg");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(header, "CUNIT2", "deg");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(header, "CUNIT3", "um");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_string(header, "ESO PRO FRNAME", "ga");
    cpl_test_eq_error(code, CPL_ERROR_NONE);
    code = cpl_propertylist_append_int(header, "ESO PRO IFUNR", 1);
    cpl_test_eq_error(code, CPL_ERROR_NONE);

    wcs = cpl_wcs_new_from_propertylist(header);

    cpl_test_error(CPL_ERROR_NONE);
    cpl_test_nonnull(wcs);

    cpl_wcs_delete(wcs);
    cpl_propertylist_delete(header);
}
