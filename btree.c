#include "global.h"

/* **************************** */
/*      table  ops              */
/* **************************** */
int Tablefd;
uint64_t idctr = 0;// id generator
uint32_t ectr = 1;

int writeEntry(uint32_t indexval, struct entry *entry);
int createTable(char *path)
{
        struct entry re;
        
        if ((Tablefd = open(path, O_RDWR | O_TRUNC | O_CREAT, 0644)) < 0) {
                perror("open table file");
                exit(-1);
        }
        memset(&re, 0, sizeof(struct entry));
        writeEntry(0, &re);
        re.id = ++idctr;
        re.pid = re.id;
        re.removal = 0;
        strcpy(re.name, "null");
        strcpy(re.path, "/");
        writeEntry(ectr, &re);
        return Tablefd;
}

void dropTable(char *path)
{
        close(Tablefd);
        unlink(path);
}

int emmfd;
int initEMM(char *path)
{
        if ((emmfd = open(path, O_RDWR | O_TRUNC | O_CREAT, 0644)) < 0) {
                perror("open emmfd file");
                exit(-1);
        }
        return emmfd; 
}

void destroyEMM(char *path)
{
        close(emmfd);
        unlink(path);
}

uint32_t allocEntry(void)
{
        struct stat statbuf;
        uint32_t pos;
        int res;

        if (fstat(emmfd, &statbuf) < 0) {
                abort();
        }
        if (statbuf.st_size > 0) {
        redo:
                if ((res = pread(emmfd, &pos, sizeof(uint32_t), statbuf.st_size - sizeof(uint32_t))) < 0) {
                        abort();
                }
                if (res != sizeof(uint32_t)) {
                        goto redo;
                }
                ftruncate(emmfd, statbuf.st_size - sizeof(uint32_t));
                return pos;
        }
        return ectr++;
}

void freeEntry(uint32_t pos)
{
        struct stat statbuf;
        int res;

        if (fstat(emmfd, &statbuf) < 0) {
                abort();
        }
        if ((res = pwrite(emmfd, &pos, sizeof(uint32_t), statbuf.st_size)) < 0) {
                abort();
        }
        return ;
}

struct entry *readEntry(uint32_t indexval)
{
        int res;
        struct entry *entry;
        
redo:
        assert((entry = calloc(1, sizeof(struct entry))) != NULL);
        if ((res = pread(Tablefd, (char *)entry, sizeof(struct entry), indexval * sizeof(struct entry))) < 0) {
                perror("read entry");
                free(entry);
                return NULL;
        }
        if (res != sizeof(struct entry)) {
                goto redo;
        }
        return entry;
}

int writeEntry(uint32_t indexval, struct entry *entry)
{
        int res;

redo:
        if ((res = pwrite(Tablefd, (char *)entry, sizeof(struct entry), indexval * sizeof(struct entry))) < 0) {
                perror("insert entry");
                return -errno;
        }
        if (res != sizeof(struct entry)) {
                goto redo;
        }
        return 0;
}

/* ******************** */
/*      index ops       */
/* ******************** */
int Indexfd;
enum {
        NONLEAF,
        LEAF,
};

/* ******************** */
/*   memory management  */
/* ******************** */
uint32_t indexctr = 1;
int mmfd;

