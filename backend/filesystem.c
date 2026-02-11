#include <stdio.h>
#include "filesystem.h"
#include "utils.h"

static TreeNode *fs_root = 0;
static TreeNode *fs_cwd = 0;

static TreeNode *fs_create_node(const char *name, NodeType type) {
    TreeNode *n = (TreeNode *)u_malloc(sizeof(TreeNode));
    n->name = u_strdup(name);
    n->type = type;
    n->content = 0;
    n->content_size = 0;
    n->content_capacity = 0;
    n->perms_read = 1;
    n->perms_write = 1;
    n->children = 0;
    n->child_count = 0;
    n->child_capacity = 0;
    n->parent = 0;
    return n;
}

void fs_init() {
    fs_root = fs_create_node("/", NODE_DIR);
    fs_root->parent = 0;
    fs_cwd = fs_root;
}

TreeNode *fs_get_root() {
    return fs_root;
}

TreeNode *fs_get_cwd() {
    return fs_cwd;
}

void fs_set_cwd(TreeNode *n) {
    if (n && n->type == NODE_DIR) {
        fs_cwd = n;
    }
}

static void fs_add_child(TreeNode *dir, TreeNode *child) {
    int i;
    if (dir->child_capacity == 0) {
        dir->child_capacity = 4;
        dir->children = (TreeNode **)u_malloc(sizeof(TreeNode *) * dir->child_capacity);
    } else if (dir->child_count >= dir->child_capacity) {
        int newcap = dir->child_capacity * 2;
        TreeNode **nc = (TreeNode **)u_malloc(sizeof(TreeNode *) * newcap);
        for (i = 0; i < dir->child_count; i++) {
            nc[i] = dir->children[i];
        }
        u_free(dir->children);
        dir->children = nc;
        dir->child_capacity = newcap;
    }
    dir->children[dir->child_count++] = child;
    child->parent = dir;
}

static TreeNode *fs_find_child(TreeNode *dir, const char *name) {
    int i;
    if (!dir || dir->type != NODE_DIR) return 0;
    for (i = 0; i < dir->child_count; i++) {
        if (u_strcmp(dir->children[i]->name, name) == 0) {
            return dir->children[i];
        }
    }
    return 0;
}

static void fs_remove_child(TreeNode *dir, int index) {
    int i;
    if (index < 0 || index >= dir->child_count) return;
    for (i = index; i < dir->child_count - 1; i++) {
        dir->children[i] = dir->children[i + 1];
    }
    dir->child_count--;
}

static void fs_free_node(TreeNode *n) {
    int i;
    if (!n) return;
    if (n->type == NODE_DIR) {
        for (i = 0; i < n->child_count; i++) {
            fs_free_node(n->children[i]);
        }
        if (n->children) u_free(n->children);
    } else {
        if (n->content) u_free(n->content);
    }
    if (n->name) u_free(n->name);
    u_free(n);
}

static TreeNode *fs_resolve_relative(TreeNode *start, const char *path) {
    int i = 0;
    TreeNode *cur = start;
    char part[256];
    int pi = 0;

    if (!path || path[0] == 0) return cur;

    while (1) {
        char c = path[i];
        if (c == '/' || c == 0) {
            part[pi] = 0;
            if (pi > 0) {
                if (u_strcmp(part, ".") == 0) {
                    /* stay */
                } else if (u_strcmp(part, "..") == 0) {
                    if (cur->parent) cur = cur->parent;
                } else {
                    TreeNode *child = fs_find_child(cur, part);
                    if (!child) return 0;
                    cur = child;
                }
            }
            pi = 0;
            if (c == 0) break;
        } else {
            if (pi < 255) {
                part[pi++] = c;
            }
        }
        i++;
    }
    return cur;
}

