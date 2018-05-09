/*
 * Created 18A30
 */

#include <errno.h>
#include <string.h>
#include "rbtree.h"

#define __UNUSED __attribute__ ((unused))

static inline ptrdiff_t _rb_raw_data_cmp(void *a, void *b)   { return a - b; }
static inline void _rb_raw_data_free(void *a __UNUSED) { /* NOP */ }

/**
 * Allocate a rbtree
 * @cmp_f   The compare function pointer(NULL for raw comparison)
 * @free_f  The free function for data(NULL for no operation)
 */
struct rb_root *rb_alloc(cmp_func_t cmp_f, free_func_t free_f)
{
    struct rb_root *r = __MALLOC(sizeof(*r));
    if (r == NULL)
        return NULL;

    r->root = NULL;
    r->size = 0;
    r->cmp = cmp_f ? cmp_f : _rb_raw_data_cmp;
    r->free = free_f ? free_f : _rb_raw_data_free;

    return r;
}

/**
 * Core utility for rbtree destruction
 * @t           rbtree reference
 * @fully       if free the rbtree fully(e.g.  if free the root)
 *
 * NOTE: behaviour of free a NULL rbtree is undefined
 */
static void rb_free_core(struct rb_root **t, int fully)
{
    struct rb_root *tree;
    struct rb_node *iter, *save;
    /* Those two will be eliminated eventually */
    size_t size0, size1;

    __ASSERT(t != NULL && *t != NULL);

    tree = *t;
    iter = tree->root;
    save = NULL;
    size0 = tree->size;
    size1 = 0;

    /*
     * Rotate away the left links into a linked list so that
     *  we can perform iterative destruction of the rbtree
     */
    while (iter != NULL) {
        if (iter->link[0] == NULL) {
            save = iter->link[1];
            tree->free(iter->data);
            __FREE(iter);
            tree->size--;
            size1++;
        } else {
            save = iter->link[0];
            iter->link[0] = save->link[1];
            save->link[1] = iter;
        }
        iter = save;
    }

    __ASSERT(size0 == size1);
    __ASSERT(tree->size == 0);

    tree->root = NULL;
    if (fully) {
        __FREE(tree);
        *t = NULL;
    }
}

void rb_free(struct rb_root **t)
{
    rb_free_core(t, 1);
}

void rb_clear(struct rb_root **t)
{
    rb_free_core(t, 0);
}

size_t rb_size(const struct rb_root *t)
{
    __ASSERT(t != NULL);
    if (t->root == NULL)
        __ASSERT(t->size == 0);
    return t->size;
}

/*
 * Do NOT modify this on-the-fly  it'll cause segment fault
 * see: uuidgen(1) or uuidgenerator.net
 */
void *rb_not_found = (void *) "fd562228-5e9c-4e3d-9aec-314485cc705d";

/**
 * Check if a specific item in rbtree
 * @return      associated data if found  rb_not_found o.w.
 *
 * NOTE: we allow NULL to be checked
 *
 *  this function exposed data node  which rbtree use it for balancing
 *  you shouldn't modify anything used in comparator  which may debalance
 *  the whole rbtree(its behaviour is thus undefined)
 *
 *  you may happen to modify some auxiliary data fields(not used in comparator)
 *  this is a suboptimal solution to KV map for small data set
 */
void *rb_get_unsafe(const struct rb_root *t, void *data)
{
    struct rb_node *it;
    ptrdiff_t dir;

    __ASSERT(t != NULL);

    it = t->root;
    while (it != NULL) {
        if ((dir = t->cmp(data, it->data)) == 0)
            return it->data;
        it = it->link[dir > 0];
    }

    return rb_not_found;
}

int rb_find(const struct rb_root *t, void *data)
{
    return rb_get_unsafe(t, data) != rb_not_found;
}

static struct rb_node *make_node(void *data)
{
    struct rb_node *n = __MALLOC(sizeof(*n));
    if (n == NULL)
        return NULL;

    n->link[0] = n->link[1] = NULL;
    n->red = 1;
    n->data = data;

    return n;
}

#define IS_RED(n) ((n) != NULL && (n)->red == 1)

static struct rb_node *rot_once(struct rb_node *root, int dir)
{
    struct rb_node *save;
    __ASSERT(root != NULL);

    save = root->link[!dir];
    root->link[!dir] = save->link[dir];
    save->link[dir] = root;

    root->red = 1;
    save->red = 0;

    return save;
}

static struct rb_node *rot_twice(struct rb_node *root, int dir)
{
    __ASSERT(root != NULL);
    root->link[!dir] = rot_once(root->link[!dir], !dir);
    return rot_once(root, dir);
}

/**
 * Insert a item into rb tree
 * @return      0 if success  errno o.w.
 *              ENOMEM if oom
 *              EEXIST if already exists(duplicates disallow)
 *
 * XXX  Undefined behaviour if insert rb_not_found
 */
