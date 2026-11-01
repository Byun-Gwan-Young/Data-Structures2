#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RANDOM_SEED UINT32_C(20261101)
#define RANDOM_MAX_VALUE UINT32_C(1000000)

static uint32_t rng_state = RANDOM_SEED;

/* Fixed seed and xorshift32 make the experiment reproducible. */
static uint32_t next_random(void) {
    rng_state ^= rng_state << 13;
    rng_state ^= rng_state >> 17;
    rng_state ^= rng_state << 5;
    return rng_state;
}

static void generate_data(int *data, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        data[i] = (int)(next_random() % (RANDOM_MAX_VALUE + 1U));
    }
}

/* Count only comparisons between array values, not index/bound checks. */
static uint64_t insertion_sort(int *data, size_t n) {
    uint64_t comparisons = 0;
    for (size_t i = 1; i < n; ++i) {
        int key = data[i];
        size_t j = i;
        while (j > 0) {
            ++comparisons; /* data[j - 1] > key */
            if (data[j - 1] > key) {
                data[j] = data[j - 1];
                --j;
            } else {
                break;
            }
        }
        data[j] = key;
    }
    return comparisons;
}

/* A single gapped insertion-sort pass. */
static uint64_t gap_insertion_pass(int *data, size_t n, size_t gap) {
    uint64_t comparisons = 0;
    for (size_t i = gap; i < n; ++i) {
        int key = data[i];
        size_t j = i;
        while (j >= gap) {
            ++comparisons; /* data[j - gap] > key */
            if (data[j - gap] > key) {
                data[j] = data[j - gap];
                j -= gap;
            } else {
                break;
            }
        }
        data[j] = key;
    }
    return comparisons;
}

/* Shell's original sequence: n/2, n/4, ..., 1. */
static uint64_t shell_sort_original(int *data, size_t n) {
    uint64_t comparisons = 0;
    for (size_t gap = n / 2; gap > 0; gap /= 2) {
        comparisons += gap_insertion_pass(data, n, gap);
    }
    return comparisons;
}

/* Knuth sequence: 1, 4, 13, 40, ...; apply gaps in descending order. */
static uint64_t shell_sort_knuth(int *data, size_t n) {
    uint64_t comparisons = 0;
    size_t gap = 1;
    if (n < 2) {
        return 0;
    }

    /* Only generate gaps strictly smaller than n; avoid overflow. */
    while (gap <= (n - 2) / 3) {
        gap = 3 * gap + 1;
    }
    while (gap > 0) {
        comparisons += gap_insertion_pass(data, n, gap);
        gap = (gap - 1) / 3;
    }
    return comparisons;
}

static int int_compare(const void *a, const void *b) {
    const int left = *(const int *)a;
    const int right = *(const int *)b;
    return (left > right) - (left < right);
}

static int is_sorted_ascending(const int *data, size_t n) {
    for (size_t i = 1; i < n; ++i) {
        if (data[i - 1] > data[i]) {
            return 0;
        }
    }
    return 1;
}

static int verify_result(const int *result, const int *expected, size_t n) {
    return is_sorted_ascending(result, n) &&
           memcmp(result, expected, n * sizeof(*result)) == 0;
}

int main(void) {
    const size_t sizes[] = {1000, 2000, 4000, 8000, 16000, 20000};
    const size_t case_count = sizeof(sizes) / sizeof(sizes[0]);

    for (size_t k = 0; k < case_count; ++k) {
        const size_t n = sizes[k];
        const size_t bytes = n * sizeof(int);
        int *original = malloc(bytes);
        int *insertion = malloc(bytes);
        int *shell_original = malloc(bytes);
        int *shell_knuth = malloc(bytes);
        int *reference = malloc(bytes);

        if (!original || !insertion || !shell_original ||
            !shell_knuth || !reference) {
            fprintf(stderr, "Error: failed to allocate arrays for n=%zu.\n", n);
            free(original);
            free(insertion);
            free(shell_original);
            free(shell_knuth);
            free(reference);
            return EXIT_FAILURE;
        }

        generate_data(original, n);
        memcpy(insertion, original, bytes);
        memcpy(shell_original, original, bytes);
        memcpy(shell_knuth, original, bytes);
        memcpy(reference, original, bytes);

        const uint64_t insertion_count = insertion_sort(insertion, n);
        const uint64_t original_count = shell_sort_original(shell_original, n);
        const uint64_t knuth_count = shell_sort_knuth(shell_knuth, n);

        /* Reference sorting and verification are excluded from measured counts. */
        qsort(reference, n, sizeof(*reference), int_compare);
        if (!verify_result(insertion, reference, n) ||
            !verify_result(shell_original, reference, n) ||
            !verify_result(shell_knuth, reference, n)) {
            fprintf(stderr, "Error: sorting verification failed for n=%zu.\n", n);
            free(original);
            free(insertion);
            free(shell_original);
            free(shell_knuth);
            free(reference);
            return EXIT_FAILURE;
        }

        printf("Data Size : %zu\n\n", n);
        printf("Insertion Sort       : %" PRIu64 " comparisons\n", insertion_count);
        printf("Shell Sort (Original): %" PRIu64 " comparisons\n", original_count);
        printf("Shell Sort (Knuth)   : %" PRIu64 " comparisons\n", knuth_count);
        if (k + 1 < case_count) {
            putchar('\n');
        }

        free(original);
        free(insertion);
        free(shell_original);
        free(shell_knuth);
        free(reference);
    }
    return EXIT_SUCCESS;
}
