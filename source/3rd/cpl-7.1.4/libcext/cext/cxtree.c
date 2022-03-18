/*
 * This file is part of the ESO C Extension Library
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
#  include <config.h>
#endif

#include "cxmemory.h"
#include "cxmessages.h"
#include "cxtree.h"



/**
 * @defgroup cxtree Balanced Binary Trees
 *
 * The module implements a balanced binary tree type, i.e. a container
 * managing key/value pairs as elements. The container is optimized for
 * lookup operations.
 *
 * @par Synopsis:
 * @code
 *   #include <cxtree.h>
 * @endcode
 */

/**@{*/

/*
 * Tree node color, tree node and tree opaque data types
 */

enum _cx_tnode_color_ {
    CX_TNODE_RED = 0,
    CX_TNODE_BLACK
};

typedef enum _cx_tnode_color_ cx_tnode_color;

struct _cx_tnode_ {
    struct _cx_tnode_ *left;
    struct _cx_tnode_ *right;
    struct _cx_tnode_ *parent;
    cx_tnode_color color;
    cxptr key;
    cxptr value;
};

typedef struct _cx_tnode_ cx_tnode;

struct _cx_tree_ {
    cx_tnode *header;
    cxsize node_count;
    cx_tree_compare_func key_compare;
    cx_free_func key_destroy;
    cx_free_func value_destroy;
};



/*
 * Some macros, mostly defined for readability purposes.
 */

#define _cx_tnode_left(node)     ((node)->left)
#define _cx_tnode_right(node)    ((node)->right)
#define _cx_tnode_parent(node)   ((node)->parent)
#define _cx_tnode_grandpa(node)  ((node)->parent->parent)
#define _cx_tnode_color(node)    ((node)->color)
#define _cx_tnode_key(node)      ((node)->key)
#define _cx_tnode_value(node)    ((node)->value)

#define _cx_tree_head(tree)           ((tree)->header)
#define _cx_tree_root(tree)           ((tree)->header->parent)
#define _cx_tree_leftmost(tree)       ((tree)->header->left)
#define _cx_tree_rightmost(tree)      ((tree)->header->right)
#define _cx_tree_node_count(tree)     ((tree)->node_count)
#define _cx_tree_key_destroy(tree)    ((tree)->key_destroy)
#define _cx_tree_value_destroy(tree)  ((tree)->value_destroy)
#define _cx_tree_compare(tree)        ((tree)->key_compare)

#define _cx_tree_key_compare(tree, a, b)  ((tree)->key_compare((a), (b)))


/*
 * Internal, node related methods
 */


inline static cx_tnode *
_cx_tnode_create(cxcptr key, cxcptr value)
{
    cx_tnode *node = cx_malloc(sizeof(cx_tnode));

    _cx_tnode_left(node) = NULL;
    _cx_tnode_right(node) = NULL;
    _cx_tnode_parent(node) = NULL;
    _cx_tnode_key(node) = (cxptr)key;
    _cx_tnode_value(node) = (cxptr)value;

    return node;
}


inline static void
_cx_tnode_destroy(cx_tnode *node, cx_free_func key_destroy,
                  cx_free_func value_destroy)
{

    if (node) {
        if (key_destroy) {
            key_destroy(_cx_tnode_key(node));
            _cx_tnode_key(node) = NULL;
        }

        if (value_destroy) {
            value_destroy(_cx_tnode_value(node));
            _cx_tnode_value(node) = NULL;
        }

        _cx_tnode_left(node) = NULL;
        _cx_tnode_right(node) = NULL;
        _cx_tnode_parent(node) = NULL;

        cx_free(node);
    }

    return;

}


inline static cx_tnode *
_cx_tnode_minimum(const cx_tnode *node)
{

    register cx_tnode *n = (cx_tnode *)node;

    while (_cx_tnode_left(n) != NULL)
        n = _cx_tnode_left(n);

    return n;

}


inline static cx_tnode *
_cx_tnode_maximum(const cx_tnode *node)
{

    register cx_tnode *n = (cx_tnode *)node;

    while (_cx_tnode_right(n) != NULL)
        n = _cx_tnode_right(n);

    return n;

}


inline static cxptr
_cx_tnode_get_key(const cx_tnode *node)
{

    return node->key;

}


inline static void
_cx_tnode_set_key(cx_tnode *node, cxcptr key)
{

    node->key = (cxptr)key;
    return;

}


inline static cxptr
_cx_tnode_get_value(const cx_tnode *node)
{

    return node->value;

}


inline static void
_cx_tnode_set_value(cx_tnode *node, cxcptr value)
{

    node->value = (cxptr)value;
    return;

}


inline static cx_tnode *
_cx_tnode_next(const cx_tnode *node)
{

    register cx_tnode *n = (cx_tnode *)node;

    if (_cx_tnode_right(n) != NULL) {
        n = _cx_tnode_right(n);
        while (_cx_tnode_left(n) != NULL)
            n = _cx_tnode_left(n);
    }
    else {
        cx_tnode *m = _cx_tnode_parent(n);

        while (n == _cx_tnode_right(m)) {
            n = m;
            m = _cx_tnode_parent(m);
        }

        if (_cx_tnode_right(n) != m)
            n = m;
    }

    return n;

}


inline static cx_tnode *
_cx_tnode_previous(const cx_tnode *node)
{

    register cx_tnode *n = (cx_tnode *)node;


    if (_cx_tnode_color(n) == CX_TNODE_RED && _cx_tnode_grandpa(n) == n)
        n = _cx_tnode_right(n);
    else
        if (_cx_tnode_left(n) != NULL) {
            cx_tnode *m = _cx_tnode_left(n);

            while (_cx_tnode_right(m) != NULL)
                m = _cx_tnode_right(m);

            n = m;
        }
        else {
            cx_tnode *m = _cx_tnode_parent(n);

            while (n == _cx_tnode_left(m)) {
                n = m;
                m = _cx_tnode_parent(m);
            }

            n = m;
        }

    return n;

}


