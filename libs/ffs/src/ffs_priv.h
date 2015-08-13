#ifndef H_FFS_PRIV_
#define H_FFS_PRIV_

#include <inttypes.h>
#include "os/queue.h"
#include "os/os_mempool.h"
#include "ffs/ffs.h"

#define FFS_ID_DIR_MIN              0
#define FFS_ID_DIR_MAX              0x10000000
#define FFS_ID_FILE_MIN             0x10000000
#define FFS_ID_FILE_MAX             0x80000000
#define FFS_ID_BLOCK_MIN            0x80000000
#define FFS_ID_BLOCK_MAX            0xffffffff

#define FFS_ID_ROOT_DIR             0
#define FFS_ID_NONE                 0xffffffff

#define FFS_AREA_MAGIC0             0xb98a31e2
#define FFS_AREA_MAGIC1             0x7fb0428c
#define FFS_AREA_MAGIC2             0xace08253
#define FFS_AREA_MAGIC3             0xb185fc8e
#define FFS_BLOCK_MAGIC             0x53ba23b9
#define FFS_INODE_MAGIC             0x925f8bc0

#define FFS_AREA_ID_NONE            0xff
#define FFS_AREA_VER                0
#define FFS_AREA_OFFSET_ID          23

#define FFS_SHORT_FILENAME_LEN      3

#define FFS_HASH_SIZE               256

#define FFS_BLOCK_MAX_DATA_SZ_MAX   2048

/** On-disk representation of an area header. */
struct ffs_disk_area {
    uint32_t fda_magic[4];  /* FFS_AREA_MAGIC{0,1,2,3} */
    uint32_t fda_length;    /* Total size of area, in bytes. */
    uint8_t fda_ver;        /* Current ffs version: 0 */
    uint8_t fda_gc_seq;     /* Garbage collection count. */
    uint8_t reserved8;
    uint8_t fda_id;         /* 0xff if scratch area. */
    /* XXX: ECC for area header. */
};

/** On-disk representation of an inode (file or directory). */
struct ffs_disk_inode {
    uint32_t fdi_magic;     /* FFS_INODE_MAGIC */
    uint32_t fdi_id;        /* Unique object ID. */
    uint32_t fdi_seq;       /* Sequence number; greater supersedes lesser. */
    uint32_t fdi_parent_id; /* Object ID of parent directory inode. */
    uint16_t reserved16;    /* FFS_INODE_F_[...] */
    uint8_t reserved8;
    uint8_t fdi_filename_len;   /* Length of filename, in bytes. */
    /* XXX: ECC for inode header and filename. */
    /* Followed by filename. */
};

/** On-disk representation of a data block. */
struct ffs_disk_block {
    uint32_t fdb_magic;     /* FFS_BLOCK_MAGIC */
    uint32_t fdb_id;        /* Unique object ID. */
    uint32_t fdb_seq;       /* Sequence number; greater supersedes lesser. */
    uint32_t fdb_inode_id;  /* Object ID of owning inode. */
    uint32_t fdb_prev_id;   /* Object ID of previous block in file;
                               FFS_ID_NONE if this is the first block. */
    uint16_t reserved16;    /* FFS_BLOCK_F_[...] */
    uint16_t fdb_data_len;  /* Length of data contents, in bytes. */
    /* XXX: ECC for block header and contents. */
    /* Followed by 'length' bytes of data. */
};

struct ffs_hash_entry {
    SLIST_ENTRY(ffs_hash_entry) fhe_next;
    uint32_t fhe_id;        /* 0 - 0x7fffffff if inode; else if block. */
    uint32_t fhe_flash_loc; /* Upper-byte = area idx; rest = area offset. */
};

SLIST_HEAD(ffs_block_list, ffs_block);
SLIST_HEAD(ffs_inode_list, ffs_inode_entry);

