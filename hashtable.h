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