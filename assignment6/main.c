#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DATA_COUNT 100
#define SEARCH_COUNT 50
#define VALUE_MIN 0
#define VALUE_MAX 1000

typedef struct Node {
    int data;
    int height;              // AVL에서 사용 (노드 수 기준 높이)
    struct Node *left;
    struct Node *right;
} Node;

static Node *createNode(int value) {
    Node *node = (Node *)malloc(sizeof(Node));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        exit(EXIT_FAILURE);
    }

    node->data = value;
    node->height = 1;
    node->left = NULL;
    node->right = NULL;
    return node;
}

static int maxInt(int a, int b) {
    return (a > b) ? a : b;
}

static int nodeHeight(Node *node) {
    return (node == NULL) ? 0 : node->height;
}

static void updateHeight(Node *node) {
    if (node != NULL) {
        node->height = 1 + maxInt(nodeHeight(node->left), nodeHeight(node->right));
    }
}

static int balanceFactor(Node *node) {
    if (node == NULL) {
        return 0;
    }
    return nodeHeight(node->left) - nodeHeight(node->right);
}

static Node *rotateRight(Node *y) {
    Node *x = y->left;
    Node *t2 = x->right;

    x->right = y;
    y->left = t2;

    updateHeight(y);
    updateHeight(x);
    return x;
}

static Node *rotateLeft(Node *x) {
    Node *y = x->right;
    Node *t2 = y->left;

    y->left = x;
    x->right = t2;

    updateHeight(x);
    updateHeight(y);
    return y;
}

/*
 * 배열 삽입
 * - 기존 원소와 새 값의 비교 1회마다 comparisons 증가
 * - 중복이면 삽입하지 않고 0 반환
 * - 새로 삽입하면 1 반환
 */
static int insertArray(int array[], int *size, int value, long long *comparisons) {
    int i;

    for (i = 0; i < *size; ++i) {
        (*comparisons)++;
        if (array[i] == value) {
            return 0;
        }
    }

    array[*size] = value;
    (*size)++;
    return 1;
}

/*
 * BST 삽입
 * 방문한 노드 1개당 숫자 비교 1회로 계산한다.
 */
static Node *insertBST(Node *root, int value, long long *comparisons, int *inserted) {
    Node *parent = NULL;
    Node *current = root;
    int lastDirection = 0; /* -1: left, 1: right */

    if (root == NULL) {
        *inserted = 1;
        return createNode(value);
    }

    while (current != NULL) {
        parent = current;
        (*comparisons)++;

        if (value == current->data) {
            *inserted = 0;
            return root;
        }

        if (value < current->data) {
            lastDirection = -1;
            current = current->left;
        } else {
            lastDirection = 1;
            current = current->right;
        }
    }

    /* 마지막 노드에서 이미 결정한 방향을 재사용하여 추가 숫자 비교를 하지 않는다. */
    if (lastDirection < 0) {
        parent->left = createNode(value);
    } else {
        parent->right = createNode(value);
    }

    *inserted = 1;
    return root;
}

/*
 * AVL 삽입
 * - 삽입 위치/중복 확인을 위해 방문한 노드 수만 comparisons에 포함
 * - 높이 계산, balance factor, 회전 판단 및 회전은 비교 횟수에서 제외
 */
static Node *insertAVL(Node *node, int value, long long *comparisons, int *inserted) {
    int balance;

    if (node == NULL) {
        *inserted = 1;
        return createNode(value);
    }

    (*comparisons)++;

    if (value == node->data) {
        *inserted = 0;
        return node;
    }

    if (value < node->data) {
        node->left = insertAVL(node->left, value, comparisons, inserted);
    } else {
        node->right = insertAVL(node->right, value, comparisons, inserted);
    }

    /* 중복으로 삽입되지 않은 경우 구조가 변하지 않으므로 그대로 반환 */
    if (!(*inserted)) {
        return node;
    }

    updateHeight(node);
    balance = balanceFactor(node);

    /* LL */
    if (balance > 1 && value < node->left->data) {
        return rotateRight(node);
    }

    /* RR */
    if (balance < -1 && value > node->right->data) {
        return rotateLeft(node);
    }

    /* LR */
    if (balance > 1 && value > node->left->data) {
        node->left = rotateLeft(node->left);
        return rotateRight(node);
    }

    /* RL */
    if (balance < -1 && value < node->right->data) {
        node->right = rotateRight(node->right);
        return rotateLeft(node);
    }

    return node;
}

static int treeHeight(Node *root) {
    int leftHeight;
    int rightHeight;

    if (root == NULL) {
        return 0;
    }

    leftHeight = treeHeight(root->left);
    rightHeight = treeHeight(root->right);
    return 1 + maxInt(leftHeight, rightHeight);
}

static int searchArray(const int array[], int size, int key, int *comparisons) {
    int i;
    *comparisons = 0;

    for (i = 0; i < size; ++i) {
        (*comparisons)++;
        if (array[i] == key) {
            return 1;
        }
    }
    return 0;
}

