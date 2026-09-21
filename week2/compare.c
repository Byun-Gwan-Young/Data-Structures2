/* 과제 02 - 3. 구현 비교 (C11, 독립 실행 파일)
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

static void line(char c, int n)
{
    int i;
    for (i = 0; i < n; i++) putchar(c);
    putchar('\n');
}

static int dispWidth(const char *s)
{
    int w = 0;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c < 0x80)              { w += 1; s += 1; }   
        else if ((c & 0xE0) == 0xC0) { w += 1; s += 2; }
        else if ((c & 0xF0) == 0xE0) { w += 2; s += 3; } 
        else                         { w += 2; s += 4; }
    }
    return w;
}

static void padPrint(const char *s, int width)
{
    int w = dispWidth(s), i;
    fputs(s, stdout);
    for (i = w; i < width; i++) putchar(' ');
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

static int arrCountNodes(const ArrayTree *t)
{
    long i; int n = 0;
    for (i = 1; i < t->capacity; i++) if (arrUsed(t, i)) n++;
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

static int lnkParse(const char **p, TreeNode **out)
{
    char lab[MAX_LABEL];
    TreeNode *n;

    *out = NULL;
    skipSpace(p);

    if (**p == '\0' || **p == ',' || **p == ')') return 1;   

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

static int lnkCount(const TreeNode *r)
{
    if (r == NULL) return 0;
    return 1 + lnkCount(r->left) + lnkCount(r->right);
}

static void memCompare(const ArrayTree *t, TreeNode *root)
{
    int    n = arrCountNodes(t);            
    int    m = lnkCount(root);              
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

static long arrFind(const ArrayTree *t, const char *lab, long *steps)
{
    long i;
    *steps = 0;
    for (i = 1; i < t->capacity; i++) {
        (*steps)++;                                  
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
    s = (idx % 2 == 0) ? idx + 1 : idx - 1;          

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
    ArrayTree array = {NULL, 0};
    TreeNode *root = NULL;
    char buf[MAX_INPUT];
    for (;;) {
        int sel;
        printf("\n[비교] 1.입력 2.현재 메모리 3.형태별 메모리 실험 4.관계 조회 0.종료\n선택 > ");
        sel = chooseMenu(4);
        if (sel == -1 || sel == 0) break;
        if (sel == -2) { puts("메뉴 번호를 다시 입력하세요."); continue; }
        if (sel == 1) {
            ArrayTree next = {NULL, 0};
            TreeNode *nextRoot = NULL;
            printf("트리 > ");
            if (!readLine(buf, sizeof(buf))) break;
            if (!arrBuild(&next, buf) || !lnkBuild(&nextRoot, buf)) {
                printf("입력 오류: %s (기존 트리 유지)\n", g_err);
                arrFree(&next); lnkFree(nextRoot); continue;
            }
            arrFree(&array); lnkFree(root);
            array = next; root = nextRoot;
            puts("저장했습니다.");
        } else if (sel == 3) memExperiment();
        else if (!root) puts("먼저 트리를 입력하세요.");
        else if (sel == 2) memCompare(&array, root);
        else if (sel == 4) {
            char lab[MAX_LABEL];
            const char *p;
            printf("노드 이름 > ");
            if (!readLine(buf, sizeof(buf))) break;
            p = buf; parsedCount = 0; skipSpace(&p);
            if (!readLabel(&p, lab)) { printf("입력 오류: %s\n", g_err); continue; }
            skipSpace(&p);
            if (*p) { puts("노드 이름 하나만 입력하세요."); continue; }
            relationCompare(&array, root, lab);
        }
    }
    arrFree(&array); lnkFree(root);
    return 0;
}
