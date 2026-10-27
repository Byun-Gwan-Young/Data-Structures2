/*
 * Assignment 10: Sorting Algorithm Performance and Stability Comparison
 * C11 | Default: 100 trials, 10,000 records per trial
 *
 * Only key1 is used for sorting. Each assignment/copy of a complete Record
 * inside a sorting function counts as one data movement.
 */

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DATA_COUNT 10000
#ifndef DEFAULT_TRIALS
#define DEFAULT_TRIALS 100
#endif
#define KEY_MAX 1000000U
#define DEFAULT_SEED 20261026U
#define PARTIAL_SWAP_PAIRS 500  /* 1,000 distinct positions = 10% */

typedef struct {
    int key1;
    int key2;
    int original_index; /* Only for validation; NEVER used for sorting. */
} Record;

typedef struct {
    uint64_t comparisons;
    uint64_t movements;
} Metrics;

typedef struct {
    uint64_t total_comparisons;
    uint64_t total_movements;
} Aggregate;

typedef void (*SortFn)(Record *, size_t, Metrics *);

enum { SELECTION, BUBBLE, BUBBLE_EARLY, INSERTION_GT, ALGO_COUNT };

static const char *ALGO_NAMES[ALGO_COUNT] = {
    "Selection Sort", "Bubble Sort", "Bubble Sort (Early Termination)",
    "Insertion Sort (>)"
};

static uint32_t rng_state = DEFAULT_SEED;

static uint32_t next_random(void) {
    /* xorshift32: deterministic, fast, and independent consecutive draws. */
    uint32_t x = rng_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    rng_state = x;
    return x;
}

static uint32_t random_bounded(uint32_t bound) {
    /* Rejection sampling avoids modulo bias; bound must be nonzero. */
    const uint32_t threshold = (uint32_t)(-bound) % bound;
    uint32_t r;
    do {
        r = next_random();
    } while (r < threshold);
    return r % bound;
}

static void generate_random_data(Record *a, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        a[i].key1 = (int)random_bounded(KEY_MAX + 1U);
        a[i].key2 = (int)random_bounded(KEY_MAX + 1U);
        a[i].original_index = (int)i;
    }
}

static void exchange(Record *a, Record *b, Metrics *m) {
    Record temp = *a; /* move #1 */
    ++m->movements;
    *a = *b;          /* move #2 */
    ++m->movements;
    *b = temp;        /* move #3 */
    ++m->movements;
}

static void selection_sort(Record *a, size_t n, Metrics *m) {
    for (size_t i = 0; i + 1 < n; ++i) {
        size_t min_index = i;
        for (size_t j = i + 1; j < n; ++j) {
            ++m->comparisons;
            if (a[j].key1 < a[min_index].key1) {
                min_index = j;
            }
        }
        if (min_index != i) {
            exchange(&a[i], &a[min_index], m);
        }
    }
}

static void bubble_sort(Record *a, size_t n, Metrics *m) {
    for (size_t pass = 0; pass + 1 < n; ++pass) {
        for (size_t j = 0; j + 1 < n - pass; ++j) {
            ++m->comparisons;
            if (a[j].key1 > a[j + 1].key1) {
                exchange(&a[j], &a[j + 1], m);
            }
        }
    }
}

static void bubble_sort_early(Record *a, size_t n, Metrics *m) {
    for (size_t pass = 0; pass + 1 < n; ++pass) {
        bool swapped = false;
        for (size_t j = 0; j + 1 < n - pass; ++j) {
            ++m->comparisons;
            if (a[j].key1 > a[j + 1].key1) {
                exchange(&a[j], &a[j + 1], m);
                swapped = true;
            }
        }
        if (!swapped) {
            break;
        }
    }
}

static void insertion_sort_core(Record *a, size_t n, Metrics *m,
                                bool shift_equal_keys) {
    for (size_t i = 1; i < n; ++i) {
        Record current = a[i]; /* Copy into temporary storage: 1 move. */
        ++m->movements;
        size_t j = i;
        while (j > 0) {
            ++m->comparisons; /* key1 comparison only. */
            const bool should_shift = shift_equal_keys
                ? (a[j - 1].key1 >= current.key1)
                : (a[j - 1].key1 > current.key1);
            if (!should_shift) {
                break;
            }
            a[j] = a[j - 1]; /* Shift one complete Record: 1 move. */
            ++m->movements;
            --j;
        }
        a[j] = current; /* Final placement: 1 move (also if j == i). */
        ++m->movements;
    }
}

static void insertion_sort_gt(Record *a, size_t n, Metrics *m) {
    insertion_sort_core(a, n, m, false);
}

