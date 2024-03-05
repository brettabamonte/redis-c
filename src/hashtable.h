#pragma once

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

void hm_insert(HMap *hmap, HNode *node);
HNode *hm_pop(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *));
void hm_destroy(HMap *hmap);
size_t hm_size(HMap *hmap);