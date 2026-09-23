/*
 * Assignment 03 - Binary Tree Manipulation
 * Build: gcc -std=c11 -Wall -Wextra -Wpedantic -O2 main.c -o tree03
 *
 * NOTE ABOUT THE ASSIGNMENT:
 * Section 3 allows insertion when a parent has fewer than 2 children,
 * while sections 4 and 5 say that the parent must be a leaf. Those rules
 * conflict: leaf-only insertion would make two children impossible.
 * The default below follows section 3 and permits insertion into an empty
 * L/R position, including the second child. Set STRICT_LEAF_PARENT to 1
 * if your instructor explicitly requires the leaf-only interpretation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_NODES 1024
#define MAX_LINE 4096
#define MAX_INPUT_TOKENS 1024
#define STRICT_LEAF_PARENT 0

typedef struct Node {
    char data;
    struct Node *left;
    struct Node *right;
    struct Node *parent;
} Node;

typedef struct {
    Node *root;
    size_t size;
    size_t capacity;
} BTree;

typedef struct {
    int has_left;
    int has_right;
    char left;
    char right;
} ChildInfo;

typedef enum {
    PATH_OK,
    PATH_INVALID,
    PATH_NOT_FOUND
} PathStatus;

/* -------- The required binary-tree ADT operations -------- */

BTree *create_btree(size_t size) {
    if (size == 0) return NULL;
    BTree *tree = calloc(1, sizeof(*tree));
    if (tree != NULL) tree->capacity = size;
    return tree;
}

static Node *make_node(char value, Node *parent) {
    Node *node = malloc(sizeof(*node));
    if (node == NULL) return NULL;
    node->data = value;
    node->left = NULL;
    node->right = NULL;
    node->parent = parent;
    return node;
}

static int uppercase_letter(const char *word) {
    return word != NULL && word[0] >= 'A' && word[0] <= 'Z'
           && word[1] == '\0';
}

BTree *insert_root(BTree *tree, char value) {
    if (tree == NULL || tree->root != NULL || tree->size >= tree->capacity
        || value < 'A' || value > 'Z') return NULL;
    Node *root = make_node(value, NULL);
    if (root == NULL) return NULL;
    tree->root = root;
    ++tree->size;
    return tree;
}

BTree *insert_child(BTree *tree, Node *parent, char child, char value) {
    if (tree == NULL || parent == NULL || (child != 'L' && child != 'R')
        || value < 'A' || value > 'Z' || tree->size >= tree->capacity)
        return NULL;
    Node **slot = child == 'L' ? &parent->left : &parent->right;
    Node *sibling = child == 'L' ? parent->right : parent->left;
    if (*slot != NULL || (sibling != NULL && sibling->data == value))
        return NULL;
#if STRICT_LEAF_PARENT
    if (parent->left != NULL || parent->right != NULL) return NULL;
#endif
    Node *node = make_node(value, parent);
    if (node == NULL) return NULL;
    *slot = node;
    ++tree->size;
    return tree;
}

BTree *delete_node(BTree *tree, Node *leaf) {
    if (tree == NULL || leaf == NULL || leaf->left != NULL || leaf->right != NULL)
        return NULL;
    if (leaf->parent == NULL) {
        if (tree->root != leaf) return NULL;
        tree->root = NULL;
    } else if (leaf->parent->left == leaf) {
        leaf->parent->left = NULL;
    } else if (leaf->parent->right == leaf) {
        leaf->parent->right = NULL;
    } else {
        return NULL;
    }
    free(leaf);
    --tree->size;
    return tree;
}

BTree *update_value(BTree *tree, Node *node, char value) {
    if (tree == NULL || node == NULL || value < 'A' || value > 'Z')
        return NULL;
    if (node->parent != NULL) {
        Node *sibling = node->parent->left == node
                            ? node->parent->right : node->parent->left;
        if (sibling != NULL && sibling->data == value) return NULL;
    }
    node->data = value;
    return tree;
}

