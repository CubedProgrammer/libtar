#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"str_int_map.h"
size_t tar_simap_hashfunc(const char *key)
{
    size_t hv = 0;
    for(const char *it = key; *it != '\0'; ++it)
        hv = hv * 43 + *it;
    return hv;
}
size_t tar_simap_find(struct tar_str_int_map_entry *members, size_t capa, const char *key)
{
    size_t h = tar_simap_hashfunc(key) % capa, i = 0;
    for(; i != capa && members[(i + h) % capa].str != NULL && strcmp(members[(i + h) % capa].str, key); ++i);
    return(i + h) % capa;
}
int tar_simap_free(struct tar_str_int_map *mp);
int tar_simap_init(struct tar_str_int_map *mp);
int tar_simap_insert(struct tar_str_int_map *mp, const char *key, long val)
{
    int succ = 0;
    size_t pos, len;
    if(mp->cnt * 5 / 3 > mp->capa)
    {
        size_t ncapa = mp->capa + (mp->capa >> 1);
        struct tar_str_int_map_entry *members = malloc(ncapa * sizeof(*members));
        if(members == NULL)
            succ = -1;
        else
        {
            memset(members, 0, ncapa * sizeof(*members));
            for(size_t i = 0; i < mp->capa; ++i)
            {
                if(mp->members[i].str != NULL)
                    members[tar_simap_find(members, ncapa, mp->members[i].str)] = mp->members[i];
            }
        }
        free(mp->members);
        mp->members = members;
        mp->capa = ncapa;
    }
    if(succ == 0)
    {
        pos = tar_simap_find(mp->members, mp->capa, key);
        if(mp->members[pos].str == NULL)
        {
            len = strlen(key) + 1;
            mp->members[pos].str = malloc(len * sizeof(mp->members[pos].str));
            memcpy(mp->members[pos].str, key, len);
        }
        mp->members[pos].val = val;
    }
    return succ;
}
long tar_simap_fetch(const struct tar_str_int_map *mp, const char *key)
{
    return mp->members[tar_simap_find(mp->members, mp->capa, key)].val;
}
