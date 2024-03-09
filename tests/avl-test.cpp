#include "../src/avl.h"
#include <assert.h>
#include <algorithm>
#include <set>

#define container_of(ptr, type, member) ({ \
    const typeof(((type *)0)->member) *__mptr = ptr; \
    (type *)((char *) __mptr - __offsetof(type, member)); \
})

struct Container {
    AVLNode *root = NULL;
};

static void add(Container *c, uint32_t val) {
    insert(&(c->root), &(c->root), val);
}

static bool remove(Container *c, uint32_t val) {
    return del(&(c->root), &(c->root), val);
}

static void node_verify(AVLNode *parent, AVLNode *node) {
    if(node == NULL) {
        return;
    }

    if(parent == NULL) {
        return;
    }

    //Verify subtrees
    node_verify(node, node->left);
    node_verify(node, node->right);

    //Verify balance factor
    assert(balanceFactor(node) >= -1 && balanceFactor(node) <= 1);

    //Verify height
    assert(node->height == 1 + std::max(node_height(node->left), node_height(node->right)));

    //Make sure data is in order
    if(node->left) {
        assert(parent->val <= node->val);
    }
    if(node->right) {
        assert(parent->val >= node->val);
    }
}

static void extract(AVLNode *node, std::multiset<uint32_t> &extracted) {
    if(!node) {
        return;
    }

    extract(node->left, extracted);
    extracted.insert(node->val);
    extract(node->right, extracted);
}

static void tree_verify(AVLNode *root, std::multiset<uint32_t> &ref) {
    //Verify node count of tree and ref are equal
    assert(node_count(root) == ref.size());

    //Verify reference is equal to 
    std::multiset<uint32_t> extracted;
    extract(root, extracted);
    assert(extracted == ref);

    //Verify nodes
    node_verify(NULL, root);
}

static void inorder_traversal(AVLNode *node) {
    if(!node) return;

    inorder_traversal(node->left);
    printf("%lu ", node->val);
    inorder_traversal(node->right);
}

static int search(AVLNode *node, uint32_t key) {
    if(!node) return 0;

    if(key < node->val) {
        return search(node->left, key);
    } else if(key > node->val) {
        return search(node->right, key);
    } else {
        return 1;
    }
}

int main() {
    Container *c = new Container();
    std::multiset<uint32_t> ref;

    //Random insertion & deletion
    for(uint32_t i = 0; i < 1000; i++) {
        //Random value
        uint32_t val = (uint32_t)rand() ^ 999999;
        
        //Check for duplicate before adding to ref
        if(search(c->root, val) == 0) {
            ref.insert(val);
        }

        //Insert to tree
        add(c, val);

        tree_verify(c->root, ref);
    }

    for(uint32_t i = 0; i < 100; i++) {
        uint32_t val = (uint32_t)rand() & 999;
        remove(c, val);
        ref.erase(val);
        tree_verify(c->root, ref);
    }
}