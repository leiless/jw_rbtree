/*
 * Created 18A30
 *
 * Red-black tree implementation based on Julienne Walker's solution
 *
 * TRYME: using BSD internal rbtree implementation in <libkern/tree.h>
 *          instead of this crappy shit
 *
 * see:
 *  eternallyconfuzzled.com/tuts/datastructures/jsw_tut_rbtree.aspx
 *  github.com/mirek/rb_tree
 *  github.com/sebhub/rb-bench
 *  en.wikipedia.org/wiki/Red–black_tree
 */

#ifndef RBTREE_H
#define RBTREE_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>

#define __MALLOC        malloc
#define __FREE          free
#define __ASSERT        assert
#define PCONV(x)        (void *) ((uintptr_t) (x))

struct rb_node {
    struct rb_node *link[2];
    uint32_t red;
    void * data;
};

typedef ptrdiff_t (*cmp_func_t)(void *, void *);
typedef void (*free_func_t)(void *);

struct rb_root {
    struct rb_node *root;
    size_t size;
    cmp_func_t cmp;
    free_func_t free;
};

struct rb_root *rb_alloc(cmp_func_t, free_func_t);
void rb_free(struct rb_root **);
void rb_clear(struct rb_root **);
size_t rb_size(const struct rb_root *);
void *rb_get_unsafe(const struct rb_root *, void *);
int rb_find(const struct rb_root *, void *);
int rb_insert(struct rb_root *, void *);
int rb_remove(struct rb_root *, void *);
void rb_assert(const struct rb_root *);
void rb_show(const struct rb_root *);

#endif  /* RBTREE_H */

