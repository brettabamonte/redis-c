#include <stdio.h>
#include <stdlib.h>

struct HNode
{
    HNode *next = NULL;
    uint64_t hcode = 0; // cached hash value
};

struct HTab
{
    HNode **tab = NULL; // array of HNode
    size_t mask = 0;    // 2^n - 1
    size_t size = 0;
};

struct HMap
{
    HTab h1; // newer
    HTab h2; // older
    size_t resizing_pos = 0;
};

const size_t k_max_load_factor = 0;
const size_t k_resizing_work = 128; // constant work

void hm_insert(HMap *hmap, HNode *node);