#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"
#include "util.h"

struct Hashmap
{
    String **keys; // array of String*
    int *data;     // array of int
    int size;
    int amount_size;
};

typedef unsigned int uint;

static uint calc_hash(uint size, int c, const String *key);
static void rehash(Hashmap *hmap);

Hashmap *
make_hashmap(int size)
{
    Hashmap *hmap;

    if (size <= 0) return NULL;

    hmap = (Hashmap*)try_malloc(sizeof(Hashmap));
    hmap->keys = (String**)try_calloc(size, sizeof(String*));
    hmap->data = (int*)try_calloc(size, sizeof(int));

    hmap->size = size;
    hmap->amount_size = 0;
    return hmap;
}

void
free_hashmap(Hashmap **hmap)
{
    Hashmap *hm = *hmap;
    int i;
    for (i = 0; i < hm->size; ++i)
    {
        if (*(hm->keys+i) == NULL) continue;
        free_string(hm->keys+i);
    }
    free(hm->keys);
    free(hm->data);
    free(*hmap);
    *hmap = NULL;
}

int
add_hashmap(Hashmap *hmap, const String *key, int data)
{
    unsigned int hash;
    int i;

    if (exists_hashmap(hmap, key)) return 0;
    if (hmap->amount_size > (int)(hmap->size*0.7)) rehash(hmap);

    for (i = 0; ; ++i)
    {
        hash = calc_hash(hmap->size, i, key);
        if (*(hmap->keys+hash) == NULL)
        {
            *(hmap->keys+hash) = new_string(key);
            *(hmap->data+hash) = data;
            break;
        }
    }

    hmap->amount_size++;
    return 1;
}

int
remove_hashmap(Hashmap *hmap, const String *key)
{
    uint hash;
    int i;

    for (i = 0; i < hmap->amount_size; ++i)
    {
        hash = calc_hash(hmap->size, i, key);
        if (*(hmap->keys+hash) == NULL) return 0;
        if (cmp_string(*(hmap->keys+hash), key) == 0)
        {
            free_string(hmap->keys+hash);
            *(hmap->keys+hash) = NULL;
            hmap->amount_size--;
            return 1;
        }
    }
    return 0;
}

int
exists_hashmap(Hashmap *hmap, const String *key)
{
    return search_hashmap(hmap, key, NULL);
}

int
search_hashmap(Hashmap *hmap, const String *key, int *data)
{
    uint hash;
    int i;
    
    for (i = 0; i < hmap->amount_size; ++i)
    {
        hash = calc_hash(hmap->size, i, key);
        if (*(hmap->keys+hash) == NULL) return 0;
        if (cmp_string(*(hmap->keys+hash), key) == 0)
        {
            if (data != NULL) *data = *(hmap->data+hash);
            return 1;
        }
    }
    return 0;
}

static uint
calc_hash(uint size, int c, const String *key)
{
    // FNV Hash
    uint hash = 2166136261u;
    int i, len = key->len;

    for (i = 0 ; i < len; ++i)
    {
        hash = (16777619u*hash) ^ (*key->str+i);
    }
    return (hash+c) % size;
}

static void
rehash(Hashmap *hmap)
{
    uint hash;
    int i, j;
    int new_size;
    String **new_keys;
    int *new_data;

    new_size = hmap->size * 2;
    new_keys = (String**)try_calloc(new_size, sizeof(String*));
    new_data = (int*)try_calloc(new_size, sizeof(int));

    for (i = 0; i < hmap->size; ++i)
    {
        if (*(hmap->keys+i) == NULL) continue;
        for (j = 0; ; ++j)
        {
            hash = calc_hash(new_size, j, *(hmap->keys+i));
            if (*(new_keys+hash) == NULL)
            {
                *(new_keys+hash) = *(hmap->keys+i);
                *(new_data+hash)  = *(hmap->data+i);
                break;
            }
        }
    }

    free(hmap->keys);
    free(hmap->data);
    hmap->size = new_size;
    hmap->keys = new_keys;
    hmap->data = new_data;
}

#ifdef TEST_HASHMAP
static void
debug_print(const Hashmap *hmap)
{
    int i;
    for (i = 0; i < hmap->size; ++i)
    {
        if (*(hmap->keys+i) != NULL)
        {
            printf("key=%s, val=%d, hash=%d\n",
                    (*(hmap->keys+i))->str,
                    *(int*)(*(hmap->data+i)),
                    calc_hash(hmap->size, 0, *(hmap->keys+i)));
        }
    }
}

static void
liberator_int(void *data) { free(data); }

int
main(int argc, char *argv[])
{
    Hashmap *map;
    const int BUFSIZE = 128;
    char buf[BUFSIZE], key[BUFSIZE];
    int val, *valp, size;

    if (argc != 2)
    {
        fprintf(stderr, "%s <Hashmap size>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    size = atoi(argv[1]);
    if (size <= 0) size = 16;
    map = make_hashmap(size);

    printf("Hashmap size: %d\n", size);

    for (;;)
    {
        printf("command> ");
        fgets(buf, BUFSIZE, stdin);
        if (strncmp(buf, "add", 3) == 0)
        {
            printf("key: "); fgets(key, BUFSIZE, stdin);
            key[strlen(key)-1] = '\0';
            printf("val: "); fgets(buf, BUFSIZE, stdin);
            valp = (int*)malloc(sizeof(int));
            *valp = atoi(buf);
            printf("Add to hashmap (%s, %d)\n", key, *valp);
            add_hashmap(map, new2_string(key), valp) ? puts("Success") : puts("Failed");
        }
        else if (strncmp(buf, "del", 3) == 0)
        {
            printf("key: "); fgets(key, BUFSIZE, stdin);
            key[strlen(key)-1] = '\0';
            printf("Delete from hashmap (%s)\n", key);
            remove_hashmap(map, new2_string(key), liberator_int) ? puts("Success") : puts("Failed");
        }
        else if (strncmp(buf, "set", 3) == 0)
        {
            printf("key: "); fgets(key, BUFSIZE, stdin);
            key[strlen(key)-1] = '\0';
            printf("val: "); fgets(buf, BUFSIZE, stdin);
            val = atoi(buf);
            printf("Set new value to hashmap (%s, %d)\n", key, val);
            valp = search_hashmap(map, new2_string(key));
            if (valp == NULL)
            {
                puts("Failed");
            }
            else
            {
                puts("Success");
                *valp = val;
            }
        }
        else if (strncmp(buf, "prn", 3) == 0)
        {
            printf("key: "); fgets(key, BUFSIZE, stdin);
            key[strlen(key)-1] = '\0';
            printf("Search from hashmap (%s)\n", key);
            valp = search_hashmap(map, new2_string(key));
            if (valp == NULL)
            {
                puts("Failed");
            }
            else
            {
                puts("Success");
                printf("val = %d\n", *(int*)valp);
            }
        }
        else if (strncmp(buf, "all", 3) == 0)
        {
            printf("Hashmap size = %d\nAll items = %d\n", map->size, map->amount_size);
            debug_print(map);
        }
        else
        {
            free_hashmap(&map, liberator_int);
            return EXIT_SUCCESS;
        }
    }
}
#endif

