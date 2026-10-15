#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define VERTICES 10
#define EDGES 20
#define MAX_WEIGHT 100
#define INF 1000000000

typedef struct {
    int u;
    int v;
    int weight;
} Edge;

/* No edge is represented by 0. All actual edge weights are positive. */
static int graph[VERTICES][VERTICES];
static Edge edgeList[EDGES];
static int edgeCount = 0;

static void initializeGraph(void) {
    int i, j;
    for (i = 0; i < VERTICES; i++) {
        for (j = 0; j < VERTICES; j++) {
            graph[i][j] = 0;
        }
    }
    edgeCount = 0;
}

/* Returns 1 only when a new, valid, undirected edge is inserted. */
static int addEdge(int u, int v, int weight) {
    int temp;
    if (u == v || graph[u][v] != 0 || edgeCount >= EDGES) {
        return 0;
    }
    if (u > v) {
        temp = u;
        u = v;
        v = temp;
    }

    graph[u][v] = weight;
    graph[v][u] = weight;
    edgeList[edgeCount].u = u;
    edgeList[edgeCount].v = v;
    edgeList[edgeCount].weight = weight;
    edgeCount++;
    return 1;
}

/* A random spanning tree guarantees connectivity; then add more edges. */
static void generateGraph(void) {
    int order[VERTICES];
    int i;
    initializeGraph();

    for (i = 0; i < VERTICES; i++) {
        order[i] = i;
    }

    /* Fisher-Yates shuffle. */
    for (i = VERTICES - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = order[i];
        order[i] = order[j];
        order[j] = temp;
    }

    /* Insert VERTICES - 1 edges to form a spanning tree. */
    for (i = 1; i < VERTICES; i++) {
        int parent = rand() % i;
        int weight = rand() % MAX_WEIGHT + 1;
        addEdge(order[i], order[parent], weight);
    }

    /* Keep adding distinct edges until there are exactly 20. */
    while (edgeCount < EDGES) {
        int u = rand() % VERTICES;
        int v = rand() % VERTICES;
        int weight = rand() % MAX_WEIGHT + 1;
        if (u != v) {
            addEdge(u, v, weight);
        }
    }
}

static int compareEdges(const void *a, const void *b) {
    const Edge *x = (const Edge *)a;
    const Edge *y = (const Edge *)b;
    if (x->u != y->u) {
        return x->u - y->u;
    }
    return x->v - y->v;
}

/* Check the graph specifications and connectivity with BFS. */
static int validateGraph(void) {
    int seen[VERTICES][VERTICES] = {{0}};
    int visited[VERTICES] = {0};
    int queue[VERTICES];
    int front = 0, rear = 0;
    int i, j, countedEdges = 0;

    if (edgeCount != EDGES) {
        return 0;
    }

    for (i = 0; i < VERTICES; i++) {
        if (graph[i][i] != 0) {
            return 0;
        }
        for (j = i + 1; j < VERTICES; j++) {
            if (graph[i][j] != graph[j][i]) {
                return 0;
            }
            if (graph[i][j] != 0) {
                if (graph[i][j] < 1 || graph[i][j] > MAX_WEIGHT) {
                    return 0;
                }
                countedEdges++;
            }
        }
    }
    if (countedEdges != EDGES) {
        return 0;
    }

    for (i = 0; i < EDGES; i++) {
        int u = edgeList[i].u;
        int v = edgeList[i].v;
        int w = edgeList[i].weight;
        if (u < 0 || u >= VERTICES || v < 0 || v >= VERTICES ||
            u >= v || seen[u][v] || w < 1 || w > MAX_WEIGHT ||
            graph[u][v] != w) {
            return 0;
        }
        seen[u][v] = 1;
    }

    visited[0] = 1;
    queue[rear++] = 0;
    while (front < rear) {
        int u = queue[front++];
        for (j = 0; j < VERTICES; j++) {
            if (graph[u][j] != 0 && !visited[j]) {
                visited[j] = 1;
                queue[rear++] = j;
            }
        }
    }
    for (i = 0; i < VERTICES; i++) {
        if (!visited[i]) {
            return 0;
        }
    }
    return 1;
}