struct ffs_inode_entry {
    struct ffs_hash_entry fi_hash_entry;
    SLIST_ENTRY(ffs_inode_entry) fi_sibling_next;
    union {
        struct ffs_inode_list fi_child_list;    /* If directory. */
        struct ffs_hash_entry *fi_last_block;   /* If file. */
    };
    uint8_t fi_refcnt;
};

struct ffs_inode {
    struct ffs_inode_entry *fi_entry;
    uint32_t fi_seq;
    struct ffs_inode_entry *fi_parent; /* Pointer to parent directory inode. */
    uint8_t fi_filename_len;
    uint8_t fi_filename[FFS_SHORT_FILENAME_LEN]; /* 3 bytes. */
};

struct ffs_block {
    struct ffs_hash_entry *fb_hash_entry;
    uint32_t fb_seq;
    struct ffs_inode_entry *fb_inode_entry;
    struct ffs_hash_entry *fb_prev;
    uint16_t fb_data_len;
    uint16_t reserved16;
};

struct ffs_file {
    struct ffs_inode_entry *ff_inode_entry;
    uint32_t ff_offset;
    uint8_t ff_access_flags;
};

struct ffs_area {
    uint32_t fa_offset;
    uint32_t fa_length;
    uint32_t fa_cur;
    uint16_t fa_id;
    uint8_t fa_gc_seq;
};

struct ffs_disk_object {
    int fdo_type;
    uint8_t fdo_area_idx;
    uint32_t fdo_offset;
    union {
        struct ffs_disk_inode fdo_disk_inode;
        struct ffs_disk_block fdo_disk_block;
    };
};

struct ffs_seek_info {
    struct ffs_block fsi_last_block;
    uint32_t fsi_block_file_off;
    uint32_t fsi_file_len;
};

#define FFS_OBJECT_TYPE_INODE   1
#define FFS_OBJECT_TYPE_BLOCK   2

#define FFS_PATH_TOKEN_NONE     0
#define FFS_PATH_TOKEN_BRANCH   1
#define FFS_PATH_TOKEN_LEAF     2

struct ffs_path_parser {
    int fpp_token_type;
    const char *fpp_path;
    const char *fpp_token;
    int fpp_token_len;
    int fpp_off;
};

extern void *ffs_file_mem;
extern void *ffs_hash_entry_mem;
extern void *ffs_inode_mem;
extern struct os_mempool ffs_file_pool;
extern struct os_mempool ffs_inode_entry_pool;
extern struct os_mempool ffs_hash_entry_pool;
extern uint32_t ffs_hash_next_file_id;
extern uint32_t ffs_hash_next_dir_id;
extern uint32_t ffs_hash_next_block_id;
extern struct ffs_area *ffs_areas;
extern uint8_t ffs_num_areas;
extern uint8_t ffs_scratch_area_idx;
extern uint16_t ffs_block_max_data_sz;

#define FFS_FLASH_BUF_SZ        256
extern uint8_t ffs_flash_buf[FFS_FLASH_BUF_SZ];

SLIST_HEAD(ffs_hash_list, ffs_hash_entry);
extern struct ffs_hash_list ffs_hash[FFS_HASH_SIZE];
extern struct ffs_inode_entry *ffs_root_dir;

struct ffs_area *ffs_flash_find_area(uint16_t logical_id);
int ffs_flash_read(uint8_t area_idx, uint32_t offset,
                   void *data, uint32_t len);
int ffs_flash_write(uint8_t area_idx, uint32_t offset,
                    const void *data, uint32_t len);
int ffs_flash_copy(uint8_t area_id_from, uint32_t offset_from,
                   uint8_t area_id_to, uint32_t offset_to,
                   uint32_t len);
uint32_t ffs_flash_loc(uint8_t area_idx, uint32_t offset);
void ffs_flash_loc_expand(uint8_t *out_area_idx, uint32_t *out_area_offset,
                          uint32_t flash_loc);

void ffs_config_init(void);