static void insertion_sort_ge(Record *a, size_t n, Metrics *m) {
    insertion_sort_core(a, n, m, true);
}

static const SortFn SORTS[ALGO_COUNT] = {
    selection_sort, bubble_sort, bubble_sort_early, insertion_sort_gt
};

static bool verify_sorted_permutation(const Record *a, const Record *original,
                                      size_t n) {
    unsigned char seen[DATA_COUNT] = {0};
    if (n > DATA_COUNT) return false;
    for (size_t i = 0; i < n; ++i) {
        if (i > 0 && a[i - 1].key1 > a[i].key1) return false;
        const int id = a[i].original_index;
        if (id < 0 || (size_t)id >= n || seen[id]) return false;
        seen[id] = 1;
        if (a[i].key1 != original[id].key1 ||
            a[i].key2 != original[id].key2) return false;
    }
    return true;
}

static bool verify_stable(const Record *a, size_t n) {
    for (size_t i = 1; i < n; ++i) {
        if (a[i - 1].key1 == a[i].key1 &&
            a[i - 1].original_index > a[i].original_index) {
            return false;
        }
    }
    return true;
}

static void *checked_malloc(size_t size) {
    void *p = malloc(size);
    if (!p) {
        fprintf(stderr, "Error: memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    return p;
}

static void print_metric_line(const char *name, Metrics m, bool valid) {
    printf("%-34s Comparisons: %12" PRIu64
           " | Movements: %12" PRIu64 " | Sorted: %s\n",
           name, m.comparisons, m.movements, valid ? "PASS" : "FAIL");
}

static void run_single_scenario(const char *name, const Record *input,
                                const Record *reference, Record *work,
                                size_t n) {
    printf("\n------------------------------------\n%s\n", name);
    for (int alg = 0; alg < ALGO_COUNT; ++alg) {
        memcpy(work, input, n * sizeof(*work)); /* NOT counted. */
        Metrics m = {0, 0};
        SORTS[alg](work, n, &m);
        bool good = verify_sorted_permutation(work, reference, n);
        print_metric_line(ALGO_NAMES[alg], m, good);
        if (!good) exit(EXIT_FAILURE);
    }
}

static void prepare_reverse(Record *a, size_t n) {
    /* Setup operation: no sort metrics accumulated. */
    for (size_t i = 0; i < n / 2; ++i) {
        Record temp = a[i];
        a[i] = a[n - 1 - i];
        a[n - 1 - i] = temp;
    }
}

static void prepare_partially_sorted(Record *a, size_t n) {
    /* Starts from sorted data. Randomly select 1,000 distinct indices by
       partial Fisher-Yates shuffle, then exchange 500 disjoint pairs. */
    size_t *positions = checked_malloc(n * sizeof(*positions));
    for (size_t i = 0; i < n; ++i) positions[i] = i;
    const size_t selected = 2U * PARTIAL_SWAP_PAIRS;
    for (size_t i = 0; i < selected; ++i) {
        size_t j = i + random_bounded((uint32_t)(n - i));
        size_t temp = positions[i];
        positions[i] = positions[j];
        positions[j] = temp;
    }
    for (size_t i = 0; i < selected; i += 2) {
        Record temp = a[positions[i]];
        a[positions[i]] = a[positions[i + 1]];
        a[positions[i + 1]] = temp;
    }
    free(positions);
}

static void print_fixture(const Record *a, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        printf(" (%d,%d)[id=%d]", a[i].key1, a[i].key2,
               a[i].original_index);
    }
    putchar('\n');
}

static void run_stability_verification(void) {
    /* Chosen so conventional selection sort MUST invert equal-key records. */
    const Record input[] = {
        {2, 100, 0}, {2, 200, 1}, {1, 300, 2},
        {3, 400, 3}, {2, 500, 4}, {1, 600, 5}
    };
    enum { N = (int)(sizeof(input) / sizeof(input[0])) };
    Record result[N];
    const char *names[] = {
        "Selection Sort", "Bubble Sort", "Insertion Sort (>)",
        "Insertion Sort (>=)"
    };
    const SortFn fns[] = {
        selection_sort, bubble_sort, insertion_sort_gt, insertion_sort_ge
    };
    printf("\n------------------------------------\n");
    printf("Stability Verification (Key 1 only; id=original input index)\n");
    printf("Original:");
    print_fixture(input, N);
    for (size_t t = 0; t < sizeof(fns) / sizeof(fns[0]); ++t) {
        memcpy(result, input, sizeof(input));
        Metrics m = {0, 0};
        fns[t](result, N, &m);
        const bool stable = verify_stable(result, N);
        const bool correct = verify_sorted_permutation(result, input, N);
        printf("%-24s: %s | Sorted: %s\n",
               names[t], stable ? "STABLE" : "UNSTABLE",
               correct ? "PASS" : "FAIL");
        printf("  Output:");
        print_fixture(result, N);
        if (!correct) exit(EXIT_FAILURE);
        if ((t == 0 || t == 3) ? stable : !stable) {
            fprintf(stderr, "Stability self-test failed!\n");
            exit(EXIT_FAILURE);
        }
    }
}

int main(int argc, char **argv) {
    uint32_t seed = DEFAULT_SEED;
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [nonzero_seed]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2) {
        char *end = NULL;
        const unsigned long input_seed = strtoul(argv[1], &end, 10);
        if (!argv[1][0] || *end != '\0' || !input_seed ||
            input_seed > UINT32_MAX) {
            fprintf(stderr, "Seed must be an integer between 1 and %" PRIu32 ".\n",
                    UINT32_MAX);
            return EXIT_FAILURE;
        }
        seed = (uint32_t)input_seed;
    }
    rng_state = seed;
    Record *original = checked_malloc(DATA_COUNT * sizeof(*original));
    Record *work = checked_malloc(DATA_COUNT * sizeof(*work));
    Record *sorted = checked_malloc(DATA_COUNT * sizeof(*sorted));
    Record *reverse = checked_malloc(DATA_COUNT * sizeof(*reverse));
    Record *partial = checked_malloc(DATA_COUNT * sizeof(*partial));
    Aggregate totals[ALGO_COUNT] = {{0, 0}};

    printf("Data Count : %d\n", DATA_COUNT);
    printf("Random Data Number of Trials : %d\n", DEFAULT_TRIALS);
    printf("Random Seed : %" PRIu32 "\n", seed);
    printf("Movement Definition: one complete Record assignment/copy = 1\n");

    for (int trial = 0; trial < DEFAULT_TRIALS; ++trial) {
        generate_random_data(original, DATA_COUNT);
        for (int alg = 0; alg < ALGO_COUNT; ++alg) {
            memcpy(work, original, DATA_COUNT * sizeof(*work)); /* Excluded. */
            Metrics m = {0, 0};
            SORTS[alg](work, DATA_COUNT, &m);
            if (!verify_sorted_permutation(work, original, DATA_COUNT)) {
                fprintf(stderr, "FAILED: trial %d, algorithm %s\n",
                        trial + 1, ALGO_NAMES[alg]);
                return EXIT_FAILURE;
            }
            totals[alg].total_comparisons += m.comparisons;
            totals[alg].total_movements += m.movements;
        }
        if ((trial + 1) % 10 == 0 || trial == DEFAULT_TRIALS - 1) {
            fprintf(stderr, "Completed random trials: %d/%d (all PASS)\n",
                    trial + 1, DEFAULT_TRIALS);
        }
    }
    printf("\n------------------------------------\nRandom Data (All %d trials PASS)\n", DEFAULT_TRIALS);
    for (int alg = 0; alg < ALGO_COUNT; ++alg) {
        printf("\n%s\n", ALGO_NAMES[alg]);
        printf("  Total Comparisons : %" PRIu64 "\n",
               totals[alg].total_comparisons);
        printf("  Average Comparisons : %.2f\n",
               (double)totals[alg].total_comparisons / DEFAULT_TRIALS);
        printf("  Total Movements : %" PRIu64 "\n",
               totals[alg].total_movements);
        printf("  Average Movements : %.2f\n",
               (double)totals[alg].total_movements / DEFAULT_TRIALS);
    }
    /* The last random trial is the shared source for the remaining states. */
    memcpy(sorted, original, DATA_COUNT * sizeof(*original));
    Metrics discarded = {0, 0};
    insertion_sort_gt(sorted, DATA_COUNT, &discarded); /* Setup, excluded. */
    memcpy(reverse, sorted, DATA_COUNT * sizeof(*reverse));
    prepare_reverse(reverse, DATA_COUNT);
    memcpy(partial, sorted, DATA_COUNT * sizeof(*partial));
    prepare_partially_sorted(partial, DATA_COUNT);

    run_single_scenario("Sorted Data", sorted, original, work, DATA_COUNT);
    run_single_scenario("Reverse-Sorted Data", reverse, original,
                        work, DATA_COUNT);
    run_single_scenario("Partially Sorted Data (500 disjoint swaps / 10% of positions)",
                        partial, original, work, DATA_COUNT);
    run_stability_verification();
    printf("\nAll experiments and sorting/permutation checks: PASS\n");

    free(original);
    free(work);
    free(sorted);
    free(reverse);
    free(partial);
    return EXIT_SUCCESS;
}