ChildInfo read_child(const BTree *tree, const Node *parent) {
    ChildInfo info = {0, 0, '\0', '\0'};
    if (tree == NULL || parent == NULL) return info;
    if (parent->left != NULL) {
        info.has_left = 1;
        info.left = parent->left->data;
    }
    if (parent->right != NULL) {
        info.has_right = 1;
        info.right = parent->right->data;
    }
    return info;
}

/* A +---B, followed by indented descendants, left child before right child. */
static void print_subtree(const Node *node, size_t depth) {
    if (node == NULL) return;
    for (size_t i = 0; i < depth; ++i) printf("    ");
    printf("+---%c\n", node->data);
    print_subtree(node->left, depth + 1);
    print_subtree(node->right, depth + 1);
}

void print_btree(const BTree *tree) {
    if (tree == NULL || tree->root == NULL) {
        puts("Tree is empty.");
        return;
    }
    printf("%c\n", tree->root->data);
    print_subtree(tree->root->left, 1);
    print_subtree(tree->root->right, 1);
}

static void free_subtree(Node *node) {
    if (node == NULL) return;
    free_subtree(node->left);
    free_subtree(node->right);
    free(node);
}

void destroy_btree(BTree *tree) {
    if (tree == NULL) return;
    free_subtree(tree->root);
    free(tree);
}

/* -------- Path and command parsing -------- */

/* A valid existing-node path has the form /A/B/C (never just /). */
static int valid_path(const char *path) {
    if (path == NULL || path[0] != '/' || path[1] == '\0') return 0;
    size_t i = 1;
    for (;;) {
        if (path[i] < 'A' || path[i] > 'Z') return 0;
        ++i;
        if (path[i] == '\0') return 1;
        if (path[i] != '/' || path[i + 1] == '\0') return 0;
        ++i;
    }
}

static PathStatus find_node(const BTree *tree, const char *path, Node **result) {
    *result = NULL;
    if (!valid_path(path)) return PATH_INVALID;
    if (tree == NULL || tree->root == NULL || tree->root->data != path[1])
        return PATH_NOT_FOUND;

    Node *current = tree->root;
    for (size_t i = 2; path[i] != '\0'; i += 2) {
        char wanted = path[i + 1]; /* path[i] is '/' */
        if (current->left != NULL && current->left->data == wanted)
            current = current->left;
        else if (current->right != NULL && current->right->data == wanted)
            current = current->right;
        else
            return PATH_NOT_FOUND;
    }
    *result = current;
    return PATH_OK;
}

static int equal_ignore_case(const char *a, const char *b) {
    if (a == NULL || b == NULL) return 0;
    while (*a != '\0' && *b != '\0') {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b))
            return 0;
        ++a;
        ++b;
    }
    return *a == '\0' && *b == '\0';
}

static int is_command(const char *token, const char *word, const char *short_name) {
    return equal_ignore_case(token, word) || equal_ignore_case(token, short_name);
}

static int parse_child(const char *text, char *child) {
    if (is_command(text, "Left", "L")) {
        *child = 'L';
        return 1;
    }
    if (is_command(text, "Right", "R")) {
        *child = 'R';
        return 1;
    }
    return 0;
}

static int resolve_or_report(const BTree *tree, const char *path, Node **node) {
    PathStatus status = find_node(tree, path, node);
    if (status == PATH_INVALID) {
        printf("Error: Invalid path '%s'. Use /A/B/...\n", path);
        return 0;
    }
    if (status == PATH_NOT_FOUND) {
        printf("Error: Node does not exist: %s\n", path);
        return 0;
    }
    return 1;
}