int ffs_hash_id_is_dir(uint32_t id);
int ffs_hash_id_is_file(uint32_t id);
int ffs_hash_id_is_inode(uint32_t id);
int ffs_hash_id_is_block(uint32_t id);
struct ffs_hash_entry *ffs_hash_find(uint32_t id);
struct ffs_inode_entry *ffs_hash_find_inode(uint32_t id);
struct ffs_hash_entry *ffs_hash_find_block(uint32_t id);
void ffs_hash_insert(struct ffs_hash_entry *entry);
void ffs_hash_remove(struct ffs_hash_entry *entry);
struct ffs_hash_entry *ffs_hash_entry_alloc(void);
void ffs_hash_entry_free(struct ffs_hash_entry *entry);
void ffs_hash_init(void);

int ffs_path_parse_next(struct ffs_path_parser *parser);
void ffs_path_parser_new(struct ffs_path_parser *parser, const char *path);
int ffs_path_find(struct ffs_path_parser *parser,
                  struct ffs_inode_entry **out_inode_entry,
                  struct ffs_inode_entry **out_parent);
int ffs_path_find_inode_entry(struct ffs_inode_entry **out_inode_entry,
                              const char *filename);
int ffs_path_unlink(const char *filename);
int ffs_path_rename(const char *from, const char *to);
int ffs_path_new_dir(const char *path);

int ffs_restore_full(const struct ffs_area_desc *area_descs);

struct ffs_inode_entry *ffs_inode_entry_alloc(void);
void ffs_inode_entry_free(struct ffs_inode_entry *inode_entry);
int ffs_inode_calc_data_length(uint32_t *out_len,
                               struct ffs_inode_entry *inode_entry);
uint32_t ffs_inode_parent_id(const struct ffs_inode *inode);
int ffs_inode_delete_from_disk(struct ffs_inode *inode);
int ffs_inode_entry_from_disk(struct ffs_inode_entry *out_inode,
                              const struct ffs_disk_inode *disk_inode,
                              uint8_t area_idx, uint32_t offset);
int ffs_inode_rename(struct ffs_inode_entry *inode_entry,
                     struct ffs_inode_entry *new_parent, const char *filename);
void ffs_inode_insert_block(struct ffs_inode *inode, struct ffs_block *block);
int ffs_inode_read_disk(struct ffs_disk_inode *out_disk_inode,
                        uint8_t area_idx, uint32_t offset);
int ffs_inode_write_disk(const struct ffs_disk_inode *disk_inode,
                         const char *filename, uint8_t area_idx,
                         uint32_t offset);
int ffs_inode_dec_refcnt(struct ffs_hash_entry **out_next,
                         struct ffs_inode_entry *inode_entry);
int ffs_inode_add_child(struct ffs_inode_entry *parent,
                        struct ffs_inode_entry *child);
void ffs_inode_remove_child(struct ffs_inode *child);
int ffs_inode_is_root(const struct ffs_disk_inode *disk_inode);
int ffs_inode_filename_cmp_ram(int *result, const struct ffs_inode *inode,
                               const char *name, int name_len);
int ffs_inode_filename_cmp_flash(int *result, const struct ffs_inode *inode1,
                                 const struct ffs_inode *inode2);
int ffs_inode_read(struct ffs_inode_entry *inode_entry, uint32_t offset,
                   uint32_t len, void *data, uint32_t *out_len);
int ffs_inode_seek(struct ffs_inode_entry *inode_entry, uint32_t offset,
                   uint32_t length, struct ffs_seek_info *out_seek_info);
int ffs_inode_from_entry(struct ffs_inode *out_inode,
                         struct ffs_inode_entry *entry);
int ffs_inode_unlink(struct ffs_inode *inode);

struct ffs_block *ffs_block_alloc(void);
void ffs_block_free(struct ffs_block *block);
int ffs_block_read_disk(struct ffs_disk_block *out_disk_block,
                        uint8_t area_idx, uint32_t offset);