/* Floyd-Warshall: next[i][j] records the first hop from i toward j. */
static void floydWarshall(int dist[VERTICES][VERTICES],
                          int next[VERTICES][VERTICES]) {
    int i, j, k;
    for (i = 0; i < VERTICES; i++) {
        for (j = 0; j < VERTICES; j++) {
            if (i == j) {
                dist[i][j] = 0;
                next[i][j] = i;
            } else if (graph[i][j] != 0) {
                dist[i][j] = graph[i][j];
                next[i][j] = j;
            } else {
                dist[i][j] = INF;
                next[i][j] = -1;
            }
        }
    }

    for (k = 0; k < VERTICES; k++) {
        for (i = 0; i < VERTICES; i++) {
            for (j = 0; j < VERTICES; j++) {
                if (dist[i][k] != INF && dist[k][j] != INF &&
                    dist[i][k] + dist[k][j] < dist[i][j]) {
                    dist[i][j] = dist[i][k] + dist[k][j];
                    next[i][j] = next[i][k];
                }
            }
        }
    }
}

/* Returns the number of vertices in the reconstructed path, or -1. */
static int buildPath(int u, int v,
                     int next[VERTICES][VERTICES],
                     int path[VERTICES]) {
    int count = 0;
    int current = u;
    if (next[u][v] == -1) {
        return -1;
    }

    path[count++] = current;
    while (current != v) {
        if (count >= VERTICES) {
            return -1; /* A valid shortest path has no repeated vertices. */
        }
        current = next[current][v];
        if (current < 0 || current >= VERTICES) {
            return -1;
        }
        path[count++] = current;
    }
    return count;
}

static void printGraph(void) {
    int i;
    qsort(edgeList, edgeCount, sizeof(Edge), compareEdges);
    printf("==================================================\n");
    printf(" Weighted Undirected Graph - All-Pairs Shortest Paths\n");
    printf("==================================================\n");
    printf("Graph Vertices : %d\n", VERTICES);
    printf("Edges          : %d\n", edgeCount);
    printf("Connected      : Yes\n\n");
    printf("[Generated Edges]\n");
    printf("Edge       Weight\n");
    for (i = 0; i < edgeCount; i++) {
        printf("(%d, %d)     %3d\n",
               edgeList[i].u, edgeList[i].v, edgeList[i].weight);
    }
}

/* Prints the 45 unordered vertex pairs, checking each path's cost. */
static int printShortestPaths(int dist[VERTICES][VERTICES],
                              int next[VERTICES][VERTICES]) {
    int u, v, count = 0;
    for (u = 0; u < VERTICES; u++) {
        for (v = u + 1; v < VERTICES; v++) {
            int path[VERTICES];
            int size = buildPath(u, v, next, path);
            int i, sum = 0;
            if (size < 2 || dist[u][v] == INF) {
                fprintf(stderr, "ERROR: Missing path (%d, %d).\n", u, v);
                return 0;
            }
            for (i = 0; i < size - 1; i++) {
                int w = graph[path[i]][path[i + 1]];
                if (w == 0) {
                    fprintf(stderr, "ERROR: Invalid path (%d, %d).\n", u, v);
                    return 0;
                }
                sum += w;
            }
            if (sum != dist[u][v]) {
                fprintf(stderr, "ERROR: Distance mismatch (%d, %d).\n", u, v);
                return 0;
            }

            printf("\n%d -> %d\n", u, v);
            printf("Path     : ");
            for (i = 0; i < size; i++) {
                if (i > 0) {
                    printf(" -> ");
                }
                printf("%d", path[i]);
            }
            printf("\nDistance : %d\n", dist[u][v]);
            count++;
        }
    }
    printf("\nTotal vertex pairs : %d\n", count);
    printf("Path verification  : PASS (all path weights match)\n");
    return count == VERTICES * (VERTICES - 1) / 2;
}

int main(int argc, char *argv[]) {
    unsigned int seed = (unsigned int)time(NULL);
    int dist[VERTICES][VERTICES];
    int next[VERTICES][VERTICES];

    /* Optional fixed seed, e.g., ./main 20261015 */
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [unsigned_integer_seed]\n", argv[0]);
        return EXIT_FAILURE;
    }
    if (argc == 2) {
        char *end;
        unsigned long parsed = strtoul(argv[1], &end, 10);
        if (argv[1][0] == '\0' || argv[1][0] == '-' ||
            *end != '\0' || parsed > 4294967295UL) {
            fprintf(stderr, "Usage: %s [unsigned_integer_seed]\n", argv[0]);
            return EXIT_FAILURE;
        }
        seed = (unsigned int)parsed;
    }
    srand(seed);
    generateGraph();
    if (!validateGraph()) {
        fprintf(stderr, "ERROR: Graph validation failed.\n");
        return EXIT_FAILURE;
    }
    printf("Random Seed    : %u\n", seed);
    printGraph();
    floydWarshall(dist, next);
    printf("\n[Shortest Paths]\n");
    if (!printShortestPaths(dist, next)) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
