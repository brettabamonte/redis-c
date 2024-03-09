#include <stdint.h>

struct AVLNode {
    struct AVLNode *left;
    struct AVLNode *right; 
    int height; 
    uint32_t val;
};

struct Data {
    struct AVLNode *node;
    uint32_t val;
};

uint32_t node_count(struct AVLNode *node);
int balanceFactor(struct AVLNode *node);
uint32_t node_height(struct AVLNode *node);
struct AVLNode *insert(struct AVLNode **root, struct AVLNode **node, uint32_t key);
bool del(struct AVLNode **root, struct AVLNode **node, uint32_t key);