int ffs_block_write_disk(uint8_t *out_area_idx, uint32_t *out_offset,
                         const struct ffs_disk_block *disk_block,
                         const void *data);
int ffs_block_delete_from_ram(struct ffs_hash_entry *entry);
void ffs_block_delete_list_from_ram(struct ffs_block *first,
                                    struct ffs_block *last);
void ffs_block_delete_list_from_disk(const struct ffs_block *first,
                                     const struct ffs_block *last);
void ffs_block_to_disk(const struct ffs_block *block,
                       struct ffs_disk_block *out_disk_block);
int ffs_block_from_hash_entry_no_ptrs(struct ffs_block *out_block,
                                      struct ffs_hash_entry *entry);
int ffs_block_from_hash_entry(struct ffs_block *out_block,
                              struct ffs_hash_entry *entry);

int ffs_misc_reserve_space(uint8_t *out_area_idx, uint32_t *out_offset,
                           uint16_t size);
int ffs_misc_set_num_areas(uint8_t num_areas);

int ffs_file_open(struct ffs_file **out_file, const char *filename,
                  uint8_t access_flags);
int ffs_file_seek(struct ffs_file *file, uint32_t offset);
int ffs_file_close(struct ffs_file *file);
int ffs_file_new(struct ffs_inode_entry **out_inode_entry,
                 struct ffs_inode_entry *parent,
                 const char *filename, uint8_t filename_len, int is_dir);

int ffs_format_area(uint8_t area_idx, int is_scratch);
int ffs_format_from_scratch_area(uint8_t area_idx, uint8_t area_id);
int ffs_format_full(const struct ffs_area_desc *area_descs);

int ffs_gc(uint8_t *out_area_idx);
int ffs_gc_until(uint8_t *out_area_idx, uint32_t space);

int ffs_area_magic_is_set(const struct ffs_disk_area *disk_area);
int ffs_area_is_scratch(const struct ffs_disk_area *disk_area);
void ffs_area_to_disk(struct ffs_disk_area *out_disk_area,
                      const struct ffs_area *area);
uint32_t ffs_area_free_space(const struct ffs_area *area);
int ffs_area_find_corrupt_scratch(uint16_t *out_good_idx,
                                  uint16_t *out_bad_idx);

int ffs_misc_validate_root(void);
int ffs_misc_validate_scratch(void);
int ffs_misc_reset(void);
void ffs_misc_set_max_block_data_size(void);

int ffs_write_to_file(struct ffs_file *file, const void *data, int len);

/** Represents a single data block. */

struct ffs_block_cache_entry {
    struct ffs_hash_entry *fbce_entry;            /* Pointer to real block. */
    uint32_t fbce_seq;
    uint16_t fbce_data_len;
    TAILQ_ENTRY(ffs_block_cache_entry) fbce_link; /* Next / prev block. */
    uint32_t fbce_file_offset;              /* File offset of this block. */
};

TAILQ_HEAD(ffs_block_cache_list, ffs_block_cache_entry);

/** Represents all or part of a file. */
struct ffs_file_cache_entry {
    SLIST_ENTRY(ffs_file_cache_entry) *ffce_next; /* Needed for hash table. */
    struct ffs_inode_entry *ffce_inode_entry;     /* Pointer to real file. */
    struct ffs_block_cache_list ffce_block_list;  /* List of cached blocks. */
    uint32_t ffce_file_size;                      /* Total file size. */
    uint32_t ffce_cache_length;       /* Cumulative length of cached data. */
};


#define FFS_HASH_FOREACH(entry, i)                                      \
    for ((i) = 0; (i) < FFS_HASH_SIZE; (i)++)                           \
        SLIST_FOREACH((entry), &ffs_hash[i], fhe_next)

#define FFS_FLASH_LOC_NONE  ffs_flash_loc(FFS_AREA_ID_NONE, 0)

#endif