static void run_insert(BTree *tree, int argc, char **argv) {
    /* Special root command: Insert / A */
    if (argc == 3 && strcmp(argv[1], "/") == 0) {
        if (!uppercase_letter(argv[2])) {
            puts("Error: New data must be one uppercase letter (A-Z).");
        } else if (tree->root != NULL) {
            puts("Error: Root already exists.");
        } else if (tree->size >= tree->capacity) {
            puts("Error: Tree capacity reached.");
        } else if (insert_root(tree, argv[2][0]) == NULL) {
            puts("Error: Could not allocate the root node.");
        }
        return;
    }
    if (argc != 4) {
        puts("Error: Usage: Insert / A  OR  Insert /A/B L|R C");
        return;
    }
    if (!uppercase_letter(argv[3])) {
        puts("Error: New data must be one uppercase letter (A-Z).");
        return;
    }
    char direction;
    if (!parse_child(argv[2], &direction)) {
        puts("Error: Child position must be L/Left or R/Right.");
        return;
    }
    Node *parent;
    if (!resolve_or_report(tree, argv[1], &parent)) return;
    Node *slot = direction == 'L' ? parent->left : parent->right;
    Node *other = direction == 'L' ? parent->right : parent->left;
    if (slot != NULL) {
        puts("Error: The specified child position is occupied.");
    } else if (other != NULL && other->data == argv[3][0]) {
        puts("Error: Siblings cannot have identical data.");
#if STRICT_LEAF_PARENT
    } else if (parent->left != NULL || parent->right != NULL) {
        puts("Error: The parent must be a leaf node.");
#endif
    } else if (tree->size >= tree->capacity) {
        puts("Error: Tree capacity reached.");
    } else if (insert_child(tree, parent, direction, argv[3][0]) == NULL) {
        puts("Error: Could not allocate the new node.");
    }
}

static void run_delete(BTree *tree, int argc, char **argv) {
    if (argc != 2) {
        puts("Error: Usage: Delete /A/B/C");
        return;
    }
    Node *node;
    if (!resolve_or_report(tree, argv[1], &node)) return;
    if (node->left != NULL || node->right != NULL) {
        puts("Error: Only a leaf node can be deleted.");
    } else if (delete_node(tree, node) == NULL) {
        puts("Error: Could not delete the node.");
    }
}

static void run_update(BTree *tree, int argc, char **argv) {
    if (argc != 3) {
        puts("Error: Usage: Update /A/B/C X");
        return;
    }
    if (!uppercase_letter(argv[2])) {
        puts("Error: New data must be one uppercase letter (A-Z).");
        return;
    }
    Node *node;
    if (!resolve_or_report(tree, argv[1], &node)) return;
    Node *sibling = NULL;
    if (node->parent != NULL) {
        sibling = node->parent->left == node
                      ? node->parent->right : node->parent->left;
    }
    if (sibling != NULL && sibling->data == argv[2][0]) {
        puts("Error: Siblings cannot have identical data.");
    } else if (update_value(tree, node, argv[2][0]) == NULL) {
        puts("Error: Could not update the node.");
    }
}

static void run_read(BTree *tree, int argc, char **argv) {
    if (argc != 2) {
        puts("Error: Usage: Read /A/B");
        return;
    }
    Node *parent;
    if (!resolve_or_report(tree, argv[1], &parent)) return;
    ChildInfo info = read_child(tree, parent);
    if (!info.has_left && !info.has_right) {
        puts("No children.");
        return;
    }
    if (info.has_left) printf("%c(L)", info.left);
    if (info.has_left && info.has_right) printf(", ");
    if (info.has_right) printf("%c(R)", info.right);
    putchar('\n');
}

/* Return 0 to exit, 1 to continue. Every complete input line is one command. */
static int execute_command(BTree *tree, int argc, char **argv) {
    if (argc == 0) return 1;
    if (is_command(argv[0], "Insert", "I")) {
        run_insert(tree, argc, argv);
    } else if (is_command(argv[0], "Delete", "D")) {
        run_delete(tree, argc, argv);
    } else if (is_command(argv[0], "Update", "U")) {
        run_update(tree, argc, argv);
    } else if (is_command(argv[0], "Read", "R")) {
        run_read(tree, argc, argv);
    } else if (is_command(argv[0], "Print", "P")) {
        if (argc != 1) puts("Error: Usage: Print");
        else print_btree(tree);
    } else if (is_command(argv[0], "Quit", "Q") ||
               equal_ignore_case(argv[0], "Exit")) {
        if (argc != 1) {
            puts("Error: Usage: Quit");
        } else {
            return 0;
        }
    } else {
        printf("Error: Unknown command '%s'.\n", argv[0]);
    }
    return 1;
}

