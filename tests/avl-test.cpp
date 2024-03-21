#include "../src/zset.h"
#include "../src/avl.h"
#include "../src/hashtable.h"
#include "../src/utils.h"
#include <assert.h>
#include <algorithm>
#include <set>

#define container_of(ptr, type, member) ({ \
    const typeof(((type *)0)->member) *__mptr = ptr; \
    (type *)((char *) __mptr - __offsetof(type, member)); \
})

struct Entry {
    struct HNode node;
    uint32_t type = 0;
    std::string key;
    std::string val; //string
    uint32_t val2;
    ZSet *zset = NULL; //sorted set
};

struct HKey {
    HNode hnode;
    const char *key = NULL;
    size_t len = 0;
};

static bool entry_eq(HNode *lhs, HNode *rhs) {
    struct Entry *le = container_of(lhs, struct Entry, node);
    struct Entry *re = container_of(rhs, struct Entry, node);
    return le->key == re->key;
}

static char *generate_key() {
    const int TEST_KEY_LENGTH = 10;
    // Allocate memory for the string, including space for the null terminator
    char *key = (char *)malloc(TEST_KEY_LENGTH + 1);

    if (key != NULL) {
        for (int i = 0; i < TEST_KEY_LENGTH; i++) {
            // Generate a random uppercase letter and store it in the key
            key[i] = rand() % (90 - 65 + 1) + 65;
        }

        // Add the null terminator at the end of the string
        key[TEST_KEY_LENGTH] = '\0';
    }

    // Return the dynamically allocated key
    return key;
}

static int node_compare(AVLNode *lhs, double score, const char *key, size_t len) {
    ZNode *zl = container_of(lhs, ZNode, tree_node);

    //First compare score
    if(zl->score != score) {
        return (zl->score < score) ? -1 : 1;
    }

    //Compare name
    int rv = memcmp(zl->key, key, min(zl->len, len));
    if(rv != 0) {
        return rv;
    }

    //Compare length
    if(zl->len != len) {
        return zl->len < len ? -1 : 1;
    }
    
    return 0;
}

int znode_compare(AVLNode *lhs, AVLNode *rhs) {
    ZNode *zr = container_of(rhs, ZNode, tree_node);
    return node_compare(lhs, zr->score, zr->key, zr->len);
}

static void add(ZSet *zset, ZNode *znode_to_add, std::string key_to_add) {
    Entry key;
    key.key.swap(key_to_add);
    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());

    //Create new entry into hashtable.
    Entry *entry = new Entry();
    entry->key.swap(key.key);
    entry->node.hcode = key.node.hcode;
    entry->val2 = znode_to_add->score;
    hm_insert(&(zset->hashmap), &entry->node);

    zset->tree_root = insert(&(zset->tree_root), &(znode_to_add->tree_node), znode_compare);
}

//TODO: Implement remove function for zset
//static bool remove(ZSet *zset, std::string key_to_delete) {
    //find node
//    Entry key;
//    key.key.swap(key_to_delete);
//    key.node.hcode = str_hash((uint8_t *)key.key.data(), key.key.size());
//
//    HNode *node = hm_lookup(&(zset->hashmap), &key.node, &entry_eq);
//
//    ZNode *znode = container_of(node, ZNode, hashmap_node);
//    
//    return del(&(zset->tree_root), &(znode->tree_node), znode_compare);
//}

static void node_verify(AVLNode *node) {
    if(node == NULL) {
        return;
    }

    //Verify subtrees
    node_verify(node->left);
    node_verify(node->right);

    //Verify balance factor
    assert(balanceFactor(node) >= -1 && balanceFactor(node) <= 1);

    //Verify height
    assert(node->height == 1 + std::max(node_height(node->left), node_height(node->right)));

    //Make sure data is in order
    struct ZNode *node_container = container_of(node, ZNode, tree_node);

    if(node->left) {
        struct ZNode *child_container = container_of(node->left, ZNode, tree_node);
        assert(child_container->score <= node_container->score);
    }
    if(node->right) {
        struct ZNode *child_container = container_of(node->right, ZNode, tree_node);
        assert(child_container->score >= node_container->score);
    }
}

static ZNode *zset_lookup(ZSet *zset, const char *key_to_lookup, size_t len) {
    if(!zset->tree_root) {
        return NULL;
    }

    HKey key;
    key.hnode.hcode = str_hash((uint8_t *)key_to_lookup, len);
    key.key = key_to_lookup;
    key.len = len;
    HNode *found = hm_lookup(&(zset->hashmap), &key.hnode, &entry_eq);
    return found ? container_of(found, ZNode, hashmap_node) : NULL;
}

static void extract(AVLNode *node, std::multiset<uint32_t> &extracted) {
    if(!node) {
        return;
    }

    extract(node->left, extracted);
    uint32_t node_score = container_of(node, ZNode, tree_node)->score;
    extracted.insert(node_score);
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
    node_verify(root);
}

static void traversal(AVLNode *node) {
    if(!node) return;

    traversal(node->left);
    ZNode *znode = container_of(node, ZNode, tree_node);
    if(znode) {
        printf("(%s, %d) and hashnode info: hcode - %lu\n", znode->key, znode->score, znode->hashmap_node.hcode);
    }
    traversal(node->right);
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
  //Create zset obj and reference multiset for testing
  //create basic hashtable
  ZSet *zset = (struct ZSet *)malloc(sizeof(struct ZSet));

  std::multiset<uint32_t> ref;
  char **keys = (char **)malloc(sizeof(char *) * 100);
  int pos = 0;

    //Random insertion & deletion
    for(uint32_t i = 0; i < 100; i++) {
        //Random value and key
        uint32_t val = (uint32_t)rand() ^ 999999;
        char *key = generate_key();
        
        //Check for duplicate before adding to ref
        if(search(zset->tree_root, val) == 0) {
            ref.insert(val);
        }
        keys[pos++] = key;
        //Create node
        ZNode *znode = new ZNode();
        znode->hashmap_node.next = NULL;
        znode->hashmap_node.hcode = str_hash((uint8_t *)key, sizeof(key));

        znode->tree_node.height = 1;
        znode->score = val;        
        znode->key = (char *)malloc(sizeof(key) + 1);
        strcpy(znode->key, key);
        znode->len = strlen(key);

        //Insert to tree
        add(zset, znode, key);
    }

    //Verify
    tree_verify(zset->tree_root, ref);

    //Traverse tree in order and print results as (key, score)
    traversal(zset->tree_root);

}
