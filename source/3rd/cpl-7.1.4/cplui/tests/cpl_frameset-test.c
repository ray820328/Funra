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

#include <cxmacros.h>

#include <cpl_msg.h>
#include <cpl_memory.h>

#include <cpl_test.h>
#include "cpl_frameset.h"

#include <stdio.h>
#include <string.h>

static int nframe = 0;

static int
frame_equal(const cpl_frame * frame1, const cpl_frame * frame2)
{

    if (frame1 == NULL) return -1;
    if (frame2 == NULL) return -1;

    return 1;
}


static int
frame_differ(const cpl_frame * frame1, const cpl_frame * frame2)
{

    if (frame1 == NULL) return -1;
    if (frame2 == NULL) return -1;

    return frame1 == frame2 ? 1 : 0;
}


static int
frame_oddeven(const cpl_frame * frame1, const cpl_frame * frame2)
{

    if (frame1 == NULL) return -1;
    if (frame2 == NULL) return -1;

    if (frame1 == frame2) return 1;

    return (nframe++ & 1);
}


int main(void)
{

    const char *names[] = {
        "flat1.fits",
        "flat2.fits",
        "flat3.fits",
        "bias1.fits",
        "bias2.fits",
        "bias3.fits",
        "mbias.fits",
        "mflat.fits",
        "science.fits",
        "product.fits"
    };

    const char *tags[] = {
        "FLAT",
        "FLAT",
        "FLAT",
        "BIAS",
        "BIAS",
        "BIAS",
        "MASTER_BIAS",
        "MASTER_FLAT",
        "SCIENCE",
        "SCIENCE_CALIBRATED"
    };

    cpl_frame_group groups[] = {
        CPL_FRAME_GROUP_RAW,
        CPL_FRAME_GROUP_RAW,
        CPL_FRAME_GROUP_RAW,
        CPL_FRAME_GROUP_RAW,
        CPL_FRAME_GROUP_RAW,
        CPL_FRAME_GROUP_RAW,
        CPL_FRAME_GROUP_CALIB,
        CPL_FRAME_GROUP_CALIB,
        CPL_FRAME_GROUP_RAW,
        CPL_FRAME_GROUP_PRODUCT
    };

    cpl_frame_level levels[] = {
        CPL_FRAME_LEVEL_NONE,
        CPL_FRAME_LEVEL_NONE,
        CPL_FRAME_LEVEL_NONE,
        CPL_FRAME_LEVEL_NONE,
        CPL_FRAME_LEVEL_NONE,
        CPL_FRAME_LEVEL_NONE,
        CPL_FRAME_LEVEL_NONE,
        CPL_FRAME_LEVEL_NONE,
        CPL_FRAME_LEVEL_NONE,
        CPL_FRAME_LEVEL_FINAL
    };

    long i;

    const char *filename1 = "cplframesetdump.txt";

    cpl_frame *frame;
    cpl_frame *_frame;

    cpl_frameset *frameset;
    cpl_frameset *_frameset;
    cpl_frameset_iterator *it  = NULL;
    cpl_frameset_iterator *_it = NULL;
    cpl_frameset *allframes;

    FILE *out;

    cpl_test_init(PACKAGE_BUGREPORT, CPL_MSG_WARNING);

    /* Insert tests below */

    /*
     * Test 1: Create a frameset and check its validity.
     */

    frameset = cpl_frameset_new();

    cpl_test_nonnull(frameset);
    cpl_test(cpl_frameset_is_empty(frameset));
    cpl_test_zero(cpl_frameset_get_size(frameset));


    /*
     * Test 2: Add frames to the frame set created in the previous test
     *         and verify the data.
     */

    for (i = 0; (size_t)i < CX_N_ELEMENTS(names); i++) {
        _frame = cpl_frame_new();

        cpl_frame_set_filename(_frame, names[i]);
        cpl_frame_set_tag(_frame, tags[i]);
        cpl_frame_set_type(_frame, CPL_FRAME_TYPE_IMAGE);
        cpl_frame_set_group(_frame, groups[i]);
        cpl_frame_set_level(_frame, levels[i]);

        cpl_frameset_insert(frameset, _frame);
    }

    cpl_test_zero(cpl_frameset_is_empty(frameset));
    cpl_test_eq(cpl_frameset_get_size(frameset),  CX_N_ELEMENTS(names));
    cpl_test_eq(cpl_frameset_count_tags(frameset, tags[0]),  3);
    cpl_test_eq(cpl_frameset_count_tags(frameset, tags[3]),  3);
    cpl_test_eq(cpl_frameset_count_tags(frameset, tags[6]),  1);
    cpl_test_eq(cpl_frameset_count_tags(frameset, tags[7]),  1);
    cpl_test_eq(cpl_frameset_count_tags(frameset, tags[8]),  1);
    cpl_test_eq(cpl_frameset_count_tags(frameset, tags[9]),  1);


    /*
     * Test 2a: Dump the frameset into a file in disk
     */

    out = fopen(filename1, "w");
    cpl_frameset_dump(frameset, out);
    fclose (out);

    allframes = cpl_frameset_duplicate(frameset);


    /*
     * Test 3: Lookup frames in the set and verify that the right frames
     *         are found.
     */

    frame = cpl_frameset_find(frameset, tags[0]);
    cpl_test_eq_string(cpl_frame_get_filename(frame), names[0]);

    frame = cpl_frameset_find(frameset, NULL);
    cpl_test_eq_string(cpl_frame_get_filename(frame), names[1]);

    frame = cpl_frameset_find(frameset, NULL);
    cpl_test_eq_string(cpl_frame_get_filename(frame), names[2]);
    cpl_test_null(cpl_frameset_find(frameset, NULL));


    /*
     * Test 4: Verify that the internal cache is reset correctly.
     */

    cpl_frameset_find(frameset, tags[0]);

    frame = cpl_frameset_find(frameset, NULL);
    cpl_test_eq_string(cpl_frame_get_filename(frame), names[1]);

    cpl_frameset_find(frameset, tags[3]);

    frame = cpl_frameset_find(frameset, NULL);
    cpl_test_eq_string(cpl_frame_get_filename(frame), names[4]);


    /*
     * Test 5: Check side effects when mixing calls to
     *         cpl_frameset_find with iterators.
     */

    it = cpl_frameset_iterator_new(frameset);

    cpl_frameset_find(frameset, tags[3]);

    frame = cpl_frameset_find(frameset, NULL);
    cpl_test_eq_string(cpl_frame_get_filename(frame), names[4]);

    frame = cpl_frameset_iterator_get(it);
    cpl_test_eq_string(cpl_frame_get_filename(frame), names[0]);

    frame = cpl_frameset_find(frameset, NULL);
    cpl_test_eq_string(cpl_frame_get_filename(frame), names[5]);

    cpl_frameset_iterator_advance(it, 1);
    frame = cpl_frameset_iterator_get(it);
    cpl_test_eq_string(cpl_frame_get_filename(frame), names[1]);

    /* Skip to flat1.fits */

    cpl_frameset_iterator_reset(it);
    frame = cpl_frameset_iterator_get(it);
    cpl_test_eq_string(cpl_frame_get_filename(frame), names[0]);

    cpl_frameset_find(frameset, tags[8]);

    cpl_frameset_iterator_advance(it, 1);
    frame = cpl_frameset_iterator_get(it);
    cpl_test_eq_string(cpl_frame_get_filename(frame), names[1]);

    cpl_frameset_iterator_delete(it);
    it = NULL;

    /*
     * Test 6: Erase frames by tag from the frame set and verify the set
     *         structure and its contents.
     */

    cpl_test_eq(cpl_frameset_erase(frameset, tags[0]),  3);
    cpl_test_eq(cpl_frameset_get_size(frameset),  (CX_N_ELEMENTS(tags) - 3));
    cpl_test_zero(cpl_frameset_count_tags(frameset, tags[0]));


    /*
     * Test 7: Erase frames from the frame set and verify the set
     *         structure and its contents.
     */

    frame = cpl_frameset_find(frameset, tags[3]);

    while (frame) {
        cpl_frame *f = frame;

        frame = cpl_frameset_find(frameset, NULL);
        cpl_frameset_erase_frame(frameset, f);
    };

    cpl_test_eq(cpl_frameset_get_size(frameset),  (CX_N_ELEMENTS(tags) - 6));
    cpl_test_zero(cpl_frameset_count_tags(frameset, tags[3]));

    i = 6;

    it = cpl_frameset_iterator_new(frameset);
    cpl_frameset_iterator_advance(it, i);

    frame = cpl_frameset_iterator_get(it);

    while (frame) {
        cpl_test_eq_string(cpl_frame_get_filename(frame), names[i++]);

        cpl_frameset_iterator_advance(it, 1);
        frame = cpl_frameset_iterator_get(it);
    }

    cpl_frameset_iterator_delete(it);
    it = NULL;


    /*
     * Test 8: Create a copy of the frame set and verify that original and
     *         copy are identical but do not share any resources.
     */

    _frameset = cpl_frameset_duplicate(frameset);
    cpl_test_nonnull(_frameset);
    cpl_test_noneq_ptr(_frameset, frameset);

    cpl_test_eq(cpl_frameset_get_size(_frameset),
                cpl_frameset_get_size(frameset));

    it  = cpl_frameset_iterator_new(frameset);
    _it = cpl_frameset_iterator_new(_frameset);

    frame = cpl_frameset_iterator_get(it);
    _frame = cpl_frameset_iterator_get(_it);

    while (frame) {
        cpl_test_noneq_ptr(cpl_frame_get_filename(_frame),
                           cpl_frame_get_filename(frame));
        cpl_test_eq_string(cpl_frame_get_filename(_frame),
                           cpl_frame_get_filename(frame));

        cpl_test_noneq_ptr(cpl_frame_get_tag(_frame), cpl_frame_get_tag(frame));
        cpl_test_eq_string(cpl_frame_get_tag(_frame),
                           cpl_frame_get_tag(frame));

        cpl_frameset_iterator_advance(it, 1);
        cpl_frameset_iterator_advance(_it, 1);

        frame = cpl_frameset_iterator_get(it);
        _frame = cpl_frameset_iterator_get(_it);
    }

    cpl_test_null(_frame);

    cpl_frameset_iterator_delete(_it);
    cpl_frameset_iterator_delete(it);


    /*
     * Test 9: Tests of cpl_frameset_labelise() and cpl_frameset_extract().
     */

    {
        cpl_size nlabs;
        const cpl_size *labnull = cpl_frameset_labelise(allframes, NULL, &nlabs);
        cpl_size *labels;

        const cpl_frameset * setnull;

        cpl_test_error(CPL_ERROR_NULL_INPUT);
        cpl_test_null(labnull);

        setnull = cpl_frameset_extract(allframes, labnull, 0);

        cpl_test_error(CPL_ERROR_NULL_INPUT);
        cpl_test_null(setnull);

        /* All frames are equal */
        labels = cpl_frameset_labelise(allframes, frame_equal, &nlabs);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(labels);

        cpl_test_eq(nlabs,  1);

        while (nlabs-- > 0) {
            cpl_test_eq(labels[nlabs],  nlabs);
        }

        cpl_free(labels);

        /* All frames differ */
        labels = cpl_frameset_labelise(allframes, frame_differ, &nlabs);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(labels);

        cpl_test_eq(nlabs,  cpl_frameset_get_size(allframes));

        while (nlabs-- > 0) {
            cpl_test_eq(labels[nlabs],  nlabs);
        }

        cpl_free(labels);

        /* Two labels */
        labels = cpl_frameset_labelise(allframes, frame_oddeven, &nlabs);

        cpl_test_error(CPL_ERROR_NONE);
        cpl_test_nonnull(labels);

        cpl_test_eq(nlabs,  2);

        cpl_free(labels);

    }

    cpl_frameset_delete(_frameset);
    cpl_frameset_delete(frameset);
    cpl_frameset_delete(allframes);

    /* End of tests */
    return cpl_test_end(0);
}
