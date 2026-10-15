/*
 * Assignment 08 - Minimum Spanning Tree Algorithm Comparison
 * C11 / GCC or Clang (macOS, Linux, Windows with MinGW-w64)
 *
 * Four cases (all graphs connected, simple, weighted, undirected):
 * 1. Sparse (V=50, E=100): adjacency list   + Kruskal
 * 2. Dense  (V=50, E=600): adjacency list   + Kruskal
 * 3. Sparse (V=50, E=100): adjacency matrix + Prim
 * 4. Dense  (V=50, E=600): adjacency matrix + Prim
 */

#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

#define VERTICES 50
#define SPARSE_EDGES 100
#define DENSE_EDGES 600
#define MAX_WEIGHT 1000
#define MST_EDGE_COUNT (VERTICES - 1)
#define BENCHMARK_REPEATS 1000

typedef struct {
    int u;
    int v;
    int weight;
} Edge;

typedef struct AdjNode {
    int to;
    int weight;
    struct AdjNode *next;
} AdjNode;

typedef struct {
    AdjNode *heads[VERTICES];
    int edge_count;
} ListGraph;

typedef struct {
    int weights[VERTICES][VERTICES]; /* 0 indicates no edge: weights are positive. */
    int edge_count;
} MatrixGraph;

typedef struct {
    int parent[VERTICES];
    int rank[VERTICES];
} DisjointSet;

typedef struct {
    Edge edges[MST_EDGE_COUNT];
    int edge_count;
    long long total_cost;
} MSTResult;

typedef struct {
    MSTResult mst;
    size_t memory_bytes;
    double execution_ms;
} CaseResult;

/* Makes results observable so the compiler cannot discard repeated MST work. */
static volatile long long benchmark_sink = 0;

static double clock_milliseconds(void) {
#ifdef _WIN32
    LARGE_INTEGER counter;
    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart * 1000.0 / (double)frequency.QuadPart;
#else
    struct timespec timestamp;
    if (clock_gettime(CLOCK_MONOTONIC, &timestamp) != 0) {
        fprintf(stderr, "Error: clock_gettime failed.\n");
        exit(EXIT_FAILURE);
    }
    return (double)timestamp.tv_sec * 1000.0
         + (double)timestamp.tv_nsec / 1000000.0;
#endif
}

static void dsu_init(DisjointSet *dsu) {
    for (int i = 0; i < VERTICES; ++i) {
        dsu->parent[i] = i;
        dsu->rank[i] = 0;
    }
}

static int dsu_find(DisjointSet *dsu, int x) {
    if (dsu->parent[x] != x) {
        dsu->parent[x] = dsu_find(dsu, dsu->parent[x]);
    }
    return dsu->parent[x];
}

/* Returns false when the vertices were already in the same component. */
static bool dsu_union(DisjointSet *dsu, int x, int y) {
    int rx = dsu_find(dsu, x);
    int ry = dsu_find(dsu, y);
    if (rx == ry) return false;
    if (dsu->rank[rx] < dsu->rank[ry]) {
        dsu->parent[rx] = ry;
    } else if (dsu->rank[rx] > dsu->rank[ry]) {
        dsu->parent[ry] = rx;
    } else {
        dsu->parent[ry] = rx;
        ++dsu->rank[rx];
    }
    return true;
}

/* Spanning-tree-first generation GUARANTEES connectivity. */
static void generate_connected_graph(Edge *edges, int edge_count) {
    unsigned char exists[VERTICES][VERTICES] = {{0}};
    int n = 0;

    /* The first V-1 edges connect each vertex i to an earlier vertex. */
    for (int i = 1; i < VERTICES; ++i) {
        int u = rand() % i;
        int v = i;
        exists[u][v] = exists[v][u] = 1;
        edges[n++] = (Edge){u, v, 1 + rand() % MAX_WEIGHT};
    }

    /* Remaining random edges have neither self-loops nor duplicates. */
    while (n < edge_count) {
        int u = rand() % VERTICES;
        int v = rand() % VERTICES;
        if (u == v || exists[u][v]) continue;
        if (u > v) { int temp = u; u = v; v = temp; }
        exists[u][v] = exists[v][u] = 1;
        edges[n++] = (Edge){u, v, 1 + rand() % MAX_WEIGHT};
    }
}