/*
 * Internal, tree related methods
 */


inline static void
_cx_tree_rotate_left(cx_tnode *node, cx_tnode **root)
{

    register cx_tnode *y = _cx_tnode_right(node);


    _cx_tnode_right(node) = _cx_tnode_left(y);

    if (_cx_tnode_left(y) != NULL)
        _cx_tnode_parent(_cx_tnode_left(y)) = node;

    _cx_tnode_parent(y) = _cx_tnode_parent(node);

    if (node == *root)
        *root = y;
    else
        if (node == _cx_tnode_left(_cx_tnode_parent(node)))
            _cx_tnode_left(_cx_tnode_parent(node)) = y;
        else
            _cx_tnode_right(_cx_tnode_parent(node)) = y;

    _cx_tnode_left(y) = node;
    _cx_tnode_parent(node) = y;

    return;

}


inline static void
_cx_tree_rotate_right(cx_tnode *node, cx_tnode **root)
{

    register cx_tnode *y = _cx_tnode_left(node);


    _cx_tnode_left(node) = _cx_tnode_right(y);

    if (_cx_tnode_right(y) != NULL)
        _cx_tnode_parent(_cx_tnode_right(y)) = node;

    _cx_tnode_parent(y) = _cx_tnode_parent(node);

    if (node == *root)
        *root = y;
    else
        if (node == _cx_tnode_right(_cx_tnode_parent(node)))
            _cx_tnode_right(_cx_tnode_parent(node)) = y;
        else
            _cx_tnode_left(_cx_tnode_parent(node)) = y;

    _cx_tnode_right(y) = node;
    _cx_tnode_parent(node) = y;

    return;

}


inline static void
_cx_tree_rebalance(cx_tnode *node, cx_tnode **root)
{

    _cx_tnode_color(node) = CX_TNODE_RED;

    while (node != *root &&
           _cx_tnode_color(_cx_tnode_parent(node)) == CX_TNODE_RED) {

        if (_cx_tnode_parent(node) ==
            _cx_tnode_left(_cx_tnode_grandpa(node))) {
            cx_tnode *y = _cx_tnode_right(_cx_tnode_grandpa(node));

            if (y && _cx_tnode_color(y) == CX_TNODE_RED) {
                _cx_tnode_color(_cx_tnode_parent(node)) = CX_TNODE_BLACK;
                _cx_tnode_color(_cx_tnode_grandpa(node)) = CX_TNODE_RED;
                _cx_tnode_color(y) = CX_TNODE_BLACK;
                node = _cx_tnode_grandpa(node);
            }
            else {
                if (node == _cx_tnode_right(_cx_tnode_parent(node))) {
                    node = _cx_tnode_parent(node);
                    _cx_tree_rotate_left(node, root);
                }

                _cx_tnode_color(_cx_tnode_parent(node)) = CX_TNODE_BLACK;
                _cx_tnode_color(_cx_tnode_grandpa(node)) = CX_TNODE_RED;

                _cx_tree_rotate_right(_cx_tnode_grandpa(node), root);
            }
        }
        else {
            cx_tnode *y = _cx_tnode_left(_cx_tnode_grandpa(node));

            if (y && _cx_tnode_color(y) == CX_TNODE_RED) {
                _cx_tnode_color(_cx_tnode_parent(node)) = CX_TNODE_BLACK;
                _cx_tnode_color(_cx_tnode_grandpa(node)) = CX_TNODE_RED;
                _cx_tnode_color(y) = CX_TNODE_BLACK;
                node = _cx_tnode_grandpa(node);
            }
            else {
                if (node == _cx_tnode_left(_cx_tnode_parent(node))) {
                    node = _cx_tnode_parent(node);
                    _cx_tree_rotate_right(node, root);
                }

                _cx_tnode_color(_cx_tnode_parent(node)) = CX_TNODE_BLACK;
                _cx_tnode_color(_cx_tnode_grandpa(node)) = CX_TNODE_RED;

                _cx_tree_rotate_left(_cx_tnode_grandpa(node), root);
            }
        }
    }

    _cx_tnode_color((*root)) = CX_TNODE_BLACK;

    return;

}