int initMM(char *path)
{
        if ((mmfd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
                perror("mm open file");
                exit(-1);
        }
        return mmfd;
}

void destroyMM(char *path)
{
        close(mmfd);
        unlink(path);
}

//entry in mmtable [uint32_t pos]
uint32_t allocIndex(void)
{
        struct stat statbuf;
        uint32_t pos;
        int res;

        if (fstat(mmfd, &statbuf) < 0) {
                abort();
        }
        if (statbuf.st_size > 0) {
        redo:
                if ((res = pread(mmfd, &pos, sizeof(uint32_t), statbuf.st_size - sizeof(uint32_t))) < 0) {
                       abort();
                }
                if (res != sizeof(uint32_t)) {
                        goto redo;
                }
                ftruncate(mmfd, statbuf.st_size - sizeof(uint32_t));
                return pos;
        } else {
                return ++indexctr;
        }
}

void freeIndex(uint32_t pos)
{
        struct stat statbuf;
        int res;

        if (fstat(mmfd, &statbuf) < 0) {
                abort();
        }
        if ((res = pwrite(mmfd, &pos, sizeof(uint32_t), statbuf.st_size)) < 0) {
                abort();
        }
        return ;
}

/* ***************************** */
/* root entry's offet = 0        */
/* root index's offet = 0        */
/* ***************************** */
int writeIndex(uint32_t indexPos, struct btreenode *node);
int32_t createIndex(char *path)
{
        struct btreenode root;

        if ((Indexfd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
                perror("open index file");
                exit(-1);
        }
        memset(&root, 0, sizeof(struct btreenode));
        writeIndex(0, &root); 
        root.isLeaf = LEAF;
        root.cnt = 0;
        root.fd = indexctr;
        writeIndex(indexctr, &root); 
        return indexctr;
}

void dropIndex(char *path)
{
        close(Indexfd);
        unlink(path);
}

struct btreenode *readIndex(uint32_t indexPos)
{
        int res;
        struct btreenode *node;

redo:
        assert((node = calloc(1, sizeof(struct btreenode))) != NULL);
        if ((res = pread(Indexfd, (char *)node, sizeof(struct btreenode), indexPos * sizeof(struct btreenode))) < 0) {
                perror("read index");
                free(node);
                return NULL;
        }
        if (res != sizeof(struct btreenode)) {
                goto redo;
        }
        return node;
}

int writeIndex(uint32_t indexPos, struct btreenode *node)
{
        int res;

redo:
        if ((res = pwrite(Indexfd, (char *)node, sizeof(struct btreenode), indexPos * sizeof(struct btreenode))) < 0) {
                perror("write index");
                return -errno;
        }
        if (res != sizeof(struct btreenode)) {
                goto redo;
        }
        return 0;
}

/* ********************************** */
/* btree index's lookup/insert/delete */
/* ********************************** */
struct lookup_result *lookupIndex(uint32_t rootfd, uint64_t indexkey)
{
        int i;
        struct lookup_result *result;
        struct btreenode *root;
        uint32_t childfd;

        assert((root = readIndex(rootfd)) != NULL);
        for (i = 0; i < root->cnt; i++) {
                if (root->key[i] >= indexkey) {
                        break;
                }
        }
        if ((i < root->cnt) && (root->key[i] == indexkey)) {
                assert((result = calloc(1, sizeof(struct lookup_result))) != NULL);
                result->fd = root->fd;
                result->i = i;
                free(root);
                return result;
        }
        if (root->isLeaf == LEAF) {
                free(root);
                return NULL;
        }
        childfd = root->child[i];
        free(root);
        return lookupIndex(childfd, indexkey);
}

void btree_split(uint32_t rootfd, int i, uint32_t childfd);
void btree_insertNONFULL(uint32_t rootfd, uint64_t indexkey, uint32_t indexval);
uint32_t insertIndex(uint32_t rootfd, uint64_t indexkey, uint32_t indexval)
{
        struct btreenode *newroot, *root;
        uint32_t pos;
        
        assert((root = readIndex(rootfd)) != NULL);
        if (root->cnt == BTREEORDER - 1) {
                assert((pos = allocIndex()) != 0);
                assert((newroot = calloc(1, sizeof(struct btreenode))) != NULL);
                newroot->isLeaf = NONLEAF;
                newroot->cnt = 0;
                newroot->child[0] = root->fd;
                newroot->fd = pos;
                writeIndex(newroot->fd, newroot);
                free(root);
                free(newroot);
                
                btree_split(pos, 0, rootfd);
                btree_insertNONFULL(pos, indexkey, indexval);
                return pos;
        } else {
                free(root);
                btree_insertNONFULL(rootfd, indexkey, indexval);
                return rootfd;
        }
}

uint32_t deleteIndex(uint32_t rootfd, uint64_t indexkey, uint32_t *indexval)
{
        struct btreenode *root, *child, *child_1, *child_2;
        uint32_t childfd;
        uint32_t childval;
        uint64_t childkey;
        int i, j;

        assert((root = readIndex(rootfd)) != NULL);
        if (root->isLeaf == LEAF) { //leaf node has no child
                for (i = 0; i < root->cnt; i++) {
                        if (root->key[i] == indexkey) {
                                *indexval = root->val[i];
                                for (; i < root->cnt - 1; i++) {
                                        root->key[i] = root->key[i + 1];
                                        root->val[i] = root->val[i + 1];
                                }
                                root->cnt -= 1;
                                writeIndex(root->fd, root);
                                free(root);
                                return rootfd;
                        }
                }
                abort();
        }
        for (i = 0; i < root->cnt; i++) {
                if (root->key[i] == indexkey) { // key in intervalnode
                        assert((child = readIndex(root->child[i])) != NULL);
                        childfd = child->fd;
                        if (child->cnt >= (BTREEORDER / 2)) {//left child has more than 32 keys slot
                                childkey = child->key[child->cnt - 1];
                                free(child);
                                
                                childfd = deleteIndex(childfd, childkey, &childval);
                                *indexval = root->val[i];
                                root->child[i] = childfd;
                                root->key[i] = childkey;
                                root->val[i] = childval;
                                writeIndex(root->fd, root);
                                free(root);
                                return rootfd;
                        }
                        free(child);
                        assert((child = readIndex(root->child[i + 1])) != NULL);
                        childfd = child->fd;
                        if (child->cnt >= BTREEORDER / 2) {// right child has more than 32 keys slot
                                childkey = child->key[0];
                                free(child);

                                childfd = deleteIndex(childfd, childkey, &childval);
                                *indexval = root->val[i];
                                root->child[i + 1] = childfd;
                                root->key[i] = childkey;
                                root->val[i] = childval;
                                writeIndex(root->fd, root);
                                free(root);
                                return rootfd;
                        }
                        //now left and right child both have 31 key slots.
                        assert((child_1 = readIndex(root->child[i])) != NULL);
                        child_1->key[child_1->cnt] = root->key[i];
                        child_1->val[child_1->cnt] = root->val[i];
                        for (j = i; j < root->cnt - 1; j++) {
                                root->key[j] = root->key[j + 1];
                                root->val[j] = root->val[j + 1];
                                root->child[j + 1] = root->child[j + 2];
                        }
                        root->child[j + 1] = root->child[j + 2];
                        root->cnt -= 1;

                        for (j = 0; j < child->cnt; j++) {
                                child_1->key[child_1->cnt + 1 + j] = child->key[j];
                                child_1->val[child_1->cnt + 1 + j] = child->val[j];
                                child_1->child[child_1->cnt + 1 + j] = child->child[j];
                        }
                        child_1->child[child_1->cnt + 1 + j] = child->child[j];
                        child_1->cnt += child->cnt + 1;
                        freeIndex(child->fd);
                        free(child);
                        writeIndex(child_1->fd, child_1);
                        free(child_1);

                        root->child[i] = deleteIndex(root->child[i], indexkey, indexval);
                        writeIndex(root->fd, root);
                        free(root);
                        return rootfd;

                } else if (root->key[i] > indexkey) {
                        break;
                }
        }
        assert((child = readIndex(root->child[i])) != NULL); 
        if (child->cnt >= BTREEORDER / 2) {
                free(child);
                root->child[i] = deleteIndex(root->child[i], indexkey, indexval);
                writeIndex(root->fd, root);
                free(root);
                return rootfd;
        }
        if (i > 0) { // child has left slibbing
                assert((child_1 = readIndex(root->child[i - 1])) != NULL);
                if (child_1->cnt >= BTREEORDER / 2) {//left slibbing has more than 32 key slots
                        child->child[child->cnt + 1] = child->child[child->cnt];
                        for (j = child->cnt; j > 0; j--) {
                                child->key[j] = child->key[j - 1];
                                child->val[j] = child->val[j - 1];
                                child->child[j] = child->child[j -1];
                        }
                        child->key[0] = root->key[i - 1];
                        child->val[0] = root->val[i - 1];
                        child->child[0] = child_1->child[child_1->cnt];
                        child->cnt += 1;
                        writeIndex(child->fd, child);

                        root->key[i - 1] = child_1->key[child_1->cnt - 1];
                        root->val[i - 1] = child_1->val[child_1->cnt - 1];
                        child_1->cnt -= 1;
                        writeIndex(child_1->fd, child_1);
                        free(child_1);
                        
                        root->child[i] = deleteIndex(root->child[i], indexkey, indexval);
                        writeIndex(root->fd, root);
                        free(root);
                        return rootfd;
                }
                if (i < root->cnt) {//has right slibbing
                        assert((child_2 = readIndex(root->child[i + 1])) != NULL);
                        if (child_2->cnt >= BTREEORDER / 2) { // right slibbing has more than 32 key slots
                                child->key[child->cnt] = root->key[i];
                                child->val[child->cnt] = root->val[i];
                                child->child[child->cnt + 1] = child_2->child[0];
                                child->cnt += 1;
                                writeIndex(child->fd, child);
                                free(child);

                                root->key[i] = child_2->key[0];
                                root->val[i] = child_2->val[0];
                                for (j = 0; j < child_2->cnt - 1; j++) {
                                        child_2->key[j] = child_2->key[j + 1];
                                        child_2->val[j] = child_2->val[j + 1];
                                        child_2->child[j] = child_2->child[j + 1];
                                }
                                child_2->child[j] = child_2->child[j + 1];
                                child_2->cnt -= 1;
                                writeIndex(child_2->fd, child_2);
                                free(child_2);

                                root->child[i] = deleteIndex(root->child[i], indexkey, indexval);
                                writeIndex(root->fd, root);
                                free(root);
                                return rootfd;
                        }
                } else { // no right slibbing, but has left slibbing
                        child_1->key[child_1->cnt] = root->key[i - 1];
                        child_1->val[child_1->cnt] = root->val[i - 1];
                        for (j = 0; j < child->cnt; j++) {
                                child_1->key[child_1->cnt + 1 + j] = child->key[j];
                                child_1->val[child_1->cnt + 1 + j] = child->val[j];
                                child_1->child[child_1->cnt + 1 + j] = child->child[j];
                        }
                        child->child[child_1->cnt + 1 + j] = child->child[j];
                        child_1->cnt += child->cnt + 1;
                        writeIndex(child_1->fd, child_1);
                        freeIndex(child->fd);
                        free(child);
                        
                        root->cnt -= 1;
                        if (root->cnt == 0) { //change root node
                                rootfd = child_1->fd;
                                freeIndex(root->fd);
                                free(root);
                                free(child_1);
                                return deleteIndex(rootfd, indexkey, indexval);
                        }
                        free(child_1);
                        root->child[i - 1] = deleteIndex(root->child[i - 1], indexkey, indexval);
                        writeIndex(root->fd, root);
                        free(root);
                        return rootfd;
                }

        } else { // no left slibbing
                assert(i < root->cnt); //has right slibbing
                assert((child_1 = readIndex(root->child[i + 1])) != NULL);
                if (child_1->cnt >= BTREEORDER / 2) {//right slibbing has more than 32 key slots
                        child->key[child->cnt] = root->key[i];
                        child->val[child->cnt] = root->val[i];
                        child->child[child->cnt + 1] = child_1->child[0];
                        child->cnt += 1;
                        writeIndex(child->fd, child);
                        free(child);

                        root->key[i] = child_1->key[0];
                        root->val[i] = child_1->val[0];
                        for (j = 0; j < child_1->cnt - 1; j++) {
                                child_1->key[j] = child_1->key[j + 1];
                                child_1->val[j] = child_1->val[j + 1];
                                child_1->child[j] = child_1->child[j + 1];
                        }
                        child_1->child[j] = child_1->child[j + 1];
                        child_1->cnt -= 1;
                        writeIndex(child_1->fd, child_1);
                        free(child_1);

                        root->child[i] = deleteIndex(root->child[i], indexkey, indexval);
                        writeIndex(root->fd, root);
                        free(root);
                        return rootfd;

                } else { // right slibbing has 31 key slots
                        child->key[child->cnt] = root->key[i];
                        child->val[child->cnt] = root->val[i];
                        for (j = 0; j < child_1->cnt; j++) {
                                child->key[child->cnt + 1 + j] = child_1->key[j];
                                child->val[child->cnt + 1 + j] = child_1->val[j];
                                child->child[child->cnt + 1 + j] = child_1->child[j];
                        }
                        child->child[child->cnt + 1 + j] = child_1->child[j];
                        child->cnt += child_1->cnt + 1;
                        writeIndex(child->fd, child);
                        freeIndex(child_1->fd);
                        root->child[i + 1] = child->fd;
                        free(child_1);
                        
                        if (root->cnt == 1) { // change root node
                                rootfd = child->fd;
                                freeIndex(root->fd);
                                free(root);
                                free(child);
                                return deleteIndex(rootfd, indexkey,indexval);
                        }
                        for (j = 0; j < root->cnt - 1; j++) {
                                root->key[j] = root->key[j + 1];
                                root->val[j] = root->val[j + 1];
                                root->child[j] = root->child[j + 1];
                        }
                        root->child[j] = root->child[j + 1];
                        root->cnt -= 1;
                        free(child);
                        root->child[i] = deleteIndex(root->child[i], indexkey, indexval);
                        writeIndex(root->fd, root);
                        free(root);
                        return rootfd;
                }
        }
        abort();
}

/* ************************** */
void btree_insertNONFULL(uint32_t rootfd, uint64_t indexkey, uint32_t indexval)
{
        struct btreenode *root, *child;
        uint32_t childfd;
        int i;

        assert((root = readIndex(rootfd)) != NULL);
        if (root->isLeaf == LEAF) {
                for (i = root->cnt - 1; i >= 0; i--) {
                        if (root->key[i] > indexkey) {
                                root->key[i + 1] = root->key[i];
                                root->val[i + 1] = root->val[i];
                        }
                }
                root->key[i] = indexkey;
                root->val[i] = indexval;
                root->cnt += 1;
                writeIndex(root->fd, root);
                free(root);
                return ;
        }
        for (i = root->cnt - 1; i >= 0; i++) {
                if (root->key[i] < indexkey) {
                        break;
                }
        }
        i++;
        assert((child = readIndex(root->child[i])) != NULL);
        if (child->cnt == BTREEORDER - 1) {//child[i] is full
                childfd = child->fd;
                free(root);
                free(child);
                
                btree_split(rootfd, i, childfd);
                assert((root = readIndex(rootfd)) != NULL);
                if (root->key[i] < indexkey) {
                        i++;
                }
                assert((child = readIndex(root->child[i])) != NULL);
        }
        childfd = child->fd;
        free(root);
        free(child);

        btree_insertNONFULL(childfd, indexkey, indexval);
        return ;
}

void btree_split(uint32_t rootfd, int i, uint32_t childfd)
{
        struct btreenode *root, *child, *otherchild;
        uint32_t otherfd;
        int j;

        assert((root = readIndex(rootfd)) != NULL);
        assert((child = readIndex(childfd)) != NULL);
        assert((otherfd = allocIndex()) != 0);
        assert((otherchild = calloc(1, sizeof(struct btreenode))) != NULL);
        
        otherchild->fd = otherfd;
        otherchild->isLeaf = child->isLeaf;
        otherchild->cnt = BTREEORDER / 2 - 1;
        for (j = 0; j < otherchild->cnt; j++) {// keys is 31
                otherchild->key[j] = child->key[BTREEORDER / 2 + j];
                child->key[BTREEORDER / 2 + j] = 0;
                otherchild->val[j] = child->val[BTREEORDER / 2 + j];
                child->val[BTREEORDER / 2 + j] = 0;
        }
        if (child->isLeaf == NONLEAF) {
                for (j = 0; j < otherchild->cnt + 1; j++) { // childs is 32
                        otherchild->child[j] = child->child[BTREEORDER / 2 + j];
                        child->child[BTREEORDER / 2 + j] = 0;
                }
        }
        child->cnt = BTREEORDER / 2 - 1;
        
        for (j = root->cnt + 1; j > i + 1; j--) { // empty slot[i + 1]
                root->child[j] = root->child[j - 1];
        }
        root->child[i + 1] = otherfd;
        for (j = root->cnt; j > i; j--) { //empty slot[i]
                root->key[j] = root->key[j - 1];
                root->val[j] = root->key[j - 1];
        }
        root->key[i] = child->key[BTREEORDER / 2];
        root->val[i] = child->val[BTREEORDER / 2];
        child->key[BTREEORDER / 2] = 0; 
        child->val[BTREEORDER / 2] = 0; 
        root->cnt += 1;
        
        writeIndex(root->fd, root);
        writeIndex(child->fd, child);
        writeIndex(otherchild->fd, otherchild);
        free(root);
        free(child);
        free(otherchild);

        return ;
}


