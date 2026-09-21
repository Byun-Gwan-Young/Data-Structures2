/*==========================================================================
  과제 02 : 이진트리 프로그램 구현
--------------------------------------------------------------------------
  이진트리를 괄호 표기법으로 입력받아
    (1) 배열을 이용한 표현
    (2) 포인터(연결 자료구조)를 이용한 표현
  두 가지로 각각 독립적으로 구성하고, 각 표현에 대해
    [1] 왼쪽으로 눕힌 계층적 출력
    [2] 트리 정보(전체/단말/비단말 노드 수, 높이, 차수)
    [3] 형태 판별(완전 / 포화 / 편향 이진트리)
  을 제공한다. 또한 두 구현의
    (3) 메모리 사용량 비교 및 자식·부모·형제 조회 효율 비교
  기능을 포함한다.
--------------------------------------------------------------------------
  괄호 표기법
      노드(왼쪽서브트리,오른쪽서브트리)
    - 자식이 없으면 괄호를 생략한다.            예) A
    - 왼쪽만 있으면 오른쪽 자리를 비운다.       예) A(B,)  또는 A(B)
    - 오른쪽만 있으면 왼쪽 자리를 비운다.       예) A(,C)
    예) A(B(D,E),C(,F))

           A
         /   \
        B     C
       / \      \
      D   E      F
--------------------------------------------------------------------------
  높이(height)의 정의 : 루트의 레벨을 1로 두고, 트리의 최대 레벨을 높이로
  한다. (노드가 1개면 높이 1)
  차수(degree)의 정의 : 트리에 존재하는 노드의 차수 중 최댓값.
--------------------------------------------------------------------------
  이름은 중복 없는 ASCII 영숫자/밑줄 1~15자, 입력은 최대 511바이트.
  컴파일 : gcc -std=c11 -Wall -Wextra -O2 -o bintree bintree.c
  실행   : ./bintree
==========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LABEL    16     /* 노드 이름 최대 길이(널 문자 포함) */
#define MAX_INPUT   512     /* 한 줄 입력 버퍼 */
#define MAX_HEIGHT   20     /* 배열 표현이 감당할 최대 높이(2^20 슬롯) */
#define INDENT        6     /* 계층 출력 시 레벨당 들여쓰기 칸 수 */

/*==========================================================================
  0. 공통 : 오류 메시지 / 문자열 유틸
==========================================================================*/

static char g_err[256];                     /* 파싱 오류 메시지 저장 */

#define ERR(...)  snprintf(g_err, sizeof(g_err), __VA_ARGS__)

static void skipSpace(const char **p)
{
    while (isspace((unsigned char)**p)) (*p)++;
}

static int isLabelChar(char c)
{
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || c == '_';
}

/* 한 번의 파싱에 등장한 이름을 기록한다. 입력 길이가 노드 수의 상한이다.
   트리를 만들 때 초기화하며, 완성된 트리의 저장 공간에는 포함하지 않는다. */
static char parsedLabels[MAX_INPUT][MAX_LABEL];
static size_t parsedCount;

/* 이름을 잘라 저장하지 않는다. 길이 초과와 중복은 오류로 반환한다. */
static int readLabel(const char **p, char *out)
{
    size_t k = 0, i;
    while (isLabelChar(**p)) {
        if (k >= MAX_LABEL - 1) {
            ERR("노드 이름은 최대 %d자입니다.", MAX_LABEL - 1);
            return 0;
        }
        out[k++] = *(*p)++;
    }
    out[k] = '\0';
    if (!k) { ERR("노드 이름이 필요합니다."); return 0; }
    for (i = 0; i < parsedCount; ++i) {
        if (strcmp(parsedLabels[i], out) == 0) {
            ERR("중복 노드 이름 '%s'은 사용할 수 없습니다.", out);
            return 0;
        }
    }
    if (parsedCount >= MAX_INPUT) { ERR("노드가 너무 많습니다."); return 0; }
    strcpy(parsedLabels[parsedCount++], out);
    return 1;
}


static void printIndent(int level)
{
    int i;
    for (i = 0; i < level * INDENT; i++) putchar(' ');
}

static void line(char c, int n)
{
    int i;
    for (i = 0; i < n; i++) putchar(c);
    putchar('\n');
}

/* 한글(UTF-8)이 섞인 문자열의 화면 출력 폭을 계산한다.
   printf 의 %-Ns 는 바이트 수로 칸을 맞추므로 표가 어긋난다.        */
static int dispWidth(const char *s)
{
    int w = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c < 0x80)              { w += 1; s += 1; }   /* ASCII */
        else if ((c & 0xE0) == 0xC0) { w += 1; s += 2; }
        else if ((c & 0xF0) == 0xE0) { w += 2; s += 3; } /* 한글 등 */
        else                         { w += 2; s += 4; }
    }
    return w;
}

/* 문자열을 출력하고 화면 폭 width에 맞게 공백을 채운다(왼쪽 정렬) */
static void padPrint(const char *s, int width)
{
    int w = dispWidth(s), i;
    fputs(s, stdout);
    for (i = w; i < width; i++) putchar(' ');
}


/*==========================================================================
  1. 배열을 이용한 이진트리 구현
--------------------------------------------------------------------------
  1-based 인덱스를 사용한다.
      루트          : 1
      i의 왼쪽 자식 : 2*i
      i의 오른쪽자식 : 2*i + 1
      i의 부모      : i / 2
  비어 있는 자리는 label[0] == '\0' 으로 표시한다.
  높이가 h인 트리의 인덱스는 최대 2^h - 1 이다. 이 구현에서는
  같은 높이의 전체 레벨을 예약하는 정책으로 capacity를 2^h로 잡는다. (0번 칸은 사용하지 않음)
==========================================================================*/

