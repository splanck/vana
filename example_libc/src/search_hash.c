/*
 * BSD 2-Clause "Simplified" License
 *
 * Copyright (c) 2025
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Purpose: Implements hcreate, hdestroy and hsearch using
 * a simple open-addressed hash table.
 */

#include "search.h"
#include "stdlib.h"
#include "string.h"
#include "memory.h"

static ENTRY *table;
static unsigned char *used;
static size_t table_size;
static size_t items;

/*
 * hash_str() - compute a simple hash for a string using the
 * djb2 algorithm. Used internally by the hash table helpers.
 */
static unsigned long hash_str(const char *s)
{
    unsigned long h = 5381;
    unsigned char c;
    while ((c = (unsigned char)*s++))
        h = ((h << 5) + h) + c;
    return h;
}

/*
 * hcreate() - allocate a hash table capable of storing at least
 * `nel` entries. Returns 1 on success and 0 on failure or if a
 * table already exists.
 */
int hcreate(size_t nel)
{
    if (table)
        return 0;
    table = calloc(nel, sizeof(ENTRY));
    if (!table)
        return 0;
    used = calloc(nel, 1);
    if (!used) {
        free(table);
        table = NULL;
        return 0;
    }
    table_size = nel;
    items = 0;
    return 1;
}

/*
 * hdestroy() - free all memory associated with the table created
 * by hcreate(). After calling this routine the hash table is
 * reset to the empty state.
 */
void hdestroy(void)
{
    free(table);
    free(used);
    table = NULL;
    used = NULL;
    table_size = 0;
    items = 0;
}

/*
 * hsearch() - look up an entry in the hash table or insert a new one
 * when `action` is ENTER. Returns a pointer to the entry on success
 * or NULL if no slot is available or the table was not created.
 */
ENTRY *hsearch(ENTRY item, ACTION action)
{
    if (!table || !item.key)
        return NULL;
    unsigned long h = hash_str(item.key);
    for (size_t i = 0; i < table_size; i++) {
        size_t idx = (h + i) % table_size;
        if (!used[idx]) {
            if (action == ENTER) {
                if (items >= table_size)
                    return NULL;
                table[idx] = item;
                used[idx] = 1;
                items++;
                return &table[idx];
            }
            return NULL;
        }
        if (strcmp(table[idx].key, item.key) == 0)
            return &table[idx];
    }
    return NULL;
}
