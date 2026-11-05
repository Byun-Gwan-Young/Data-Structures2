#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Assignment 12: Comparison of Quick Sort pivot selection strategies. */
#define DATA_SEED UINT32_C(20261103)
#define PIVOT_SEED UINT32_C(20261212)
#define VALUE_RANGE 100000U
#define BENCHMARK_SIZE 10000
#define BENCHMARK_RUNS 200

typedef enum {
    PIVOT_FIRST,
    PIVOT_RANDOM,
    PIVOT_MEDIAN_THREE
} PivotMethod;

typedef struct {
    int left;
    int right;
} Range;

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
    uint32_t state = DATA_SEED;  /* Same initial sequence at every data size. */
    for (int i = 0; i < n; ++i) {
        array[i] = (int)(next_random(&state) % VALUE_RANGE);
    }
}

static void swap_int(int *a, int *b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

/* Only data-value comparisons increment this counter. */
static int value_less(int a, int b, uint64_t *comparisons)
{
    ++(*comparisons);
    return a < b;
}

/* Median of values at left, middle, right; count all decision comparisons. */
static int median_of_three_index(const int *array, int left, int right,
                                 uint64_t *comparisons)
{
    int middle = left + (right - left) / 2;

    if (value_less(array[left], array[middle], comparisons)) {
        if (value_less(array[middle], array[right], comparisons)) {
            return middle;
        }
        return value_less(array[left], array[right], comparisons)
                   ? right : left;
    }

    if (value_less(array[left], array[right], comparisons)) {
        return left;
    }
    return value_less(array[middle], array[right], comparisons)
               ? right : middle;
}

static int choose_pivot_index(const int *array, int left, int right,
                              PivotMethod method, uint64_t *comparisons,
                              uint32_t *random_state)
{
    switch (method) {
    case PIVOT_FIRST:
        return left;
    case PIVOT_RANDOM:
        return left + (int)(next_random(random_state)
                            % (uint32_t)(right - left + 1));
    case PIVOT_MEDIAN_THREE:
        return median_of_three_index(array, left, right, comparisons);
    }
    return left;
}

/* Same Lomuto partition for every pivot strategy and implementation. */
static int partition(int *array, int left, int right, PivotMethod method,
                     uint64_t *comparisons, uint32_t *random_state)
{
    int pivot_index = choose_pivot_index(array, left, right, method,
                                         comparisons, random_state);
    swap_int(&array[pivot_index], &array[right]);

    int pivot_value = array[right];
    int boundary = left;
    for (int scan = left; scan < right; ++scan) {
        ++(*comparisons);                 /* array[scan] <= pivot_value */
        if (array[scan] <= pivot_value) {
            swap_int(&array[boundary], &array[scan]);
            ++boundary;
        }
    }
    swap_int(&array[boundary], &array[right]);
    return boundary;
}

static void quick_sort_recursive(int *array, int left, int right,
                                 PivotMethod method, uint64_t *comparisons,
                                 uint32_t *random_state)
{
    if (left >= right) {
        return;
    }

    int pivot_index = partition(array, left, right, method,
                                comparisons, random_state);
    quick_sort_recursive(array, left, pivot_index - 1,
                         method, comparisons, random_state);
    quick_sort_recursive(array, pivot_index + 1, right,
                         method, comparisons, random_state);
}

static int quick_sort_iterative(int *array, int n, PivotMethod method,
                                uint64_t *comparisons, uint32_t *random_state)
{
    if (n < 2) {
        return 1;
    }

    Range *stack = (Range *)malloc((size_t)n * sizeof(*stack));
    if (stack == NULL) {
        return 0;
    }

    size_t top = 0;
    stack[top++] = (Range){0, n - 1};

    while (top != 0) {
        Range current = stack[--top];
        int pivot_index = partition(array, current.left, current.right,
                                    method, comparisons, random_state);

        /* Push right first so left is processed next (as in recursion). */
        if (pivot_index + 1 < current.right) {
            stack[top++] = (Range){pivot_index + 1, current.right};
        }
        if (current.left < pivot_index - 1) {
            stack[top++] = (Range){current.left, pivot_index - 1};
        }
    }

    free(stack);
    return 1;
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

static int run_pivot_comparison(int n)
{
    int *original = (int *)malloc((size_t)n * sizeof(*original));
    int *sorted = (int *)malloc((size_t)n * sizeof(*sorted));
    int *reference = (int *)malloc((size_t)n * sizeof(*reference));
    if (original == NULL || sorted == NULL || reference == NULL) {
        fprintf(stderr, "Memory allocation failed (n=%d).\n", n);
        free(original);
        free(sorted);
        free(reference);
        return 0;
    }

    generate_data(original, n);
    const char *names[] = {"First", "Random", "Median-3"};
    printf("Data Size : %d\n", n);

    for (int method = PIVOT_FIRST; method <= PIVOT_MEDIAN_THREE; ++method) {
        memcpy(sorted, original, (size_t)n * sizeof(*sorted));
        uint64_t comparisons = 0;
        uint32_t random_state = PIVOT_SEED ^ (uint32_t)n;
        quick_sort_recursive(sorted, 0, n - 1, (PivotMethod)method,
                             &comparisons, &random_state);

        if (!is_sorted(sorted, n)) {
            fprintf(stderr, "Sort verification failed (n=%d, %s).\n",
                    n, names[method]);
            free(original);
            free(sorted);
            free(reference);
            return 0;
        }
        if (method == PIVOT_FIRST) {
            memcpy(reference, sorted, (size_t)n * sizeof(*reference));
        } else if (memcmp(reference, sorted,
                          (size_t)n * sizeof(*sorted)) != 0) {
            fprintf(stderr, "Results differ among pivot strategies (n=%d).\n", n);
            free(original);
            free(sorted);
            free(reference);
            return 0;
        }

        printf("Quick Sort (%s)%*s: %" PRIu64 " comparisons\n",
               names[method], 9 - (int)strlen(names[method]), "", comparisons);
    }
    putchar('\n');
    free(original);
    free(sorted);
    free(reference);
    return 1;
}

/* Clock timing encloses sorting only; copies and verification are excluded. */
static int benchmark_variant(const int *original, int *work, int n,
                             int iterative, double *average_ms,
                             uint64_t *first_comparisons)
{
    double sum_ticks = 0.0;
    for (int trial = 0; trial < BENCHMARK_RUNS; ++trial) {
        memcpy(work, original, (size_t)n * sizeof(*work));
        uint64_t comparisons = 0;
        uint32_t random_state = PIVOT_SEED ^ (uint32_t)n;

        clock_t start = clock();
        int ok = 1;
        if (iterative) {
            ok = quick_sort_iterative(work, n, PIVOT_MEDIAN_THREE,
                                      &comparisons, &random_state);
        } else {
            quick_sort_recursive(work, 0, n - 1, PIVOT_MEDIAN_THREE,
                                 &comparisons, &random_state);
        }
        clock_t end = clock();

        if (!ok || start == (clock_t)-1 || end == (clock_t)-1 ||
            !is_sorted(work, n)) {
            return 0;
        }
        if (trial == 0) {
            *first_comparisons = comparisons;
        } else if (comparisons != *first_comparisons) {
            return 0;
        }
        sum_ticks += (double)(end - start);
    }

    *average_ms = sum_ticks * 1000.0 /
                  ((double)CLOCKS_PER_SEC * BENCHMARK_RUNS);
    return 1;
}

static int run_bonus(void)
{
    const int n = BENCHMARK_SIZE;
    int *original = (int *)malloc((size_t)n * sizeof(*original));
    int *work = (int *)malloc((size_t)n * sizeof(*work));
    if (original == NULL || work == NULL) {
        fprintf(stderr, "Memory allocation failed for benchmark.\n");
        free(original);
        free(work);
        return 0;
    }
    generate_data(original, n);

    double recursive_ms = 0.0;
    double iterative_ms = 0.0;
    uint64_t recursive_comparisons = 0;
    uint64_t iterative_comparisons = 0;
    int ok = benchmark_variant(original, work, n, 0,
                               &recursive_ms, &recursive_comparisons) &&
             benchmark_variant(original, work, n, 1,
                               &iterative_ms, &iterative_comparisons);
    if (!ok || recursive_comparisons != iterative_comparisons) {
        fprintf(stderr, "Bonus benchmark verification failed.\n");
        free(original);
        free(work);
        return 0;
    }

    printf("[Bonus: Recursive vs Iterative]\n");
    printf("Data Size            : %d\n", n);
    printf("Pivot                : Median-of-Three\n");
    printf("Repeated Runs        : %d (average CPU time)\n", BENCHMARK_RUNS);
    printf("Recursive Quick Sort : %.6f ms\n", recursive_ms);
    printf("Iterative Quick Sort : %.6f ms\n", iterative_ms);
    printf("Value Comparisons    : %" PRIu64 " (same for both)\n",
           recursive_comparisons);

    free(original);
    free(work);
    return 1;
}

int main(void)
{
    const int sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    const size_t count = sizeof(sizes) / sizeof(sizes[0]);

    printf("Quick Sort - Pivot Selection Comparison\n");
    printf("Data Seed: %" PRIu32 ", Range: 0..%u (duplicates allowed)\n\n",
           DATA_SEED, VALUE_RANGE - 1U);

    for (size_t i = 0; i < count; ++i) {
        if (!run_pivot_comparison(sizes[i])) {
            return EXIT_FAILURE;
        }
    }
    if (!run_bonus()) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
