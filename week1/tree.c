/*
 * 과제 01 : 괄호 표기법으로 입력받은 트리의 정보를 출력하는 프로그램
 *
 * - 트리 자료구조(노드 구조체 + 포인터 연결)를 만들지 않는다.
 * - 입력 문자열을 왼쪽에서 오른쪽으로 순차 스캔하면서 스택만으로 계산한다.
 *
 *   사용하는 스택
 *     counter stack (degStack)  : 각 레벨에서 지금까지 나온 서브트리(자식)의 개수
 *     node stack    (nodeStack) : 현재 열려 있는 괄호들의 '부모 노드' 이름
 *
 *   현재 깊이(depth)는 counter stack 의 크기(degTop + 1)와 항상 같다.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define MAX_LEN    1024   /* 입력 문자열 최대 길이 */
#define MAX_NODES   256   /* 노드 최대 개수       */
#define MAX_DEPTH   256   /* 최대 중첩 깊이       */

/* 루트 노드의 레벨. 
 * (레벨 1 기준이면 예제 트리의 높이는 3, 레벨 0 기준이면 2) */
#define ROOT_LEVEL    0

/* ------------------------------------------------------------------ */
/* 0. 입력 정규화 : 공백 제거                                          */
/* ------------------------------------------------------------------ */
/*
 * 가독성을 위해 "A(B, C)" 처럼 공백을 섞어 입력할 수 있게 한다.
 * 공백(스페이스, 탭 등)을 모두 걸러낸 문자열을 dst 에 만들고,
 * 걸러낸 뒤의 각 문자가 원본에서 몇 번째였는지를 origPos 에 기록해 둔다.
 * (오류 메시지에서 사용자가 실제로 입력한 위치를 알려주기 위한 것)
 * 반환값은 공백을 제거한 문자열의 길이.
 */
int normalize(const char *src, char *dst, int *origPos)
{
    int i, j = 0;

    for (i = 0; src[i] != '\0'; i++) {
        if (isspace((unsigned char)src[i]))
            continue;
        dst[j]     = src[i];
        origPos[j] = i + 1;      /* 1부터 세는 원본 위치 */
        j++;
    }
    dst[j] = '\0';
    return j;
}

/* ------------------------------------------------------------------ */
/* 1. 입력 검증                                                        */
/* ------------------------------------------------------------------ */
/*
 * 문법
 *     tree    ::= node | node '(' tree { ',' tree } ')'
 *     node    ::= 'A' ~ 'Z'
 *
 * 상태 전이로 검사한다.
 *     EXPECT_NODE  : 노드 이름(대문자)이 와야 하는 자리
 *     AFTER_NODE   : 노드 이름을 막 읽은 직후  -> '(' 가능
 *     AFTER_CLOSE  : ')' 로 서브트리를 막 닫은 직후 -> '(' 불가
 *
 * 검증에 성공하면 1, 실패하면 오류 메시지를 출력하고 0 을 반환한다.
 */
enum { EXPECT_NODE, AFTER_NODE, AFTER_CLOSE };

