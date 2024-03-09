#include <stdio.h>
#include <stdlib.h>
#include "avl.h"

uint32_t node_height(struct AVLNode *node) {
    uint32_t height_left, height_right;

    height_left = node && node->left ? node->left->height : 0;
    height_right = node && node->right ? node->right->height : 0;

    return 1 + (height_left > height_right ? height_left : height_right);
}

uint32_t node_count(struct AVLNode *node) {
    if(!node) return 0;

    return 1 + node_count(node->left) + node_count(node->right);
}

int balanceFactor(struct AVLNode *node) {
    int height_left, height_right;

    height_left = node && node->left ? node->left->height : 0;
    height_right = node && node->right ? node->right->height : 0;

    return height_left - height_right;
}

struct AVLNode *ll_rotation(struct AVLNode **root, struct AVLNode **node) {
    struct AVLNode *left_child = (*node)->left;

    (*node)->left = left_child->right;
    left_child->right = (*node);

    (*node)->height = node_height((*node));
    left_child->height = node_height(left_child);

    if(*root == (*node)) {
        *root = left_child;
    }

    return left_child;
}

/*
Ex. 
1               3
 3       =>   1   4
2 4            2
*/
struct AVLNode *rr_rotation(struct AVLNode **root, struct AVLNode **node) {
    struct AVLNode *right_child = (*node)->right;

    //Peform a single left rotation
    (*node)->right = right_child->left;
    right_child->left = (*node);

    //Recalculate heights
    (*node)->height = node_height((*node));
    right_child->height = node_height(right_child);

    if(*root == *node) {
        *root = right_child;
    }

    return right_child;
}

struct AVLNode *lr_rotation(struct AVLNode **root, struct AVLNode **node) {

    //Rotate right on the right child
    (*node)->left = rr_rotation(root, &((*node)->left));

    //Rotate left on the node    
    return ll_rotation(root, node);
}

/*
Left subtree that is right heavy. We need to make it left heavy
Ex.
 30                 30              20
10          =>     20  =>        10   30
 20               10  
*/
struct AVLNode *rl_rotation(struct AVLNode **root, struct AVLNode **node) {
    (*node)->right = ll_rotation(root, &((*node)->right));
    return rr_rotation(root, node);
}

struct AVLNode *insert(struct AVLNode **root, struct AVLNode **node, uint32_t key) {
    //Create node
    if(*node == NULL) {
        *node = new AVLNode();
        (*node)->height = 1;
        (*node)->left = (*node)->right = NULL;
        (*node)->val = key;

        if(*root == NULL) {
            *root = *node;
        }

        return *node;
    }

    if(key < (*node)->val) {
        (*node)->left = insert(root, &((*node)->left), key);
    } else if(key > (*node)->val) {
        (*node)->right = insert(root, &((*node)->right), key);
    }

    (*node)->height = node_height((*node));

    //Check balance factors. Perform appropriate rotations
    if(balanceFactor((*node)) == 2 && balanceFactor((*node)->left) == 1) {
        return ll_rotation(root, node);
    } else if(balanceFactor((*node)) == 2 && balanceFactor((*node)->left) == -1) {
        return lr_rotation(root, node);
    } else if(balanceFactor((*node)) == -2 && balanceFactor((*node)->right) == -1) {
        return rr_rotation(root, node);
    } else if(balanceFactor((*node)) == -2 && balanceFactor((*node)->right) == 1) {
        return rl_rotation(root, node);
    }

    return *node;
}

bool del(struct AVLNode **root, struct AVLNode **node, uint32_t key) {
    if(*node == NULL) {
        return false;
    }

    if(key < (*node)->val) {
        return del(root, &((*node)->left), key);
    } else if(key > (*node)->val) {
        return del(root, &((*node)->right), key);
    } else {
        //We found node to delete
        
        //Check for 1 child or leaf
        if((*node)->left == NULL || (*node)->right == NULL) {
            struct AVLNode *temp = (*node)->left ? (*node)->left : (*node)->right;

            //Leaf
            if(!temp) {
                //temp = (*node);
                (*node) = NULL;
                return true;
            } else {
                //One child
                struct AVLNode *to_delete = *node;
                *node = temp;
                free(to_delete);
                return true;
            }
        } else {
            //Multiple children. Get inorder successor
            //Inorder successor is deepest left child in right sub-tree
            struct AVLNode **succ_parent = &((*node)->right);

            while((*succ_parent)->left != NULL) {
                succ_parent = &((*succ_parent)->left);
            }

            //copy contents of inorder successor
            (*node)->val = (*succ_parent)->val;

            //delete inorder successor
            del(root, succ_parent, (*succ_parent)->val);

            return true;
        }
    }

    if(*node != NULL) {
        ///Update heights
        (*node)->height = node_height((*node));

        //Check balance factors and perform rotations
        //Check balance factors. Perform appropriate rotations
        if(balanceFactor((*node)) == 2 && balanceFactor((*node)->left) == 1) {
            ll_rotation(root, node);
        } else if(balanceFactor((*node)) == 2 && balanceFactor((*node)->left) == -1) {
            rl_rotation(root, node);
        } else if(balanceFactor((*node)) == -2 && balanceFactor((*node)->right) == -1) {
            rr_rotation(root, node);
        } else if(balanceFactor((*node)) == -2 && balanceFactor((*node)->right) == 1) {
            lr_rotation(root, node);
        }
    }
}