static int searchTree(Node *root, int key, int *comparisons) {
    Node *current = root;
    *comparisons = 0;

    while (current != NULL) {
        (*comparisons)++;

        if (key == current->data) {
            return 1;
        }

        if (key < current->data) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    return 0;
}

static void freeTree(Node *root) {
    if (root == NULL) {
        return;
    }

    freeTree(root->left);
    freeTree(root->right);
    free(root);
}

static void printNumberList(const int values[], int count) {
    int i;

    for (i = 0; i < count; ++i) {
        printf("%4d", values[i]);
        if ((i + 1) % 10 == 0 || i == count - 1) {
            printf("\n");
        }
    }
}

int main(int argc, char *argv[]) {
    int generated[DATA_COUNT];
    int searchKeys[SEARCH_COUNT];
    int array[DATA_COUNT];
    int arraySize = 0;
    int uniqueCount = 0;
    int duplicateCount;
    int i;

    Node *bstRoot = NULL;
    Node *avlRoot = NULL;

    long long arrayBuildComparisons = 0;
    long long bstBuildComparisons = 0;
    long long avlBuildComparisons = 0;

    long long arraySearchTotal = 0;
    long long bstSearchTotal = 0;
    long long avlSearchTotal = 0;

    unsigned int seed;

    if (argc >= 2) {
        seed = (unsigned int)strtoul(argv[1], NULL, 10);
    } else {
        seed = (unsigned int)time(NULL);
    }
    srand(seed);

    for (i = 0; i < DATA_COUNT; ++i) {
        generated[i] = VALUE_MIN + rand() % (VALUE_MAX - VALUE_MIN + 1);
    }

    for (i = 0; i < DATA_COUNT; ++i) {
        int insertedArray;
        int insertedBST = 0;
        int insertedAVL = 0;

        insertedArray = insertArray(array, &arraySize, generated[i], &arrayBuildComparisons);
        bstRoot = insertBST(bstRoot, generated[i], &bstBuildComparisons, &insertedBST);
        avlRoot = insertAVL(avlRoot, generated[i], &avlBuildComparisons, &insertedAVL);

        /* 세 자료구조가 항상 동일한 서로 다른 값을 가져야 함 */
        if (insertedArray != insertedBST || insertedArray != insertedAVL) {
            fprintf(stderr, "Internal error: insertion result mismatch.\n");
            freeTree(bstRoot);
            freeTree(avlRoot);
            return EXIT_FAILURE;
        }

        if (insertedArray) {
            uniqueCount++;
        }
    }

    duplicateCount = DATA_COUNT - uniqueCount;

    printf("============================================================\n");
    printf(" Sequential Search vs BST vs AVL Tree Performance Comparison\n");
    printf("============================================================\n\n");
    printf("Random Seed : %u\n\n", seed);

    printf("[Generated %d Integers]\n", DATA_COUNT);
    printNumberList(generated, DATA_COUNT);

    printf("\n[Construction Summary]\n");
    printf("Stored values                       : %d\n", uniqueCount);
    printf("Duplicate values                    : %d\n", duplicateCount);
    printf("Array construction comparisons      : %lld\n", arrayBuildComparisons);
    printf("BST construction comparisons        : %lld\n", bstBuildComparisons);
    printf("AVL construction comparisons        : %lld\n", avlBuildComparisons);

    printf("\n[Structure]\n");
    printf("Array length                        : %d\n", arraySize);
    printf("BST height                          : %d\n", treeHeight(bstRoot));
    printf("AVL height                          : %d\n", treeHeight(avlRoot));

    for (i = 0; i < SEARCH_COUNT; ++i) {
        searchKeys[i] = VALUE_MIN + rand() % (VALUE_MAX - VALUE_MIN + 1);
    }

    printf("\n[Generated %d Search Keys]\n", SEARCH_COUNT);
    printNumberList(searchKeys, SEARCH_COUNT);

    printf("\n[Search Results]\n");
    printf("--------------------------------------------------------------------------------\n");
    printf(" Key | Result    | Sequential Comparisons | BST Comparisons | AVL Comparisons\n");
    printf("--------------------------------------------------------------------------------\n");

    for (i = 0; i < SEARCH_COUNT; ++i) {
        int arrayComparisons;
        int bstComparisons;
        int avlComparisons;
        int arrayFound;
        int bstFound;
        int avlFound;

        arrayFound = searchArray(array, arraySize, searchKeys[i], &arrayComparisons);
        bstFound = searchTree(bstRoot, searchKeys[i], &bstComparisons);
        avlFound = searchTree(avlRoot, searchKeys[i], &avlComparisons);

        if (arrayFound != bstFound || arrayFound != avlFound) {
            fprintf(stderr, "Internal error: search result mismatch.\n");
            freeTree(bstRoot);
            freeTree(avlRoot);
            return EXIT_FAILURE;
        }

        arraySearchTotal += arrayComparisons;
        bstSearchTotal += bstComparisons;
        avlSearchTotal += avlComparisons;

        printf("%4d | %-9s | %22d | %15d | %15d\n",
               searchKeys[i],
               arrayFound ? "Found" : "Not Found",
               arrayComparisons,
               bstComparisons,
               avlComparisons);
    }

    printf("--------------------------------------------------------------------------------\n");
    printf("\n[Search Summary]\n");
    printf("Searches                            : %d\n", SEARCH_COUNT);
    printf("Sequential Search total comparisons : %lld\n", arraySearchTotal);
    printf("Sequential Search avg comparisons   : %.2f\n",
           (double)arraySearchTotal / SEARCH_COUNT);
    printf("BST Search total comparisons        : %lld\n", bstSearchTotal);
    printf("BST Search avg comparisons          : %.2f\n",
           (double)bstSearchTotal / SEARCH_COUNT);
    printf("AVL Search total comparisons        : %lld\n", avlSearchTotal);
    printf("AVL Search avg comparisons          : %.2f\n",
           (double)avlSearchTotal / SEARCH_COUNT);

    freeTree(bstRoot);
    freeTree(avlRoot);
    return 0;
}
