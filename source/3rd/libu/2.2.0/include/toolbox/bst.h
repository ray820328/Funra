/*
 * Copyright (c) 2005-2012 by KoanLogic s.r.l. - All rights reserved.
 */

#ifndef _U_BST_H_
#define _U_BST_H_

#include <u/libu_conf.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

/* forward decls */ 
struct u_bst_s;
struct u_bst_node_s;

/**
 *  \addtogroup bst
 *  \{
 */ 

/** \brief  The binary search tree type. */
typedef struct u_bst_s u_bst_t;

/** \brief  The binary search tree node type. */
typedef struct u_bst_node_s u_bst_node_t;


/** \brief  Available types for BST containers. */
typedef enum {
    U_BST_TYPE_STRING,  /**< String, i.e. NUL-terminated char array. */
    U_BST_TYPE_OPAQUE,  /**< Any type, given by its byte size.  */
    U_BST_TYPE_PTR      /**< Pointer type. */
} u_bst_type_t;

/** \brief BST rotation types */
typedef enum {
    U_BST_ROT_LEFT,     /**< Left rotation. */
    U_BST_ROT_RIGHT     /**< Right rotation. */
} u_bst_rot_t;

/** \brief  BST customization options. */
typedef enum {
    U_BST_OPT_NONE          = 0,
    U_BST_OPT_PUSH_TOP      = (1 << 0),   /**< New nodes added on BST top. */
    U_BST_OPT_PUSH_BOTTOM   = (1 << 1),   /**< New nodes added at BST bottom. */
    U_BST_OPT_RANDOMIZED    = (1 << 2)    /**< Operations are randomized. */
} u_bst_opt_t;

/* Base interface. */
int u_bst_new (int opts, u_bst_t **pbst);
void u_bst_free (u_bst_t *bst);
ssize_t u_bst_count (u_bst_t *bst);
u_bst_node_t *u_bst_search (u_bst_t *bst, const void *key);
int u_bst_push (u_bst_t *bst, const void *key, const void *val);
int u_bst_foreach (u_bst_t *bst, void (*cb)(u_bst_node_t *, void *), 
        void *cb_args);
u_bst_node_t *u_bst_rotate (u_bst_node_t *node, u_bst_rot_t dir);
int u_bst_empty (u_bst_t *bst);
u_bst_node_t *u_bst_find_nth (u_bst_t *bst, size_t n);
int u_bst_delete (u_bst_t *bst, const void *key);
int u_bst_balance (u_bst_t *bst);

/* BST configuration. */
int u_bst_set_cmp (u_bst_t *bst, int (*f)(const void *, const void *));
int u_bst_set_keyattr (u_bst_t *bst, u_bst_type_t kt, size_t ks);
int u_bst_set_valattr (u_bst_t *bst, u_bst_type_t vt, size_t vs);
int u_bst_set_keyfree (u_bst_t *bst, void (*f)(void *));
int u_bst_set_valfree (u_bst_t *bst, void (*f)(void *));

/* Getters/Setters. */
const void *u_bst_node_key (u_bst_node_t *node);
const void *u_bst_node_val (u_bst_node_t *node);
ssize_t u_bst_node_count (u_bst_node_t *node);

/**
 *  \}
 */

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif  /* !_U_BST_H_ */
