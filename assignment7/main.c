#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#define VERTICES 100
#define SPARSE_EDGES 100
#define DENSE_EDGES 4000
#define POSSIBLE_EDGES (VERTICES * (VERTICES - 1) / 2)
#define DEFAULT_SEED 20261015u

typedef struct {
    int u;
    int v;
} Edge;

/* An adjacency matrix consists of 100 x 100 integers and two counters. */
typedef struct {
    int vertex_count;
    int edge_count;
    int adj[VERTICES][VERTICES];
} MatrixGraph;

typedef struct Node {
    int vertex;
    struct Node *next;
} Node;

/* For an undirected edge (u, v), the list stores TWO nodes. */
typedef struct {
    int vertex_count;
    int edge_count;
    Node *head[VERTICES];
} ListGraph;

typedef struct {
    size_t memory_bytes;
    int insert_comparisons;
    int delete_comparisons;
    int connect_comparisons;
    int adjacent_comparisons;
    int connected;
    int neighbors[VERTICES];
    int neighbor_count;
} Result;

/* A fixed-seed PRNG gives the same test graph on Windows and macOS. */
static uint32_t random_state = DEFAULT_SEED;

static uint32_t next_random(void) {
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;
    return random_state;
}

/* Shuffle all 4,950 possible edges; the first edge_count are unique. */
static void generate_edges(Edge edges[POSSIBLE_EDGES]) {
    int i = 0;
    int u, v;

    for (u = 0; u < VERTICES; u++) {
        for (v = u + 1; v < VERTICES; v++) {
            edges[i].u = u;
            edges[i].v = v;
            i++;
        }
    }

    for (i = POSSIBLE_EDGES - 1; i > 0; i--) {
        int j = (int)(next_random() % (uint32_t)(i + 1));
        Edge temp = edges[i];
        edges[i] = edges[j];
        edges[j] = temp;
    }

}

static Node *new_node(int vertex, Node *next) {
    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }
    node->vertex = vertex;
    node->next = next;
    return node;
}

static void init_matrix(MatrixGraph *graph) {
    int u, v;
    graph->vertex_count = VERTICES;
    graph->edge_count = 0;
    for (u = 0; u < VERTICES; u++) {
        for (v = 0; v < VERTICES; v++) {
            graph->adj[u][v] = 0;
        }
    }
}

static void init_list(ListGraph *graph) {
    int u;
    graph->vertex_count = VERTICES;
    graph->edge_count = 0;
    for (u = 0; u < VERTICES; u++) {
        graph->head[u] = NULL;
    }
}

/* Build-time insertions are NOT part of the four measured operations. */
static void build_matrix(MatrixGraph *graph, const Edge *edges, int count) {
    int i;
    init_matrix(graph);
    for (i = 0; i < count; i++) {
        int u = edges[i].u;
        int v = edges[i].v;
        graph->adj[u][v] = 1;
        graph->adj[v][u] = 1;
        graph->edge_count++;
    }
}

static void add_list_edge(ListGraph *graph, int u, int v) {
    graph->head[u] = new_node(v, graph->head[u]);
    graph->head[v] = new_node(u, graph->head[v]);
    graph->edge_count++;
}

static void build_list(ListGraph *graph, const Edge *edges, int count) {
    int i;
    init_list(graph);
    for (i = 0; i < count; i++) {
        add_list_edge(graph, edges[i].u, edges[i].v);
    }
}

/* One matrix element is checked for insertion or deletion. */
static int matrix_insert(MatrixGraph *graph, int u, int v, int *comparisons) {
    (*comparisons)++;
    if (graph->adj[u][v] != 0) return 0;
    graph->adj[u][v] = 1;
    graph->adj[v][u] = 1;
    graph->edge_count++;
    return 1;
}

static int matrix_delete(MatrixGraph *graph, int u, int v, int *comparisons) {
    (*comparisons)++;
    if (graph->adj[u][v] == 0) return 0;
    graph->adj[u][v] = 0;
    graph->adj[v][u] = 0;
    graph->edge_count--;
    return 1;
}

/* To avoid duplicate edges, check each node in u's list first. */
static int list_insert(ListGraph *graph, int u, int v, int *comparisons) {
    Node *cur = graph->head[u];
    while (cur != NULL) {
        (*comparisons)++;
        if (cur->vertex == v) return 0;
        cur = cur->next;
    }
    /* Adding nodes to the heads needs no extra element comparisons. */
    add_list_edge(graph, u, v);
    return 1;
}

