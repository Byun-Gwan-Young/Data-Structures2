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


/* BST 삽입 */
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

    /*
     * 0 : 왼쪽에 삽입
     * 1 : 오른쪽에 삽입
     */
    int direction = 0;

    while (current != NULL) {
        parent = current;

        /*
         * 삽입할 값과 현재 노드의 값을 비교하는 것을
         * 숫자 비교 1회로 계산한다.
         */
        (*comparisonCount)++;

        if (data < current->data) {
            direction = 0;
            current = current->left;
        }
        else {
            direction = 1;
            current = current->right;
        }
    }

    /*
     * while문에서 마지막으로 결정한 방향을 사용하므로
     * 삽입 위치를 결정하기 위한 불필요한 추가 비교가 없다.
     */
    if (direction == 0)
        parent->left = newNode;
    else
        parent->right = newNode;
}


/* 순차 탐색 */
int sequentialSearch(int array[], int size, int key, int *comparisonCount)
{
    *comparisonCount = 0;

    for (int i = 0; i < size; i++) {
        (*comparisonCount)++;

        if (array[i] == key)
            return 1;
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
         * 탐색 대상과 현재 노드의 값을 비교하여
         * 일치 / 왼쪽 / 오른쪽을 결정하는 것을
         * 숫자 비교 1회로 계산한다.
         */
        (*comparisonCount)++;

        if (key == current->data)
            return 1;

        if (key < current->data)
            current = current->left;
        else
            current = current->right;
    }

    return 0;
}


/* BST 높이 계산
   루트 노드의 높이는 1로 계산 */
int getHeight(TreeNode *root)
{
    if (root == NULL)
        return 0;

    int leftHeight = getHeight(root->left);
    int rightHeight = getHeight(root->right);

    if (leftHeight > rightHeight)
        return leftHeight + 1;
    else
        return rightHeight + 1;
}


/* 리프 노드 개수 */
int countLeafNodes(TreeNode *root)
{
    if (root == NULL)
        return 0;

    if (root->left == NULL && root->right == NULL)
        return 1;

    return countLeafNodes(root->left)
         + countLeafNodes(root->right);
}


/* 내부 노드 개수 */
int countInternalNodes(TreeNode *root)
{
    if (root == NULL)
        return 0;

    if (root->left == NULL && root->right == NULL)
        return 0;

    return 1
         + countInternalNodes(root->left)
         + countInternalNodes(root->right);
}


/* 모든 노드 깊이의 합
   루트 깊이는 1로 계산 */
