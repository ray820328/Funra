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

#ifndef CPL_PHYS_CONST_H
#define CPL_PHYS_CONST_H

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                Defines
 -----------------------------------------------------------------------------*/

/* Fundamental physical constants from http://physics.nist.gov/constants/
   (the above page has been moved, probably to http://physics.nist.gov/cuu/)
   SI-units are used unless otherwise stated.

   The constants are listed in numerical order
*/

/* Planck constant [Js] */
#define CPL_PHYS_H 6.6260693E-34 

/* Boltzmann constant [J/K] */
#define CPL_PHYS_K 1.3806505E-23

/* Wien displacement law constant [mK] (meter * Kelvin)*/
#define CPL_PHYS_Wien 2.8977685E-3

/* The speed of light in vacuum [m/s] */
#define CPL_PHYS_C 299792458.0

enum _cpl_unit_ {
    CPL_UNIT_LESS           =  1,  /* Dimension-less */
    CPL_UNIT_RADIAN         =  2,  /* [radian] */
    CPL_UNIT_LENGTH         =  3,  /* [m] */
    CPL_UNIT_TIME           =  5,  /* [s] */
    CPL_UNIT_PERLENGTH      =  7,  /* [1/m] */
    CPL_UNIT_FREQUENCY      =  11, /* [1/s] */
    CPL_UNIT_MASS           =  13, /* [kg] */

    /* Derived quantities */

    CPL_UNIT_ACCELERATION          /* [m/s^2]*/
      = CPL_UNIT_LENGTH * CPL_UNIT_FREQUENCY * CPL_UNIT_FREQUENCY,
    CPL_UNIT_FORCE                 /* [N] = [kg * m/s^2]*/
    =     CPL_UNIT_MASS * CPL_UNIT_ACCELERATION,
    CPL_UNIT_ENERGY                /* [J] = [m * N] */
    = CPL_UNIT_LENGTH * CPL_UNIT_FORCE,
    CPL_UNIT_PHOTONRADIANCE        /* [  radian/s/m^3] */
    = CPL_UNIT_RADIAN * CPL_UNIT_FREQUENCY * CPL_UNIT_PERLENGTH
    * CPL_UNIT_PERLENGTH * CPL_UNIT_PERLENGTH,
    CPL_UNIT_ENERGYRADIANCE        /* [J*radian/s/m^3] */
    = CPL_UNIT_ENERGY * CPL_UNIT_PHOTONRADIANCE
};

typedef enum _cpl_unit_ cpl_unit;

CPL_END_DECLS

#endif

