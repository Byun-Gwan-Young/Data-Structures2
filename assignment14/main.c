#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Assignment 14: in-place Heap Sort, Merge Sort, and Quick Sort (three pivots).
   Data generation, merge and quick logic match Assignment 13. */
#define DATA_SEED UINT32_C(20261103)
#define PIVOT_SEED UINT32_C(20261212)
#define VALUE_RANGE UINT32_C(100000)
#define BENCHMARK_RUNS 100

typedef enum {
    HEAP_SORT,
    MERGE_SORT,
    QUICK_FIRST,
    QUICK_RANDOM,
    QUICK_MEDIAN3,
    ALGORITHM_COUNT
} Algorithm;

typedef enum {
    PIVOT_FIRST,
    PIVOT_RANDOM,
    PIVOT_MEDIAN_THREE
} PivotMethod;

typedef struct {
    uint64_t comparisons;
    size_t workspace_bytes;
    size_t peak_extra_bytes;
} SortStats;

typedef struct {
    uint64_t comparisons;
    double average_ms;
    size_t peak_extra_bytes;
} Result;

/* Source-level stack-frame models. Estimates, NOT actual ABI frames.
   sizeof follows pointer size and structure alignment. All algorithms use
   the same model convention, excluding shared SortStats and input arrays. */
typedef struct {
    int *array;
    int n;
    int start;
    int end;
    SortStats *stats;
} HeapRootFrame;

typedef struct {
    int *array;
    int root;
    int end;
    int child;
    int largest;
    int temp;
    SortStats *stats;
} HeapSiftFrame;

typedef struct {
    int *array;
    int *temporary;
    int n;
    SortStats *stats;
} MergeRootFrame;

typedef struct {
    int *array;
    int *temporary;
    int left;
    int right;
    int middle;
    SortStats *stats;
} MergeRecursiveFrame;

typedef struct {
    int *array;
    int *temporary;
    int left;
    int middle;
    int right;
    int i;
    int j;
    int k;
    SortStats *stats;
} MergeCombineFrame;

typedef struct {
    int *array;
    int n;
    PivotMethod method;
    SortStats *stats;
    uint32_t *random_state;
} QuickRootFrame;

typedef struct {
    int *array;
    int left;
    int right;
    PivotMethod method;
    int pivot_index;
    SortStats *stats;
    uint32_t *random_state;
} QuickRecursiveFrame;

typedef struct {
    int *array;
    int left;
    int right;
    PivotMethod method;
    int pivot_index;
    int pivot_value;
    int boundary;
    int scan;
    SortStats *stats;
    uint32_t *random_state;
} QuickPartitionFrame;

typedef struct {
    const int *array;
    int left;
    int right;
    PivotMethod method;
    SortStats *stats;
    uint32_t *random_state;
} QuickSelectFrame;

typedef struct {
    const int *array;
    int left;
    int middle;
    int right;
    SortStats *stats;
} QuickMedianFrame;

static const char *const ALGORITHM_NAMES[ALGORITHM_COUNT] = {
    "Heap Sort", "Merge Sort", "Quick (First)", "Quick (Random)", "Quick (Median3)"
};

