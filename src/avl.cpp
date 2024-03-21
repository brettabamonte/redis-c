#include <stdio.h>
#include <stdlib.h>
#include "avl.h"
#include "zset.h"
#include "utils.h"

uint32_t node_height(struct AVLNode *node) {

    if(!node) return 0;

    uint32_t height_left, height_right;

    height_left = node->left ? node->left->height : 0;
    height_right = node->right ? node->right->height : 0;

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

static struct AVLNode *ll_rotation(struct AVLNode *node) {
    struct AVLNode *left_child = node->left;

    node->left = left_child->right;
    left_child->right = node;

    node->height = node_height(node);
    left_child->height = node_height(left_child);

    return left_child;
}

/*
Ex. 
1               3
 3       =>   1   4
2 4            2
*/
static struct AVLNode *rr_rotation(struct AVLNode *node) {
    struct AVLNode *right_child = node->right;

    //Peform a single left rotation
    node->right = right_child->left;
    right_child->left = node;

    //Recalculate heights
    node->height = node_height(node);
    right_child->height = node_height(right_child);
    return right_child;
}

static struct AVLNode *lr_rotation(struct AVLNode *node) {

    //Rotate right on the right child
    node->left = rr_rotation(node->left);

    //Rotate left on the node    
    return ll_rotation(node);
}

/*
Left subtree that is right heavy. We need to make it left heavy then perform rr rotation.
Ex.
 30                 30              20
10          =>     20  =>        10   30
 20               10  
*/
static struct AVLNode *rl_rotation(struct AVLNode *node) {
    node->right = ll_rotation(node->right);
    return rr_rotation(node);
}

struct AVLNode *insert(struct AVLNode **cur_node, struct AVLNode *new_node, int(node_compare_func)(AVLNode *, AVLNode *)) {
    //Check if tree is empty
    if(!*cur_node) {
        *cur_node = new_node;
        return *cur_node;
    }

    //Compare znodes
    if(node_compare_func(new_node, *cur_node) == -1) {
        (*cur_node)->left = insert(&((*cur_node)->left), new_node, node_compare_func);
    } else if(node_compare_func(new_node, *cur_node) == 1) {
        (*cur_node)->right = insert(&((*cur_node)->right), new_node, node_compare_func);
    } else if(node_compare_func(new_node, *cur_node) == 0) {
        //Duplicate nodes??/?
        return *cur_node;
    }

    //Re-calculate height of nodes
    (*cur_node)->height = node_height(*cur_node);

    //Check balance factors. Perform appropriate rotations
    if(balanceFactor(*cur_node) == 2 && balanceFactor((*cur_node)->left) == 1) {
        return ll_rotation(*cur_node);
    } else if(balanceFactor(*cur_node) == 2 && balanceFactor((*cur_node)->left) == -1) {
        return lr_rotation(*cur_node);
    } else if(balanceFactor(*cur_node) == -2 && balanceFactor((*cur_node)->right) == -1) {
        return rr_rotation(*cur_node);
    } else if(balanceFactor(*cur_node) == -2 && balanceFactor((*cur_node)->right) == 1) {
        return rl_rotation(*cur_node);
    }

    return *cur_node;
}

//FIXME: Nodes are not getting deleted from tree
//Things to check
//1. Double check node compare function
//2. Verify correct traversal of tree
//3. Verify proper rotating of tree after deletion..bottom-up towards root
bool del(struct AVLNode **cur_node, struct AVLNode *node_to_delete, int(node_compare_func)(AVLNode *, AVLNode *)) {
    
    //Didn't find node to delete
    if(*cur_node == NULL) {
        return false;
    }

    if(node_compare_func(node_to_delete, *cur_node) == -1) {
        return del(&((*cur_node)->left), node_to_delete, node_compare_func);
    } else if(node_compare_func(node_to_delete, *cur_node) == 1) {
        return del(&((*cur_node)->right), node_to_delete, node_compare_func);
    } else {
        //We found node to delete

        //Check for 1 child or leaf
        if((*cur_node)->left == NULL || (*cur_node)->right == NULL) {
            struct AVLNode *temp = (*cur_node)->left ? (*cur_node)->left : (*cur_node)->right;

            //Leaf
            if(!temp) {
                //temp = (*node);
                (*cur_node) = NULL;
            } else {
                //One child
                struct AVLNode *to_delete = *cur_node;
                *cur_node = temp;
                free(to_delete);
            }
        } else {
            //Multiple children. Get inorder successor
            //Inorder successor is deepest left child in right sub-tree
            struct AVLNode **inorder_successor = &((*cur_node)->right);

            while((*inorder_successor)->left != NULL) {
                inorder_successor = &((*inorder_successor)->left);
            }

            //copy contents of inorder successor
            ZNode *cur_node_container = container_of(*cur_node, ZNode, tree_node);
            ZNode *inorder__successor_container = container_of(*inorder_successor, ZNode, tree_node);

            double temp_score = inorder__successor_container->score;
            char temp_key[0];
            memcpy(&temp_key, inorder__successor_container->key, inorder__successor_container->len);
            size_t temp_len = inorder__successor_container->len;

            //delete inorder successor
            del(cur_node, *inorder_successor, node_compare_func);

            //paste contents from inorder succ into current node
            cur_node_container->score = temp_score;
            memcpy(&(cur_node_container->key), temp_key, temp_len);
            cur_node_container->len = temp_len;

            //delete znode
            free(inorder__successor_container);
        }
    }

    //If cur_node isn't deleted
    if(*cur_node) {
        ///Update height
        (*cur_node)->height = node_height((*cur_node));

        //Check balance factors and perform rotations
        if(balanceFactor((*cur_node)) == 2 && balanceFactor((*cur_node)->left) == 1) {
            *cur_node = ll_rotation(*cur_node);
        } else if(balanceFactor((*cur_node)) == 2 && balanceFactor((*cur_node)->left) == -1) {
            *cur_node = rl_rotation(*cur_node);
        } else if(balanceFactor((*cur_node)) == -2 && balanceFactor((*cur_node)->right) == -1) {
            *cur_node = rr_rotation(*cur_node);
        } else if(balanceFactor((*cur_node)) == -2 && balanceFactor((*cur_node)->right) == 1) {
            *cur_node = lr_rotation(*cur_node);
        }
    }

    return true;
}

void inorder_traversal(AVLNode *node) {
    
    if(node == NULL) return;

    inorder_traversal(node->left);
    ZNode *znode = container_of(node, ZNode, tree_node);
    printf("(%s, %lu)\n", znode->key, (unsigned long)znode->score);
    inorder_traversal(node->right);
}