typedef struct {
    char label[MAX_LABEL];      /* 빈 문자열이면 비어 있는 자리 */
} Cell;

typedef struct {
    Cell *cell;                 /* 동적 할당된 배열 */
    long  capacity;             /* 할당된 칸 수. 유효 인덱스 1 .. capacity-1 */
} ArrayTree;

static void arrInit(ArrayTree *t)
{
    t->cell = NULL;
    t->capacity = 0;
}

static void arrFree(ArrayTree *t)
{
    free(t->cell);
    arrInit(t);
}

/* 인덱스 i가 실제 노드를 담고 있는가? */
static int arrUsed(const ArrayTree *t, long i)
{
    return (i >= 1 && i < t->capacity && t->cell[i].label[0] != '\0');
}

/* idx를 담을 수 있도록 배열을 확장(2배씩) */
static int arrEnsure(ArrayTree *t, long idx)
{
    long ncap, old;
    Cell *nc;

    if (idx < t->capacity) return 1;

    ncap = (t->capacity < 2) ? 2 : t->capacity;
    while (ncap <= idx) {
        if (ncap >= (1L << MAX_HEIGHT)) {
            ERR("트리가 너무 깊습니다. 배열 표현은 높이 %d 까지만 지원합니다.",
                MAX_HEIGHT);
            return 0;
        }
        ncap *= 2;
    }
    old = t->capacity;
    nc = (Cell *)realloc(t->cell, (size_t)ncap * sizeof(Cell));
    if (nc == NULL) { ERR("메모리 할당에 실패했습니다."); return 0; }
    memset(nc + old, 0, (size_t)(ncap - old) * sizeof(Cell));
    t->cell = nc;
    t->capacity = ncap;
    return 1;
}

/*--------------------------------------------------------------------------
  괄호 표기법 -> 배열  (재귀 하강 파싱)
--------------------------------------------------------------------------*/
static int arrParse(const char **p, ArrayTree *t, long idx)
{
    char lab[MAX_LABEL];

    skipSpace(p);

    /* 빈 서브트리 : 문자열 끝, 콤마, 닫는 괄호 */
    if (**p == '\0' || **p == ',' || **p == ')') return 1;

    if (!isLabelChar(**p)) {
        ERR("노드 이름이 와야 할 자리에 '%c' 가 있습니다.", **p);
        return 0;
    }
    if (idx >= (1L << MAX_HEIGHT)) {
        ERR("트리가 너무 깊습니다. 배열 표현은 높이 %d 까지만 지원합니다.",
            MAX_HEIGHT);
        return 0;
    }
    if (!arrEnsure(t, idx)) return 0;

    if (!readLabel(p, lab)) return 0;
    strcpy(t->cell[idx].label, lab);

    skipSpace(p);
    if (**p == '(') {                       /* 자식이 있다 */
        (*p)++;
        if (!arrParse(p, t, 2 * idx)) return 0;         /* 왼쪽 */
        skipSpace(p);
        if (**p == ',') {
            (*p)++;
            if (!arrParse(p, t, 2 * idx + 1)) return 0; /* 오른쪽 */
            skipSpace(p);
        }
        if (**p != ')') {
            ERR("')' 가 필요한 자리에 '%c' 가 있습니다.",
                **p ? **p : ' ');
            return 0;
        }
        (*p)++;
    }
    return 1;
}

/* 전방 선언 : 배열 높이 계산 */
static int  arrHeight(const ArrayTree *t);

/* 문자열로부터 배열 이진트리 생성. 성공 1, 실패 0(g_err에 사유) */
static int arrBuild(ArrayTree *t, const char *s)
{
    const char *p = s;
    long want;
    int  h;
    Cell *nc;

    parsedCount = 0;
    arrFree(t);
    if (!arrParse(&p, t, 1)) { arrFree(t); return 0; }

    skipSpace(&p);
    if (*p != '\0') {
        ERR("입력의 끝에 불필요한 문자 '%c' 가 남아 있습니다.", *p);
        arrFree(t);
        return 0;
    }
    if (t->capacity == 0) { ERR("빈 트리입니다."); return 0; }

    /* 배열 크기를 2^h 로 정규화한다(이 구현의 전체 레벨 예약 정책) */
    h = arrHeight(t);
    want = 1L << h;
    if (want != t->capacity) {
        long old = t->capacity;
        nc = (Cell *)realloc(t->cell, (size_t)want * sizeof(Cell));
        if (nc == NULL) { ERR("메모리 할당에 실패했습니다."); arrFree(t); return 0; }
        if (want > old)
            memset(nc + old, 0, (size_t)(want - old) * sizeof(Cell));
        t->cell = nc;
        t->capacity = want;
    }
    return 1;
}