static TreeNode *fs_resolve(const char *path, int parent_for_new, char *last_name) {
    /* parent_for_new: if 1, return parent dir of final component and copy final name into last_name */
    if (!path || path[0] == 0) return fs_cwd;
    TreeNode *start;
    int offset = 0;
    if (path[0] == '/') {
        start = fs_root;
        offset = 1;
    } else {
        start = fs_cwd;
    }
    if (!parent_for_new) {
        return fs_resolve_relative(start, path + offset);
    } else {
        int len = u_strlen(path + offset);
        int i = len - 1;
        int pos = -1;
        while (i >= 0) {
            if (path[offset + i] == '/') {
                pos = offset + i;
                break;
            }
            i--;
        }
        if (pos == -1) {
            /* parent is start, name is whole */
            TreeNode *p = start;
            int j = 0;
            while (path[offset + j] != 0 && j < 255) {
                last_name[j] = path[offset + j];
                j++;
            }
            last_name[j] = 0;
            return p;
        } else {
            int j;
            TreeNode *p;
            char parent_path[512];
            int l = pos - offset;
            for (j = 0; j < l; j++) {
                parent_path[j] = path[offset + j];
            }
            parent_path[l] = 0;
            p = fs_resolve_relative(start, parent_path);
            if (!p) return 0;
            j = 0;
            pos++;
            while (path[pos] != 0 && j < 255) {
                last_name[j] = path[pos];
                j++;
                pos++;
            }
            last_name[j] = 0;
            return p;
        }
    }
}

TreeNode *fs_find_node(const char *path) {
    return fs_resolve(path, 0, 0);
}

int fs_mkdir(const char *path) {
    char name[256];
    TreeNode *parent = fs_resolve(path, 1, name);
    TreeNode *exist;
    if (!parent || parent->type != NODE_DIR) return -1;
    exist = fs_find_child(parent, name);
    if (exist) return -2;
    fs_add_child(parent, fs_create_node(name, NODE_DIR));
    return 0;
}

int fs_touch(const char *path) {
    char name[256];
    TreeNode *parent = fs_resolve(path, 1, name);
    TreeNode *exist;
    if (!parent || parent->type != NODE_DIR) return -1;
    exist = fs_find_child(parent, name);
    if (exist) {
        if (exist->type == NODE_FILE) return 0;
        return -2;
    }
    fs_add_child(parent, fs_create_node(name, NODE_FILE));
    return 0;
}

int fs_ls(const char *path, char ***names, int *count) {
    TreeNode *dir;
    int i;
    if (path && path[0] != 0) {
        dir = fs_resolve(path, 0, 0);
    } else {
        dir = fs_cwd;
    }
    if (!dir || dir->type != NODE_DIR) return -1;
    *count = dir->child_count;
    if (dir->child_count == 0) {
        *names = 0;
        return 0;
    }
    *names = (char **)u_malloc(sizeof(char *) * dir->child_count);
    for (i = 0; i < dir->child_count; i++) {
        (*names)[i] = u_strdup(dir->children[i]->name);
    }
    return 0;
}

int fs_cd(const char *path) {
    TreeNode *dir;
    if (!path || path[0] == 0) {
        fs_cwd = fs_root;
        return 0;
    }
    dir = fs_resolve(path, 0, 0);
    if (!dir || dir->type != NODE_DIR) return -1;
    fs_cwd = dir;
    return 0;
}

char *fs_pwd() {
    UBuffer b;
    TreeNode *cur = fs_cwd;
    char *parts[128];
    int pc = 0;
    int i;
    if (cur == fs_root) {
        char *r = (char *)u_malloc(2);
        r[0] = '/';
        r[1] = 0;
        return r;
    }
    while (cur && cur != fs_root) {
        parts[pc++] = cur->name;
        cur = cur->parent;
    }
    ubuf_init(&b);
    ubuf_append_char(&b, '/');
    for (i = pc - 1; i >= 0; i--) {
        ubuf_append_str(&b, parts[i]);
        if (i > 0) ubuf_append_char(&b, '/');
    }
    return ubuf_to_string(&b);
}

