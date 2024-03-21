#pragma once

#include "../src/avl.h"
#include "../src/hashtable.h"
#include "hashtable.h"

struct ZSet {
    AVLNode *tree_root = NULL;
    HMap hashmap;
};

struct ZNode {
    AVLNode tree_node; //index by (score, key)
    HNode hashmap_node; //index by key
    uint32_t score = 0;
    size_t len = 0;
    char *key;
};

int znode_compare(AVLNode *lhs, AVLNode *rhs);
ZNode *zset_pop(ZSet *zset, const char *key, size_t len);
bool zset_add(ZSet *zset, const char *key, size_t len, double score);
ZNode *zset_lookup(ZSet *zset, double score, const char *name, size_t len);
void znode_del(ZNode *node);