inline static cx_tnode *
_cx_tree_rebalance_for_erase(cx_tnode *node, cx_tnode **root,
                             cx_tnode **leftmost, cx_tnode **rightmost)
{

    cx_tnode *y = node;
    cx_tnode *x = NULL;
    cx_tnode *x_parent = NULL;


    if (_cx_tnode_left(y) == NULL) {

        /*
         * node has at most one non-null child. y == node. x might be null.
         */

        x = _cx_tnode_right(y);
    }
    else {
        if (_cx_tnode_right(y) == NULL) {

            /*
             * node has exactly one non-null child. y == node. x is not null.
             */

            x = _cx_tnode_left(y);
        }
        else {

            /*
             * node has 2 non-null children. Set y to node's successor.
             * x might be null.
             */

            y = _cx_tnode_right(y);

            while (_cx_tnode_left(y) != NULL)
                y = _cx_tnode_left(y);

            x = _cx_tnode_right(y);
        }
    }

    if (y != node) {

        cx_tnode_color tcolor;

        /*
         * relink y in place of node. y is node's successor
         */

        _cx_tnode_parent(_cx_tnode_left(node)) = y;
        _cx_tnode_left(y) = _cx_tnode_left(node);

        if (y != _cx_tnode_right(node)) {
            x_parent = _cx_tnode_parent(y);

            if (x)
                _cx_tnode_parent(x) = _cx_tnode_parent(y);

            _cx_tnode_left(_cx_tnode_parent(y)) = x;
            _cx_tnode_right(y) = _cx_tnode_right(node);
            _cx_tnode_parent(_cx_tnode_right(node)) = y;
        }
        else
            x_parent = y;


        if (*root == node)
            *root = y;
        else
            if (_cx_tnode_left(_cx_tnode_parent(node)) == node)
                _cx_tnode_left(_cx_tnode_parent(node)) = y;
            else
                _cx_tnode_right(_cx_tnode_parent(node)) = y;

        _cx_tnode_parent(y) = _cx_tnode_parent(node);

        /*
         * Swap the colors of y an node.
         */

        tcolor = _cx_tnode_color(node);
        _cx_tnode_color(node) = _cx_tnode_color(y);
        _cx_tnode_color(y) = tcolor;

        /*
         * Make y point to the node to be actually deleted.
         */

        y = node;
    }
    else {

        /*
         * y == node
         */

        x_parent = _cx_tnode_parent(y);

        if (x)
            _cx_tnode_parent(x) = _cx_tnode_parent(y);

        if (*root == node)
            *root = x;
        else
            if (_cx_tnode_left(_cx_tnode_parent(node)) == node)
                _cx_tnode_left(_cx_tnode_parent(node)) = x;
            else
                _cx_tnode_right(_cx_tnode_parent(node)) = x;

        if (*leftmost == node) {
            if (_cx_tnode_right(node) == NULL) {

                /*
                 * If node == *root, *leftmost will be the header node.
                 */

                *leftmost = _cx_tnode_parent(node);
            }
            else
                *leftmost = _cx_tnode_minimum(x);
        }

        if (*rightmost == node) {
            if (_cx_tnode_left(node) == NULL) {

                /*
                 *  If node == *root, *rightmost will be the header node.
                 */

                *rightmost = _cx_tnode_parent(node);
            }
            else
                *rightmost = _cx_tnode_maximum(x);
        }
    }

    if (_cx_tnode_color(y) != CX_TNODE_RED) {
        while (x != *root &&
               (x == NULL || _cx_tnode_color(x) == CX_TNODE_BLACK))
            if (x == _cx_tnode_left(x_parent)) {
                cx_tnode *w = _cx_tnode_right(x_parent);

                if (_cx_tnode_color(w) == CX_TNODE_RED) {
                    _cx_tnode_color(w) = CX_TNODE_BLACK;
                    _cx_tnode_color(x_parent) = CX_TNODE_RED;
                    _cx_tree_rotate_left(x_parent, root);
                    w = _cx_tnode_right(x_parent);
                }

                if ((_cx_tnode_left(w) == NULL ||
                     _cx_tnode_color(_cx_tnode_left(w)) == CX_TNODE_BLACK) &&
                    (_cx_tnode_right(w) == NULL ||
                     _cx_tnode_color(_cx_tnode_right(w)) == CX_TNODE_BLACK)) {

                    _cx_tnode_color(w) = CX_TNODE_RED;
                    x = x_parent;
                    x_parent = _cx_tnode_parent(x_parent);

                }
                else {
                    if (_cx_tnode_right(w) == NULL ||
                        _cx_tnode_color(_cx_tnode_right(w)) ==
                        CX_TNODE_BLACK) {
                        if (_cx_tnode_left(w)) {
                            cx_tnode *v = _cx_tnode_left(w);
                            _cx_tnode_color(v) = CX_TNODE_BLACK;
                        }

                        _cx_tnode_color(w) = CX_TNODE_RED;
                        _cx_tree_rotate_right(w, root);
                        w = _cx_tnode_right(x_parent);
                    }

                    _cx_tnode_color(w) = _cx_tnode_color(x_parent);
                    _cx_tnode_color(x_parent) = CX_TNODE_BLACK;

                    if (_cx_tnode_right(w))
                        _cx_tnode_color(_cx_tnode_right(w)) = CX_TNODE_BLACK;

                    _cx_tree_rotate_left(x_parent, root);
                    break;
                }
            }
            else {

                /*
                 * Same as above with left and right exchanged.
                 */

                cx_tnode *w = _cx_tnode_left(x_parent);

                if (_cx_tnode_color(w) == CX_TNODE_RED) {
                    _cx_tnode_color(w) = CX_TNODE_BLACK;
                    _cx_tnode_color(x_parent) = CX_TNODE_RED;
                    _cx_tree_rotate_right(x_parent, root);
                    w = _cx_tnode_left(x_parent);
                }

                if ((_cx_tnode_right(w) == NULL ||
                     _cx_tnode_color(_cx_tnode_right(w)) == CX_TNODE_BLACK) &&
                    (_cx_tnode_left(w) == NULL ||
                     _cx_tnode_color(_cx_tnode_left(w)) == CX_TNODE_BLACK)) {

                    _cx_tnode_color(w) = CX_TNODE_RED;
                    x = x_parent;
                    x_parent = _cx_tnode_parent(x_parent);

                }
                else {
                    if (_cx_tnode_left(w) == NULL ||
                        _cx_tnode_color(_cx_tnode_left(w)) ==
                        CX_TNODE_BLACK) {
                        if (_cx_tnode_right(w)) {
                            cx_tnode *v = _cx_tnode_right(w);
                            _cx_tnode_color(v) = CX_TNODE_BLACK;
                        }

                        _cx_tnode_color(w) = CX_TNODE_RED;
                        _cx_tree_rotate_left(w, root);
                        w = _cx_tnode_left(x_parent);
                    }

                    _cx_tnode_color(w) = _cx_tnode_color(x_parent);
                    _cx_tnode_color(x_parent) = CX_TNODE_BLACK;

                    if (_cx_tnode_left(w))
                        _cx_tnode_color(_cx_tnode_left(w)) = CX_TNODE_BLACK;

                    _cx_tree_rotate_right(x_parent, root);
                    break;
                }
            }

        if (x)
            _cx_tnode_color(x) = CX_TNODE_BLACK;

    }

    return y;

}


/*
 * Convenience function used in cx_tree_verify()
 */

inline static cxsize
_cx_tree_black_count(cx_tnode *node, cx_tnode *root)
{
    cxsize sum = 0;

    if (node == NULL)
        return 0;

    do {
        if (_cx_tnode_color(node) == CX_TNODE_BLACK)
            ++sum;

        if (node == root)
            break;

        node = _cx_tnode_parent(node);
    } while (1);

    return sum;

}