static void fs_ensure_file_buffer(TreeNode *n, int extra) {
    int need;
    int newcap;
    char *nc;
    int i;
    if (!n->content) {
        n->content_capacity = extra + 1;
        n->content = (char *)u_malloc(n->content_capacity);
        n->content[0] = 0;
        n->content_size = 0;
        return;
    }
    need = n->content_size + extra + 1;
    if (need <= n->content_capacity) return;
    newcap = n->content_capacity * 2;
    while (newcap < need) newcap *= 2;
    nc = (char *)u_malloc(newcap);
    for (i = 0; i < n->content_size; i++) {
        nc[i] = n->content[i];
    }
    nc[n->content_size] = 0;
    u_free(n->content);
    n->content = nc;
    n->content_capacity = newcap;
}

int fs_write(const char *path, const char *data, int append) {
    TreeNode *f = fs_resolve(path, 0, 0);
    int len = u_strlen(data);
    int i;
    if (!f) {
        int r = fs_touch(path);
        if (r != 0) return r;
        f = fs_resolve(path, 0, 0);
    }
    if (!f || f->type != NODE_FILE) return -1;
    if (!f->perms_write) return -2;
    if (!append) {
        if (!f->content) {
            f->content_capacity = len + 1;
            f->content = (char *)u_malloc(f->content_capacity);
        } else if (f->content_capacity < len + 1) {
            u_free(f->content);
            f->content_capacity = len + 1;
            f->content = (char *)u_malloc(f->content_capacity);
        }
        for (i = 0; i < len; i++) {
            f->content[i] = data[i];
        }
        f->content[len] = 0;
        f->content_size = len;
    } else {
        fs_ensure_file_buffer(f, len);
        for (i = 0; i < len; i++) {
            f->content[f->content_size + i] = data[i];
        }
        f->content_size += len;
        f->content[f->content_size] = 0;
    }
    return 0;
}

char *fs_read(const char *path) {
    TreeNode *f = fs_resolve(path, 0, 0);
    char *copy;
    int i;
    if (!f || f->type != NODE_FILE) return 0;
    if (!f->perms_read) return 0;
    if (!f->content) {
        copy = (char *)u_malloc(1);
        copy[0] = 0;
        return copy;
    }
    copy = (char *)u_malloc(f->content_size + 1);
    for (i = 0; i < f->content_size; i++) {
        copy[i] = f->content[i];
    }
    copy[f->content_size] = 0;
    return copy;
}

int fs_rm(const char *path) {
    TreeNode *f = fs_resolve(path, 0, 0);
    TreeNode *p;
    int i;
    if (!f || f->type != NODE_FILE) return -1;
    p = f->parent;
    if (!p) return -2;
    for (i = 0; i < p->child_count; i++) {
        if (p->children[i] == f) {
            fs_remove_child(p, i);
            fs_free_node(f);
            return 0;
        }
    }
    return -3;
}

int fs_rmdir(const char *path) {
    TreeNode *d = fs_resolve(path, 0, 0);
    TreeNode *p;
    int i;
    if (!d || d->type != NODE_DIR) return -1;
    if (d == fs_root) return -2;
    if (d->child_count > 0) return -3;
    p = d->parent;
    if (!p) return -4;
    for (i = 0; i < p->child_count; i++) {
        if (p->children[i] == d) {
            fs_remove_child(p, i);
            fs_free_node(d);
            return 0;
        }
    }
    return -5;
}

static void fs_search_in_file(const char *path, TreeNode *f,
                              const char *keyword,
                              FsSearchCallback cb, void *user) {
    int i = 0;
    int line = 1;
    UBuffer linebuf;
    int klen = u_strlen(keyword);
    if (!f->content || klen == 0) return;
    ubuf_init(&linebuf);
    while (i <= f->content_size) {
        char c = (i == f->content_size) ? '\n' : f->content[i];
        if (c == '\n') {
            char *line_str = ubuf_to_string(&linebuf);
            int j;
            int l = u_strlen(line_str);
            for (j = 0; j + klen <= l; j++) {
                int ok = 1;
                int t;
                for (t = 0; t < klen; t++) {
                    if (line_str[j + t] != keyword[t]) {
                        ok = 0;
                        break;
                    }
                }
                if (ok) {
                    cb(path, line, line_str, user);
                    break;
                }
            }
            u_free(line_str);
            line++;
            linebuf.length = 0;
            linebuf.data[0] = 0;
        } else {
            ubuf_append_char(&linebuf, c);
        }
        i++;
    }
    ubuf_free(&linebuf);
}

