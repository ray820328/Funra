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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * CPL Core
 */

#include "cpl_bivector.h"
#include "cpl_error.h"
#include "cpl_errorstate.h"
#include "cpl_fits.h"
#include "cpl_image.h"
#include "cpl_image_basic.h"
#include "cpl_image_iqe.h"
#include "cpl_image_bpm.h"
#include "cpl_image_filter.h"
#include "cpl_image_gen.h"
#include "cpl_image_io.h"
#include "cpl_image_resample.h"
#include "cpl_imagelist.h"
#include "cpl_imagelist_basic.h"
#include "cpl_imagelist_io.h"
#include "cpl_image_stats.h"
#include "cpl_init.h"
#include "cpl_io.h"
#include "cpl_macros.h"
#include "cpl_mask.h"
#include "cpl_matrix.h"
#include "cpl_memory.h"
#include "cpl_msg.h"
#include "cpl_plot.h"
#include "cpl_polynomial.h"
#include "cpl_property.h"
#include "cpl_propertylist.h"
#include "cpl_stats.h"
#include "cpl_table.h"
#include "cpl_test.h"
#include "cpl_type.h"
#include "cpl_vector.h"
#include "cpl_math_const.h"
#include "cpl_version.h"
#include "cpl_func.h"

/*
 * CPL UI
 */

#include "cpl_frame.h"
#include "cpl_frameset.h"
#include "cpl_frameset_io.h"
#include "cpl_framedata.h"
#include "cpl_parameter.h"
#include "cpl_parameterlist.h"
#include "cpl_plugin.h"
#include "cpl_plugininfo.h"
#include "cpl_pluginlist.h"
#include "cpl_recipe.h"
#include "cpl_recipeconfig.h"
#include "cpl_recipedefine.h"

/*
 * CPL DRS
 */

#include "cpl_apertures.h"
#include "cpl_apertures_img.h"
#include "cpl_apertures_img.h"
#include "cpl_detector.h"
#include "cpl_fit.h"
#include "cpl_geom_img.h"
#include "cpl_photom.h"
#include "cpl_phys_const.h"
#include "cpl_ppm.h"
#include "cpl_wcs.h"
#include "cpl_fft.h"

/*
 * CPL DFS
 */

#include "cpl_dfs.h"
#include "cpl_multiframe.h"