/* Explicit validation of graph constraints, including connectivity. */
static bool validate_graph(const Edge *edges, int edge_count) {
    if (edge_count < MST_EDGE_COUNT ||
        edge_count > VERTICES * (VERTICES - 1) / 2) return false;

    unsigned char seen[VERTICES][VERTICES] = {{0}};
    DisjointSet dsu;
    dsu_init(&dsu);

    for (int i = 0; i < edge_count; ++i) {
        int u = edges[i].u;
        int v = edges[i].v;
        int w = edges[i].weight;
        if (u < 0 || u >= VERTICES || v < 0 || v >= VERTICES ||
            u == v || w <= 0 || seen[u][v]) return false;
        seen[u][v] = seen[v][u] = 1;
        (void)dsu_union(&dsu, u, v);
    }

    int root = dsu_find(&dsu, 0);
    for (int v = 1; v < VERTICES; ++v) {
        if (dsu_find(&dsu, v) != root) return false;
    }
    return true;
}

static void list_free(ListGraph *graph) {
    for (int i = 0; i < VERTICES; ++i) {
        AdjNode *cur = graph->heads[i];
        while (cur != NULL) {
            AdjNode *next = cur->next;
            free(cur);
            cur = next;
        }
        graph->heads[i] = NULL;
    }
    graph->edge_count = 0;
}

static bool list_initialize(ListGraph *graph, const Edge *edges, int edge_count) {
    memset(graph, 0, sizeof(*graph));
    for (int i = 0; i < edge_count; ++i) {
        int u = edges[i].u;
        int v = edges[i].v;
        int w = edges[i].weight;
        AdjNode *uv = (AdjNode *)malloc(sizeof(*uv));
        AdjNode *vu = (AdjNode *)malloc(sizeof(*vu));
        if (uv == NULL || vu == NULL) {
            free(uv);
            free(vu);
            list_free(graph);
            return false;
        }
        *uv = (AdjNode){v, w, graph->heads[u]};
        *vu = (AdjNode){u, w, graph->heads[v]};
        graph->heads[u] = uv;
        graph->heads[v] = vu;
    }
    graph->edge_count = edge_count;
    return true;
}

static void matrix_initialize(MatrixGraph *graph, const Edge *edges, int edge_count) {
    memset(graph, 0, sizeof(*graph));
    for (int i = 0; i < edge_count; ++i) {
        int u = edges[i].u;
        int v = edges[i].v;
        graph->weights[u][v] = graph->weights[v][u] = edges[i].weight;
    }
    graph->edge_count = edge_count;
}

static int compare_edges(const void *left, const void *right) {
    const Edge *a = (const Edge *)left;
    const Edge *b = (const Edge *)right;
    if (a->weight != b->weight) return (a->weight > b->weight) - (a->weight < b->weight);
    if (a->u != b->u) return (a->u > b->u) - (a->u < b->u);
    return (a->v > b->v) - (a->v < b->v);
}

/* Includes list edge extraction, qsort, Union-Find, MST construction. */
static bool kruskal(const ListGraph *graph, MSTResult *out) {
    Edge *sorted = (Edge *)malloc((size_t)graph->edge_count * sizeof(*sorted));
    if (sorted == NULL) return false;

    int n = 0;
    for (int u = 0; u < VERTICES; ++u) {
        for (AdjNode *node = graph->heads[u]; node != NULL; node = node->next) {
            if (u < node->to) {
                if (n >= graph->edge_count) { free(sorted); return false; }
                sorted[n++] = (Edge){u, node->to, node->weight};
            }
        }
    }
    if (n != graph->edge_count) { free(sorted); return false; }

    qsort(sorted, (size_t)n, sizeof(*sorted), compare_edges);
    DisjointSet dsu;
    dsu_init(&dsu);
    out->edge_count = 0;
    out->total_cost = 0;

    for (int i = 0; i < n && out->edge_count < MST_EDGE_COUNT; ++i) {
        if (dsu_union(&dsu, sorted[i].u, sorted[i].v)) {
            out->edges[out->edge_count++] = sorted[i];
            out->total_cost += sorted[i].weight;
        }
    }
    free(sorted);
    return out->edge_count == MST_EDGE_COUNT;
}

