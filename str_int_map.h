#ifndef Included_str_int_map_h
#define Included_str_int_map_h
struct tar_str_int_map_entry
{
    char *str;
    long val;
};
struct tar_str_int_map
{
    struct tar_str_int_map_entry *members;
    unsigned cnt;
    unsigned capa;
};
void tar_simap_free(struct tar_str_int_map *mp);
int tar_simap_init(struct tar_str_int_map *mp);
int tar_simap_insert(struct tar_str_int_map *mp, const char *key, long val);
long tar_simap_fetch(const struct tar_str_int_map *mp, const char *key);
#endif
