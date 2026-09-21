/* 과제 02 - 2. 연결 구현 (C11, 독립 실행 파일)
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

static int lnkIsComplete(TreeNode *root)
{
    TreeNode **q;
    int n, head = 0, tail = 0, seenNull = 0, ok = 1;

    if (root == NULL) return 1;
    n = lnkCount(root);
    q = (TreeNode **)malloc(sizeof(TreeNode *) * (size_t)(2 * n + 2));
    if (q == NULL) return -1; 

    q[tail++] = root;
    while (head < tail) {
        TreeNode *cur = q[head++];
        if (cur == NULL) { seenNull = 1; continue; }
        if (seenNull) { ok = 0; break; }        
        q[tail++] = cur->left;
        q[tail++] = cur->right;
    }
    free(q);
    return ok;
}

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
    if (lnkCount(r) == 1) return 4;          
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
    TreeNode *root = NULL;
    char buf[MAX_INPUT];
    for (;;) {
        int sel;
        printf("\n[연결] 1.입력 2.출력 3.정보 4.형태 0.종료\n선택 > ");
        sel = chooseMenu(4);
        if (sel == -1 || sel == 0) break;
        if (sel == -2) { puts("메뉴 번호를 다시 입력하세요."); continue; }
        if (sel == 1) {
            TreeNode *next = NULL;
            printf("트리 > ");
            if (!readLine(buf, sizeof(buf))) break;
            if (!lnkBuild(&next, buf)) {
                printf("입력 오류: %s (기존 트리 유지)\n", g_err);
                lnkFree(next); continue;
            }
            lnkFree(root); root = next;
            puts("저장했습니다.");
        } else if (!root) puts("먼저 트리를 입력하세요.");
        else switch (sel) {
            case 2: lnkPrint(root); break;
            case 3: lnkInfo(root); break;
            case 4: lnkShape(root); break;
        }
    }
    lnkFree(root);
    return 0;
}
