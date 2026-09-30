#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define DATA_SIZE 100
#define SEARCH_SIZE 50
#define MAX_VALUE 1000

typedef struct TreeNode {
    int data;
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;


/* 새로운 노드 생성 */
TreeNode *createNode(int data)
{
    TreeNode *newNode = (TreeNode *)malloc(sizeof(TreeNode));

    if (newNode == NULL) {
        printf("Memory allocation failed.\n");
        exit(1);
    }

    newNode->data = data;
    newNode->left = NULL;
    newNode->right = NULL;

    return newNode;
}


/* BST에 데이터 삽입
   기존 노드와 숫자를 비교할 때마다 comparisonCount 증가 */
void insertBST(TreeNode **root, int data, long long *comparisonCount)
{
    TreeNode *newNode = createNode(data);

    /* 첫 번째 노드는 비교 없이 루트가 된다. */
    if (*root == NULL) {
        *root = newNode;
        return;
    }

    TreeNode *current = *root;
    TreeNode *parent = NULL;

    while (current != NULL) {
        parent = current;

        /*
         * 현재 노드의 값과 삽입할 값을 비교하는 것을
         * 숫자 비교 1회로 계산한다.
         */
        (*comparisonCount)++;

        if (data < current->data) {
            current = current->left;
        }
        else {
            current = current->right;
        }
    }

    if (data < parent->data) {
        parent->left = newNode;
    }
    else {
        parent->right = newNode;
    }
}


/* 순차 탐색 */
int sequentialSearch(int array[], int size, int key, int *comparisonCount)
{
    *comparisonCount = 0;

    for (int i = 0; i < size; i++) {
        (*comparisonCount)++;

        if (array[i] == key) {
            return 1;
        }
    }

    return 0;
}


/* BST 탐색 */
int bstSearch(TreeNode *root, int key, int *comparisonCount)
{
    TreeNode *current = root;

    *comparisonCount = 0;

    while (current != NULL) {

        /*
         * 탐색 대상과 현재 노드 값을 판단하는 것을
         * 숫자 비교 1회로 계산한다.
         */
        (*comparisonCount)++;

        if (key == current->data) {
            return 1;
        }
        else if (key < current->data) {
            current = current->left;
        }
        else {
            current = current->right;
        }
    }

    return 0;
}


/* BST 메모리 해제 */
void freeBST(TreeNode *root)
{
    if (root == NULL)
        return;

    freeBST(root->left);
    freeBST(root->right);

    free(root);
}


/* 배열 출력 */
void printArray(int array[], int size)
{
    for (int i = 0; i < size; i++) {
        printf("%4d", array[i]);

        if ((i + 1) % 10 == 0)
            printf("\n");
    }

    if (size % 10 != 0)
        printf("\n");
}


int main(void)
{
    int data[DATA_SIZE];
    int searchKeys[SEARCH_SIZE];

    /*
     * 0 ~ 1000의 숫자가 이미 생성되었는지 확인하기 위한 배열
     * 0이면 사용하지 않음
     * 1이면 이미 사용함
     */
    int used[MAX_VALUE + 1] = {0};

    TreeNode *root = NULL;

    long long bstBuildComparisons = 0;
    long long sequentialTotal = 0;
    long long bstSearchTotal = 0;

    int successCount = 0;
    int failCount = 0;

    unsigned int seed = (unsigned int)time(NULL);
    srand(seed);

    printf("============================================================\n");
    printf(" Sequential Search vs Binary Search Tree Search\n");
    printf("============================================================\n\n");

    printf("Random Seed : %u\n\n", seed);


    /* ========================================================
       1. 서로 다른 정수 100개 생성
       ======================================================== */

    for (int i = 0; i < DATA_SIZE; i++) {

        int value;

        do {
            value = rand() % (MAX_VALUE + 1);
        } while (used[value]);

        used[value] = 1;

        /*
         * 생성된 순서 그대로 배열에 저장
         * 배열을 정렬하지 않는다.
         */
        data[i] = value;

        /*
         * 같은 순서로 BST에 삽입
         */
        insertBST(&root, value, &bstBuildComparisons);
    }


    printf("[Generated 100 Unique Integers]\n\n");

    printArray(data, DATA_SIZE);

    printf("\n");

    printf("BST Build Total Comparisons : %lld\n",
           bstBuildComparisons);


    /* ========================================================
       2. 탐색 대상 50개 생성
       ======================================================== */

    for (int i = 0; i < SEARCH_SIZE; i++) {
        /*
         * 탐색 대상은 중복되어도 상관없다.
         * 데이터에 존재할 수도 있고 존재하지 않을 수도 있다.
         */
        searchKeys[i] = rand() % (MAX_VALUE + 1);
    }


    printf("\n============================================================\n");
    printf("[Generated 50 Search Keys]\n\n");

    printArray(searchKeys, SEARCH_SIZE);


    /* ========================================================
       3. 탐색 실행
       ======================================================== */

    printf("\n============================================================\n");
    printf("[Search Results]\n");
    printf("============================================================\n\n");

    printf("%-5s %-12s %-10s %-18s %-18s\n",
           "No.",
           "Search Key",
           "Result",
           "Sequential Comp.",
           "BST Comp.");

    printf("---------------------------------------------------------------------\n");


    for (int i = 0; i < SEARCH_SIZE; i++) {

        int seqComparison = 0;
        int bstComparison = 0;

        int seqFound;
        int bstFound;

        seqFound = sequentialSearch(
            data,
            DATA_SIZE,
            searchKeys[i],
            &seqComparison
        );

        bstFound = bstSearch(
            root,
            searchKeys[i],
            &bstComparison
        );

        sequentialTotal += seqComparison;
        bstSearchTotal += bstComparison;

        if (seqFound)
            successCount++;
        else
            failCount++;


        printf("%-5d %-12d %-10s %-18d %-18d\n",
               i + 1,
               searchKeys[i],
               seqFound ? "Found" : "Not Found",
               seqComparison,
               bstComparison);


        /*
         * 배열과 BST에는 동일한 데이터가 저장되어 있으므로
         * 탐색 결과가 서로 다르면 오류
         */
        if (seqFound != bstFound) {
            printf("ERROR: Sequential Search and BST Search "
                   "results are different.\n");
        }
    }


    /* ========================================================
       4. 최종 통계
       ======================================================== */

    double sequentialAverage =
        (double)sequentialTotal / SEARCH_SIZE;

    double bstAverage =
        (double)bstSearchTotal / SEARCH_SIZE;

    long long bstTotalCost =
        bstBuildComparisons + bstSearchTotal;


    printf("\n============================================================\n");
    printf("[Search Statistics]\n");
    printf("============================================================\n\n");

    printf("Number of searches                : %d\n",
           SEARCH_SIZE);

    printf("Successful searches               : %d\n",
           successCount);

    printf("Failed searches                   : %d\n",
           failCount);

    printf("\n");

    printf("Sequential Search Total comparisons : %lld\n",
           sequentialTotal);

    printf("Sequential Search Average           : %.2f\n",
           sequentialAverage);

    printf("\n");

    printf("BST Search Total comparisons        : %lld\n",
           bstSearchTotal);

    printf("BST Search Average                  : %.2f\n",
           bstAverage);

    printf("\n");

    printf("BST Build Total comparisons         : %lld\n",
           bstBuildComparisons);

    printf("BST Build + Search comparisons      : %lld\n",
           bstTotalCost);


    /* ========================================================
       추가 성능 분석
       ======================================================== */

    printf("\n============================================================\n");
    printf("[Performance Analysis]\n");
    printf("============================================================\n\n");

    if (sequentialTotal > 0) {

        double searchReduction =
            (1.0 -
             ((double)bstSearchTotal /
              (double)sequentialTotal))
            * 100.0;

        printf("BST search comparison reduction : %.2f%%\n",
               searchReduction);
    }


    /*
     * 한 번의 탐색에서 BST가 평균적으로 절약하는 비교 횟수를
     * 이용하여 BST 생성 비용을 회수하기 위해 필요한
     * 대략적인 탐색 횟수를 계산한다.
     */
    double savingPerSearch =
        sequentialAverage - bstAverage;

    if (savingPerSearch > 0) {

        double breakEven =
            (double)bstBuildComparisons / savingPerSearch;

        printf("Estimated break-even searches    : %.2f\n",
               breakEven);
    }
    else {
        printf("BST did not reduce average search comparisons "
               "in this experiment.\n");
    }


    printf("\n");

    if (bstTotalCost < sequentialTotal) {

        printf("Including BST construction cost, BST used fewer "
               "comparisons in this experiment.\n");
    }
    else {

        printf("Including BST construction cost, Sequential Search "
               "used fewer comparisons in this experiment.\n");
    }


    /* 동적 메모리 해제 */
    freeBST(root);

    return 0;
}
