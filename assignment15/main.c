/*
 * Assignment 15 - Quick Sort vs AVL Tree Sort
 * Build (macOS/Linux): gcc -std=c11 -O2 -Wall -Wextra -pedantic main.c -o main
 * Build (Windows, MinGW): gcc -std=c11 -O2 -Wall -Wextra -pedantic main.c -o main.exe
 *
 * Data value comparisons: one 3-way comparison of two integer keys = 1.
 * Data movements: explicit integer KEY copies/assignments (including swap
 * temporaries and quicksort pivot temporaries); exclude pointer/metadata
 * updates, passing function arguments, and initial input preparation.
 * Memory: peak requested bytes for working data plus source-level estimated
 * recursion-frame bytes. See README.
 */
#if !defined(_WIN32)
# define _POSIX_C_SOURCE 200809L
#endif

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
# include <windows.h>
#endif

#define DATA_SEED      UINT32_C(20261103)
#define OPERATION_SEED UINT32_C(20261116)
#define NUMBER_RANGE   UINT32_C(100000)  /* values 0..99999 */

static const size_t DATA_SIZES[] = {1000, 2000, 4000, 8000, 16000, 20000};

typedef struct AVLNode {
    int key;
    size_t count;
    int height;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

typedef struct {
    bool insert;
    int value;
} Operation;

/* Only declared C-level parameters/locals, not compiler stack overhead. */
typedef struct {
    int *array;
    size_t low, high, i, less, greater, pivot_index;
    int pivot, comparison;
    void *metrics;
} QuickFrameModel;

typedef struct {
    AVLNode *root, *child, *result;
    void *metrics;
    int value, comparison, balance;
    bool removed;
} AvlFrameModel;

typedef struct {
    uint64_t comparisons;
    uint64_t movements;
    size_t fixed_bytes;       /* allocated working array */
    size_t live_nodes;
    size_t current_depth;
    size_t peak_bytes;
    size_t frame_bytes;       /* source-level recursion frame model */
} Metrics;

typedef struct {
    double total_ms;
    double sort_ms;
    double update_ms;
    uint64_t sort_comparisons;
    uint64_t sort_movements;
    uint64_t insertion_movements;
    uint64_t deletion_movements;
    Metrics metric;
    size_t final_count;
    size_t requested_inserts;
    size_t successful_deletes;
} Result;

static void die(const char *message)
{
    fprintf(stderr, "ERROR: %s\n", message);
    exit(EXIT_FAILURE);
}

static void *checked_malloc(size_t bytes)
{
    void *p = malloc(bytes == 0 ? 1 : bytes);
    if (p == NULL) die("out of memory");
    return p;
}

static uint32_t next_random(uint32_t *state)
{
    uint32_t x = *state;
    if (x == 0) x = UINT32_C(0x9e3779b9);
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static double monotonic_ms(void)
{
#ifdef _WIN32
    LARGE_INTEGER freq, tick;
    if (!QueryPerformanceFrequency(&freq) || !QueryPerformanceCounter(&tick))
        die("QueryPerformanceCounter failed");
    return (double)tick.QuadPart * 1000.0 / (double)freq.QuadPart;
#else
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
        die("clock_gettime failed");
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec * 1e-6;
#endif
}

static void refresh_peak(Metrics *m)
{
    if (m == NULL) return;
    const size_t current = m->fixed_bytes
                         + m->live_nodes * sizeof(AVLNode)
                         + m->current_depth * m->frame_bytes;
    if (current > m->peak_bytes) m->peak_bytes = current;
}

static void push_frame(Metrics *m)
{
    if (m == NULL) return;
    ++m->current_depth;
    refresh_peak(m);
}

static void pop_frame(Metrics *m)
{
    if (m != NULL) --m->current_depth;
}

static int key_cmp(int a, int b, Metrics *m)
{
    if (m != NULL) ++m->comparisons;
    return (a > b) - (a < b);
}

static void swap_keys(int *a, int *b, Metrics *m)
{
    /* Preserve Assignment 14 Lomuto behavior, including self-swaps. */
    int temp = *a;
    *a = *b;
    *b = temp;
    if (m != NULL) m->movements += 3;
}

/* Assignment 14 Quick (Median3): same pivot selection and Lomuto logic. */
static bool key_less(int a, int b, Metrics *m)
{
    return key_cmp(a, b, m) < 0;
}

static size_t median_of_three_index(const int *a, size_t left, size_t right,
                                    Metrics *m)
{
    const size_t middle = left + (right - left) / 2;
    if (key_less(a[left], a[middle], m)) {
        if (key_less(a[middle], a[right], m)) return middle;
        return key_less(a[left], a[right], m) ? right : left;
    }
    if (key_less(a[left], a[right], m)) return left;
    return key_less(a[middle], a[right], m) ? right : middle;
}

/* Bounds [left, right); same as previous inclusive-right implementation. */
static size_t partition_lomuto(int *a, size_t left, size_t right, Metrics *m)
{
    const size_t last = right - 1;
    const size_t pivot_index = median_of_three_index(a, left, last, m);
    swap_keys(&a[pivot_index], &a[last], m);
    const int pivot = a[last];
    if (m != NULL) ++m->movements; /* explicit pivot temporary key copy */
    size_t boundary = left;
    for (size_t scan = left; scan < last; ++scan) {
        if (key_cmp(a[scan], pivot, m) <= 0) {
            swap_keys(&a[boundary], &a[scan], m);
            ++boundary;
        }
    }
    swap_keys(&a[boundary], &a[last], m);
    return boundary;
}

static void quick_sort_rec(int *a, size_t left, size_t right, Metrics *m)
{
    push_frame(m);
    if (right - left > 1) {
        const size_t pivot = partition_lomuto(a, left, right, m);
        quick_sort_rec(a, left, pivot, m);
        quick_sort_rec(a, pivot + 1, right, m);
    }
    pop_frame(m);
}

/* Quick array insertion/deletion: LINEAR search, then shifts. */
static void array_insert(int *a, size_t *length, int value, Metrics *m)
{
    size_t pos = 0;
    while (pos < *length && key_cmp(a[pos], value, m) <= 0) ++pos;
    for (size_t i = *length; i > pos; --i) {
        a[i] = a[i - 1];
        if (m != NULL) ++m->movements;
    }
    a[pos] = value;
    if (m != NULL) ++m->movements;
    ++*length;
}

static bool array_delete_one(int *a, size_t *length, int value, Metrics *m)
{
    for (size_t i = 0; i < *length; ++i) {
        const int cmp = key_cmp(a[i], value, m);
        if (cmp > 0) return false; /* sorted array, no further matches */
        if (cmp == 0) {
            for (size_t j = i; j + 1 < *length; ++j) {
                a[j] = a[j + 1];
                if (m != NULL) ++m->movements;
            }
            --*length;
            return true;
        }
    }
    return false;
}

static int height(const AVLNode *p) { return p ? p->height : 0; }

static void update_height(AVLNode *p)
{
    int hl = height(p->left), hr = height(p->right);
    p->height = 1 + (hl > hr ? hl : hr);
}

static AVLNode *rotate_left(AVLNode *a)
{
    AVLNode *b = a->right;
    a->right = b->left;
    b->left = a;
    update_height(a);
    update_height(b);
    return b;
}

static AVLNode *rotate_right(AVLNode *a)
{
    AVLNode *b = a->left;
    a->left = b->right;
    b->right = a;
    update_height(a);
    update_height(b);
    return b;
}

static AVLNode *rebalance(AVLNode *node)
{
    update_height(node);
    const int balance = height(node->left) - height(node->right);
    if (balance > 1) {
        if (height(node->left->left) < height(node->left->right))
            node->left = rotate_left(node->left);
        return rotate_right(node);
    }
    if (balance < -1) {
        if (height(node->right->right) < height(node->right->left))
            node->right = rotate_right(node->right);
        return rotate_left(node);
    }
    return node;
}

static AVLNode *make_node(int key, Metrics *m)
{
    AVLNode *p = (AVLNode *)checked_malloc(sizeof(*p));
    p->key = key;
    p->count = 1;
    p->height = 1;
    p->left = p->right = NULL;
    if (m != NULL) {
        ++m->movements; /* one key assignment; count/height are metadata */
        ++m->live_nodes;
        refresh_peak(m);
    }
    return p;
}

static void release_node(AVLNode *p, Metrics *m)
{
    free(p);
    if (m != NULL) --m->live_nodes;
}

static AVLNode *avl_insert(AVLNode *root, int value, Metrics *m)
{
    push_frame(m);
    AVLNode *result;
    if (root == NULL) {
        result = make_node(value, m);
    } else {
        const int cmp = key_cmp(value, root->key, m);
        if (cmp == 0) {
            ++root->count; /* metadata only, not a key data movement */
            result = root;
        } else {
            if (cmp < 0) root->left = avl_insert(root->left, value, m);
            else root->right = avl_insert(root->right, value, m);
            result = rebalance(root);
        }
    }
    pop_frame(m);
    return result;
}

/* Extract and free exactly one entire minimum-key NODE (including count). */
static AVLNode *avl_remove_min_node(AVLNode *root, Metrics *m)
{
    push_frame(m);
    AVLNode *result;
    if (root->left == NULL) {
        result = root->right;
        release_node(root, m);
    } else {
        root->left = avl_remove_min_node(root->left, m);
        result = rebalance(root);
    }
    pop_frame(m);
    return result;
}

static AVLNode *avl_delete_one(AVLNode *root, int value, bool *deleted, Metrics *m)
{
    push_frame(m);
    AVLNode *result = root;
    if (root != NULL) {
        const int cmp = key_cmp(value, root->key, m);
        if (cmp < 0) {
            root->left = avl_delete_one(root->left, value, deleted, m);
            if (*deleted) result = rebalance(root);
        } else if (cmp > 0) {
            root->right = avl_delete_one(root->right, value, deleted, m);
            if (*deleted) result = rebalance(root);
        } else {
            *deleted = true;
            if (root->count > 1) {
                --root->count;
            } else if (root->left == NULL || root->right == NULL) {
                result = root->left != NULL ? root->left : root->right;
                release_node(root, m);
            } else {
                /* Copy the successor's key AND multiplicity, then remove
                 * its entire physical node: one BST node per distinct key. */
                AVLNode *succ = root->right;
                while (succ->left != NULL) succ = succ->left;
                root->key = succ->key;
                root->count = succ->count;
                if (m != NULL) ++m->movements; /* successor key assignment */
                root->right = avl_remove_min_node(root->right, m);
                result = rebalance(root);
            }
        }
    }
    pop_frame(m);
    return result;
}

static void free_tree(AVLNode *root)
{
    if (root == NULL) return;
    free_tree(root->left);
    free_tree(root->right);
    free(root);
}

/* Validation-only routines: excluded from times/metrics. */
static int int_compare_qsort(const void *a, const void *b)
{
    int x = *(const int *)a, y = *(const int *)b;
    return (x > y) - (x < y);
}

static bool avl_verify_inorder(const AVLNode *root, int *a, size_t capacity,
                               size_t *position, bool *has_prev, int *prev,
                               int *out_height)
{
    if (root == NULL) {
        *out_height = 0;
        return true;
    }
    int lh, rh;
    if (!avl_verify_inorder(root->left, a, capacity, position,
                            has_prev, prev, &lh)) return false;
    if (root->count == 0 || (*has_prev && root->key <= *prev)) return false;
    for (size_t j = 0; j < root->count; ++j) {
        if (*position >= capacity) return false;
        a[(*position)++] = root->key;
    }
    *prev = root->key;
    *has_prev = true;
    if (!avl_verify_inorder(root->right, a, capacity, position,
                            has_prev, prev, &rh)) return false;
    if (lh - rh > 1 || rh - lh > 1) return false;
    *out_height = 1 + (lh > rh ? lh : rh);
    return *out_height == root->height;
}

static void assert_same_state(const int *array, size_t count,
                              const AVLNode *root, int *scratch, size_t capacity,
                              size_t n, size_t operation_index)
{
    size_t written = 0;
    bool has_prev = false;
    int previous = 0, root_height = 0;
    bool ok = avl_verify_inorder(root, scratch, capacity, &written,
                                  &has_prev, &previous, &root_height);
    if (!ok || written != count || memcmp(array, scratch, count * sizeof(int)) != 0) {
        fprintf(stderr, "FAIL: n=%zu, operation=%zu: array/AVL data or AVL balance differs\n",
                n, operation_index);
        exit(EXIT_FAILURE);
    }
}

static void make_input(int *source, size_t n)
{
    uint32_t state = DATA_SEED;
    for (size_t i = 0; i < n; ++i)
        source[i] = (int)(next_random(&state) % NUMBER_RANGE);
}

static void make_operations(Operation *ops, size_t n, size_t count)
{
    uint32_t state = OPERATION_SEED ^ (uint32_t)n;
    for (size_t i = 0; i < count; ++i) {
        uint32_t selector = next_random(&state); /* decide insertion/deletion */
        uint32_t key = next_random(&state);      /* independently drawn value */
        ops[i].insert = (selector % 2u) == 0;
        ops[i].value = (int)(key % NUMBER_RANGE);
    }
}

static Result run_quick(const int *source, const Operation *ops,
                        size_t n, size_t op_count, int **final_array)
{
    const size_t capacity = n + op_count;
    int *a = (int *)checked_malloc(capacity * sizeof(*a));
    memcpy(a, source, n * sizeof(*a));   /* initialization, NOT timed */

    Result r = {0};
    r.metric.fixed_bytes = capacity * sizeof(*a);
    r.metric.frame_bytes = sizeof(QuickFrameModel);
    refresh_peak(&r.metric);
    size_t length = n;
    double start = monotonic_ms();
    quick_sort_rec(a, 0, n, &r.metric);
    double sorted = monotonic_ms();
    r.sort_comparisons = r.metric.comparisons;
    r.sort_movements = r.metric.movements;
    for (size_t i = 0; i < op_count; ++i) {
        uint64_t before = r.metric.movements;
        if (ops[i].insert) {
            array_insert(a, &length, ops[i].value, &r.metric);
            ++r.requested_inserts;
            r.insertion_movements += r.metric.movements - before;
        } else if (array_delete_one(a, &length, ops[i].value, &r.metric)) {
            ++r.successful_deletes;
            r.deletion_movements += r.metric.movements - before;
        }
    }
    double end = monotonic_ms();
    r.sort_ms = sorted - start;
    r.update_ms = end - sorted;
    r.total_ms = end - start;
    r.final_count = length;
    *final_array = a;
    return r;
}

static Result run_avl(const int *source, const Operation *ops,
                      size_t n, size_t op_count, AVLNode **final_root)
{
    Result r = {0};
    r.metric.frame_bytes = sizeof(AvlFrameModel);
    AVLNode *root = NULL;
    size_t length = n;
    double start = monotonic_ms();
    for (size_t i = 0; i < n; ++i)
        root = avl_insert(root, source[i], &r.metric);
    double sorted = monotonic_ms();
    r.sort_comparisons = r.metric.comparisons;
    r.sort_movements = r.metric.movements;
    for (size_t i = 0; i < op_count; ++i) {
        uint64_t before = r.metric.movements;
        if (ops[i].insert) {
            root = avl_insert(root, ops[i].value, &r.metric);
            ++r.requested_inserts;
            ++length;
            r.insertion_movements += r.metric.movements - before;
        } else {
            bool deleted = false;
            root = avl_delete_one(root, ops[i].value, &deleted, &r.metric);
            if (deleted) {
                ++r.successful_deletes;
                --length;
                r.deletion_movements += r.metric.movements - before;
            }
        }
    }
    double end = monotonic_ms();
    r.sort_ms = sorted - start;
    r.update_ms = end - sorted;
    r.total_ms = end - start;
    r.final_count = length;
    *final_root = root;
    return r;
}

/* Reconstruct a separate run, checking entire multisets and AVL invariants
 * after EACH modification. Its work is intentionally EXCLUDED from benchmark. */
static void validate_every_operation(const int *source, const Operation *ops,
                                      size_t n, size_t op_count)
{
    const size_t capacity = n + op_count;
    int *a = (int *)checked_malloc(capacity * sizeof(*a));
    int *scratch = (int *)checked_malloc(capacity * sizeof(*scratch));
    memcpy(a, source, n * sizeof(*a));
    qsort(a, n, sizeof(*a), int_compare_qsort); /* independent reference sort */
    AVLNode *tree = NULL;
    for (size_t i = 0; i < n; ++i) tree = avl_insert(tree, source[i], NULL);
    size_t length = n;
    assert_same_state(a, length, tree, scratch, capacity, n, 0);
    for (size_t i = 0; i < op_count; ++i) {
        bool deleted_array = false, deleted_tree = false;
        if (ops[i].insert) {
            array_insert(a, &length, ops[i].value, NULL);
            tree = avl_insert(tree, ops[i].value, NULL);
        } else {
            deleted_array = array_delete_one(a, &length, ops[i].value, NULL);
            tree = avl_delete_one(tree, ops[i].value, &deleted_tree, NULL);
            if (deleted_array != deleted_tree) {
                fprintf(stderr, "FAIL: n=%zu, operation=%zu: deletion differs\n", n, i+1);
                exit(EXIT_FAILURE);
            }
        }
        assert_same_state(a, length, tree, scratch, capacity, n, i + 1);
    }
    free_tree(tree);
    free(scratch);
    free(a);
}

static void print_result(size_t n, size_t ops, const Result *quick, const Result *avl)
{
    printf("\nData Size : %zu  Operations: %zu\n", n, ops);
    printf("%-19s %18s %18s\n", "", "Quick Sort", "AVL Tree");
    printf("-------------------------------------------------------------\n");
    printf("%-19s %15.3f ms %15.3f ms\n", "Total Time",
           quick->total_ms, avl->total_ms);
    printf("%-19s %18" PRIu64 " %18" PRIu64 "\n", "Comparisons",
           quick->metric.comparisons, avl->metric.comparisons);
    printf("%-19s %18" PRIu64 " %18" PRIu64 "\n", "Data Movements",
           quick->metric.movements, avl->metric.movements);
    printf("%-19s %12zu Bytes %12zu Bytes\n", "Memory Usage*",
           quick->metric.peak_bytes, avl->metric.peak_bytes);
    printf("%-19s %18zu %18zu\n", "Final Data Count",
           quick->final_count, avl->final_count);
    printf("Phase Times (ms): sort=%.3f/%.3f, updates=%.3f/%.3f (Quick/AVL)\n",
           quick->sort_ms, avl->sort_ms, quick->update_ms, avl->update_ms);
    printf("Phase Comparisons: sort=%" PRIu64 "/%" PRIu64 ", updates=%" PRIu64 "/%" PRIu64 " (Quick/AVL)\n",
           quick->sort_comparisons, avl->sort_comparisons,
           quick->metric.comparisons - quick->sort_comparisons,
           avl->metric.comparisons - avl->sort_comparisons);
    printf("Movement Breakdowns: sort=%" PRIu64 "/%" PRIu64
           ", inserts=%" PRIu64 "/%" PRIu64
           ", deletes=%" PRIu64 "/%" PRIu64 " (Quick/AVL)\n",
           quick->sort_movements, avl->sort_movements,
           quick->insertion_movements, avl->insertion_movements,
           quick->deletion_movements, avl->deletion_movements);
    printf("Inserts: %zu / Successful deletes: %zu / Verification: PASS\n",
           quick->requested_inserts, quick->successful_deletes);
}

int main(void)
{
    puts("Assignment 15 - Quick Sort vs AVL Tree Sort");
    printf("Data Seed: %" PRIu32 "  Operation Seed: %" PRIu32
           "  Range: 0..99999\n", DATA_SEED, OPERATION_SEED);
    puts("Pivot: Assignment 14 Quick (Median3), Lomuto partition");
    puts("Timing: one full sort + all insert/delete operations (monotonic wall clock)");
    puts("*Memory: peak requested bytes + source-level recursion-frame estimate");
    puts("Validation: independent replay checks EVERY operation + AVL invariants");

    for (size_t test = 0; test < sizeof(DATA_SIZES) / sizeof(DATA_SIZES[0]); ++test) {
        size_t n = DATA_SIZES[test];
        size_t count = n / 5;
        int *source = (int *)checked_malloc(n * sizeof(*source));
        Operation *ops = (Operation *)checked_malloc(count * sizeof(*ops));
        make_input(source, n);
        make_operations(ops, n, count);

        int *array = NULL;
        AVLNode *root = NULL;
        Result quick = run_quick(source, ops, n, count, &array);
        Result avl = run_avl(source, ops, n, count, &root);
        if (quick.final_count != avl.final_count ||
            quick.requested_inserts != avl.requested_inserts ||
            quick.successful_deletes != avl.successful_deletes)
            die("quick / AVL operation counts do not match");
        int *scratch = (int *)checked_malloc((n + count) * sizeof(*scratch));
        assert_same_state(array, quick.final_count, root, scratch, n + count, n, count);
        free(scratch);
        free(array);
        free_tree(root);

        validate_every_operation(source, ops, n, count);
        print_result(n, count, &quick, &avl);
        free(ops);
        free(source);
    }
    puts("\nALL TESTS PASSED");
    return EXIT_SUCCESS;
}