/*
 * Initialize a tree to a valid empty tree.
 */

inline static void
_cx_tree_initialize(cx_tree *tree, cx_tree_compare_func compare,
                    cx_free_func key_destroy, cx_free_func value_destroy)
{

    /*
     * Used to distinguish header from root in the next operator.
     * Check this!!
     */

    _cx_tnode_color(_cx_tree_head(tree)) = CX_TNODE_RED;
    _cx_tree_root(tree) = NULL;
    _cx_tree_leftmost(tree) = _cx_tree_head(tree);
    _cx_tree_rightmost(tree) = _cx_tree_head(tree);

    _cx_tree_node_count(tree) = 0;
    _cx_tree_compare(tree) = compare;
    _cx_tree_key_destroy(tree) = key_destroy;
    _cx_tree_value_destroy(tree) = value_destroy;

    return;

}


/*
 * Inserting elements
 */

inline static cx_tnode *
_cx_tree_insert(cx_tree *tree, cx_tnode *x, cx_tnode *y, cxcptr key,
                cxcptr value)
{

    cx_tnode *z;


    if (y == _cx_tree_head(tree) || x != NULL ||
        _cx_tree_key_compare(tree, key, _cx_tnode_key(y))) {
        z = _cx_tnode_create(key, value);

        /*
         * This also makes _cx_tree_leftmost(tree) = z, if
         *  y == _cx_tree_head(tree).
         */

        _cx_tnode_left(y) = z;

        if (y == _cx_tree_head(tree)) {
            _cx_tree_root(tree) = z;
            _cx_tree_rightmost(tree) = z;
        }
        else {
            if (y == _cx_tree_leftmost(tree)) {

                /*
                 * Maintain _cx_tree_leftmost(tree) pointing to the
                 * minimum node.
                 */

                _cx_tree_leftmost(tree) = z;
            }
        }
    }
    else {
        z = _cx_tnode_create(key, value);
        _cx_tnode_right(y) = z;

        if (y == _cx_tree_rightmost(tree)) {

            /*
             * Maintain _cx_tree_rightmost(tree) pointing to the
             * maximum node.
             */

            _cx_tree_rightmost(tree) = z;
        }
    }

    _cx_tnode_parent(z) = y;
    _cx_tnode_left(z) = NULL;
    _cx_tnode_right(z) = NULL;

    _cx_tree_rebalance(z, &_cx_tree_root(tree));
    ++_cx_tree_node_count(tree);

    return z;

}


inline static cx_tnode *
_cx_tree_insert_equal(cx_tree *tree, cxcptr key, cxcptr value)
{

    cx_tnode *x = _cx_tree_root(tree);
    cx_tnode *y = _cx_tree_head(tree);


    while (x != NULL) {
        cxbool result;

        y = x;
        result = _cx_tree_key_compare(tree, key, _cx_tnode_key(x));
        x = result ? _cx_tnode_left(x) : _cx_tnode_right(x);
    }

    return _cx_tree_insert(tree, x, y, key, value);

}


inline static cx_tnode *
_cx_tree_insert_unique(cx_tree *tree, cxcptr key, cxcptr value)
{

    cx_tnode *x = _cx_tree_root(tree);
    cx_tnode *y = _cx_tree_head(tree);
    cx_tnode *pos;

    cxbool result = TRUE;


    while (x != NULL) {
        y = x;
        result = _cx_tree_key_compare(tree, key, _cx_tnode_key(x));
        x = result ? _cx_tnode_left(x) : _cx_tnode_right(x);
    }

    pos = y;

    if (result) {
        if (pos == _cx_tree_leftmost(tree))
            return _cx_tree_insert(tree, x, y, key, value);
        else
            pos = _cx_tnode_previous(pos);
    }

    if (_cx_tree_key_compare(tree, _cx_tnode_key(pos), key))
        return _cx_tree_insert(tree, x, y, key, value);

    return NULL;

}


/*
 * Element removal
 */

/*
 * Recursive erase without rebalancing
 */

inline static void
_cx_tree_erase_all(cx_tree *tree, cx_tnode *x)
{

    while (x != NULL) {
        cx_tnode *y;

        _cx_tree_erase_all(tree, _cx_tnode_right(x));
        y = _cx_tnode_left(x);
        _cx_tnode_destroy(x, _cx_tree_key_destroy(tree),
                          _cx_tree_value_destroy(tree));
        --_cx_tree_node_count(tree);
        x = y;
    }

    return;
}


inline static void
_cx_tree_erase(cx_tree *tree, cx_tnode *x)
{

   cx_tnode *y = _cx_tree_rebalance_for_erase(x, &_cx_tree_root(tree),
                                               &_cx_tree_leftmost(tree),
                                               &_cx_tree_rightmost(tree));

    _cx_tnode_destroy(y, _cx_tree_key_destroy(tree),
                      _cx_tree_value_destroy(tree));
    --_cx_tree_node_count(tree);

    return;

}


inline static void
_cx_tree_clear(cx_tree *tree)
{

    cx_assert(tree != NULL);

    if (_cx_tree_node_count(tree) != 0) {
        _cx_tree_erase_all(tree, _cx_tree_root(tree));
        _cx_tree_root(tree) = NULL;
        _cx_tree_leftmost(tree) = _cx_tree_head(tree);
        _cx_tree_rightmost(tree) = _cx_tree_head(tree);

        cx_assert(_cx_tree_node_count(tree) == 0);
    }

    return;

}


/*
 * Iteration
 */

inline static cx_tree_iterator
_cx_tree_begin(const cx_tree *tree)
{

    return _cx_tree_leftmost(tree);

}


inline static cx_tree_iterator
_cx_tree_end(const cx_tree *tree)
{

    return _cx_tree_head(tree);

}


/*
 * Basic search
 */