/* Find and unlink one neighbor; count each node examined. */
static int remove_neighbor(Node **head, int target, int *comparisons) {
    Node *prev = NULL;
    Node *cur = *head;
    while (cur != NULL) {
        (*comparisons)++;
        if (cur->vertex == target) {
            if (prev == NULL) *head = cur->next;
            else prev->next = cur->next;
            free(cur);
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

static int list_delete(ListGraph *graph, int u, int v, int *comparisons) {
    int first = remove_neighbor(&graph->head[u], v, comparisons);
    int second = remove_neighbor(&graph->head[v], u, comparisons);
    if (first != second) {
        fprintf(stderr, "Error: inconsistent adjacency list.\n");
        exit(EXIT_FAILURE);
    }
    if (!first) return 0;
    graph->edge_count--;
    return 1;
}

static int matrix_connected(const MatrixGraph *graph, int u, int v,
                            int *comparisons) {
    (*comparisons)++;
    return graph->adj[u][v] != 0;
}

static int list_connected(const ListGraph *graph, int u, int v,
                          int *comparisons) {
    Node *cur = graph->head[u];
    while (cur != NULL) {
        (*comparisons)++;
        if (cur->vertex == v) return 1;
        cur = cur->next;
    }
    return 0;
}

static int matrix_neighbors(const MatrixGraph *graph, int u,
                            int neighbors[VERTICES], int *comparisons) {
    int v, count = 0;
    for (v = 0; v < VERTICES; v++) {
        (*comparisons)++;
        if (graph->adj[u][v] != 0) neighbors[count++] = v;
    }
    return count;
}

static int list_neighbors(const ListGraph *graph, int u,
                          int neighbors[VERTICES], int *comparisons) {
    const Node *cur = graph->head[u];
    int count = 0;
    while (cur != NULL) {
        (*comparisons)++;
        neighbors[count++] = cur->vertex;
        cur = cur->next;
    }
    return count;
}

static int int_compare(const void *a, const void *b) {
    int x = *(const int *)a;
    int y = *(const int *)b;
    return (x > y) - (x < y);
}

/* Memory counts only the graph representation and its own management data. */
static size_t matrix_memory(void) {
    return sizeof(MatrixGraph);
}

static size_t list_memory(const ListGraph *graph) {
    return sizeof(ListGraph) +
           (size_t)(2 * graph->edge_count) * sizeof(Node);
}

/* Verify equal edges and edge counts; verification is outside measurements. */
static int verify_same_graph(const MatrixGraph *matrix, const ListGraph *list) {
    int u, v, directed_count = 0;
    if (matrix->edge_count != list->edge_count) return 0;
    for (u = 0; u < VERTICES; u++) {
        int seen[VERTICES] = {0};
        const Node *cur = list->head[u];
        while (cur != NULL) {
            if (cur->vertex < 0 || cur->vertex >= VERTICES ||
                cur->vertex == u || seen[cur->vertex]) return 0;
            seen[cur->vertex] = 1;
            directed_count++;
            cur = cur->next;
        }
        for (v = 0; v < VERTICES; v++) {
            if (matrix->adj[u][v] != seen[v]) return 0;
            if (matrix->adj[u][v] != matrix->adj[v][u]) return 0;
        }
    }
    return directed_count == 2 * matrix->edge_count;
}

static void free_list(ListGraph *graph) {
    int u;
    for (u = 0; u < VERTICES; u++) {
        Node *cur = graph->head[u];
        while (cur != NULL) {
            Node *next = cur->next;
            free(cur);
            cur = next;
        }
        graph->head[u] = NULL;
    }
    graph->edge_count = 0;
}

static void print_result(int case_no, const char *density,
                         const char *representation, const Result *result,
                         Edge insert_edge, Edge delete_edge,
                         Edge check_edge, int adjacent_vertex) {
    int i;
    printf("Case %d : %s Graph - %s\n", case_no, density, representation);
    printf("Memory                         : %zu Bytes\n", result->memory_bytes);
    printf("Edge insertion comparisons     : %d\n", result->insert_comparisons);
    printf("Edge deletion comparisons      : %d\n", result->delete_comparisons);
    printf("Connectivity check comparisons : %d\n", result->connect_comparisons);
    printf("Adjacent vertices comparisons  : %d\n", result->adjacent_comparisons);
    printf("Inserted edge                  : (%d, %d)\n", insert_edge.u, insert_edge.v);
    printf("Deleted edge                   : (%d, %d)\n", delete_edge.u, delete_edge.v);
    printf("Connectivity query             : (%d, %d) => %s\n",
           check_edge.u, check_edge.v, result->connected ? "YES" : "NO");
    printf("Adjacent vertices of %d (%d)    : ",
           adjacent_vertex, result->neighbor_count);
    if (result->neighbor_count == 0) printf("(none)");
    for (i = 0; i < result->neighbor_count; i++) {
        if (i > 0) printf(" ");
        printf("%d", result->neighbors[i]);
    }
    printf("\n\n");
}

static void run_experiment(const char *density, int edges_count,
                           int first_case) {
    Edge pool[POSSIBLE_EDGES];
    Edge inserted, deleted, checked;
    int adjacent_vertex;
    MatrixGraph matrix;
    ListGraph list;
    Result matrix_result = {0};
    Result list_result = {0};
    int i;

    generate_edges(pool);
    inserted = pool[edges_count];     /* Not in the original graph. */
    deleted = pool[edges_count / 2]; /* In the original graph. */
    checked = pool[edges_count / 3]; /* Different from deleted. */
    adjacent_vertex = pool[edges_count / 4].u;

    build_matrix(&matrix, pool, edges_count);
    build_list(&list, pool, edges_count);

    if (!verify_same_graph(&matrix, &list)) {
        fprintf(stderr, "Error: initial graphs are not identical.\n");
        free_list(&list);
        exit(EXIT_FAILURE);
    }

    /* Identical operations and identical input vertices for both formats. */
    if (!matrix_insert(&matrix, inserted.u, inserted.v,
                       &matrix_result.insert_comparisons) ||
        !list_insert(&list, inserted.u, inserted.v,
                     &list_result.insert_comparisons)) {
        fprintf(stderr, "Error: insertion failed.\n");
        free_list(&list);
        exit(EXIT_FAILURE);
    }
    if (!matrix_delete(&matrix, deleted.u, deleted.v,
                       &matrix_result.delete_comparisons) ||
        !list_delete(&list, deleted.u, deleted.v,
                     &list_result.delete_comparisons)) {
        fprintf(stderr, "Error: deletion failed.\n");
        free_list(&list);
        exit(EXIT_FAILURE);
    }

    matrix_result.connected = matrix_connected(&matrix, checked.u, checked.v,
                                               &matrix_result.connect_comparisons);
    list_result.connected = list_connected(&list, checked.u, checked.v,
                                           &list_result.connect_comparisons);
    matrix_result.neighbor_count = matrix_neighbors(&matrix, adjacent_vertex,
                                                     matrix_result.neighbors,
                                                     &matrix_result.adjacent_comparisons);
    list_result.neighbor_count = list_neighbors(&list, adjacent_vertex,
                                                 list_result.neighbors,
                                                 &list_result.adjacent_comparisons);

    /* Sorting is only for comparable output, not for locating graph elements. */
    qsort(matrix_result.neighbors, (size_t)matrix_result.neighbor_count,
          sizeof(int), int_compare);
    qsort(list_result.neighbors, (size_t)list_result.neighbor_count,
          sizeof(int), int_compare);

    if (matrix.edge_count != edges_count || list.edge_count != edges_count ||
        !verify_same_graph(&matrix, &list) ||
        matrix_result.connected != list_result.connected ||
        matrix_result.neighbor_count != list_result.neighbor_count) {
        fprintf(stderr, "Error: final graph or result mismatch.\n");
        free_list(&list);
        exit(EXIT_FAILURE);
    }
    for (i = 0; i < matrix_result.neighbor_count; i++) {
        if (matrix_result.neighbors[i] != list_result.neighbors[i]) {
            fprintf(stderr, "Error: adjacent vertices mismatch.\n");
            free_list(&list);
            exit(EXIT_FAILURE);
        }
    }

    matrix_result.memory_bytes = matrix_memory();
    list_result.memory_bytes = list_memory(&list);

    printf("[%s Graph] Vertices: %d, Edges after tests: %d\n",
           density, VERTICES, matrix.edge_count);
    printf("Representation verification: PASS\n\n");
    print_result(first_case, density, "Adjacency Matrix", &matrix_result,
                 inserted, deleted, checked, adjacent_vertex);
    print_result(first_case + 1, density, "Adjacency List", &list_result,
                 inserted, deleted, checked, adjacent_vertex);
    free_list(&list);
}

int main(int argc, char *argv[]) {
    unsigned long seed_value = DEFAULT_SEED;

    if (argc > 2) {
        fprintf(stderr, "Usage: %s [nonzero_seed]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2) {
        char *end;
        errno = 0;
        seed_value = strtoul(argv[1], &end, 10);
        if (errno != 0 || *argv[1] == '\0' || *end != '\0' ||
            seed_value == 0 || seed_value > UINT32_MAX) {
            fprintf(stderr, "Error: seed must be an integer in 1..4294967295.\n");
            return EXIT_FAILURE;
        }
    }
    random_state = (uint32_t)seed_value;

    printf("============================================================\n");
    printf(" Adjacency Matrix vs Adjacency List Performance Comparison\n");
    printf("============================================================\n");
    printf("Random seed: %lu (xorshift32)\n", seed_value);
    printf("Undirected simple graph: 100 vertices, no loops/duplicates\n");
    printf("Comparison: number of graph elements actually inspected\n");
    printf("List memory: sizeof(ListGraph) + 2 * edges * sizeof(Node)\n\n");

    run_experiment("Sparse", SPARSE_EDGES, 1);
    run_experiment("Dense", DENSE_EDGES, 3);
    printf("All four cases completed successfully.\n");
    return EXIT_SUCCESS;
}