int validate(const char *s, const int *origPos)
{
    int i;
    int depth = 0;                /* 열려 있는 괄호의 수 */
    int state = EXPECT_NODE;

    if (s[0] == '\0') {
        printf("오류: 입력이 비어 있습니다.\n");
        return 0;
    }

    for (i = 0; s[i] != '\0'; i++) {
        char c = s[i];

        if (state == EXPECT_NODE) {
            if (!isupper((unsigned char)c)) {
                printf("오류: %d번째 문자 '%c' - 노드 이름(영문 대문자)이 와야 하는 자리입니다.\n",
                       origPos[i], c);
                return 0;
            }
            state = AFTER_NODE;
            continue;
        }

        /* state == AFTER_NODE 또는 AFTER_CLOSE */
        switch (c) {
        case '(':
            if (state != AFTER_NODE) {
                printf("오류: %d번째 문자 '(' - 여는 괄호는 노드 이름 바로 뒤에만 올 수 있습니다.\n",
                       origPos[i]);
                return 0;
            }
            if (depth + 1 >= MAX_DEPTH) {
                printf("오류: 괄호 중첩이 너무 깊습니다.\n");
                return 0;
            }
            depth++;
            state = EXPECT_NODE;
            break;

        case ',':
            if (depth == 0) {
                printf("오류: %d번째 문자 ',' - 괄호 밖에서는 쉼표를 쓸 수 없습니다.\n", origPos[i]);
                return 0;
            }
            state = EXPECT_NODE;
            break;

        case ')':
            if (depth == 0) {
                printf("오류: %d번째 문자 ')' - 짝이 맞지 않는 닫는 괄호입니다.\n", origPos[i]);
                return 0;
            }
            depth--;
            state = AFTER_CLOSE;
            break;

        default:
            if (isupper((unsigned char)c))
                printf("오류: %d번째 문자 '%c' - 노드 이름 사이에는 구분자가 필요합니다.\n",
                       origPos[i], c);
            else
                printf("오류: %d번째 문자 '%c' - 허용되지 않는 문자입니다.\n", origPos[i], c);
            return 0;
        }
    }

    if (state == EXPECT_NODE) {
        printf("오류: 노드 이름이 와야 하는 자리에서 입력이 끝났습니다.\n");
        return 0;
    }
    if (depth != 0) {
        printf("오류: 닫히지 않은 괄호가 %d개 있습니다.\n", depth);
        return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------ */
/* 2. 순차 스캔으로 트리 정보 계산 + 출력                              */
/* ------------------------------------------------------------------ */
void printTreeInfo(const char *s)
{
    int i;

    /* 개수 관련 */
    int total = 0, leaf = 0, nonleaf = 0;

    /* 높이 */
    int maxLevel = ROOT_LEVEL;

    /* 차수 : 카운터 스택 */
    int degStack[MAX_DEPTH];
    int degTop = -1;
    int maxDegree = 0;

    /* 노드 C 의 부모 : 노드 스택 */
    char nodeStack[MAX_DEPTH];
    int  nodeTop = -1;
    char lastNode = 0;            /* 가장 최근에 읽은 노드 이름 */
    int  foundC = 0, cIsRoot = 0;
    char parentOfC = 0;

    /* 노드 C 의 자식 : 카운터 스택의 크기를 깊이로 이용 */
    char cChild[MAX_NODES];
    int  cChildCount = 0;
    int  cChildDepth = -1;        /* C 의 자식들이 놓이는 깊이 */
    int  collecting = 0;

    for (i = 0; s[i] != '\0'; i++) {
        char c = s[i];
        int  depth = degTop + 1;  /* 현재 깊이 = 카운터 스택의 크기 */

        if (isupper((unsigned char)c)) {
            total++;

            /* 뒤에 '(' 가 오면 비단말, 아니면 단말 */
            if (s[i + 1] == '(') nonleaf++;
            else                 leaf++;

            /* 높이 : 이 노드의 레벨 */
            if (ROOT_LEVEL + depth > maxLevel)
                maxLevel = ROOT_LEVEL + depth;

            if (c == 'C' && !foundC) {
                foundC = 1;
                /* 부모는 노드 스택의 top */
                if (nodeTop < 0) cIsRoot = 1;
                else             parentOfC = nodeStack[nodeTop];
                /* 자식이 있다면, 자식들은 한 단계 아래 깊이에 놓인다 */
                if (s[i + 1] == '(') {
                    cChildDepth = depth + 1;
                    collecting  = 1;
                }
            } else if (collecting && depth == cChildDepth) {
                /* 카운터 스택의 크기가 C 의 자식 깊이와 같으면 C 의 직계 자식 */
                cChild[cChildCount++] = c;
            }

            lastNode = c;
        }
        else if (c == '(') {
            degStack[++degTop] = 1;          /* 자식 1개를 이미 만난 상태로 시작 */
            nodeStack[++nodeTop] = lastNode; /* 이 괄호의 주인(부모) */
        }
        else if (c == ',') {
            degStack[degTop]++;              /* 같은 레벨의 형제 하나 추가 */
        }
        else if (c == ')') {
            if (degStack[degTop] > maxDegree)
                maxDegree = degStack[degTop];
            degTop--;
            nodeTop--;
            /* C 의 괄호가 닫혔으면 수집 종료 */
            if (collecting && degTop + 1 < cChildDepth)
                collecting = 0;
        }
    }

    printf("전체 노드의 수   : %d\n", total);
    printf("단말 노드의 수   : %d\n", leaf);
    printf("비단말 노드의 수 : %d\n", nonleaf);
    printf("트리의 높이      : %d\n", maxLevel);
    printf("트리의 차수      : %d\n", maxDegree);

    printf("노드 C의 부모    : ");
    if (!foundC)      printf("노드 C가 트리에 없습니다.\n");
    else if (cIsRoot) printf("없음 (C가 루트 노드입니다.)\n");
    else              printf("%c\n", parentOfC);

    printf("노드 C의 자식    : ");
    if (!foundC) {
        printf("노드 C가 트리에 없습니다.\n");
    } else if (cChildCount == 0) {
        printf("없음 (C는 단말 노드입니다.)\n");
    } else {
        int k;
        for (k = 0; k < cChildCount; k++)
            printf("%c%s", cChild[k], (k == cChildCount - 1) ? "" : ", ");
        printf("\n");
    }
}

/* ------------------------------------------------------------------ */
/* 3. 계층 출력 : '+', '-' 와 들여쓰기만 사용                          */
/* ------------------------------------------------------------------ */
/*
 * 괄호 문자열을 읽는 순서가 그대로 전위(preorder) 순서이므로,
 * 현재 깊이만 알면 스캔하면서 바로 출력할 수 있다.
 */
void drawTree(const char *s)
{
    int i, j;
    int depth = 0;

    for (i = 0; s[i] != '\0'; i++) {
        char c = s[i];

        if (isupper((unsigned char)c)) {
            for (j = 1; j < depth; j++) printf("    ");
            if (depth > 0)              printf("+---");
            printf("%c\n", c);
        }
        else if (c == '(') depth++;
        else if (c == ')') depth--;
    }
}

/* ------------------------------------------------------------------ */
/* 4. 계층 출력 : '|' 를 추가한 형태                                   */
/* ------------------------------------------------------------------ */
/*
 * 어떤 노드를 출력하는 시점에, 그 노드의 '조상들'이 각각 뒤에 형제를 더
 * 가지고 있는지를 알아야 한다. 이는 아직 읽지 않은 오른쪽 정보이므로
 * 한 번의 스캔으로는 출력할 수 없다. 그래서 두 단계로 나눈다.
 *
 *   (1) 스캔하면서 (이름, 깊이, 막내인지) 를 선형 배열에 기록한다.
 *       ',' 를 만나면 "그 깊이에서 가장 최근에 시작된 노드"는 막내가 아니다.
 *   (2) 기록을 앞에서부터 출력하면서, 레벨별 '|' 표시 여부를 갱신한다.
 */
void drawTreeWithPipe(const char *s)
{
    char name[MAX_NODES];
    int  dep[MAX_NODES];
    int  isLast[MAX_NODES];
    int  lastIdxAtDepth[MAX_DEPTH];
    int  pipe[MAX_DEPTH];
    int  cnt = 0;
    int  i, k, L;
    int  depth = 0;

    /* (1) 기록 */
    for (i = 0; s[i] != '\0'; i++) {
        char c = s[i];

        if (isupper((unsigned char)c)) {
            name[cnt]   = c;
            dep[cnt]    = depth;
            isLast[cnt] = 1;               /* 일단 막내라고 가정 */
            lastIdxAtDepth[depth] = cnt;
            cnt++;
        }
        else if (c == '(') depth++;
        else if (c == ',') isLast[lastIdxAtDepth[depth]] = 0;  /* 뒤에 형제가 있다 */
        else if (c == ')') depth--;
    }

    /* (2) 출력 */
    for (k = 0; k < MAX_DEPTH; k++) pipe[k] = 0;

    for (k = 0; k < cnt; k++) {
        int d = dep[k];

        for (L = 1; L < d; L++)
            printf(pipe[L] ? "|   " : "    ");
        if (d > 0)
            printf("+---");
        printf("%c\n", name[k]);

        pipe[d] = !isLast[k];   /* 이 노드 뒤에 형제가 남았으면 세로선을 잇는다 */
    }
}

/* ------------------------------------------------------------------ */
int main(void)
{
    char raw[MAX_LEN];        /* 사용자가 입력한 원본            */
    char tree[MAX_LEN];       /* 공백을 제거한 문자열            */
    int  origPos[MAX_LEN];    /* tree[j] 가 원본에서 몇 번째였는지 */
    int  len;

    printf("트리를 괄호 표기법으로 입력하세요 : ");
    if (fgets(raw, sizeof(raw), stdin) == NULL) {
        printf("오류: 입력을 읽을 수 없습니다.\n");
        return 1;
    }

    /* 줄 끝의 개행 문자 제거 */
    len = (int)strlen(raw);
    while (len > 0 && (raw[len - 1] == '\n' || raw[len - 1] == '\r'))
        raw[--len] = '\0';

    /* 공백을 걸러낸 뒤 검사하고, 이후 처리는 모두 tree 로 한다 */
    normalize(raw, tree, origPos);

    if (!validate(tree, origPos))
        return 1;

    printf("\n입력된 트리 : %s\n\n", tree);

    printTreeInfo(tree);

    printf("\n[계층 출력]\n");
    drawTree(tree);

    printf("\n[계층 출력 - '|' 사용]\n");
    drawTreeWithPipe(tree);

    return 0;
}
