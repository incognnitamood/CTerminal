#ifndef FILESYSTEM_H
#define FILESYSTEM_H

typedef enum {
    NODE_FILE,
    NODE_DIR
} NodeType;

typedef struct TreeNode {
    char *name;
    NodeType type;
    char *content;
    int content_size;
    int content_capacity;
    int perms_read;
    int perms_write;
    struct TreeNode **children;
    int child_count;
    int child_capacity;
    struct TreeNode *parent;
    unsigned long long created_at;
    unsigned long long modified_at;
} TreeNode;

void fs_init();
TreeNode *fs_get_root();
TreeNode *fs_get_cwd();
void fs_set_cwd(TreeNode *n);

int fs_mkdir(const char *path);
int fs_touch(const char *path);
int fs_ls(const char *path, char ***names, int *count);
int fs_cd(const char *path);
char *fs_pwd();
int fs_write(const char *path, const char *data, int append);
char *fs_read(const char *path);
int fs_rm(const char *path);
int fs_rmdir(const char *path);

/* search: callback(path, line_no, line_text) */
typedef void (*FsSearchCallback)(const char *, int, const char *, void *);
void fs_search(const char *start_path, const char *keyword,
               FsSearchCallback cb, void *user);

int fs_chmod(const char *path, int readable, int writable);
TreeNode *fs_find_node(const char *path);
int fs_copy(const char *src_path, const char *dst_path);
int fs_move(const char *src_path, const char *dst_path);
int fs_rename(const char *path, const char *new_name);
void fs_export_to_file(const char *filename, int *status);
void fs_import_from_file(const char *filename, int *status);

void fs_clear();
unsigned long long fs_get_time();

#endif