inline static cx_tnode *
_cx_tree_lower_bound(const cx_tree *tree, cxcptr key)
{

    cx_tnode *x = _cx_tree_root(tree);  /* Last node not less than key */
    cx_tnode *y = _cx_tree_head(tree);  /* Current node */


    while (x != NULL) {
        if (!_cx_tree_key_compare(tree, _cx_tnode_key(x), key)) {
            y = x;
            x = _cx_tnode_left(x);
        }
        else
            x = _cx_tnode_right(x);
    }

    return y;

}


inline static cx_tnode *
_cx_tree_upper_bound(const cx_tree *tree, cxcptr key)
{

    cx_tnode *x = _cx_tree_root(tree);  /* Last node greater than key */
    cx_tnode *y = _cx_tree_head(tree);  /* Current node */


    while (x != NULL) {
        if (_cx_tree_key_compare(tree, key, _cx_tnode_key(x))) {
            y = x;
            x = _cx_tnode_left(x);
        }
        else
            x = _cx_tnode_right(x);
    }

    return y;

}


inline static cx_tnode *
_cx_tree_find(const cx_tree *tree, cxcptr key)
{

    cx_tnode *x = _cx_tree_root(tree);
    cx_tnode *y = _cx_tree_head(tree);
    cx_tnode *pos;


    while (x != NULL) {
        if (!_cx_tree_key_compare(tree, _cx_tnode_key(x), key)) {
            y = x;
            x = _cx_tnode_left(x);
        }
        else
            x = _cx_tnode_right(x);
    }

    pos = y;

    if (pos == _cx_tree_head(tree) ||
        _cx_tree_key_compare(tree, key, _cx_tnode_key(pos)))
        return _cx_tree_head(tree);

    return pos;

}


inline static cxbool
_cx_tree_exists(const cx_tree *tree, cx_tnode *x)
{

    cx_tnode *y;
    cxptr key = _cx_tnode_key(x);


    y = _cx_tree_lower_bound(tree, key);

    if (y != x) {
        cx_tnode *z = _cx_tree_upper_bound(tree, key);

        y = _cx_tnode_next(y);
        while (y != x && y != z)
            y = _cx_tnode_next(y);
    }

    return x == y ? TRUE : FALSE;

}


/*
 * Public methods
 */

/**
 * @brief
 *   Get an iterator to the first pair in the tree.
 *
 * @param tree  The tree to query.
 *
 * @return Iterator for the first pair or @b cx_tree_end() if the tree is
 *   empty.
 *
 * The function returns a handle for the first pair in the tree @em tree.
 * The returned iterator cannot be used directly to access the value field
 * of the key/value pair, but only through the appropriate methods.
 */

cx_tree_iterator
cx_tree_begin(const cx_tree *tree)
{

    cx_assert(tree != NULL);

    return _cx_tree_begin(tree);

}


/**
 * @brief
 *   Get an iterator for the position after the last pair in the tree.
 *
 * @param tree  The tree to query.
 *
 * @return Iterator for the end of the tree.
 *
 * The function returns an iterator for the position one past the last pair
 * in the tree @em tree. The iteration is done in ascending order according
 * to the keys. The returned iterator cannot be used directly to access the
 * value field of the key/value pair, but only through the appropriate
 * methods.
 */

cx_tree_iterator
cx_tree_end(const cx_tree *tree)
{

    cx_assert(tree != NULL);

    return _cx_tree_end(tree);

}


/**
 * @brief
 *   Get an iterator for the next pair in the tree.
 *
 * @param tree      A tree.
 * @param position  Current iterator position.
 *
 * @return Iterator for the pair immediately following @em position.
 *
 * The function returns an iterator for the next pair in the tree @em tree
 * with respect to the current iterator position @em position. Iteration
 * is done in ascending order according to the keys. If the tree is empty
 * or @em position points to the end of the tree the function returns
 * @b cx_tree_end().
 */

cx_tree_iterator
cx_tree_next(const cx_tree *tree, cx_tree_const_iterator position)
{

    cx_assert(tree != NULL);
    cx_assert(position != NULL);
    cx_assert(position == _cx_tree_end(tree) ||
              _cx_tree_exists(tree, (cx_tree_iterator)position));

    if (position == _cx_tree_end(tree)) {
        return _cx_tree_end(tree);
    }

    return _cx_tnode_next(position);

}


/**
 * @brief
 *   Get an iterator for the previous pair in the tree.
 *
 * @param tree      A tree.
 * @param position  Current iterator position.
 *
 * @return Iterator for the pair immediately preceding @em position.
 *
 * The function returns an iterator for the previous pair in the tree
 * @em tree with respect to the current iterator position @em position.
 * Iteration is done in ascending order according to the keys. If the
 * tree is empty or @em position points to the beginning of the tree the
 * function returns @b cx_tree_end().
 */

cx_tree_iterator
cx_tree_previous(const cx_tree *tree, cx_tree_const_iterator position)
{

    cx_assert(tree != NULL);
    cx_assert(position != NULL);
    cx_assert(position == _cx_tree_end(tree) ||
              _cx_tree_exists(tree, (cx_tree_iterator)position));

    if (position == _cx_tree_begin(tree)) {
        return _cx_tree_begin(tree);
    }

    return _cx_tnode_previous(position);

}


/**
 * @brief
 *   Remove all pairs from a tree.
 *
 * @param tree  Tree to be cleared.
 *
 * @return Nothing.
 *
 * The tree @em tree is cleared, i.e. all pairs are removed from the tree.
 * Keys and values are destroyed using the key and value destructors set up
 * during tree creation. After calling this function the tree is empty.
 */

void
cx_tree_clear(cx_tree *tree)
{

    cx_assert(tree != NULL);
    _cx_tree_clear(tree);

    return;

}


/**
 * @brief
 *   Check whether a tree is empty.
 *
 * @param tree  A tree.
 *
 * @return The function returns @c TRUE if the tree is empty, and @c FALSE
 *   otherwise.
 *
 * The function checks if the tree contains any pairs. Calling this function
 * is equivalent to the statement:
 * @code
 *   return (cx_tree_size(tree) == 0);
 * @endcode
 */

