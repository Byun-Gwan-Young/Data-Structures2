/* 과제 02 - 1. 배열 구현 (C11, 독립 실행 파일)
   입력: 중복 없는 영숫자/밑줄 이름 1~15자, 한 줄 최대 511바이트.
   실행 및 비교 분석: 과제02_정리.md 참조. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LABEL    16     
#define MAX_INPUT   512     
#define MAX_HEIGHT   20     
#define INDENT        6     

static char g_err[256];                     

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

static char parsedLabels[MAX_INPUT][MAX_LABEL];
static size_t parsedCount;

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

typedef struct {
    char label[MAX_LABEL];      
} Cell;

typedef struct {
    Cell *cell;                 
    long  capacity;             
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

static int arrUsed(const ArrayTree *t, long i)
{
    return (i >= 1 && i < t->capacity && t->cell[i].label[0] != '\0');
}

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

static int arrParse(const char **p, ArrayTree *t, long idx)
{
    char lab[MAX_LABEL];

    skipSpace(p);

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
    if (**p == '(') {                       
        (*p)++;
        if (!arrParse(p, t, 2 * idx)) return 0;         
        skipSpace(p);
        if (**p == ',') {
            (*p)++;
            if (!arrParse(p, t, 2 * idx + 1)) return 0; 
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

static int  arrHeight(const ArrayTree *t);

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
        for (k = i; k >= 1; k >>= 1) lv++;   
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

static int arrIsComplete(const ArrayTree *t)
{
    int n = arrCountNodes(t);
    long i;
    for (i = 1; i <= n; i++) if (!arrUsed(t, i)) return 0;
    return 1;   
}

static int arrIsPerfect(const ArrayTree *t)
{
    int n = arrCountNodes(t);
    int h = arrHeight(t);
    return arrIsComplete(t) && (n == (1 << h) - 1);
}

static int arrSkewKind(const ArrayTree *t)
{
    long i;
    int  left = 0, right = 0;

    if (arrDegree(t) > 1) return 0;          
    if (arrCountNodes(t) == 1) return 4;     
    for (i = 1; i < t->capacity; i++) {
        if (!arrUsed(t, i)) continue;
        if (arrUsed(t, 2 * i))     left++;
        if (arrUsed(t, 2 * i + 1)) right++;
    }
    if (left > 0 && right > 0) return 3;
    if (right > 0) return 2;
    return 1;                                 
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

static int chooseMenu(int maximum)
{
    char buf[MAX_INPUT], *end;
    long value;
    if (!readLine(buf, sizeof(buf))) return -1;
    errno = 0;
    value = strtol(buf, &end, 10);
    if (end == buf) return -2;
    while (isspace((unsigned char)*end)) ++end;
    if (errno || *end || value < 0 || value > maximum) return -2;
    return (int)value;
}

int main(void)
{
    ArrayTree tree = {NULL, 0};
    char buf[MAX_INPUT];
    for (;;) {
        int sel;
        printf("\n[배열] 1.입력 2.출력 3.정보 4.형태 5.배열 내부 0.종료\n선택 > ");
        sel = chooseMenu(5);
        if (sel == -1 || sel == 0) break;
        if (sel == -2) { puts("메뉴 번호를 다시 입력하세요."); continue; }
        if (sel == 1) {
            ArrayTree next = {NULL, 0};
            printf("트리 > ");
            if (!readLine(buf, sizeof(buf))) break;
            if (!arrBuild(&next, buf)) {
                printf("입력 오류: %s (기존 트리 유지)\n", g_err);
                arrFree(&next); continue;
            }
            arrFree(&tree); tree = next;
            puts("저장했습니다.");
        } else if (!tree.cell) puts("먼저 트리를 입력하세요.");
        else switch (sel) {
            case 2: arrPrint(&tree); break;
            case 3: arrInfo(&tree); break;
            case 4: arrShape(&tree); break;
            case 5: arrDump(&tree); break;
        }
    }
    arrFree(&tree);
    return 0;
}