static void fs_search_rec(TreeNode *dir, const char *prefix,
                          const char *keyword,
                          FsSearchCallback cb, void *user) {
    int i;
    char path[512];
    if (!dir) return;
    for (i = 0; i < dir->child_count; i++) {
        TreeNode *ch = dir->children[i];
        if (prefix[0] == 0 || (prefix[0] == '/' && prefix[1] == 0)) {
            path[0] = '/';
            path[1] = 0;
            if (ch != fs_root) {
                u_strcat(path, ch->name);
            }
        } else {
            int plen = u_strlen(prefix);
            int j;
            for (j = 0; j < plen && j < 511; j++) {
                path[j] = prefix[j];
            }
            if (plen < 511) {
                if (!(plen == 1 && prefix[0] == '/')) {
                    path[plen] = '/';
                    plen++;
                }
            }
            path[plen] = 0;
            if (u_strlen(path) + u_strlen(ch->name) < 511) {
                u_strcat(path, ch->name);
            }
        }
        if (ch->type == NODE_FILE) {
            fs_search_in_file(path, ch, keyword, cb, user);
        } else if (ch->type == NODE_DIR) {
            fs_search_rec(ch, path, keyword, cb, user);
        }
    }
}

void fs_search(const char *start_path, const char *keyword,
               FsSearchCallback cb, void *user) {
    TreeNode *start;
    char rootpath[2];
    rootpath[0] = '/';
    rootpath[1] = 0;
    if (start_path && start_path[0] != 0) {
        start = fs_resolve(start_path, 0, 0);
        if (!start) start = fs_root;
    } else {
        start = fs_root;
    }
    if (start->type == NODE_FILE) {
        fs_search_in_file(rootpath, start, keyword, cb, user);
    } else {
        fs_search_rec(start, rootpath, keyword, cb, user);
    }
}

int fs_chmod(const char *path, int readable, int writable) {
    TreeNode *n = fs_resolve(path, 0, 0);
    if (!n) return -1;
    n->perms_read = readable ? 1 : 0;
    n->perms_write = writable ? 1 : 0;
    return 0;
}

int fs_copy(const char *src_path, const char *dst_path) {
    TreeNode *src = fs_resolve(src_path, 0, 0);
    if (!src) return -1;
    if (src->type == NODE_FILE) {
        char *content = fs_read(src_path);
        int rc;
        if (!content) return -2;
        rc = fs_write(dst_path, content, 0);
        u_free(content);
        return rc;
    } else {
        /* simple: create dir only, not deep copy of children */
        return fs_mkdir(dst_path);
    }
}

int fs_move(const char *src_path, const char *dst_path) {
    TreeNode *src = fs_resolve(src_path, 0, 0);
    int rc;
    if (!src) return -1;
    rc = fs_copy(src_path, dst_path);
    if (rc != 0) return rc;
    if (src->type == NODE_FILE) {
        return fs_rm(src_path);
    } else {
        return fs_rmdir(src_path);
    }
}

