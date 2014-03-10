#include "global.h"

#define BTREETEST 1
//#define TRIETEST
#ifdef TRIETEST
struct trienode *root;
char *teststr[6] = {
    "tea",
    "ten",
    "to",
    "in",
    "inn",
    "int",
};

int main(int ac, char *av[])
{
        int i;

        if (ac != 2) {
                printf("usage:%s <teststr>\n", av[0]);
                exit(-1);
        }
        root = allocTrie(0);
        for (i = 0; i < 6; i++) {
                insertTrie(root, teststr[i]);
        }
        if (searchTrie(root, av[1])) {
                printf("%s in Trie tree\n", av[1]);
        } else {
                printf("%s not in Trie tree\n", av[1]);
        }
        return 0;
}
#endif

#ifdef BTREETEST
//                        /
//          |             |               |             |
//       dir001         dir002          file002      file005
//    |     |             |      |
//  dir003 file001     dir005  dir004
//                            |       |
//                          file003 file004
char *node_info[20] = {
        "dir001",
        "/dir001",
        "file001",
        "/dir001/file001",
        "dir002",
        "/dir002",
        "dir003",
        "/dir001/dir003",
        "dir005",
        "/dir002/dir005",
        "dir004",
        "/dir002/dir004",
        "file002",
        "/file002",
        "file005",
        "/file005",
        "file003",
        "/dir002/dir004/file003",
        "file004",
        "/dir002/dir004/file004",
};

void printEntry(struct entry *e)
{
        printf("id %llu, pid %llu, name %s, path %s, removal %u\n",
                e->id, e->pid, e->name, e->path, e->removal);
        return ;
}

int main(int ac, char *av[])
{
        uint32_t rootfd; // index of rooot node
        uint32_t tpos; 
        uint64_t key;
        struct entry entry, *e;
        struct btreenode *node;
        struct lookup_result *result;
        int i;
        
        if (ac != 5) {
                printf("usage:%s <tableFile> <tEntryFile> <indexFile> <iEntryFile>\n", av[0]);
                exit(-1);
        }
        
        { /* init table & index file */
                createTable(av[1]);
                initEMM(av[2]);
                assert((rootfd = createIndex(av[3])) != 0);
                initMM(av[4]);
                rootfd = 1;
        }

        for (i = 0; i < 20; i += 2) {
                assert((tpos = allocEntry()) != 0);
                memset(&entry, 0, sizeof(struct entry));
                assert(((key = ++idctr) != 1) && (key != 0));
                entry.id = key;
                entry.removal = 0;
                entry.pid = 1;
                strcpy(entry.name, node_info[i]);
                strcpy(entry.path, node_info[i + 1]);
                writeEntry(tpos, &entry);
                rootfd = insertIndex(rootfd, key, tpos);
        }
        
        for (i = 1; i <= 10; i++) {
                assert((result = lookupIndex(rootfd, i)) == NULL);
                assert((node = readIndex(result->fd)) != NULL);
                assert((e = readEntry(node->val[result->i])) != NULL);
                printf("lookup node: ");
                printEntry(e);
                if (i >= 2) {
                        rootfd = deleteIndex(rootfd, i, &tpos);
                        freeEntry(node->key[result->i]);
                        freeIndex(result->fd);
                        if (node->val[result->i] == tpos) {
                                printf("delete node: ");
                                printEntry(e);
                        }
                }
                free(e);
                free(node);
                free(result);
        }
        
        

        { /* destroy table & index file */
                destroyMM(av[4]);
                dropIndex(av[3]);
                destroyEMM(av[2]);
                dropTable(av[1]);
        }
        return 0;
}

#else

int main(int ac, char *av[])
{
    printf("algorithm hello world\n");    
    return 0;
}

#endif

