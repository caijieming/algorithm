#ifndef BTREE_H
#define BTREE_H

#define BTREEORDER 64

extern uint64_t idctr;
/* *************************** */
/*      entry size = 128       */
/* *************************** */
struct entry {
        uint64_t id; // key of entry
        uint32_t removal; // logic deletion
        uint64_t pid; // (pid, name, removal) is a unique key
        char name[32];
        char path[76];
};

/* *************************** */
/*      node size = 1024       */
/* *************************** */
struct btreenode {
        uint32_t isLeaf; // isLeaf = 1 represents for leafnode
        uint32_t cnt; // # of valid key in key[]
        uint32_t child[64]; //child node's offset in indexfile
        uint64_t key[63];//key of entry
        uint32_t val[63];//key of entry 's offset in tablefile
        uint32_t fd;
};

struct lookup_result {
        uint32_t fd;
        uint32_t i;
};

/* table entry alloc/free management */
int initEMM(char *path);
void destroyEMM(char *path);
uint32_t allocEntry(void);
void freeEntry(uint32_t pos);

/* write/read table entry */
int createTable(char *path);
void dropTable(char *path);
struct entry *readEntry(uint32_t indexval);
int writeEntry(uint32_t indexval, struct entry *entry);

/* index alloc/free management */
int initMM(char *path);
void destroyMM(char *path);
uint32_t allocIndex(void);
void freeIndex(uint32_t pos);

/* read/write index */
int createIndex(char *path);
void dropIndex(char *path);
struct btreenode *readIndex(uint32_t indexPos);
int writeIndex(uint32_t indexPos, struct btreenode *node);

/* insert/lookup/delete index */
struct lookup_result *lookupIndex(uint32_t rootfd, uint64_t indexkey);
uint32_t insertIndex(uint32_t rootfd, uint64_t indexkey, uint32_t indexval);
uint32_t deleteIndex(uint32_t rootfd, uint64_t indexkey, uint32_t *indexval);

/* default root node's index is 1 */

#endif