int rb_insert(struct rb_root *tree, void *data)
{
    int ins = 0;
    __ASSERT(tree != NULL);

    if (tree->root == NULL) {
        __ASSERT(tree->size == 0);
        tree->root = make_node(data);
        if (tree->root == NULL)
            return ENOMEM;
        ins = 1;
        goto out_exit;
    }

    struct rb_node head = { .red = 0 };     /* Fake tree root */
    struct rb_node *g, *t;                  /* Grandparent & parent */
    struct rb_node *p, *q;                  /* Iterator & parent */
    int dir = 0, last = 0;                  /* Directions */

    t = &head;
    g = p = NULL;
    q = t->link[1] = tree->root;

    while (1) {
        if (q == NULL) {
            /* Insert a node at first null link(also set its parent link) */
            p->link[dir] = q = make_node(data);
            if (q == NULL)
                return ENOMEM;
            ins = 1;
        } else if (IS_RED(q->link[0]) && IS_RED(q->link[1])) {
            /* Simple red violation: color flip */
            q->red = 1;
            q->link[0]->red = 0;
            q->link[1]->red = 0;
        }

        if (IS_RED(q) && IS_RED(p)) {
            /* Hard red violation: rotate */
            __ASSERT(t != NULL);
            int dir2 = t->link[1] == g;
            if (q == p->link[last])
                t->link[dir2] = rot_once(g, !last);
            else
                t->link[dir2] = rot_twice(g, !last);
        }

        if (ins == 1)
            break;

        last = dir;
        dir = tree->cmp(data, q->data);
        if (dir == 0)
            break;

        dir = dir > 0;

        if (g != NULL)
            t = g;

        g = p;
        p = q;
        q = q->link[dir];
    }

    /* Update root(it may different due to root rotation) */
    tree->root = head.link[1];

out_exit:
    /* Invariant: root is black */
    tree->root->red = 0;
    if (ins == 1)
        tree->size++;

    return ins == 1 ? 0 : EEXIST;
}

/**
 * Delete a item from rbtree
 * @return          0 if success  errno o.w.
 *                  ENOENT if no such item
 */
int rb_remove(struct rb_root *t,void *data)
{
    __ASSERT(t != NULL);

    if (t->root == NULL) {
        __ASSERT(t->size == 0);
        return ENOENT;
    }

    struct rb_node head = { .red = 0 };
    struct rb_node *q, *p, *g;
    struct rb_node *f = NULL;
    int dir = 1, last;

    q = &head;
    g = p = NULL;
    q->link[1] = t->root;

    /* Find in-order predecessor */
    while (q->link[dir] != NULL) {
        last = dir;

        g = p;
        p = q;
        q = q->link[dir];

        dir = t->cmp(data, q->data);
        if (dir == 0)
            f = q;

        dir = dir > 0;

        if (!IS_RED(q) && !IS_RED(q->link[dir])) {
            if (IS_RED(q->link[!dir])) {
                p = p->link[last] = rot_once(q, dir);
            } else {
                struct rb_node *s = p->link[!last];

                if (s != NULL) {
                    if (!IS_RED(s->link[!last]) && !IS_RED(s->link[last])) {
                        /* Color flip */
                        p->red = 0;
                        s->red = 1;
                        q->red = 1;
                    } else {
                        int dir2 = g->link[1] == p;

                        if (IS_RED(s->link[last]))
                            g->link[dir2] = rot_twice(p, last);
                        else
                            g->link[dir2] = rot_once(p, last);

                        /* Ensure correct coloring */
                        q->red = g->link[dir2]->red = 1;
                        g->link[dir2]->link[0]->red = 0;
                        g->link[dir2]->link[1]->red = 0;
                    }
                }
            }
        }
    }

    /* Replace and remove if found */
    if (f != NULL) {
        f->data = q->data;
        p->link[p->link[1] == q] = q->link[q->link[0] == NULL];
        t->free(q->data);
        __FREE(q);
        t->size--;
    }

    /* Update root node */
    t->root = head.link[1];
    if (t->root != NULL)
        t->root->red = 0;
    else
        __ASSERT(t->size == 0);

    return f != NULL ? 0 : ENOENT;
}

static uint32_t rb_assert_recur(const struct rb_root *t, struct rb_node *root)
{
    /* NULL leaf node is black(should count) */
    if (root == NULL)
        return 1;

    struct rb_node *l = root->link[0];
    struct rb_node *r = root->link[1];

    if (IS_RED(root))
        __ASSERT(!IS_RED(l) && !IS_RED(r));

    if (l != NULL)
        __ASSERT(t->cmp(l->data, root->data) < 0);

    if (r != NULL)
        __ASSERT(t->cmp(r->data, root->data) > 0);

    uint32_t lh = rb_assert_recur(t, l);
    uint32_t rh = rb_assert_recur(t, r);

    /*
     * Every path from a given node to its NULL nodes
     *  contains same number of black nodes
     */
    __ASSERT(lh == rh);
    return IS_RED(root) ? lh : lh + 1;
}

/**
 * Assert the rbtree(mainly for debug)
 */
void rb_assert(const struct rb_root *t)
{
    __ASSERT(t != NULL);
    if (t->root == NULL) {
        __ASSERT(t->size == 0);
        return;
    }
    rb_assert_recur(t, t->root);
}

static void rb_show_recur(const struct rb_node *n)
{
out_again:
    if (n == NULL) return;

    if (n->link[0])
        rb_show_recur(n->link[0]);

    printf("%lu ", (uintptr_t) n->data);
    n = n->link[1];
    goto out_again;
}

/**
 * Simple tree display(debugging)
 */
void rb_show(const struct rb_root *t)
{
    __ASSERT(t != NULL);
    __ASSERT(t->root ? t->size != 0 : t->size == 0);

    printf("rbtree %#lx  size: %zu\n", (uintptr_t) t, t->size);
    rb_show_recur(t->root);
    printf("\n");
}

