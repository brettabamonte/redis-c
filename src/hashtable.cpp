#include "hashtable.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

const size_t k_max_load_factor = 8;
const size_t k_resizing_work = 128; // constant work

//Initalizes a hashtable that is a power of 2
static void h_init(HTab *htab, size_t n)
{
    assert(n > 0 && ((n - 1) & n) == 0);
    htab->tab = (HNode **)calloc(sizeof(HNode *), n);
    htab->mask = n - 1;
    htab->size = 0;
}

//Inserts a node into a hashtable
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

//Deletes node from hashtable
static HNode *h_detach(HTab *htab, HNode **from)
{
    HNode *node = *from;
    *from = node->next;
    htab->size--;
    return node;
}

//Moves the newer hashtable to the older hashtable. Then increases size of newer hashtable by factor of 2
static void hm_start_resizing(HMap *hmap)
{
    assert(hmap->h2.tab == NULL);

    // Create a bigger hashtable and swap them
    hmap->h2 = hmap->h1;
    h_init(&hmap->h1, (hmap->h1.mask + 1) * 2);
    hmap->resizing_pos = 0;
}

//Moves some keys to the new table. Triggered from lookups and updates
static void hm_help_resizing(HMap *hmap)
{
    size_t nwork = 0;

    while (nwork < k_resizing_work && hmap->h2.size > 0)
    {
        // Scan for nodes in ht2 and move them to ht1
        HNode **from = &hmap->h2.tab[hmap->resizing_pos];

        if (!*from)
        {
            hmap->resizing_pos++;
            continue;
        }

        h_insert(&hmap->h1, h_detach(&hmap->h2, from));
        nwork++;
    }

    if (hmap->h2.size == 0 && hmap->h2.tab)
    {
        // We are done. Free memory of older table.
        free(hmap->h2.tab);

        // Init a new struct for the older table
        hmap->h2 = HTab();
    }
}

//Inserts a node into the hashmap
void hm_insert(HMap *hmap, HNode *node)
{
    if (!hmap->h1.tab)
    {
        // Init a new hashtable of size 4 (2^2)
        h_init(&hmap->h1, 4);
    }

    // Insert key into newer table
    h_insert(&hmap->h1, node);

    // Check the load factor of the newer table
    if (!hmap->h2.tab)
    {
        size_t load_factor = hmap->h1.size / (hmap->h1.mask + 1);
        if (load_factor >= k_max_load_factor)
        {
            // Create a larger table
            hm_start_resizing(hmap);
        }
    }

    // Move some keys into the newer table
    hm_help_resizing(hmap);
}

//Check both tables in hashmap
HNode *hm_lookup(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *))
{
    hm_help_resizing(hmap);
    HNode **from = h_lookup(&hmap->h1, key, eq);
    from = from ? from : h_lookup(&hmap->h2, key, eq);
    return from ? *from : NULL;
}

//Deletes node from hashtable
HNode *hm_pop(HMap *hmap, HNode *key, bool (*eq)(HNode *, HNode *))
{
    hm_help_resizing(hmap);

    if (HNode **from = h_lookup(&hmap->h1, key, eq))
    {
        return h_detach(&hmap->h1, from);
    }

    if (HNode **from = h_lookup(&hmap->h2, key, eq))
    {
        return h_detach(&hmap->h2, from);
    }

    return NULL;
}

//Returns size of the two hashtables
size_t hm_size(HMap *hmap)
{
    return hmap->h1.size + hmap->h2.size;
}

//Destroys hashmap
void hm_destroy(HMap *hmap)
{
    free(hmap->h1.tab);
    free(hmap->h2.tab);
    *hmap = HMap();
}