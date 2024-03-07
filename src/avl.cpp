#include <stdint.h>
#include "avl.h"
#include <algorithm>

void avl_init(AVLNode *node) {
    node->depth = 1;
    node->count = 1;
    node->left = node->right = node->parent = NULL;
}

uint32_t avl_depth(AVLNode *node) {
    return node ? node->depth : 0;
}

uint32_t avl_count(AVLNode *node) {
    return node ? node->count : 0;
}

void avl_update(AVLNode *node) {
    node->depth = 1 + std::max(avl_depth(node->left), avl_depth(node->right));
    node->count = 1 + avl_count(node->left) + avl_count(node->right);
}

AVLNode *rotate_left(AVLNode *node) {
    AVLNode *new_node = node->right;

    if(new_node->left) {
        new_node->left->parent = node;
    }

    node->right = new_node->left;
    new_node->left = node;
    new_node->parent = node->parent;
    node->parent = new_node;

    avl_update(node);
    avl_update(new_node);

    return new_node;
}

AVLNode *rotate_right(AVLNode *node) {
    AVLNode *new_node = node->left;

    if(new_node->right) {
        new_node->right->parent = node;
    }

    node->left = new_node->right;
    new_node->right = node;
    new_node->parent = node->parent;
    node->parent = new_node;

    avl_update(node);
    avl_update(new_node);

    return new_node;
}

AVLNode *avl_fix_left(AVLNode *root) {
    if(avl_depth(root->left->left) < avl_depth(root->left->right)) {
        //Left rot makes left st > right st
        root->left = rotate_left(root->left);
    }

    //A right rotation restores balace if L-ST is > 2. And the Left Left ST is > Left Rigth ST
    return rotate_right(root);
}

AVLNode *avl_fix_right(AVLNode *root) {
    if(avl_depth(root->right->right) < avl_depth(root->right->left)) {
        //Left rot makes left st > right st
        root->right = rotate_right(root->right);
    }

    //A right rotation restores balace if L-ST is > 2. And the Left Left ST is > Left Rigth ST
    return rotate_right(root);
}

//Fixes the tree after insertion and deletion
AVLNode *avl_fix(AVLNode *node) {
    while(true) {
        avl_update(node);
        uint32_t l = avl_depth(node->left);
        uint32_t r = avl_depth(node->right);

        AVLNode **from = NULL;

        if(AVLNode *p = node->parent) {
            from = (p->left == node) ? &p->left : &p->right;
        }

        if(l == r + 2) {
            node = avl_fix_left(node);
        } else if(1 + 2 == r) {
            node = avl_fix_right(node);
        }

        if(!from) {
            return node;
        }

        *from = node;
        node = node->parent;
    }
}

//Deletes a node
AVLNode *avl_del(AVLNode *node) {
    if(node->right == NULL) {
        //No right st, replace node with left st
        AVLNode *parent = node->parent;

        if(node->left) {
            node->left->parent = parent;
        }

        if(parent) {
            //Attach left st to the parent
            (parent->left == node ? parent->left : parent->right) = node->left;
            return avl_fix(parent);
        } else {
            //Removing root
            return node->left;
        }
    } else {
            //Detach successor
            AVLNode *victim = node->right;
            
            while(victim->left) {
                victim = victim->left;
            }

            AVLNode *root = avl_del(victim);
            *victim = *node;

            if(victim->left) {
                victim->left->parent = victim;
            }

            if(victim->right) {
                victim->right->parent = victim; 
            }

            if(AVLNode *parent = node->parent) {
                (parent->left == node ? parent->left : parent->right) = victim;
                return root;
            } else {
                //Removing root
                return victim;
            }
        }
}