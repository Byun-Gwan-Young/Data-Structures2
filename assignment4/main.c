#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NODES 1000
#define MAX_INPUT 4096
#define DATA_SIZE 32

typedef struct TreeNode {
    char data[DATA_SIZE];
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

/* 괄호 입력 해석용 스택 */
typedef struct {
    TreeNode *node;
    int stage;
} ParseFrame;

/* 트리 출력용 스택 */
typedef struct {
    TreeNode *node;
    int depth;
} PrintFrame;


/* 함수 선언 */
TreeNode *createNode(const char *data);
TreeNode *parseTree(const char *input, char *errorMessage);

void printTree(TreeNode *root);

void preorder(TreeNode *root);
void inorder(TreeNode *root);
void postorder(TreeNode *root);

void freeTree(TreeNode *root);


/* 노드 생성 */
TreeNode *createNode(const char *data)
{
    TreeNode *newNode = (TreeNode *)malloc(sizeof(TreeNode));

    if (newNode == NULL) {
        return NULL;
    }

    strcpy(newNode->data, data);

    newNode->left = NULL;
    newNode->right = NULL;

    return newNode;
}


/*
    괄호 표기법을 이용해 트리를 생성한다.

    예:
    A(B,C)
    A(B(D,E),C(F,G))
    A(,B)
    A(B,)
*/
TreeNode *parseTree(const char *input, char *errorMessage)
{
    ParseFrame stack[MAX_NODES];

    int top = -1;
    int nodeCount = 0;

    int i = 0;
    int previousWasData = 0;

    TreeNode *root = NULL;
    TreeNode *lastNode = NULL;

    while (input[i] != '\0') {

        /* 공백 무시 */
        if (isspace((unsigned char)input[i])) {
            i++;
            continue;
        }

        /* '(' 처리 */
        if (input[i] == '(') {

            if (!previousWasData || lastNode == NULL) {
                strcpy(errorMessage,
                       "오류: '(' 앞에는 노드 데이터가 있어야 합니다.");
                freeTree(root);
                return NULL;
            }

            if (top + 1 >= MAX_NODES) {
                strcpy(errorMessage,
                       "오류: 트리의 깊이가 너무 큽니다.");
                freeTree(root);
                return NULL;
            }

            top++;

            stack[top].node = lastNode;

            /*
                stage 0 : 왼쪽 자식을 기다리는 상태
                stage 1 : 왼쪽 자식 입력 완료
                stage 2 : 오른쪽 자식을 기다리는 상태
                stage 3 : 오른쪽 자식 입력 완료
            */
            stack[top].stage = 0;

            previousWasData = 0;

            i++;
            continue;
        }

        /* ',' 처리 */
        if (input[i] == ',') {

            if (top < 0) {
                strcpy(errorMessage,
                       "오류: 잘못된 위치에 ','가 있습니다.");
                freeTree(root);
                return NULL;
            }

            /*
                stage 0이면 왼쪽 자식이 없는 경우
                stage 1이면 왼쪽 자식 입력이 끝난 경우
            */
            if (stack[top].stage == 0 ||
                stack[top].stage == 1) {

                stack[top].stage = 2;
            }

            else if (stack[top].stage == 3){
                strcpy(errorMessage,
                    "오류: 이진트리가 아닙니다. 한 노드는 최대 2개의 자식만 가질 수 있습니다.");
                freeTree(root);
                return NULL;
            }

            else {
                strcpy(errorMessage,
                       "오류: ','의 위치가 올바르지 않습니다.");
                freeTree(root);
                return NULL;
            }

            previousWasData = 0;

            i++;
            continue;
        }

        /* ')' 처리 */
        if (input[i] == ')') {

            if (top < 0) {
                strcpy(errorMessage,
                       "오류: 대응되는 '('가 없습니다.");
                freeTree(root);
                return NULL;
            }

            /*
                stage 2 : 오른쪽 자식이 비어 있음
                stage 3 : 오른쪽 자식 입력 완료
            */
            if (stack[top].stage != 2 &&
                stack[top].stage != 3) {

                strcpy(errorMessage,
                       "오류: 자식 노드 표현이 올바르지 않습니다.");
                freeTree(root);
                return NULL;
            }

            top--;

            previousWasData = 0;

            i++;
            continue;
        }


        /*
            여기부터는 노드 데이터 읽기
            괄호, 쉼표, 공백 전까지 하나의 데이터로 처리
        */
        char buffer[DATA_SIZE];

        int index = 0;

        while (input[i] != '\0' &&
               input[i] != '(' &&
               input[i] != ')' &&
               input[i] != ',' &&
               !isspace((unsigned char)input[i])) {

            if (index >= DATA_SIZE - 1) {
                strcpy(errorMessage,
                       "오류: 노드 데이터가 너무 깁니다.");
                freeTree(root);
                return NULL;
            }

            buffer[index++] = input[i++];
        }

        buffer[index] = '\0';

        if (index == 0) {
            strcpy(errorMessage,
                   "오류: 노드 데이터가 올바르지 않습니다.");
            freeTree(root);
            return NULL;
        }

        if (nodeCount >= MAX_NODES) {
            strcpy(errorMessage,
                   "오류: 노드 개수가 너무 많습니다.");
            freeTree(root);
            return NULL;
        }

        TreeNode *newNode = createNode(buffer);

        if (newNode == NULL) {
            strcpy(errorMessage,
                   "오류: 메모리 할당에 실패했습니다.");
            freeTree(root);
            return NULL;
        }

        nodeCount++;


        /* 루트 노드인 경우 */
        if (root == NULL) {

            if (top >= 0) {
                strcpy(errorMessage,
                       "오류: 잘못된 트리 구조입니다.");
                free(newNode);
                freeTree(root);
                return NULL;
            }

            root = newNode;
        }

        /* 자식 노드인 경우 */
        else {

            if (top < 0) {
                strcpy(errorMessage,
                       "오류: 하나의 트리만 입력할 수 있습니다.");
                free(newNode);
                freeTree(root);
                return NULL;
            }

            /* 왼쪽 자식 */
            if (stack[top].stage == 0) {

                stack[top].node->left = newNode;

                stack[top].stage = 1;
            }

            /* 오른쪽 자식 */
            else if (stack[top].stage == 2) {

                stack[top].node->right = newNode;

                stack[top].stage = 3;
            }

            else {
                strcpy(errorMessage,
                       "오류: 노드 위치가 올바르지 않습니다.");
                free(newNode);
                freeTree(root);
                return NULL;
            }
        }

        lastNode = newNode;
        previousWasData = 1;
    }


    if (root == NULL) {
        strcpy(errorMessage,
               "오류: 트리가 입력되지 않았습니다.");
        return NULL;
    }

    /* 닫히지 않은 괄호 검사 */
    if (top >= 0) {
        strcpy(errorMessage,
               "오류: 닫히지 않은 '('가 있습니다.");
        freeTree(root);
        return NULL;
    }

    return root;
}


/*
    트리를 왼쪽으로 눕힌 형태로 출력

    오른쪽
    루트
    왼쪽

    순서로 출력
*/
void printTree(TreeNode *root)
{
    if (root == NULL) {
        return;
    }

    PrintFrame stack[MAX_NODES];

    int top = -1;

    TreeNode *current = root;

    int depth = 0;

    while (current != NULL || top >= 0) {

        /*
            오른쪽 끝까지 이동
        */
        while (current != NULL) {

            top++;

            stack[top].node = current;
            stack[top].depth = depth;

            current = current->right;

            depth++;
        }

        PrintFrame frame = stack[top--];

        current = frame.node;
        depth = frame.depth;

        /* 깊이에 따라 들여쓰기 */
        for (int i = 0; i < depth; i++) {
            printf("    ");
        }

        printf("%s\n", current->data);

        /*
            왼쪽 서브트리로 이동
        */
        current = current->left;

        depth = frame.depth + 1;
    }
}


/*
    전위 순회

    Root -> Left -> Right
*/
void preorder(TreeNode *root)
{
    TreeNode *stack[MAX_NODES];

    int top = -1;

    if (root == NULL) {
        return;
    }

    stack[++top] = root;

    while (top >= 0) {

        TreeNode *current = stack[top--];

        printf("%s ", current->data);

        /*
            스택은 나중에 넣은 것이 먼저 나오므로
            Right를 먼저 넣고 Left를 나중에 넣는다.
        */
        if (current->right != NULL) {
            stack[++top] = current->right;
        }

        if (current->left != NULL) {
            stack[++top] = current->left;
        }
    }
}


/*
    중위 순회

    Left -> Root -> Right
*/
void inorder(TreeNode *root)
{
    TreeNode *stack[MAX_NODES];

    int top = -1;

    TreeNode *current = root;

    while (current != NULL || top >= 0) {

        /*
            왼쪽 끝까지 이동
        */
        while (current != NULL) {

            stack[++top] = current;

            current = current->left;
        }

        current = stack[top--];

        printf("%s ", current->data);

        /*
            오른쪽 서브트리로 이동
        */
        current = current->right;
    }
}


/*
    후위 순회

    Left -> Right -> Root

    두 개의 스택을 사용
*/
void postorder(TreeNode *root)
{
    TreeNode *stack1[MAX_NODES];
    TreeNode *stack2[MAX_NODES];

    int top1 = -1;
    int top2 = -1;

    if (root == NULL) {
        return;
    }

    stack1[++top1] = root;

    while (top1 >= 0) {

        TreeNode *current = stack1[top1--];

        stack2[++top2] = current;

        if (current->left != NULL) {
            stack1[++top1] = current->left;
        }

        if (current->right != NULL) {
            stack1[++top1] = current->right;
        }
    }

    /*
        두 번째 스택을 역순으로 꺼내면

        Left -> Right -> Root

        순서가 된다.
    */
    while (top2 >= 0) {
        printf("%s ", stack2[top2--]->data);
    }
}


/*
    동적 할당된 트리 메모리 해제

    이 함수 역시 재귀를 사용하지 않는다.
*/
void freeTree(TreeNode *root)
{
    if (root == NULL) {
        return;
    }

    TreeNode *stack[MAX_NODES];

    int top = -1;

    stack[++top] = root;

    while (top >= 0) {

        TreeNode *current = stack[top--];

        if (current->left != NULL) {
            stack[++top] = current->left;
        }

        if (current->right != NULL) {
            stack[++top] = current->right;
        }

        free(current);
    }
}


/* 메인 함수 */
int main(void)
{
    char input[MAX_INPUT];
    char errorMessage[200];

    TreeNode *root;


    printf("========================================\n");
    printf("        이진트리 순회 프로그램\n");
    printf("========================================\n\n");

    printf("괄호 표기법으로 이진트리를 입력하세요.\n");
    printf("예) A(B(D,E),C(F,G))\n");
    printf("오른쪽 자식만 있는 경우 예) A(,B)\n\n");

    printf("이진트리 입력: ");

    if (fgets(input, sizeof(input), stdin) == NULL) {

        printf("입력 오류가 발생했습니다.\n");

        return 1;
    }


    root = parseTree(input, errorMessage);

    if (root == NULL) {

        printf("\n%s\n", errorMessage);

        return 1;
    }


    printf("\n========================================\n");
    printf("입력된 이진트리 구조\n");
    printf("========================================\n\n");

    printTree(root);


    printf("\n========================================\n");
    printf("이진트리 순회 결과\n");
    printf("========================================\n\n");


    printf("Preorder  : ");
    preorder(root);
    printf("\n");


    printf("Inorder   : ");
    inorder(root);
    printf("\n");


    printf("Postorder : ");
    postorder(root);
    printf("\n");


    freeTree(root);

    return 0;
}