long long getTotalDepth(TreeNode *root, int depth)
{
    if (root == NULL)
        return 0;

    return depth
         + getTotalDepth(root->left, depth + 1)
         + getTotalDepth(root->right, depth + 1);
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

    int used[MAX_VALUE + 1] = {0};

    TreeNode *root = NULL;

    long long bstBuildComparisons = 0;
    long long sequentialTotal = 0;
    long long bstSearchTotal = 0;

    long long sequentialSuccessTotal = 0;
    long long sequentialFailTotal = 0;

    long long bstSuccessTotal = 0;
    long long bstFailTotal = 0;

    int successCount = 0;
    int failCount = 0;

    int sequentialMin = DATA_SIZE + 1;
    int sequentialMax = 0;

    int bstMin = DATA_SIZE + 1;
    int bstMax = 0;

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
         * 발생한 순서 그대로 배열에 저장한다.
         * 배열은 정렬하지 않는다.
         */
        data[i] = value;

        /*
         * 동일한 값을 동일한 순서대로 BST에 삽입한다.
         */
        insertBST(&root, value, &bstBuildComparisons);
    }


    printf("[Generated 100 Unique Integers]\n\n");

    printArray(data, DATA_SIZE);

    printf("\n");

    printf("BST Build Total Comparisons : %lld\n",
           bstBuildComparisons);


    /* ========================================================
       2. BST 구조 분석
       ======================================================== */

    int treeHeight = getHeight(root);
    int leafCount = countLeafNodes(root);
    int internalCount = countInternalNodes(root);

    long long totalDepth = getTotalDepth(root, 1);

    double averageDepth =
        (double)totalDepth / DATA_SIZE;

    double averageBuildComparisons =
        (double)bstBuildComparisons / (DATA_SIZE - 1);


    printf("\n============================================================\n");
    printf("[BST Structure Statistics]\n");
    printf("============================================================\n\n");

    printf("Number of nodes                         : %d\n",
           DATA_SIZE);

    printf("BST Height                              : %d\n",
           treeHeight);

    printf("Leaf Nodes                              : %d\n",
           leafCount);

    printf("Internal Nodes                          : %d\n",
           internalCount);

    printf("Average Node Depth                      : %.2f\n",
           averageDepth);

    printf("Average Build Comparisons per Insertion : %.2f\n",
           averageBuildComparisons);


    /* ========================================================
       3. 탐색 대상 50개 생성
       ======================================================== */

    for (int i = 0; i < SEARCH_SIZE; i++) {
        /*
         * 탐색 대상은 데이터에 존재할 수도 있고
         * 존재하지 않을 수도 있다.
         *
         * 탐색 대상끼리의 중복은 과제에서 제한하지 않았으므로
         * 중복을 허용한다.
         */
        searchKeys[i] = rand() % (MAX_VALUE + 1);
    }


    printf("\n============================================================\n");
    printf("[Generated 50 Search Keys]\n\n");

    printArray(searchKeys, SEARCH_SIZE);


    /* ========================================================
       4. 탐색 수행
       ======================================================== */

    printf("\n============================================================\n");
    printf("[Search Results]\n");
    printf("============================================================\n\n");

    printf("%-4s %-10s %-12s %-10s %-12s %-10s\n",
           "No.",
           "Key",
           "Seq Result",
           "Seq Comp.",
           "BST Result",
           "BST Comp.");

    printf("--------------------------------------------------------------------\n");


    for (int i = 0; i < SEARCH_SIZE; i++) {

        int sequentialComparison = 0;
        int bstComparison = 0;

        int sequentialFound;
        int bstFound;


        sequentialFound =
            sequentialSearch(
                data,
                DATA_SIZE,
                searchKeys[i],
                &sequentialComparison
            );


        bstFound =
            bstSearch(
                root,
                searchKeys[i],
                &bstComparison
            );


        sequentialTotal += sequentialComparison;
        bstSearchTotal += bstComparison;


        /* 최소 비교 횟수 */
        if (sequentialComparison < sequentialMin)
            sequentialMin = sequentialComparison;

        if (bstComparison < bstMin)
            bstMin = bstComparison;


        /* 최대 비교 횟수 */
        if (sequentialComparison > sequentialMax)
            sequentialMax = sequentialComparison;

        if (bstComparison > bstMax)
            bstMax = bstComparison;


        /*
         * 성공/실패별 통계
         */
        if (sequentialFound) {
            successCount++;

            sequentialSuccessTotal += sequentialComparison;
            bstSuccessTotal += bstComparison;
        }
        else {
            failCount++;

            sequentialFailTotal += sequentialComparison;
            bstFailTotal += bstComparison;
        }


        printf("%-4d %-10d %-12s %-10d %-12s %-10d\n",
               i + 1,
               searchKeys[i],
               sequentialFound ? "Found" : "Not Found",
               sequentialComparison,
               bstFound ? "Found" : "Not Found",
               bstComparison);


        /*
         * 배열과 BST에는 동일한 데이터가 저장되어 있으므로
         * 결과가 서로 다르면 프로그램 오류이다.
         */
        if (sequentialFound != bstFound) {
            printf("ERROR: Search results are different.\n");
        }
    }


    /* ========================================================
       5. 기본 통계 계산
       ======================================================== */

    double sequentialAverage =
        (double)sequentialTotal / SEARCH_SIZE;

    double bstAverage =
        (double)bstSearchTotal / SEARCH_SIZE;


    double sequentialSuccessAverage = 0.0;
    double bstSuccessAverage = 0.0;

    double sequentialFailAverage = 0.0;
    double bstFailAverage = 0.0;


    if (successCount > 0) {
        sequentialSuccessAverage =
            (double)sequentialSuccessTotal / successCount;

        bstSuccessAverage =
            (double)bstSuccessTotal / successCount;
    }


    if (failCount > 0) {
        sequentialFailAverage =
            (double)sequentialFailTotal / failCount;

        bstFailAverage =
            (double)bstFailTotal / failCount;
    }


    long long bstTotalCost =
        bstBuildComparisons + bstSearchTotal;


    printf("\n============================================================\n");
    printf("[Search Statistics]\n");
    printf("============================================================\n\n");

    printf("Number of searches                    : %d\n",
           SEARCH_SIZE);

    printf("Successful searches                   : %d\n",
           successCount);

    printf("Failed searches                       : %d\n",
           failCount);

    printf("Search success rate                   : %.2f%%\n",
           (double)successCount / SEARCH_SIZE * 100.0);


    printf("\n[Sequential Search]\n");

    printf("Total comparisons                     : %lld\n",
           sequentialTotal);

    printf("Average comparisons                   : %.2f\n",
           sequentialAverage);

    printf("Minimum comparisons                   : %d\n",
           sequentialMin);

    printf("Maximum comparisons                   : %d\n",
           sequentialMax);

    printf("Average comparisons when Found        : %.2f\n",
           sequentialSuccessAverage);

    printf("Average comparisons when Not Found    : %.2f\n",
           sequentialFailAverage);


    printf("\n[BST Search]\n");

    printf("Total comparisons                     : %lld\n",
           bstSearchTotal);

    printf("Average comparisons                   : %.2f\n",
           bstAverage);

    printf("Minimum comparisons                   : %d\n",
           bstMin);

    printf("Maximum comparisons                   : %d\n",
           bstMax);

    printf("Average comparisons when Found        : %.2f\n",
           bstSuccessAverage);

    printf("Average comparisons when Not Found    : %.2f\n",
           bstFailAverage);


    printf("\n[BST Construction Cost]\n");

    printf("BST Build Total Comparisons            : %lld\n",
           bstBuildComparisons);

    printf("BST Build Average per non-root insert  : %.2f\n",
           averageBuildComparisons);

    printf("BST Build + Search Comparisons         : %lld\n",
           bstTotalCost);


    /* ========================================================
       6. 추가 성능 분석
       ======================================================== */

    printf("\n============================================================\n");
    printf("[Performance Analysis]\n");
    printf("============================================================\n\n");


    double searchReduction =
        (1.0 -
         ((double)bstSearchTotal /
          (double)sequentialTotal))
        * 100.0;


    double totalReduction =
        (1.0 -
         ((double)bstTotalCost /
          (double)sequentialTotal))
        * 100.0;


    printf("BST Search Comparison Reduction       : %.2f%%\n",
           searchReduction);

    printf("Reduction Including BST Build Cost    : %.2f%%\n",
           totalReduction);


    double savingPerSearch =
        sequentialAverage - bstAverage;


    if (savingPerSearch > 0) {

        double breakEven =
            (double)bstBuildComparisons / savingPerSearch;

        int breakEvenRounded = (int)breakEven;

        if ((double)breakEvenRounded < breakEven)
            breakEvenRounded++;


        printf("Estimated Break-even Searches         : %.2f\n",
               breakEven);

        printf("Approx. Whole-number Break-even       : %d searches\n",
               breakEvenRounded);
    }
    else {
        printf("BST did not reduce average comparisons.\n");
    }


    printf("\n");

    if (bstTotalCost < sequentialTotal) {

        printf(
            "Including BST construction cost, "
            "BST used fewer comparisons in this experiment.\n"
        );
    }
    else {

        printf(
            "Including BST construction cost, "
            "Sequential Search used fewer comparisons "
            "in this experiment.\n"
        );
    }


    /* 메모리 해제 */
    freeBST(root);

    return 0;
}
