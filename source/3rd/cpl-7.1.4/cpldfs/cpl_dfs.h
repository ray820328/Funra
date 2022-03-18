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

#ifndef CPL_DFS_H
#define CPL_DFS_H

#include <cpl_imagelist.h>
#include <cpl_table.h>
#include <cpl_frameset.h>
#include <cpl_propertylist.h>
#include <cpl_parameterlist.h>
#include <cpl_error.h>

CPL_BEGIN_DECLS

/*-----------------------------------------------------------------------------
                                 Defines
 -----------------------------------------------------------------------------*/

#define CPL_DFS_FITS ".fits"
#define CPL_DFS_PAF ".paf"


/*----------------------------------------------------------------------------*/
/**
   @ingroup cpl_dfs
   @brief  The name of the Product Category key
   @see cpl_dfs_save_image()
   @note A pipeline product must contain a string property with this name
 */
/*----------------------------------------------------------------------------*/
#define CPL_DFS_PRO_CATG "ESO PRO CATG"

/*----------------------------------------------------------------------------*/
/**
   @ingroup cpl_dfs
   @brief  The name of the Product Type key
   @see cpl_dfs_save_image()
   @note A pipeline product should contain a string property with this name

 */
/*----------------------------------------------------------------------------*/
#define CPL_DFS_PRO_TYPE "ESO PRO TYPE"

/*----------------------------------------------------------------------------*/
/**
   @ingroup cpl_dfs
   @brief  The name of the Product Tech key
   @see cpl_dfs_save_image()
   @note A pipeline product should contain a string property with this name

 */
/*----------------------------------------------------------------------------*/
#define CPL_DFS_PRO_TECH "ESO PRO TECH"

/*----------------------------------------------------------------------------*/
/**
   @ingroup cpl_dfs
   @brief  The name of the Product Science key
   @see cpl_dfs_save_image()
   @note A pipeline product should contain a boolean property with this name
 */
/*----------------------------------------------------------------------------*/
#define CPL_DFS_PRO_SCIENCE "ESO PRO SCIENCE"


/**
 * @ingroup cpl_dfs
 *
 * @brief
 *  Pipeline products digital signature flags.
 *
 * Flags to select the different digital signatures to compute for pipeline
 * product files. The values may be combined using bitwise or.
 */

enum {

    /**
     * Do not compute any signatures
     * @hideinitializer
     */

    CPL_DFS_SIGNATURE_NONE     = 0,

    /**
     * Compute the DATAMD5 data hash
     * @hideinitializer
     */

    CPL_DFS_SIGNATURE_DATAMD5  = 1 << 0,

    /**
     * Compute FITS standard CHECKSUM and DATASUM
     * @hideinitializer
     */

    CPL_DFS_SIGNATURE_CHECKSUM = 1 << 1
};


/*-----------------------------------------------------------------------------
                              Function prototypes
 -----------------------------------------------------------------------------*/

cpl_error_code cpl_dfs_save_image(cpl_frameset *,
                                  cpl_propertylist *,
                                  const cpl_parameterlist *,
                                  const cpl_frameset *,
                                  const cpl_frame *,
                                  const cpl_image *,
                                  cpl_type,
                                  const char *,
                                  const cpl_propertylist *,
                                  const char *,
                                  const char *,
                                  const char *);

cpl_error_code cpl_dfs_save_propertylist(cpl_frameset *,
                                         cpl_propertylist *,
                                         const cpl_parameterlist *,
                                         const cpl_frameset *,
                                         const cpl_frame *,
                                         const char *,
                                         const cpl_propertylist *,
                                         const char *,
                                         const char *,
                                         const char *);

cpl_error_code cpl_dfs_save_imagelist(cpl_frameset *,
                                      cpl_propertylist *,
                                      const cpl_parameterlist *,
                                      const cpl_frameset *,
                                      const cpl_frame *,
                                      const cpl_imagelist *,
                                      cpl_type,
                                      const char *,
                                      const cpl_propertylist *,
                                      const char *,
                                      const char *,
                                      const char *);

cpl_error_code cpl_dfs_save_table(cpl_frameset *,
                                  cpl_propertylist *,
                                  const cpl_parameterlist *,
                                  const cpl_frameset *,
                                  const cpl_frame *,
                                  const cpl_table *,
                                  const cpl_propertylist *,
                                  const char *,
                                  const cpl_propertylist *,
                                  const char *,
                                  const char *,
                                  const char *);

cpl_error_code cpl_dfs_save_paf(const char *, const char *,
                                const cpl_propertylist *, const char *);

cpl_error_code cpl_dfs_setup_product_header(cpl_propertylist *, 
                                            const cpl_frame *, 
                                            const cpl_frameset *, 
                                            const cpl_parameterlist *, 
                                            const char *,
                                            const char *,
                                            const char *,
                                            const cpl_frame *);

cpl_error_code cpl_dfs_update_product_header(cpl_frameset *);

cpl_error_code cpl_dfs_sign_products(const cpl_frameset *set,
                                      unsigned int flags);

CPL_END_DECLS

#endif