/*--------------------------------------------------------------------------
  [1] 배열 이진트리 출력 : 왼쪽으로 눕힌 계층 출력
      - 오른쪽 서브트리를 먼저 출력하고, 자기 자신, 그 다음 왼쪽 서브트리
      - 한쪽 자식만 있는 경우 비어 있는 쪽을 '-' 로 표시하여
        왼쪽/오른쪽 구분이 눈에 보이도록 한다.
--------------------------------------------------------------------------*/
static void arrPrintRec(const ArrayTree *t, long i, int level)
{
    long l = 2 * i, r = 2 * i + 1;
    int hasL, hasR;

    if (!arrUsed(t, i)) return;
    hasL = arrUsed(t, l);
    hasR = arrUsed(t, r);

    if (hasR)       arrPrintRec(t, r, level + 1);
    else if (hasL) { printIndent(level + 1); printf("-\n"); }

    printIndent(level);
    printf("%s\n", t->cell[i].label);

    if (hasL)       arrPrintRec(t, l, level + 1);
    else if (hasR) { printIndent(level + 1); printf("-\n"); }
}

static void arrPrint(const ArrayTree *t)
{
    printf("\n[배열 표현] 이진트리 (왼쪽으로 눕힌 형태)\n");
    line('-', 50);
    arrPrintRec(t, 1, 0);
    line('-', 50);
}

/* 배열의 내부 저장 상태를 그대로 보여준다(확인용) */
static void arrDump(const ArrayTree *t)
{
    long i;
    printf("\n[배열 표현] 내부 저장 상태 (인덱스 : 값)\n");
    line('-', 50);
    for (i = 1; i < t->capacity; i++) {
        printf("%4ld : %-4s", i, arrUsed(t, i) ? t->cell[i].label : ".");
        if (i % 8 == 0) putchar('\n');
    }
    if ((t->capacity - 1) % 8 != 0) putchar('\n');
    printf("(칸 수 = %ld, '.' 은 비어 있는 자리)\n", t->capacity - 1);
    line('-', 50);
}

/*--------------------------------------------------------------------------
  [2] 배열 이진트리 정보
--------------------------------------------------------------------------*/
static int arrCountNodes(const ArrayTree *t)
{
    long i; int n = 0;
    for (i = 1; i < t->capacity; i++) if (arrUsed(t, i)) n++;
    return n;
}

static int arrCountLeaf(const ArrayTree *t)
{
    long i; int n = 0;
    for (i = 1; i < t->capacity; i++)
        if (arrUsed(t, i) && !arrUsed(t, 2 * i) && !arrUsed(t, 2 * i + 1)) n++;
    return n;
}

static int arrHeight(const ArrayTree *t)
{
    long i, k; int h = 0, lv;
    for (i = 1; i < t->capacity; i++) {
        if (!arrUsed(t, i)) continue;
        lv = 0;
        for (k = i; k >= 1; k >>= 1) lv++;   /* 인덱스 i의 레벨 */
        if (lv > h) h = lv;
    }
    return h;
}

static int arrDegree(const ArrayTree *t)
{
    long i; int d = 0, c;
    for (i = 1; i < t->capacity; i++) {
        if (!arrUsed(t, i)) continue;
        c = (arrUsed(t, 2 * i) ? 1 : 0) + (arrUsed(t, 2 * i + 1) ? 1 : 0);
        if (c > d) d = c;
    }
    return d;
}

static void arrInfo(const ArrayTree *t)
{
    int n    = arrCountNodes(t);
    int leaf = arrCountLeaf(t);

    printf("\n[배열 표현] 트리 정보\n");
    line('-', 50);
    printf("  1. 전체 노드의 수   : %d\n", n);
    printf("  2. 단말 노드의 수   : %d\n", leaf);
    printf("  3. 비단말 노드의 수 : %d\n", n - leaf);
    printf("  4. 트리의 높이      : %d\n", arrHeight(t));
    printf("  5. 트리의 차수      : %d\n", arrDegree(t));
    line('-', 50);
}

/*--------------------------------------------------------------------------
  [3] 배열 이진트리 형태 판별
      완전 이진트리 : 노드가 인덱스 1 .. n 에 빈틈없이 들어 있다.
      포화 이진트리 : 완전 이진트리이면서 n == 2^h - 1
      편향 이진트리 : 모든 노드의 차수가 1 이하 (한 줄로 늘어선 트리)
--------------------------------------------------------------------------*/
static int arrIsComplete(const ArrayTree *t)
{
    int n = arrCountNodes(t);
    long i;
    for (i = 1; i <= n; i++) if (!arrUsed(t, i)) return 0;
    return 1;   /* 전체 개수가 n이므로 1..n이 모두 차 있으면 그 밖은 비어 있다 */
}

static int arrIsPerfect(const ArrayTree *t)
{
    int n = arrCountNodes(t);
    int h = arrHeight(t);
    return arrIsComplete(t) && (n == (1 << h) - 1);
}

/* 0: 편향 아님, 1: 왼쪽 편향, 2: 오른쪽 편향, 3: 편향(방향 혼합) */
static int arrSkewKind(const ArrayTree *t)
{
    long i;
    int  left = 0, right = 0;

    if (arrDegree(t) > 1) return 0;          /* 차수 2인 노드가 있으면 편향 아님 */
    if (arrCountNodes(t) == 1) return 4;     /* 노드 1개 : 자명한 경우 */
    for (i = 1; i < t->capacity; i++) {
        if (!arrUsed(t, i)) continue;
        if (arrUsed(t, 2 * i))     left++;
        if (arrUsed(t, 2 * i + 1)) right++;
    }
    if (left > 0 && right > 0) return 3;
    if (right > 0) return 2;
    return 1;                                 /* 노드 1개인 경우도 여기 */
}

