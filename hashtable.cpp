#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static void h_init(HTab *htab, size_t n)
{
    assert(n > 0 && ((n - 1) & n) == 0);
    htab->tab = (HNode **)calloc(sizeof(HNode *), n);
    htab->mask = n - 1;
    htab->size = 0;
}

static void h_insert(HTab *htab, HNode *node)
{
    size_t pos = node->hcode & htab->mask; // slot index
    HNode *next = htab->tab[pos];          // prepend the list
    node->next = next;
    htab->tab[pos] = node;
    htab->size++;
}

// Lookup HNode in HTab
// Returns: Pointer to HNode
static HNode **h_lookup(HTab *htab, HNode *key, bool (*eq)(HNode *, HNode *))
{
    // Check if a table is init for hashtable
    if (!htab->tab)
    {
        return NULL;
    }

    // Get position of key
    size_t pos = key->hcode & htab->mask;

    HNode **from = &htab->tab[pos]; // incoming pointer to the result

    // Loop through chained array til we find matching node
    for (HNode *cur; (cur = *from) != NULL; from = &cur->next)
    {
        if (cur->hcode == key->hcode && eq(cur, key))
        {
            return from;
        }
    }

    return NULL;
}