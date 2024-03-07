#include "../src/avl.h"
#include <assert.h>
#include <algorithm>
#include <set>

#define container_of(ptr, type, member) ({ \
    const typeof(((type *)0)->member) *__mptr = ptr; \
    (type *)((char *) __mptr - __offsetof(type, member)); \
})


struct Data {
    AVLNode node;
    uint32_t val = 0;
};

struct Container {
    AVLNode *root = NULL;
};

static void add(Container &c, uint32_t val) {
    Data *data = new Data();
    avl_init(&data->node);
    data->val = val;

    AVLNode *cur = NULL; //current node
    AVLNode **from = &c.root; //Incoming pointer to next node. Helps avoid special case of inserting root
    while(*from) {
        cur = *from;
        uint32_t node_val = container_of(cur, Data, node)->val;
        from = (val < node_val) ? &cur->left : &cur->right;
    }

    //Attach the new node
    *from = &data->node;
    data->node.parent = cur;
    c.root = avl_fix(&data->node);
}

static bool del(Container &c, uint32_t val) {
    AVLNode *cur = c.root;

    while(cur) {
        uint32_t node_val = container_of(cur, Data, node)->val;
        
        if(val == node_val) {
            break;
        }

        cur = val < node_val ? cur->left : cur->right;
    }
    //Didn't find node to delete
    if(!cur) {
        return false;
    }

    c.root = avl_del(cur);
    delete container_of(cur, Data, node);
    return true;
}

static void avl_verify(AVLNode *parent, AVLNode *node) {
    if(!node) {
        return;
    }

    //Verify subtrees
    avl_verify(node, node->left);
    avl_verify(node, node->right);

    //Verify parent
    assert(node->parent == parent);

    //Make sure auxiliary data is good
    assert(node->count == 1 + avl_count(node->left) + avl_count(node->right));
    uint32_t left_depth = avl_depth(node->left);
    uint32_t right_depth = avl_depth(node->right);
    assert(node->depth == 1 + std::max(left_depth, right_depth));

    //Check height
    assert(left_depth == right_depth || left_depth + 1 == right_depth || right_depth + 1 == left_depth);

    //Make sure data is in order
    uint32_t parent_val = container_of(node, Data, node)->val;
    if(node->left) {
        assert(node->left->parent == node);
        assert(container_of(node->left, Data, node)->val <= parent_val);
    }
    if(node->right) {
        assert(node->right->parent == node);
        assert(container_of(node->right, Data, node)->val >= parent_val);
    }
}

static void extract(AVLNode *node, std::multiset<uint32_t> &extracted) {
    if(!node) {
        return;
    }

    extract(node->left, extracted);
    extracted.insert(container_of(node, Data, node)->val);
    extract(node->right, extracted);
}

static void container_verify(Container &c, std::multiset<uint32_t> &ref) {
    avl_verify(NULL, c.root);
    assert(avl_count(c.root) == ref.size());
    std::multiset<uint32_t> extracted;
    extract(c.root, extracted);
    assert(extracted == ref);
}

static void dispose(Container &c) {
    while(c.root) {
        AVLNode *node = c.root;
        c.root = avl_del(c.root);
        delete container_of(node, Data, node);
    }
}

static void test_insert(uint32_t test_size) {
    for(uint32_t val = 0; val < test_size; ++val) {
        Container c;
        std::multiset<uint32_t> ref;
        for(uint32_t i = 0; i < test_size; ++i) {
            if(i == val) {
                continue;
            }

            add(c, i);
            ref.insert(i);
        }

        container_verify(c, ref);

        add(c, val);
        ref.insert(val);
        container_verify(c, ref);
        dispose(c);
    }
}

static void test_remove(uint32_t test_size) {
    for(uint32_t val = 0; val < test_size; ++val) {
        Container c;
        std::multiset<uint32_t> ref;
        for(uint32_t i = 0; i < test_size; ++i) {
            add(c, i);
            ref.insert(i);
        }

        container_verify(c, ref);

        assert(del(c, val));
        ref.erase(val);
        container_verify(c, ref);
        dispose(c);
    }
}

int main() {
    Container c;
    std::multiset<uint32_t> ref;

    add(c, 123);
    assert(!del(c, 200));
    assert(del(c, 123));

    //TODO: 
    //Tree is not rotating properly. BF hits 2 and tests fail

    //Sequential insertion
    for(uint32_t i = 0; i < 10; i++) {
        //uint32_t val = (uint32_t)rand() % 1000;
        add(c, i);
        ref.insert(i);
        printf("%lu, ", i);
        container_verify(c, ref);
    }
}