/* O(V^2) Prim: scan matrix using minimum key, parent and visited arrays. */
static bool prim(const MatrixGraph *graph, MSTResult *out) {
    int key[VERTICES];
    int parent[VERTICES];
    unsigned char visited[VERTICES] = {0};
    out->edge_count = 0;
    out->total_cost = 0;

    for (int v = 0; v < VERTICES; ++v) {
        key[v] = INT_MAX;
        parent[v] = -1;
    }
    key[0] = 0;

    for (int step = 0; step < VERTICES; ++step) {
        int u = -1;
        for (int v = 0; v < VERTICES; ++v) {
            if (!visited[v] && (u == -1 || key[v] < key[u])) u = v;
        }
        if (u == -1 || key[u] == INT_MAX) return false;
        visited[u] = 1;

        if (parent[u] != -1) {
            int p = parent[u];
            out->edges[out->edge_count++] = (Edge){p, u, graph->weights[p][u]};
            out->total_cost += graph->weights[p][u];
        }
        for (int v = 0; v < VERTICES; ++v) {
            int weight = graph->weights[u][v];
            if (!visited[v] && weight > 0 && weight < key[v]) {
                key[v] = weight;
                parent[v] = u;
            }
        }
    }
    return out->edge_count == MST_EDGE_COUNT;
}

/* Independent result checks: real graph edges, no cycles, connectivity, cost. */
static bool validate_mst(const MSTResult *result, const Edge *source, int count) {
    if (result->edge_count != MST_EDGE_COUNT) return false;
    DisjointSet dsu;
    dsu_init(&dsu);
    long long total = 0;

    for (int i = 0; i < result->edge_count; ++i) {
        Edge selected = result->edges[i];
        bool exists = false;
        for (int j = 0; j < count; ++j) {
            Edge e = source[j];
            if (e.weight == selected.weight &&
                ((e.u == selected.u && e.v == selected.v) ||
                 (e.u == selected.v && e.v == selected.u))) {
                exists = true;
                break;
            }
        }
        if (!exists || !dsu_union(&dsu, selected.u, selected.v)) return false;
        total += selected.weight;
    }
    if (total != result->total_cost) return false;
    int root = dsu_find(&dsu, 0);
    for (int v = 1; v < VERTICES; ++v) {
        if (dsu_find(&dsu, v) != root) return false;
    }
    return true;
}

/* Memory includes representation, all explicitly allocated MST structures,
 * and the MSTResult output buffer. It excludes graph generation, printing,
 * allocator metadata and library-internal qsort stack usage.
 */
static size_t list_kruskal_memory(int edge_count) {
    return sizeof(ListGraph)
         + (size_t)(2 * edge_count) * sizeof(AdjNode)
         + (size_t)edge_count * sizeof(Edge)
         + sizeof(DisjointSet)
         + sizeof(MSTResult);
}

static size_t matrix_prim_memory(void) {
    return sizeof(MatrixGraph)
         + (size_t)VERTICES * sizeof(int) /* key */
         + (size_t)VERTICES * sizeof(int) /* parent */
         + (size_t)VERTICES * sizeof(unsigned char) /* visited */
         + sizeof(MSTResult);
}

static bool benchmark_kruskal(const Edge *edges, int edge_count, CaseResult *result) {
    ListGraph graph;
    if (!list_initialize(&graph, edges, edge_count)) return false;

    double start = clock_milliseconds();
    for (int i = 0; i < BENCHMARK_REPEATS; ++i) {
        if (!kruskal(&graph, &result->mst)) {
            list_free(&graph);
            return false;
        }
        benchmark_sink += result->mst.total_cost;
    }
    double end = clock_milliseconds();
    result->execution_ms = (end - start) / BENCHMARK_REPEATS;
    result->memory_bytes = list_kruskal_memory(edge_count);
    list_free(&graph);
    return validate_mst(&result->mst, edges, edge_count);
}

static bool benchmark_prim(const Edge *edges, int edge_count, CaseResult *result) {
    MatrixGraph graph;
    matrix_initialize(&graph, edges, edge_count);

    double start = clock_milliseconds();
    for (int i = 0; i < BENCHMARK_REPEATS; ++i) {
        if (!prim(&graph, &result->mst)) return false;
        benchmark_sink += result->mst.total_cost;
    }
    double end = clock_milliseconds();
    result->execution_ms = (end - start) / BENCHMARK_REPEATS;
    result->memory_bytes = matrix_prim_memory();
    return validate_mst(&result->mst, edges, edge_count);
}

static void print_case(int case_number, const char *graph_label,
                       const char *representation, const char *algorithm,
                       int edge_count, const CaseResult *result) {
    printf("\n=============================================================\n");
    printf("Case %d : %s - %s - %s\n", case_number,
           graph_label, representation, algorithm);
    printf("=============================================================\n");
    printf("Vertices       : %d\n", VERTICES);
    printf("Edges          : %d\n", edge_count);
    printf("MST Edges      : %d\n", result->mst.edge_count);
    printf("MST Cost       : %lld\n", result->mst.total_cost);
    printf("Memory         : %zu Bytes\n", result->memory_bytes);
    printf("Execution Time : %.6f ms (mean of %d runs)\n",
           result->execution_ms, BENCHMARK_REPEATS);
    printf("\n[MST Edges: (Vertex A, Vertex B, Weight)]\n");
    for (int i = 0; i < result->mst.edge_count; ++i) {
        Edge e = result->mst.edges[i];
        printf("%2d. (%2d, %2d, %4d)\n", i + 1, e.u, e.v, e.weight);
    }
}