cxbool
cx_tree_empty(const cx_tree *tree)
{

    return (_cx_tree_node_count(tree) == 0);

}


/**
 * @brief
 *   Create a new tree without any elements.
 *
 * @param  compare        Function used to compare keys.
 * @param  key_destroy    Destructor for the keys.
 * @param  value_destroy  Destructor for the value field.
 *
 * @return Handle for the newly allocated tree.
 *
 * Memory for a new tree is allocated and the tree is initialized to be a
 * valid empty tree.
 *
 * The tree's key comparison function is set to @em compare. It must
 * return @c TRUE or @c FALSE if the comparison of the first argument
 * passed to it with the second argument is found to be true or false
 * respectively.
 *
 * The destructors for a tree node's key and value field are set to
 * @em key_destroy and @em value_destroy. Whenever a tree node is
 * destroyed these functions are used to deallocate the memory used
 * by the key and the value. Each of the destructors might be @c NULL, i.e.
 * keys and values are not deallocated during destroy operations.
 *
 * @see cx_tree_compare_func()
 */

cx_tree *
cx_tree_new(cx_tree_compare_func compare, cx_free_func key_destroy,
            cx_free_func value_destroy)
{

    cx_tree *tree;


    cx_assert(compare != NULL);

    tree = cx_malloc(sizeof *tree);
    _cx_tree_head(tree) = cx_malloc(sizeof(cx_tnode));

    _cx_tree_initialize(tree, compare, key_destroy, value_destroy);

    return tree;

}


/**
 * @brief
 *   Destroy a tree and all its elements.
 *
 * @param tree  The tree to destroy.
 *
 * @return Nothing.
 *
 * The tree @em tree is deallocated. All data values and keys are
 * deallocated using the tree's key and value destructor. If no
 * key and/or value destructor was set when the @em tree was created
 * the keys and the stored data values are left untouched. In this
 * case the key and value deallocation is the responsibility of the
 * user.
 *
 * @see cx_tree_new()
 */

void
cx_tree_delete(cx_tree *tree)
{

    if (tree) {
        _cx_tree_clear(tree);
        cx_free(_cx_tree_head(tree));
        cx_free(tree);
    }

    return;

}


/**
 * @brief
 *   Get the actual number of pairs in the tree.
 *
 * @param tree  A tree.
 *
 * @return The current number of pairs, or 0 if the tree is empty.
 *
 * Retrieves the current number of pairs stored in the tree.
 */

cxsize
cx_tree_size(const cx_tree *tree)
{

    cx_assert(tree != NULL);

    return _cx_tree_node_count(tree);

}


/**
 * @brief
 *   Get the maximum number of pairs possible.
 *
 * @param tree  A tree.
 *
 * @return The maximum number of pairs that can be stored in the tree.
 *
 * Retrieves the tree's capacity, i.e. the maximum possible number of
 * pairs a tree can manage.
 */

cxsize
cx_tree_max_size(const cx_tree *tree)
{

    (void) tree;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(tree != NULL);

    return (cxsize)(-1);

}


/**
 * @brief
 *   Get the key comparison function.
 *
 * @param tree  The tree to query.
 *
 * @return Handle for the tree's key comparison function.
 *
 * The function retrieves the function used by the tree methods
 * for comparing keys. The key comparison function is set during
 * tree creation.
 *
 * @see cx_tree_new()
 */

cx_tree_compare_func
cx_tree_key_comp(const cx_tree *tree)
{

    cx_assert(tree != NULL);

    return _cx_tree_compare(tree);

}


/**
 * @brief
 *   Swap the contents of two trees.
 *
 * @param tree1  First tree.
 * @param tree2  Second tree.
 *
 * @return Nothing.
 *
 * All pairs stored in the first tree @em tree1 are moved to the second tree
 * @em tree2, while the pairs from @em tree2 are moved to @em tree1. Also
 * the key comparison function, the key and the value destructor are
 * exchanged.
 */

void
cx_tree_swap(cx_tree *tree1, cx_tree *tree2)
{

    cx_tnode *tmp;
    cxsize sz;
    cx_tree_compare_func cmp;
    cx_free_func destroy;


    cx_assert(tree1 != NULL);
    cx_assert(tree2 != NULL);

    tmp = _cx_tree_head(tree2);
    _cx_tree_head(tree2) = _cx_tree_head(tree1);
    _cx_tree_head(tree1) = tmp;

    sz = _cx_tree_node_count(tree2);
    _cx_tree_node_count(tree2) = _cx_tree_node_count(tree1);
    _cx_tree_node_count(tree1) = sz;

    cmp = _cx_tree_compare(tree2);
    _cx_tree_compare(tree2) = _cx_tree_compare(tree1);
    _cx_tree_compare(tree1) = cmp;

    destroy = _cx_tree_key_destroy(tree2);
    _cx_tree_key_destroy(tree2) = _cx_tree_key_destroy(tree1);
    _cx_tree_key_destroy(tree1) = destroy;

    destroy = _cx_tree_value_destroy(tree2);
    _cx_tree_value_destroy(tree2) = _cx_tree_value_destroy(tree1);
    _cx_tree_value_destroy(tree1) = destroy;

    return;

}


/**
 * @brief
 *   Assign data to an iterator position.
 *
 * @param tree      A tree.
 * @param position  Iterator positions where the data will be stored.
 * @param data      Data to store.
 *
 * @return Handle to the previously stored data object.
 *
 * The function assigns a data object reference @em data to the iterator
 * position @em position of the tree @em tree.
 */

cxptr
cx_tree_assign(cx_tree *tree, cx_tree_iterator position, cxcptr data)
{

    cxptr tmp;


    (void) tree;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(tree != NULL);
    cx_assert(position != NULL);
    cx_assert(_cx_tree_exists(tree, position));

    tmp = _cx_tnode_get_value(position);
    _cx_tnode_set_value(position, data);

    return tmp;

}