static uint32_t next_random(uint32_t *state)
{
    uint32_t x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void generate_data(int *array, int n)
{
    uint32_t state = DATA_SEED;
    for (int i = 0; i < n; ++i) {
        array[i] = (int)(next_random(&state) % VALUE_RANGE);
    }
}

static void update_peak(SortStats *stats, size_t stack_model_bytes)
{
    const size_t additional = stats->workspace_bytes + stack_model_bytes;
    if (additional > stats->peak_extra_bytes) {
        stats->peak_extra_bytes = additional;
    }
}

/* ------------------------ In-place Max Heap Sort ---------------------- */
/* end is exclusive. Every comparison of two DATA values is counted,
   during both initial heap construction and each sift-down after extraction.
   Loop/index bounds and parent/child index checks are not counted. */
static void heap_sift_down(int *array, int root, int end, SortStats *stats)
{
    update_peak(stats, sizeof(HeapRootFrame) + sizeof(HeapSiftFrame));
    while (root < end / 2) { /* Has at least a left child? */
        const int child = root * 2 + 1;
        int largest = root;

        ++stats->comparisons; /* array[child] > array[largest] */
        if (array[child] > array[largest]) {
            largest = child;
        }
        if (child + 1 < end) {
            ++stats->comparisons; /* Right child vs current largest. */
            if (array[child + 1] > array[largest]) {
                largest = child + 1;
            }
        }
        if (largest == root) {
            break;
        }
        const int temp = array[root];
        array[root] = array[largest];
        array[largest] = temp;
        root = largest;
    }
}

static void heap_sort(int *array, int n, SortStats *stats)
{
    update_peak(stats, sizeof(HeapRootFrame));

    /* Floyd's bottom-up maximum-heap construction. No heap buffer. */
    for (int start = n / 2; start > 0;) {
        --start;
        heap_sift_down(array, start, n, stats);
    }

    /* Move the maximum to the sorted suffix, restore heap property. */
    for (int end = n - 1; end > 0; --end) {
        const int temp = array[0];
        array[0] = array[end];
        array[end] = temp;
        heap_sift_down(array, 0, end, stats);
    }
}

/* ----------------------------- Merge Sort ----------------------------- */
static void merge_combine(int *array, int *temporary, int left, int middle,
                          int right, SortStats *stats)
{
    int i = left;
    int j = middle + 1;
    int k = left;

    while (i <= middle && j <= right) {
        ++stats->comparisons; /* Compare two DATA values; not loop indexes. */
        if (array[i] <= array[j]) {
            temporary[k++] = array[i++];
        } else {
            temporary[k++] = array[j++];
        }
    }
    while (i <= middle) {
        temporary[k++] = array[i++];
    }
    while (j <= right) {
        temporary[k++] = array[j++];
    }
    for (int t = left; t <= right; ++t) {
        array[t] = temporary[t];
    }
}

static void merge_sort_recursive(int *array, int *temporary, int left,
                                 int right, SortStats *stats, size_t depth)
{
    const size_t base = sizeof(MergeRootFrame)
                      + depth * sizeof(MergeRecursiveFrame);
    update_peak(stats, base);
    if (left >= right) {
        return;
    }

    const int middle = left + (right - left) / 2;
    merge_sort_recursive(array, temporary, left, middle, stats, depth + 1);
    merge_sort_recursive(array, temporary, middle + 1, right, stats, depth + 1);

    /* Parent recursive frames and this merge helper are simultaneously live. */
    update_peak(stats, base + sizeof(MergeCombineFrame));
    merge_combine(array, temporary, left, middle, right, stats);
}

/* Temporary array allocation/free occur INSIDE the timed sorting call. */
static int merge_sort(int *array, int n, SortStats *stats)
{
    update_peak(stats, sizeof(MergeRootFrame));
    int *temporary = (int *)malloc((size_t)n * sizeof(*temporary));
    if (temporary == NULL) {
        return 0;
    }
    stats->workspace_bytes = (size_t)n * sizeof(*temporary);
    update_peak(stats, sizeof(MergeRootFrame));
    if (n > 0) {
        merge_sort_recursive(array, temporary, 0, n - 1, stats, 1);
    }
    free(temporary);
    stats->workspace_bytes = 0;
    return 1;
}

/* ----------------------------- Quick Sort ----------------------------- */
static void swap_int(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

/* Identical median-of-three choice, counting, and Lomuto logic to A12. */
static int value_less(int a, int b, SortStats *stats)
{
    ++stats->comparisons;
    return a < b;
}

static int median_of_three_index(const int *array, int left, int right,
                                 SortStats *stats)
{
    const int middle = left + (right - left) / 2;

    if (value_less(array[left], array[middle], stats)) {
        if (value_less(array[middle], array[right], stats)) {
            return middle;
        }
        return value_less(array[left], array[right], stats) ? right : left;
    }
    if (value_less(array[left], array[right], stats)) {
        return left;
    }
    return value_less(array[middle], array[right], stats) ? right : middle;
}

static int choose_pivot_index(const int *array, int left, int right,
                              PivotMethod method, SortStats *stats,
                              uint32_t *random_state)
{
    switch (method) {
    case PIVOT_FIRST:
        return left;
    case PIVOT_RANDOM:
        return left + (int)(next_random(random_state)
                            % (uint32_t)(right - left + 1));
    case PIVOT_MEDIAN_THREE:
        return median_of_three_index(array, left, right, stats);
    }
    return left;
}

static int partition(int *array, int left, int right, PivotMethod method,
                     SortStats *stats, uint32_t *random_state)
{
    const int pivot_index = choose_pivot_index(array, left, right, method,
                                                stats, random_state);
    swap_int(&array[pivot_index], &array[right]);

    const int pivot_value = array[right];
    int boundary = left;
    for (int scan = left; scan < right; ++scan) {
        ++stats->comparisons; /* array[scan] <= pivot_value */
        if (array[scan] <= pivot_value) {
            swap_int(&array[boundary], &array[scan]);
            ++boundary;
        }
    }
    swap_int(&array[boundary], &array[right]);
    return boundary;
}

static void quick_sort_recursive(int *array, int left, int right,
                                 PivotMethod method, SortStats *stats,
                                 uint32_t *random_state, size_t depth)
{
    const size_t base = sizeof(QuickRootFrame)
                      + depth * sizeof(QuickRecursiveFrame);
    update_peak(stats, base);
    if (left >= right) {
        return;
    }

    size_t active_helpers = sizeof(QuickPartitionFrame)
                          + sizeof(QuickSelectFrame);
    if (method == PIVOT_MEDIAN_THREE) {
        active_helpers += sizeof(QuickMedianFrame);
    }
    update_peak(stats, base + active_helpers);

    const int pivot_index = partition(array, left, right, method,
                                      stats, random_state);
    quick_sort_recursive(array, left, pivot_index - 1, method, stats,
                         random_state, depth + 1);
    quick_sort_recursive(array, pivot_index + 1, right, method, stats,
                         random_state, depth + 1);
}

static void quick_sort(int *array, int n, PivotMethod method, SortStats *stats,
                       uint32_t *random_state)
{
    update_peak(stats, sizeof(QuickRootFrame));
    if (n > 0) {
        quick_sort_recursive(array, 0, n - 1, method, stats, random_state, 1);
    }
}

/* --------------------------- Benchmark I/O --------------------------- */
static int compare_ints_for_reference(const void *a, const void *b)
{
    const int x = *(const int *)a;
    const int y = *(const int *)b;
    return (x > y) - (x < y);
}

static int is_sorted(const int *array, int n)
{
    for (int i = 1; i < n; ++i) {
        if (array[i - 1] > array[i]) {
            return 0;
        }
    }
    return 1;
}

static int benchmark(const int *original, const int *reference, int *work,
                     int n, Algorithm algorithm, Result *result)
{
    double total_ticks = 0.0;
    result->comparisons = 0;
    result->peak_extra_bytes = 0;

    for (int trial = 0; trial < BENCHMARK_RUNS; ++trial) {
        memcpy(work, original, (size_t)n * sizeof(*work)); /* Not timed. */
        SortStats stats = {0, 0, 0};
        uint32_t random_state = PIVOT_SEED ^ (uint32_t)n;
        int ok = 1;

        const clock_t start = clock();
        if (algorithm == HEAP_SORT) {
            heap_sort(work, n, &stats);
        } else if (algorithm == MERGE_SORT) {
            ok = merge_sort(work, n, &stats);
        } else {
            const PivotMethod method = (PivotMethod)(algorithm - QUICK_FIRST);
            quick_sort(work, n, method, &stats, &random_state);
        }
        const clock_t end = clock();

        if (!ok || start == (clock_t)-1 || end == (clock_t)-1 || end < start) {
            fprintf(stderr, "Timing or allocation failed (n=%d, %s).\n",
                    n, ALGORITHM_NAMES[algorithm]);
            return 0;
        }
        /* Checks run OUTSIDE the timed region. */
        if (!is_sorted(work, n) ||
            memcmp(work, reference, (size_t)n * sizeof(*work)) != 0) {
            fprintf(stderr, "Sort verification FAILED (n=%d, %s).\n",
                    n, ALGORITHM_NAMES[algorithm]);
            return 0;
        }
        if (trial == 0) {
            result->comparisons = stats.comparisons;
            result->peak_extra_bytes = stats.peak_extra_bytes;
        } else if (result->comparisons != stats.comparisons ||
                   result->peak_extra_bytes != stats.peak_extra_bytes) {
            fprintf(stderr, "Non-reproducible results (n=%d, %s).\n",
                    n, ALGORITHM_NAMES[algorithm]);
            return 0;
        }
        total_ticks += (double)(end - start);
    }
    result->average_ms = 1000.0 * total_ticks /
                         ((double)CLOCKS_PER_SEC * BENCHMARK_RUNS);
    return 1;
}

static int run_size(int n)
{
    int *original = (int *)malloc((size_t)n * sizeof(*original));
    int *work = (int *)malloc((size_t)n * sizeof(*work));
    int *reference = (int *)malloc((size_t)n * sizeof(*reference));
    if (original == NULL || work == NULL || reference == NULL) {
        fprintf(stderr, "Input allocation failed (n=%d).\n", n);
        free(original);
        free(work);
        free(reference);
        return 0;
    }

    generate_data(original, n);
    memcpy(reference, original, (size_t)n * sizeof(*reference));
    qsort(reference, (size_t)n, sizeof(*reference), compare_ints_for_reference);

    Result results[ALGORITHM_COUNT];
    int ok = 1;
    for (int a = 0; a < ALGORITHM_COUNT; ++a) {
        if (!benchmark(original, reference, work, n, (Algorithm)a,
                       &results[a])) {
            ok = 0;
            break;
        }
    }
    if (ok) {
        printf("Data Size : %d\n", n);
        printf("%-17s %14s %12s %16s\n",
               "Algorithm", "Comparisons", "Time(ms)", "Memory(Bytes)");
        puts("---------------------------------------------------------------");
        for (int a = 0; a < ALGORITHM_COUNT; ++a) {
            printf("%-17s %14" PRIu64 " %12.6f %16zu\n",
                   ALGORITHM_NAMES[a], results[a].comparisons,
                   results[a].average_ms, results[a].peak_extra_bytes);
        }
        putchar('\n');
    }
    free(original);
    free(work);
    free(reference);
    return ok;
}

int main(void)
{
    static const int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    printf("Assignment 14 - Heap Sort vs Merge Sort vs Quick Sort\n");
    printf("Data Seed: %" PRIu32 ", Pivot Seed: %" PRIu32
           ", Range: 0..%" PRIu32 "\n", DATA_SEED, PIVOT_SEED,
           VALUE_RANGE - 1U);
    printf("Timing: %d runs, average CPU time (ms, clock())\n", BENCHMARK_RUNS);
    printf("Memory: calculated peak additional bytes (source-level frame model)\n");
    printf("Frame sizes (bytes): Heap root=%zu, sift=%zu; "
           "Merge root=%zu, recursive=%zu, combine=%zu; "
           "Quick root=%zu, recursive=%zu, partition=%zu, select=%zu, median=%zu\n\n",
           sizeof(HeapRootFrame), sizeof(HeapSiftFrame),
           sizeof(MergeRootFrame), sizeof(MergeRecursiveFrame),
           sizeof(MergeCombineFrame), sizeof(QuickRootFrame),
           sizeof(QuickRecursiveFrame), sizeof(QuickPartitionFrame),
           sizeof(QuickSelectFrame), sizeof(QuickMedianFrame));

    for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
        if (!run_size(sizes[i])) {
            return EXIT_FAILURE;
        }
    }
    puts("Verification: PASS (all algorithms, all sizes, all runs)");
    return EXIT_SUCCESS;
}
