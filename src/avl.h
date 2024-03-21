#pragma once

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
struct AVLNode *insert(struct AVLNode **cur_node, struct AVLNode *new_node, int(node_compare_func)(AVLNode *, AVLNode *));
bool del(struct AVLNode **cur_node, struct AVLNode *node_to_delete, int(node_compare_func)(AVLNode *, AVLNode *));
void inorder_traversal(AVLNode *node);