/**
 * @brief
 *   Get the key from a given iterator position.
 *
 * @param tree      A tree.
 * @param position  Iterator position the data is retrieved from.
 *
 * @return Reference for the key.
 *
 * The function returns a reference to the key associated with the iterator
 * position @em position in the tree @em tree.
 *
 * @note
 *   One must not modify the key of @em position through the returned
 *   reference, since this might corrupt the tree!
 */

cxptr
cx_tree_get_key(const cx_tree *tree, cx_tree_const_iterator position)
{

    (void) tree;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(tree != NULL);
    cx_assert(position != NULL);
    cx_assert(_cx_tree_exists(tree, (cx_tree_iterator)position));

    return _cx_tnode_get_key(position);

}


/**
 * @brief
 *   Get the data from a given iterator position.
 *
 * @param tree      A tree.
 * @param position  Iterator position the data is retrieved from.
 *
 * @return Handle for the data object.
 *
 * The function returns a reference to the data stored at iterator position
 * @em position in the tree @em tree.
 */

cxptr
cx_tree_get_value(const cx_tree *tree, cx_tree_const_iterator position)
{

    (void) tree;  /* Prevent warnings if cx_assert is disabled. */
    cx_assert(tree != NULL);
    cx_assert(position != NULL);
    cx_assert(_cx_tree_exists(tree, (cx_tree_iterator)position));

    return _cx_tnode_get_value(position);

}


/**
 * @brief
 *   Locate an element in the tree.
 *
 * @param tree  A tree.
 * @param key   Key of the (key, value) pair to locate.
 *
 * @return Iterator pointing to the sought-after element, or @b cx_tree_end()
 *   if it was not found.
 *
 * The function searches the tree @em tree for an element with a key
 * matching @em key. If the search was successful an iterator to the
 * sought-after pair is returned. If the search did not succeed, i.e.
 * @em key is not present in the tree, a one past the end iterator is
 * returned.
 */

cx_tree_iterator
cx_tree_find(const cx_tree *tree, cxcptr key)
{

    cx_assert(tree != NULL);
    cx_assert(key != NULL);

    return _cx_tree_find(tree, key);

}


/**
 * @brief
 *   Find the beginning of a subsequence.
 *
 * @param tree  A tree.
 * @param key   Key of the (key, value) pair(s) to locate.
 *
 * @return Iterator pointing to the first position where an element with
 *   key @em key would get inserted, i.e. the first element with a key greater
 *   or equal than @em key.
 *
 * The function returns the first element of a subsequence of elements in the
 * tree that match the given key @em key. If @em key is not present in the
 * tree @em tree an iterator pointing to the first element that has a greater
 * key than @em key or @b cx_tree_end() if no such element exists.
 */

cx_tree_iterator
cx_tree_lower_bound(const cx_tree *tree, cxcptr key)
{

    cx_assert(tree != NULL);
    cx_assert(key != NULL);

    return _cx_tree_lower_bound(tree, key);

}


/**
 * @brief
 *   Find the end of a subsequence.
 *
 * @param tree  A tree.
 * @param key   Key of the (key, value) pair(s) to locate.
 *
 * @return Iterator pointing to the last position where an element with
 *   key @em key would get inserted, i.e. the first element with a key
 *   greater than @em key.
 *
 * The function returns the last element of a subsequence of elements in the
 * tree that match the given key @em key. If @em key is not present in the
 * tree @em tree an iterator pointing to the first element that has a greater
 * key than @em key or @b cx_tree_end() if no such element exists.
 */

cx_tree_iterator
cx_tree_upper_bound(const cx_tree *tree, cxcptr key)
{

    cx_assert(tree != NULL);
    cx_assert(key != NULL);

    return _cx_tree_upper_bound(tree, key);
}


/**
 * @brief
 *   Find a subsequence matching a given key.
 *
 * @param tree   A tree.
 * @param key    The key of the (key, value) pair(s) to be located.
 * @param begin  First element with key @em key.
 * @param end    Last element with key @em key.
 *
 * @return Nothing.
 *
 * The function returns the beginning and the end of a subsequence of
 * tree elements with the key @em key through through the @em begin and
 * @em end arguments. After calling this function @em begin possibly points
 * to the first element of @em tree matching the key @em key and @em end
 * possibly points to the last element of the sequence. If key is not
 * present in the tree @em begin points to the next greater element or,
 * if no such element exists, to @b cx_tree_end().
 */

void
cx_tree_equal_range(const cx_tree *tree, cxcptr key,
                    cx_tree_iterator *begin, cx_tree_iterator *end)
{

    cx_assert(tree != NULL);
    cx_assert(key != NULL);

    *begin = _cx_tree_lower_bound(tree, key);
    *end = _cx_tree_upper_bound(tree, key);

    return;

}


/**
 * @brief
 *   Get the number of elements matching a key.
 *
 * @param tree  A tree.
 * @param key   Key of the (key, value) pair(s) to locate.
 *
 * @return The number of elements with the specified key.
 *
 * Counts all elements of the tree @em tree matching the key @em key.
 */

cxsize
cx_tree_count(const cx_tree *tree, cxcptr key)
{

    cx_tnode *x, *y;
    cxsize count = 0;


    cx_assert(tree != NULL);
    cx_assert(key != NULL);

    x = _cx_tree_lower_bound(tree, key);
    y = _cx_tree_upper_bound(tree, key);

    /*
     * Only if key is not present in the tree x and y are identical,
     * pointing to the element with the next greater key.
     */

    while (x != y) {
        ++count;
        x = _cx_tnode_next(x);
    }

    return count;

}


/**
 * @brief
 *   Attempt to insert data into a tree.
 *
 * @param tree  A tree.
 * @param key   Key used to store the data.
 * @param data  Data to insert.
 *
 * @return An iterator that points to the inserted pair, or @c NULL if the
 *   pair could not be inserted.
 *
 * This function attempts to insert a (key, value) pair into the tree
 * @em tree. The insertion fails if the key already present in the tree,
 * i.e. if the key is not unique.
 */