static int known_command_token(const char *token) {
    return is_command(token, "Insert", "I") ||
           is_command(token, "Delete", "D") ||
           is_command(token, "Update", "U") ||
           is_command(token, "Read", "R") ||
           is_command(token, "Print", "P") ||
           is_command(token, "Quit", "Q") ||
           equal_ignore_case(token, "Exit");
}

/*
 * Process one input line.  Normally one command is entered per line, but
 * this also accepts several commands pasted on the same line, e.g.
 *   Insert / A Insert /A L B Print
 */
static int process_line(BTree *tree, char *line) {
    char *tokens[MAX_INPUT_TOKENS];
    int count = 0;

    for (char *token = strtok(line, " \t\r\n"); token != NULL;
         token = strtok(NULL, " \t\r\n")) {
        if (count >= MAX_INPUT_TOKENS) {
            puts("Error: Input contains too many tokens.");
            return 1;
        }
        tokens[count++] = token;
    }

    int i = 0;
    while (i < count) {
        int expected = 1;

        if (is_command(tokens[i], "Insert", "I")) {
            /* Root insertion is: Insert / A (3 tokens total). */
            if (i + 1 < count && strcmp(tokens[i + 1], "/") == 0)
                expected = 3;
            else
                expected = 4;
        } else if (is_command(tokens[i], "Delete", "D") ||
                   is_command(tokens[i], "Read", "R")) {
            expected = 2;
        } else if (is_command(tokens[i], "Update", "U")) {
            expected = 3;
        } else if (is_command(tokens[i], "Print", "P") ||
                   is_command(tokens[i], "Quit", "Q") ||
                   equal_ignore_case(tokens[i], "Exit")) {
            expected = 1;
        } else {
            /* Unknown command: report it and continue with the next token. */
            if (!execute_command(tree, 1, &tokens[i])) return 0;
            ++i;
            continue;
        }

        int end = i + expected;
        if (end > count) end = count;

        /*
         * If extra non-command tokens follow, include them in this command so
         * execute_command() can correctly report an argument-count error.
         * If the next token is another command, start a new command there.
         */
        if (end < count && !known_command_token(tokens[end])) {
            while (end < count && !known_command_token(tokens[end])) ++end;
        }

        if (!execute_command(tree, end - i, &tokens[i])) return 0;
        i = end;
    }

    return 1;
}

int main(void) {
    BTree *tree = create_btree(MAX_NODES);
    if (tree == NULL) {
        fputs("Error: Could not create the binary tree.\n", stderr);
        return EXIT_FAILURE;
    }

    puts("========================================");
    puts(" Binary Tree Manipulation Program");
    puts("========================================");
    puts("Enter a command using one of the forms below:");
    puts("  Insert / A              : create root node");
    puts("  Insert /A L B           : insert left child");
    puts("  Insert /A R C           : insert right child");
    puts("  Delete /A/B             : delete a leaf node");
    puts("  Update /A/B X           : change node data");
    puts("  Read /A                 : show child information");
    puts("  Print                    : print the whole tree");
    puts("  Quit                     : exit program");
    puts("(Command names can also be entered as I, D, U, R, P, Q.)");
    puts("");

    char line[MAX_LINE];
    for (;;) {
        printf("Command > ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) break;
        /* Reject, then drain, any line longer than the input buffer. */
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] != '\n' && !feof(stdin)) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {}
            puts("Error: Input line is too long.");
            continue;
        }

        if (!process_line(tree, line)) break;
    }

    destroy_btree(tree);
    return EXIT_SUCCESS;
}