int main(int argc, char **argv) {
    Edge sparse[SPARSE_EDGES];
    Edge dense[DENSE_EDGES];
    CaseResult results[4] = {0};
    unsigned int seed = (unsigned int)time(NULL);
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [unsigned_integer_seed]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2) {
        char *end = NULL;
        errno = 0;
        unsigned long value = strtoul(argv[1], &end, 10);
        if (errno == ERANGE || argv[1][0] == '-' ||
            *argv[1] == '\0' || *end != '\0' || value > UINT_MAX) {
            fprintf(stderr, "Error: seed must be an unsigned integer <= UINT_MAX.\n");
            return EXIT_FAILURE;
        }
        seed = (unsigned int)value;
    }
    srand(seed);

    generate_connected_graph(sparse, SPARSE_EDGES);
    generate_connected_graph(dense, DENSE_EDGES);
    if (!validate_graph(sparse, SPARSE_EDGES) ||
        !validate_graph(dense, DENSE_EDGES)) {
        fprintf(stderr, "Error: input graph constraints or connectivity check failed.\n");
        return EXIT_FAILURE;
    }

    /* The same sparse[] is passed to cases 1 and 3; the same dense[] to 2 and 4. */
    if (!benchmark_kruskal(sparse, SPARSE_EDGES, &results[0]) ||
        !benchmark_kruskal(dense, DENSE_EDGES, &results[1]) ||
        !benchmark_prim(sparse, SPARSE_EDGES, &results[2]) ||
        !benchmark_prim(dense, DENSE_EDGES, &results[3])) {
        fprintf(stderr, "Error: MST construction or validation failed.\n");
        return EXIT_FAILURE;
    }

    if (results[0].mst.total_cost != results[2].mst.total_cost ||
        results[1].mst.total_cost != results[3].mst.total_cost) {
        fprintf(stderr, "Error: Kruskal and Prim MST costs do not match.\n");
        return EXIT_FAILURE;
    }

    printf("=============================================================\n");
    printf(" Assignment 08 - MST Algorithm Comparison\n");
    printf("=============================================================\n");
    printf("Random Seed    : %u\n", seed);
    printf("Graph Check    : PASS (connected, no loops/duplicates, weights > 0)\n");
    printf("Timing         : High-resolution monotonic wall-clock timer\n");
    printf("Benchmark      : %d MST computations per case (average)\n", BENCHMARK_REPEATS);

    print_case(1, "Sparse Graph", "Adjacency List", "Kruskal",
               SPARSE_EDGES, &results[0]);
    print_case(2, "Dense Graph", "Adjacency List", "Kruskal",
               DENSE_EDGES, &results[1]);
    print_case(3, "Sparse Graph", "Adjacency Matrix", "Prim",
               SPARSE_EDGES, &results[2]);
    print_case(4, "Dense Graph", "Adjacency Matrix", "Prim",
               DENSE_EDGES, &results[3]);

    printf("\n=============================================================\n");
    printf("Comparison Summary\n");
    printf("=============================================================\n");
    printf("Case | V  | E   | MST Edges | MST Cost | Memory (Bytes) | Time (ms)\n");
    printf("-----+----+-----+-----------+----------+----------------+----------\n");
    const int edge_counts[4] = {SPARSE_EDGES, DENSE_EDGES, SPARSE_EDGES, DENSE_EDGES};
    for (int i = 0; i < 4; ++i) {
        printf("%4d | %2d | %3d | %9d | %8lld | %14zu | %9.6f\n",
               i + 1, VERTICES, edge_counts[i], results[i].mst.edge_count,
               results[i].mst.total_cost, results[i].memory_bytes,
               results[i].execution_ms);
    }
    printf("\nSparse cost agreement (Case 1 == Case 3): PASS (%lld)\n",
           results[0].mst.total_cost);
    printf("Dense  cost agreement (Case 2 == Case 4): PASS (%lld)\n",
           results[1].mst.total_cost);
    printf("All 4 MSTs contain 49 valid edges: PASS\n");
    return EXIT_SUCCESS;
}