void fs_export_to_file(const char *filename, int *status) {
    FILE *f = fopen(filename, "w");
    int i;
    if (!f) {
        if (status) *status = -1;
        return;
    }
    /* Files and dirs DFS */
    /* helper stack of nodes */
    TreeNode *stack[256];
    char *pathstack[256];
    int sp = 0;
    stack[sp] = fs_root;
    pathstack[sp] = "/";
    sp++;
    while (sp > 0) {
        TreeNode *n;
        char *path;
        sp--;
        n = stack[sp];
        path = pathstack[sp];
        if (n->type == NODE_DIR) {
            fprintf(f, "DIR:%s:%d:%d\n", path, n->perms_read, n->perms_write);
        } else {
            fprintf(f, "FILE:%s:%d:%d:", path, n->perms_read, n->perms_write);
            if (n->content) {
                for (i = 0; i < n->content_size; i++) {
                    char c = n->content[i];
                    if (c == '\n') {
                        fputc('\\', f);
                        fputc('n', f);
                    } else if (c == '\\') {
                        fputc('\\', f);
                        fputc('\\', f);
                    } else {
                        fputc(c, f);
                    }
                }
            }
            fputc('\n', f);
        }
        if (n->type == NODE_DIR) {
            for (i = 0; i < n->child_count; i++) {
                TreeNode *ch = n->children[i];
                char *child_path;
                UBuffer pb;
                ubuf_init(&pb);
                if (path[0] == '/' && path[1] == 0) {
                    ubuf_append_char(&pb, '/');
                    ubuf_append_str(&pb, ch->name);
                } else {
                    ubuf_append_str(&pb, path);
                    ubuf_append_char(&pb, '/');
                    ubuf_append_str(&pb, ch->name);
                }
                child_path = ubuf_to_string(&pb);
                stack[sp] = ch;
                pathstack[sp] = child_path;
                sp++;
            }
        }
    }
    fprintf(f, "END\n");
    fclose(f);
    if (status) *status = 0;
}

void fs_import_from_file(const char *filename, int *status) {
    FILE *f = fopen(filename, "r");
    char line[1024];
    if (!f) {
        if (status) *status = -1;
        return;
    }
    fs_clear();
    fs_init();
    while (fgets(line, sizeof(line), f)) {
        int i = 0;
        char kind[6];
        int ki = 0;
        if (line[0] == 0) continue;
        while (line[i] != ':' && line[i] != 0 && ki < 5) {
            kind[ki++] = line[i++];
        }
        kind[ki] = 0;
        if (line[i] != ':') continue;
        i++;
        if (kind[0] == 'E') {
            break;
        } else if (kind[0] == 'D') {
            char path[512];
            char *p = path;
            int rbit, wbit;
            while (line[i] != ':' && line[i] != 0) {
                *p++ = line[i++];
            }
            *p = 0;
            if (line[i] != ':') continue;
            i++;
            rbit = line[i] - '0';
            while (line[i] != ':' && line[i] != 0) i++;
            if (line[i] != ':') continue;
            i++;
            wbit = line[i] - '0';
            fs_mkdir(path);
            fs_chmod(path, rbit, wbit);
        } else if (kind[0] == 'F') {
            char path[512];
            char *p = path;
            char content[512];
            char *c = content;
            int rbit, wbit;
            while (line[i] != ':' && line[i] != 0) {
                *p++ = line[i++];
            }
            *p = 0;
            if (line[i] != ':') continue;
            i++;
            rbit = line[i] - '0';
            while (line[i] != ':' && line[i] != 0) i++;
            if (line[i] != ':') continue;
            i++;
            wbit = line[i] - '0';
            while (line[i] != ':' && line[i] != 0) i++;
            if (line[i] != ':')
                continue;
            i++;
            while (line[i] != 0 && line[i] != '\n') {
                char ch = line[i++];
                if (ch == '\\') {
                    char n = line[i++];
                    if (n == 'n') {
                        *c++ = '\n';
                    } else if (n == '\\') {
                        *c++ = '\\';
                    }
                } else {
                    *c++ = ch;
                }
            }
            *c = 0;
            fs_write(path, content, 0);
            fs_chmod(path, rbit, wbit);
        }
    }
    fclose(f);
    if (status) *status = 0;
}

void fs_clear() {
    if (fs_root) {
        fs_free_node(fs_root);
        fs_root = 0;
        fs_cwd = 0;
    }
}

