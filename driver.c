#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include "rbtree.h"

#define DBG(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

static void rnd_seed(void)
{
    struct timeval tv;
    clock_t clk;
    unsigned int seed;

    clk = clock() / CLOCKS_PER_SEC;
    gettimeofday(&tv, NULL);
    seed = (getpid() << 16) ^ tv.tv_sec ^ tv.tv_usec ^ clk;
    srandom(seed);
}

#define ARR_INT_FULL 10000000
#define ARR_INT_HALF (ARR_INT_FULL >> 1)
static int arr_int[ARR_INT_FULL];

static void test_rand_int(struct rb_root *t)
{
    size_t i;
    int res;
    size_t nr_enomem = 0;
    size_t nr_eexist = 0;
    size_t nr_success = 0;
    size_t nr_unknown = 0;

    for (i = 0; i < ARR_INT_FULL; i++)
        arr_int[i] = rand() % (ARR_INT_FULL << 4);

    for (i = 0; i < ARR_INT_FULL; i++) {
        res = rb_insert(t, PCONV(arr_int[i]));
        switch (res) {
        case ENOMEM:
            nr_enomem++;
            break;
        case EEXIST:
            nr_eexist++;
            break;
        case 0:
            nr_success++;
            break;
        default:
            nr_unknown++;
            break;
        }
    }

    for (i = 0; i < ARR_INT_FULL; i++)
        assert(rb_find(t, PCONV(arr_int[i])) == 1);

    DBG("ENOMEM: %zu, EEXIST: %zu OK: %zu UNK: %zu",
            nr_enomem, nr_eexist, nr_success, nr_unknown);

    DBG("int rbtree size: %zu", rb_size(t));

    __ASSERT(nr_unknown == 0);
    __ASSERT(nr_enomem + nr_eexist + nr_success == ARR_INT_FULL);
    __ASSERT(nr_success == rb_size(t));

    rb_assert(t);
    //rb_show(t);

    DBG("int rbtree removing");

#if 1
    for (i = 0; i < ARR_INT_FULL; i++)
        rb_remove(t, PCONV(arr_int[i]));

    DBG("int rbtree size: %zu", rb_size(t));
#endif

    rb_assert(t);
    rb_show(t);

    //rb_free(&t);
    rb_clear(&t);

    rb_assert(t);   /* Assert the empty tree */
    rb_show(t);

    rb_free(&t);
}

static void test_rand_int2(struct rb_root *t)
{
    size_t i;
    int res;
    size_t nr_enomem = 0;
    size_t nr_eexist = 0;
    size_t nr_success = 0;
    size_t nr_unknown = 0;

    for (i = 0; i < ARR_INT_FULL; i++)
        arr_int[i] = rand() % (ARR_INT_FULL << 4);

    /* Only insert half of them */
    for (i = 0; i < ARR_INT_HALF; i++) {
        res = rb_insert(t, PCONV(arr_int[i]));
        switch (res) {
        case ENOMEM:
            nr_enomem++;
            break;
        case EEXIST:
            nr_eexist++;
            break;
        case 0:
            nr_success++;
            break;
        default:
            nr_unknown++;
            break;
        }
    }

    for (i = 0; i < ARR_INT_HALF; i++)
        assert(rb_find(t, PCONV(arr_int[i])) == 1);

    DBG("ENOMEM: %zu, EEXIST: %zu OK: %zu UNK: %zu",
            nr_enomem, nr_eexist, nr_success, nr_unknown);

    DBG("int rbtree size: %zu", rb_size(t));

    size_t nr_found = 0;    /* May collapsed */
    size_t nr_not_found = 0;

    for (i = ARR_INT_HALF; i < ARR_INT_FULL; i++) {
        res = rb_find(t, PCONV(arr_int[i]));
        res ? nr_found++ : nr_not_found++;
    }
    DBG("right half  found: %zu not found: %zu", nr_found, nr_not_found);

    __ASSERT(nr_unknown == 0);
    __ASSERT(nr_enomem + nr_eexist + nr_success == ARR_INT_HALF);
    __ASSERT(nr_success == rb_size(t));

    rb_assert(t);
    //rb_show(t);

    DBG("int rbtree removing");

    /* Delete from right half */
    for (i = ARR_INT_HALF; i < ARR_INT_FULL; i++)
        rb_remove(t, PCONV(arr_int[i]));
    DBG("int rbtree size: %zu", rb_size(t));

    rb_assert(t);
    //rb_show(t);

    //rb_free(&t);
    rb_clear(&t);

    rb_assert(t);   /* Assert the empty tree */
    rb_show(t);

    rb_free(&t);
}

int main(void)
{
    struct rb_root *t = rb_alloc(NULL, NULL);
    if (t == NULL) {
        DBG("Memory exhausted");
        return 1;
    }

    rnd_seed();

    rb_assert(t);       /* Assert the empty tree */
    rb_show(t);

    //test_rand_int(t);
    test_rand_int2(t);
    return 0;
}