cx_tree_iterator
cx_tree_insert_unique(cx_tree *tree, cxcptr key, cxcptr data)
{

    cx_assert(tree != NULL);
    cx_assert(key != NULL);

    return _cx_tree_insert_unique(tree, key, data);

}


/**
 * @brief
 *   Insert data into a tree.
 *
 * @param tree  A tree.
 * @param key   Key used to store the data.
 * @param data  Data to insert.
 *
 * @return An iterator that points to the inserted pair.
 *
 * This function inserts a (key, value) pair into the tree @em tree.
 * Contrary to @b cx_tree_insert_unique() the key @em key used for inserting
 * @em data may already be present in the tree.
 */

cx_tree_iterator
cx_tree_insert_equal(cx_tree *tree, cxcptr key, cxcptr data)
{

    cx_assert(tree != NULL);
    cx_assert(key != NULL);

    return _cx_tree_insert_equal(tree, key, data);

}


/**
 * @brief
 *   Erase an element from a tree.
 *
 * @param tree      A tree.
 * @param position  Iterator position of the element to be erased.
 *
 * @return Nothing.
 *
 * This function erases an element, specified by the iterator @em position,
 * from @em tree. Key and value associated with the erased pair are
 * deallocated using the tree's key and value destructors, provided
 * they have been set.
 */

void
cx_tree_erase_position(cx_tree *tree, cx_tree_iterator position)
{

    cx_assert(tree != NULL);

    if (!position || _cx_tree_node_count(tree) == 0) {
        return;
    }

    cx_assert(_cx_tree_exists(tree, position));
    _cx_tree_erase(tree, position);

    return;

}


/**
 * @brief
 *   Erase a range of elements from a tree.
 *
 * @param tree   A tree.
 * @param begin  Iterator pointing to the start of the range to erase.
 * @param end    Iterator pointing to the end of the range to erase.
 *
 * @return Nothing.
 *
 * This function erases all elements in the range [begin, end) from
 * the tree @em tree. Key and value associated with the erased pair(s) are
 * deallocated using the tree's key and value destructors, provided
 * they have been set.
 */

void
cx_tree_erase_range(cx_tree *tree, cx_tree_iterator begin,
                    cx_tree_iterator end)
{

    cx_assert(tree != NULL);
    cx_assert(begin == _cx_tree_head(tree) || _cx_tree_exists(tree, begin));
    cx_assert(end == _cx_tree_head(tree) || _cx_tree_exists(tree, end));

    while (begin != end) {
        cx_tnode *pos = begin;

        begin = _cx_tnode_next(pos);
        _cx_tree_erase(tree, pos);
    }

    return;

}


/**
 * @brief
 *   Erase all elements from a tree matching the provided key.
 *
 * @param tree  A tree.
 * @param key   Key of the element to be erased.
 *
 * @return The number of removed elements.
 *
 * This function erases all elements with the specified key @em key,
 * from @em tree. Key and value associated with the erased pairs are
 * deallocated using the tree's key and value destructors, provided
 * they have been set.
 */

cxsize
cx_tree_erase(cx_tree *tree, cxcptr key)
{

    cx_tnode *x, *y;
    cxsize count = 0;


    cx_assert(tree != NULL);
    cx_assert(key != NULL);

    x = _cx_tree_lower_bound(tree, key);
    y = _cx_tree_upper_bound(tree, key);

    while (x != y) {
        cx_tnode *pos = x;

        x = _cx_tnode_next(x);
        _cx_tree_erase(tree, pos);

        ++count;
    }

    return count;

}


/**
 * @brief
 *   Validate a tree.
 *
 * @param tree  The tree to verify.
 *
 * @return Returns @c TRUE if the tree is valid, or @c FALSE otherwise.
 *
 * The function is provided for debugging purposes. It verifies that
 * the internal tree structure of @em tree is valid.
 */

cxbool
cx_tree_verify(const cx_tree *tree)
{

    cx_tnode *it;
    cxsize len = 0;


    cx_assert(tree != NULL);

    if (_cx_tree_node_count(tree) == 0 ||
        _cx_tree_begin(tree) == _cx_tree_end(tree)) {
        if (_cx_tree_node_count(tree) == 0 &&
            _cx_tree_begin(tree) == _cx_tree_end(tree) &&
            _cx_tnode_left(_cx_tree_head(tree)) == _cx_tree_head(tree) &&
            _cx_tnode_right(_cx_tree_head(tree)) == _cx_tree_head(tree))
            return TRUE;
        else
            return FALSE;
    }

    len = _cx_tree_black_count(_cx_tree_leftmost(tree), _cx_tree_root(tree));

    for (it = _cx_tree_begin(tree); it != _cx_tree_end(tree);
         it = _cx_tnode_next(it)) {

        cx_tnode *x = it;
        cx_tnode *L = _cx_tnode_left(x);
        cx_tnode *R = _cx_tnode_right(x);

        if (_cx_tnode_color(x) == CX_TNODE_RED)
            if ((L && _cx_tnode_color(L) == CX_TNODE_RED) ||
                (R && _cx_tnode_color(R) == CX_TNODE_RED))
                return FALSE;

        if (L && _cx_tree_key_compare(tree, _cx_tnode_key(x),
                                      _cx_tnode_key(L)))
            return FALSE;

        if (R && _cx_tree_key_compare(tree, _cx_tnode_key(R),
                                      _cx_tnode_key(x)))
            return FALSE;

        if (!L && !R && _cx_tree_black_count(x, _cx_tree_root(tree)) != len)
            return FALSE;

    }

    if (_cx_tree_leftmost(tree) != _cx_tnode_minimum(_cx_tree_root(tree)))
        return FALSE;

    if (_cx_tree_rightmost(tree) != _cx_tnode_maximum(_cx_tree_root(tree)))
        return FALSE;

    return TRUE;

}
/**@}*/
