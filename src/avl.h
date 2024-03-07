#include <stdint.h>
#include <stdlib.h>

struct AVLNode {
    uint32_t depth = 0; //subtree height
    uint32_t count = 0; //subtree size
    AVLNode *left = NULL;
    AVLNode *right = NULL;
    AVLNode *parent = NULL;
};

void avl_update(AVLNode *node);
AVLNode *rotate_left(AVLNode *node);
AVLNode *rotate_right(AVLNode *node);
AVLNode *avl_fix_left(AVLNode *root);
AVLNode *avl_fix_right(AVLNode *root);
AVLNode *avl_fix(AVLNode *node);
AVLNode *avl_del(AVLNode *node);
void avl_init(AVLNode *node);