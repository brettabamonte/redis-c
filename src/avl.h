#include <stdint.h>
#include <stdlib.h>

struct AVLNode {
    uint32_t depth = 0; //subtree height
    uint32_t count = 0; //subtree size
    AVLNode *left = NULL;
    AVLNode *right = NULL;
    AVLNode *parent = NULL;
};