static const char *skewText(int kind)
{
    switch (kind) {
        case 1: return "예 (왼쪽 편향)";
        case 2: return "예 (오른쪽 편향)";
        case 3: return "예 (차수는 모두 1 이하이나 방향이 섞인 지그재그 형태)";
        case 4: return "예 (노드가 1개뿐인 자명한 경우)";
        default: return "아니오";
    }
}

static void arrShape(const ArrayTree *t)
{
    int n = arrCountNodes(t);
    int h = arrHeight(t);

    printf("\n[배열 표현] 이진트리 형태 판별\n");
    line('-', 50);
    printf("  노드 수 n = %d, 높이 h = %d\n\n", n, h);
    printf("  완전 이진트리 여부 : %s\n",
           arrIsComplete(t) ? "예" : "아니오");
    printf("  포화 이진트리 여부 : %s   (포화 조건 n == 2^h-1 = %d)\n",
           arrIsPerfect(t) ? "예" : "아니오", (1 << h) - 1);
    printf("  편향 이진트리 여부 : %s\n", skewText(arrSkewKind(t)));
    line('-', 50);
}


/*==========================================================================
  2. 포인터를 이용한 연결 자료구조 구현
==========================================================================*/

typedef struct TreeNode {
    char label[MAX_LABEL];
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

static void lnkFree(TreeNode *r)
{
    if (r == NULL) return;
    lnkFree(r->left);
    lnkFree(r->right);
    free(r);
}

/*--------------------------------------------------------------------------
  괄호 표기법 -> 연결 자료구조 (재귀 하강 파싱)
--------------------------------------------------------------------------*/
static int lnkParse(const char **p, TreeNode **out)
{
    char lab[MAX_LABEL];
    TreeNode *n;

    *out = NULL;
    skipSpace(p);

    if (**p == '\0' || **p == ',' || **p == ')') return 1;   /* 빈 서브트리 */

    if (!isLabelChar(**p)) {
        ERR("노드 이름이 와야 할 자리에 '%c' 가 있습니다.", **p);
        return 0;
    }
    if (!readLabel(p, lab)) return 0;

    n = (TreeNode *)calloc(1, sizeof(TreeNode));
    if (n == NULL) { ERR("메모리 할당에 실패했습니다."); return 0; }
    strcpy(n->label, lab);

    skipSpace(p);
    if (**p == '(') {
        (*p)++;
        if (!lnkParse(p, &n->left))  { lnkFree(n); return 0; }
        skipSpace(p);
        if (**p == ',') {
            (*p)++;
            if (!lnkParse(p, &n->right)) { lnkFree(n); return 0; }
            skipSpace(p);
        }
        if (**p != ')') {
            ERR("')' 가 필요한 자리에 '%c' 가 있습니다.", **p ? **p : ' ');
            lnkFree(n);
            return 0;
        }
        (*p)++;
    }
    *out = n;
    return 1;
}

static int lnkBuild(TreeNode **root, const char *s)
{
    const char *p = s;
    TreeNode *r = NULL;

    parsedCount = 0;
    lnkFree(*root);
    *root = NULL;

    if (!lnkParse(&p, &r)) { lnkFree(r); return 0; }
    skipSpace(&p);
    if (*p != '\0') {
        ERR("입력의 끝에 불필요한 문자 '%c' 가 남아 있습니다.", *p);
        lnkFree(r);
        return 0;
    }
    if (r == NULL) { ERR("빈 트리입니다."); return 0; }
    *root = r;
    return 1;
}

/*--------------------------------------------------------------------------
  [1] 연결 이진트리 출력 : 왼쪽으로 눕힌 계층 출력
--------------------------------------------------------------------------*/
static void lnkPrintRec(const TreeNode *r, int level)
{
    if (r == NULL) return;

    if (r->right)      lnkPrintRec(r->right, level + 1);
    else if (r->left) { printIndent(level + 1); printf("-\n"); }

    printIndent(level);
    printf("%s\n", r->label);

    if (r->left)       lnkPrintRec(r->left, level + 1);
    else if (r->right){ printIndent(level + 1); printf("-\n"); }
}

static void lnkPrint(const TreeNode *r)
{
    printf("\n[연결 자료구조] 이진트리 (왼쪽으로 눕힌 형태)\n");
    line('-', 50);
    lnkPrintRec(r, 0);
    line('-', 50);
}

/*--------------------------------------------------------------------------
  [2] 연결 이진트리 정보
--------------------------------------------------------------------------*/
static int lnkCount(const TreeNode *r)
{
    if (r == NULL) return 0;
    return 1 + lnkCount(r->left) + lnkCount(r->right);
}

static int lnkLeaf(const TreeNode *r)
{
    if (r == NULL) return 0;
    if (r->left == NULL && r->right == NULL) return 1;
    return lnkLeaf(r->left) + lnkLeaf(r->right);
}

static int lnkHeight(const TreeNode *r)
{
    int hl, hr;
    if (r == NULL) return 0;
    hl = lnkHeight(r->left);
    hr = lnkHeight(r->right);
    return (hl > hr ? hl : hr) + 1;
}

static int lnkDegree(const TreeNode *r)
{
    int c, dl, dr;
    if (r == NULL) return 0;
    c  = (r->left ? 1 : 0) + (r->right ? 1 : 0);
    dl = lnkDegree(r->left);
    dr = lnkDegree(r->right);
    if (dl > c) c = dl;
    if (dr > c) c = dr;
    return c;
}

static void lnkInfo(const TreeNode *r)
{
    int n    = lnkCount(r);
    int leaf = lnkLeaf(r);

    printf("\n[연결 자료구조] 트리 정보\n");
    line('-', 50);
    printf("  1. 전체 노드의 수   : %d\n", n);
    printf("  2. 단말 노드의 수   : %d\n", leaf);
    printf("  3. 비단말 노드의 수 : %d\n", n - leaf);
    printf("  4. 트리의 높이      : %d\n", lnkHeight(r));
    printf("  5. 트리의 차수      : %d\n", lnkDegree(r));
    line('-', 50);
}

/*--------------------------------------------------------------------------
  [3] 연결 이진트리 형태 판별
      완전 이진트리 판별은 배열 인덱스를 쓰지 않고 레벨 순회(큐)로 한다.
      : 레벨 순서로 방문하다가 NULL을 만난 뒤에 또 노드가 나오면 완전이 아니다.
--------------------------------------------------------------------------*/
static int lnkIsComplete(TreeNode *root)
{
    TreeNode **q;
    int n, head = 0, tail = 0, seenNull = 0, ok = 1;

    if (root == NULL) return 1;
    n = lnkCount(root);
    q = (TreeNode **)malloc(sizeof(TreeNode *) * (size_t)(2 * n + 2));
    if (q == NULL) return -1; /* 판별 불가를 거짓과 구분 */

    q[tail++] = root;
    while (head < tail) {
        TreeNode *cur = q[head++];
        if (cur == NULL) { seenNull = 1; continue; }
        if (seenNull) { ok = 0; break; }        /* NULL 뒤에 노드가 나왔다 */
        q[tail++] = cur->left;
        q[tail++] = cur->right;
    }
    free(q);
    return ok;
}

/* 좌우 높이가 같고 양쪽이 모두 포화일 때만 포화이다. 시프트 불필요. */
static int perfectHeight(const TreeNode *r, int *h)
{
    int left = 0, right = 0;
    if (!r) { *h = 0; return 1; }
    if (!perfectHeight(r->left, &left) ||
        !perfectHeight(r->right, &right) || left != right) return 0;
    *h = left + 1;
    return 1;
}
static int lnkIsPerfect(TreeNode *root)
{
    int h;
    return perfectHeight(root, &h);
}


/* 차수 1 이하인지 검사하면서 왼쪽/오른쪽 간선 수를 센다 */
static void lnkSkewScan(const TreeNode *r, int *left, int *right)
{
    if (r == NULL) return;
    if (r->left)  (*left)++;
    if (r->right) (*right)++;
    lnkSkewScan(r->left, left, right);
    lnkSkewScan(r->right, left, right);
}

static int lnkSkewKind(TreeNode *r)
{
    int left = 0, right = 0;
    if (lnkDegree(r) > 1) return 0;
    if (lnkCount(r) == 1) return 4;          /* 노드 1개 : 자명한 경우 */
    lnkSkewScan(r, &left, &right);
    if (left > 0 && right > 0) return 3;
    if (right > 0) return 2;
    return 1;
}

static void lnkShape(TreeNode *r)
{
    int n = lnkCount(r);
    int h = lnkHeight(r);

    printf("\n[연결 자료구조] 이진트리 형태 판별\n");
    line('-', 50);
    printf("  노드 수 n = %d, 높이 h = %d\n\n", n, h);
    {
        int complete = lnkIsComplete(r);
        printf("  완전 이진트리 여부 : %s\n",
               complete < 0 ? "판별 실패 (메모리 부족)" : complete ? "예" : "아니오");
    }
    printf("  포화 이진트리 여부 : %s   (모든 레벨이 채워짐: n == 2^h-1)\n",
           lnkIsPerfect(r) ? "예" : "아니오");
    printf("  편향 이진트리 여부 : %s\n", skewText(lnkSkewKind(r)));
    line('-', 50);
}


/*==========================================================================
  3. 배열 구현과 연결 구현의 비교
==========================================================================*/

/*--------------------------------------------------------------------------
  [1] 메모리 사용량 측정
      배열  : (2^h) * sizeof(Cell)      - 노드가 없는 자리도 모두 차지한다
      연결  : n * sizeof(TreeNode)      - 실제 노드 수에만 비례한다
--------------------------------------------------------------------------*/
static void memCompare(const ArrayTree *t, TreeNode *root)
{
    int    n = arrCountNodes(t);            /* 배열 쪽에서 센 노드 수 */
    int    m = lnkCount(root);              /* 연결 쪽에서 센 노드 수 */
    int    h = arrHeight(t);
    double arrBytes = (double)t->capacity * (double)sizeof(Cell);
    double lnkBytes = (double)m * (double)sizeof(TreeNode);
    long   maxIdx = 0, i;

    for (i = 1; i < t->capacity; i++) if (arrUsed(t, i)) maxIdx = i;

    printf("\n[3-1] 현재 트리의 메모리 사용량 비교\n");
    line('=', 68);
    printf("  노드 1개당 크기\n");
    printf("    배열 원소 Cell     = %2lu 바이트 (이름 %d바이트)\n",
           (unsigned long)sizeof(Cell), MAX_LABEL);
    printf("    연결 노드 TreeNode = %2lu 바이트 (이름 %d + 포인터 %lu x 2)\n\n",
           (unsigned long)sizeof(TreeNode), MAX_LABEL,
           (unsigned long)sizeof(TreeNode *));

    printf("  노드 수 n          = %d  (배열 %d개 / 연결 %d개 : 일치 확인)\n",
           n, n, m);
    printf("  높이   h           = %d\n", h);
    printf("  배열 할당 칸 수     = 2^h = %ld  (0번 칸 미사용, 유효 1..%ld)\n",
           t->capacity, t->capacity - 1);
    printf("  그중 실제 사용 칸   = %d칸  (할당 대비 %.1f%%)\n",
           n, 100.0 * n / (double)t->capacity);
    printf("  가장 큰 사용 인덱스 = %ld\n\n", maxIdx);

    printf("  배열 구현 메모리   = %.0f 바이트\n", arrBytes);
    printf("  연결 구현 메모리   = %.0f 바이트\n", lnkBytes);
    printf("  배열 / 연결        = %.2f 배\n", lnkBytes > 0 ? arrBytes / lnkBytes : 0.0);
    if (arrBytes < lnkBytes)
        printf("\n  -> 이 트리에서는 배열 구현이 더 적은 메모리를 쓴다.\n");
    else if (arrBytes > lnkBytes)
        printf("\n  -> 이 트리에서는 연결 구현이 더 적은 메모리를 쓴다.\n");
    else
        printf("\n  -> 두 구현의 메모리 사용량이 같다.\n");
    printf("  측정 범위: 동적 할당 요청량. 관리 변수/파서/큐/스택/할당기 비용 제외.\n");
    printf("  2^h칸은 이 구현의 예약 정책이며 배열의 필수 최소 크기는 아닙니다.\n");
    line('=', 68);
}

/* 형태별 메모리 사용량 종합 실험 (실제 구성 기반) */
/* 실제 문자열을 두 구현에 적재한 뒤 실제 할당량을 계산한다. */
static int genComplete(char *buf, int pos, int i, int n)
{
    pos += sprintf(buf + pos, "N%d", i);
    if (2 * i <= n) {
        pos += sprintf(buf + pos, "(");
        pos = genComplete(buf, pos, 2 * i, n);
        pos += sprintf(buf + pos, ",");
        if (2 * i + 1 <= n) pos = genComplete(buf, pos, 2 * i + 1, n);
        pos += sprintf(buf + pos, ")");
    }
    return pos;
}
static void genSkewed(char *buf, int n)
{
    int i, pos = 0;
    /* 내부 실험은 n <= 20: 결과는 MAX_INPUT보다 충분히 짧다. */
    for (i = 1; i < n; ++i) pos += sprintf(buf + pos, "N%d(", i);
    pos += sprintf(buf + pos, "N%d", n);
    for (i = 1; i < n; ++i) pos += sprintf(buf + pos, ",)");
}
static void memExperiment(void)
{
    int sizes[] = {10, 15}, si, kind;
    const char *names[] = {"일반", "완전", "편향"};
    const char *general[] = {
        "A(B(D,E(,H)),C(,F(G(I,J),)))",
        "A(B(D(H,),E(,I(J,))),C(F,G(K(,L(M,N)),O)))"
    };
    printf("\n[3-1] 실제 구성한 일반/완전/편향 트리의 할당량 비교\n");
    printf("  측정: 배열 capacity*sizeof(Cell), 연결 n*sizeof(TreeNode)\n");
    printf("  관리 변수, 파서, 임시 큐, 스택, 할당기 부가 비용은 제외합니다.\n");
    printf("  형태   n   h   배열칸   배열(B)   연결(B)   배열/연결   판별\n");
    for (si = 0; si < 2; ++si) {
        for (kind = 0; kind < 3; ++kind) {
            char expr[MAX_INPUT];
            ArrayTree a = {NULL, 0};
            TreeNode *r = NULL;
            int n, h;
            size_t ab, lb;
            if (!kind) strcpy(expr, general[si]);
            else if (kind == 1) genComplete(expr, 0, 1, sizes[si]);
            else genSkewed(expr, sizes[si]);
            if (!arrBuild(&a, expr) || !lnkBuild(&r, expr)) {
                printf("  실험 실패: %s\n", g_err);
                arrFree(&a); lnkFree(r); continue;
            }
            n = arrCountNodes(&a); h = arrHeight(&a);
            ab = (size_t)a.capacity * sizeof(Cell);
            lb = (size_t)lnkCount(r) * sizeof(TreeNode);
            if (n != sizes[si] || n != lnkCount(r)) {
                printf("  실험 실패: 노드 수 불일치\n");
            } else {
                printf("  "); padPrint(names[kind], 7);
                printf("%-4d%-4d%-9ld%-10zu%-10zu%.2f배   %s\n",
                       n, h, a.capacity, ab, lb, (double)ab / (double)lb,
                       arrSkewKind(&a) ? "편향" : arrIsComplete(&a) ?
                       (arrIsPerfect(&a) ? "완전(포화)" : "완전") : "일반");
                printf("    입력: %s\n", expr);
            }
            arrFree(&a); lnkFree(r);
        }
    }
    printf("  2^h칸은 이 구현의 정책이며, 최대 사용 인덱스+1만 예약할 수도 있습니다.\n");
    printf("  이 측정은 실행 환경의 sizeof를 사용한 할당 요청량이며 RSS가 아닙니다.\n");
}


/*--------------------------------------------------------------------------
  [2] 특정 노드의 자식 / 부모 / 형제 조회 (두 구현 비교)
      각 연산이 몇 번의 "탐색 단계"를 거쳤는지 세어 효율을 비교한다.
--------------------------------------------------------------------------*/

/* --- 배열 구현 --- */
static long arrFind(const ArrayTree *t, const char *lab, long *steps)
{
    long i;
    *steps = 0;
    for (i = 1; i < t->capacity; i++) {
        (*steps)++;                                  /* 칸 하나 확인 */
        if (t->cell[i].label[0] != '\0' &&
            strcmp(t->cell[i].label, lab) == 0) return i;
    }
    return -1;
}

static void arrRelation(const ArrayTree *t, const char *lab)
{
    long idx, steps, tot;
    long p, l, r, s;

    idx = arrFind(t, lab, &steps);
    tot = steps;

    printf("\n[배열 표현] '%s' 의 관계 노드\n", lab);
    line('-', 60);
    if (idx < 0) {
        printf("  트리에 '%s' 노드가 없습니다. (탐색 %ld단계)\n", lab, steps);
        line('-', 60);
        return;
    }
    printf("  위치(인덱스)    : %ld            [탐색 %ld단계]\n", idx, steps);

    l = 2 * idx; r = 2 * idx + 1; p = idx / 2;
    s = (idx % 2 == 0) ? idx + 1 : idx - 1;          /* 형제 = idx XOR 1 */

    printf("  왼쪽 자식       : ");
    padPrint(arrUsed(t, l) ? t->cell[l].label : "없음", 12);
    printf("[계산 1단계 : 2*i = %ld]\n", l);

    printf("  오른쪽 자식     : ");
    padPrint(arrUsed(t, r) ? t->cell[r].label : "없음", 12);
    printf("[계산 1단계 : 2*i+1 = %ld]\n", r);

    printf("  부모            : ");
    padPrint((idx > 1 && arrUsed(t, p)) ? t->cell[p].label : "없음(루트)", 12);
    printf("[계산 1단계 : i/2 = %ld]\n", p);

    printf("  형제            : ");
    padPrint((idx > 1 && arrUsed(t, s)) ? t->cell[s].label : "없음", 12);
    printf("[계산 1단계 : i^1 = %ld]\n", s);
    tot += 4;
    printf("\n  총 비용 : 노드 탐색 %ld단계 + 관계 계산 4단계 = %ld단계\n",
           steps, tot);
    printf("  (관계 계산은 배열 첨자 산술만으로 끝나므로 각각 O(1))\n");
    line('-', 60);
}

/* --- 연결 구현 --- */
static TreeNode *lnkFind(TreeNode *r, const char *lab, TreeNode *parent,
                         TreeNode **foundParent, long *steps)
{
    TreeNode *f;
    if (!r) return NULL;
    ++*steps;
    if (!strcmp(r->label, lab)) { *foundParent = parent; return r; }
    f = lnkFind(r->left, lab, r, foundParent, steps);
    return f ? f : lnkFind(r->right, lab, r, foundParent, steps);
}





static void lnkRelation(TreeNode *root, const char *lab)
{
    long steps = 0;
    TreeNode *par = NULL, *sib;
    TreeNode *node = lnkFind(root, lab, NULL, &par, &steps);
    printf("\n[연결 자료구조] '%s' 의 관계 노드\n", lab);
    if (!node) {
        printf("  트리에 '%s' 노드가 없습니다. (탐색 %ld단계)\n", lab, steps);
        return;
    }
    sib = par ? (par->left == node ? par->right : par->left) : NULL;
    printf("  왼쪽 자식       : %s\n", node->left ? node->left->label : "없음");
    printf("  오른쪽 자식     : %s\n", node->right ? node->right->label : "없음");
    printf("  부모            : %s\n", par ? par->label : "없음(루트)");
    printf("  형제            : %s\n", sib ? sib->label : "없음");
    printf("  총 비용 : 노드 탐색 %ld + 관계 확인 4 = %ld단계\n", steps, steps + 4);
    printf("  검색할 때 부모도 함께 얻으므로 재탐색과 부모 포인터 추가가 필요 없습니다.\n");
}


static void relationCompare(const ArrayTree *t, TreeNode *root, const char *lab)
{
    long aSteps, fSteps = 0;
    TreeNode *par = NULL;
    arrRelation(t, lab);
    lnkRelation(root, lab);
    if (arrFind(t, lab, &aSteps) < 0 ||
        !lnkFind(root, lab, NULL, &par, &fSteps)) return;
    printf("\n[3-2] 이름 입력부터 관계 확인까지의 비교\n");
    printf("  배열: %ld칸 확인 + 관계 4 = %ld단계; 최악 O(2^h)\n", aSteps, aSteps + 4);
    printf("  연결: %ld노드 방문 + 관계 4 = %ld단계; 최악 O(n)\n", fSteps, fSteps + 4);
    printf("  둘 다 검색 후 관계 확인은 O(1)입니다. 연결은 재귀 스택 O(h)를 씁니다.\n");
    printf("  위치만 알고 부모를 모르는 연결 노드는 별도 부모 검색이 필요합니다.\n");
    printf("  단계는 슬롯/노드 방문 및 관계 확인을 세는 비교 모형이며 CPU 시간은 아닙니다.\n");
    printf("  조밀한 배열은 빈칸 낭비가 적고, 성긴 배열은 이름 검색 시 빈칸도 확인합니다.\n");
}



/*==========================================================================
  4. 메인 : 메뉴
==========================================================================*/

static void banner(void)
{
    printf("\n");
    line('=', 68);
    printf("          과제 02 : 이진트리 프로그램 (배열 표현 / 연결 표현)\n");
    line('=', 68);
    printf("  괄호 표기법 예시\n");
    printf("    일반 : A(B(D,E),C(,F))\n");
    printf("    포화 : A(B(D,E),C(F,G))\n");
    printf("    완전 : A(B(D,E),C(F,))\n");
    printf("    편향 : A(B(C(D,),),)   또는  A(,B(,C(,D)))\n");
    line('=', 68);
}

static void menu(void)
{
    printf("\n");
    line('-', 68);
    printf("  [입력]  1. 이진트리 입력 (괄호 표기법)\n");
    printf("  [배열]  2. 트리 출력   3. 트리 정보   4. 형태 판별   5. 배열 내부 보기\n");
    printf("  [연결]  6. 트리 출력   7. 트리 정보   8. 형태 판별\n");
    printf("  [비교]  9. 현재 트리 메모리 비교   10. 형태별 메모리 실험\n");
    printf("         11. 노드의 자식/부모/형제 조회 (두 구현 비교)\n");
    printf("          0. 종료\n");
    line('-', 68);
    printf("  선택 > ");
}

/* 한 줄 입력받아 개행 제거 */
/* 초과 줄은 끝까지 소비한 뒤 같은 입력을 다시 받는다. EOF면 0. */
static int readLine(char *buf, int size)
{
    for (;;) {
        size_t len;
        int c;
        fflush(stdout);
        if (!fgets(buf, size, stdin)) return 0;
        len = strlen(buf);
        if (!strchr(buf, '\n')) {
            c = getchar();
            if (c != '\n' && c != EOF) {
                while ((c = getchar()) != '\n' && c != EOF) {}
                printf("  ! 입력이 너무 깁니다 (최대 %d바이트). 다시 입력 > ", size - 1);
                if (c == EOF) return 0;
                continue;
            }
        }
        while (len && (buf[len - 1] == '\n' || buf[len - 1] == '\r')) buf[--len] = '\0';
        return 1;
    }
}


int main(void)
{
    ArrayTree atree;
    TreeNode *ltree = NULL;
    char buf[MAX_INPUT];
    int  loaded = 0;
    int  sel;

    arrInit(&atree);
    banner();

    for (;;) {
        menu();
        if (!readLine(buf, sizeof(buf))) break;
        if (buf[0] == '\0') continue;
        {
            char *end;
            long choice;
            errno = 0;
            choice = strtol(buf, &end, 10);
            if (end == buf) {
                printf("  ! 메뉴 번호를 입력하세요.\n");
                continue;
            }
            while (isspace((unsigned char)*end)) ++end;
            if (errno || end == buf || *end || choice < 0 || choice > 11) {
                printf("  ! 0부터 11까지의 메뉴 번호를 입력하세요.\n");
                continue;
            }
            sel = (int)choice;
        }

        if (sel == 0) break;

        if (sel != 1 && sel != 10 && !loaded) {
            printf("\n  ! 먼저 1번으로 이진트리를 입력하세요.\n");
            continue;
        }

        switch (sel) {
        case 1:
            printf("\n  괄호 표기법으로 이진트리를 입력하세요.\n");
            printf("  (왼쪽이 비어 있으면 콤마 앞을 비웁니다. 예: A(,C))\n");
            printf("  트리 > ");
            if (!readLine(buf, sizeof(buf))) goto cleanup;
            if (buf[0] == '\0') { printf("\n  ! 입력이 비어 있습니다.\n"); break; }

            {
                ArrayTree nextArray = {NULL, 0};
                TreeNode *nextLinked = NULL;
                g_err[0] = '\0';
                if (!arrBuild(&nextArray, buf) || !lnkBuild(&nextLinked, buf)) {
                    printf("\n  ! 입력 오류 : %s (기존 트리는 유지됩니다.)\n", g_err);
                    arrFree(&nextArray); lnkFree(nextLinked);
                    break;
                }
                arrFree(&atree); lnkFree(ltree);
                atree = nextArray; ltree = nextLinked;
            }
            loaded = 1;
            printf("\n  트리를 두 가지 방식으로 저장했습니다. "
                   "(노드 %d개, 높이 %d)\n",
                   arrCountNodes(&atree), arrHeight(&atree));
            break;

        case 2:  arrPrint(&atree);  break;
        case 3:  arrInfo(&atree);   break;
        case 4:  arrShape(&atree);  break;
        case 5:  arrDump(&atree);   break;

        case 6:  lnkPrint(ltree);   break;
        case 7:  lnkInfo(ltree);    break;
        case 8:  lnkShape(ltree);   break;

        case 9:  memCompare(&atree, ltree); break;
        case 10: memExperiment();   break;

        case 11:
            printf("\n  조회할 노드 이름 > ");
            if (!readLine(buf, sizeof(buf))) goto cleanup;
            if (buf[0] == '\0') { printf("\n  ! 이름이 비어 있습니다.\n"); break; }
            {
                const char *p = buf;
                char lab[MAX_LABEL];
                parsedCount = 0;
                skipSpace(&p);
                if (!readLabel(&p, lab)) { printf("  ! %s\n", g_err); break; }
                skipSpace(&p);
                if (*p) { printf("  ! 노드 이름 하나만 입력하세요.\n"); break; }
                relationCompare(&atree, ltree, lab);
            }
            break;

        default:
            printf("\n  ! 메뉴에 없는 번호입니다.\n");
        }
    }

cleanup:
    arrFree(&atree);
    lnkFree(ltree);
    printf("\n  프로그램을 종료합니다.\n\n");
    return 0;
}
