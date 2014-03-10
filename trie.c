#include "global.h"

/* ********************* */
/*   data structure      */
/* ********************* */
#define TRIE_MAX_SLOT 26
enum TRIE_NODE_TYPE {
    COM,
    UNCOM,
};

struct trienode {
    enum TRIE_NODE_TYPE type;
    char ch;
    struct trienode *child[TRIE_MAX_SLOT];
};

/* ********************* */
/*   interface           */
/* ********************* */
#define TRIEINDEX(x) ((x) - 'a')
struct trienode *allocTrie(char ch)
{
    struct trienode *node;

    assert((node = calloc(1, sizeof(struct trienode))) != NULL);
    node->ch = ch;
    node->type = UNCOM;
    return node;
}

void freeTrie(struct trienode *node)
{
    free(node);
}

int searchTrie(struct trienode *root, char *text)
{
    struct trienode *ptr;
    int i;
    
    ptr = root;
    for (i = 0; i < strlen(text); i++) {
        if (ptr->child[TRIEINDEX(text[i])] == NULL) {
            break;
        }
        ptr = ptr->child[TRIEINDEX(text[i])];
    }
    return (i == strlen(text)) && (ptr->type == COM);
}

int insertTrie(struct trienode *root, char *text)
{
    struct trienode *ptr;
    int i;

    ptr = root;
    for (i = 0; i < strlen(text); i++) {
        if (ptr->child[TRIEINDEX(text[i])] == NULL) {
            ptr->child[TRIEINDEX(text[i])] = allocTrie(text[i]);
        }
        ptr = ptr->child[TRIEINDEX(text[i])];
    }
    ptr->type = COM;
    return 0;
}

int deleteTrie(struct trienode *root, char *text)
{
    struct trienode *ptr, *pptr;
    int i, freeable = 1;

    pptr = ptr = root;
    for (i = 0; i < strlen(text); i++) {
        if (ptr->child[TRIEINDEX(text[i])] == NULL) {
            break;
        }
        pptr = ptr;
        ptr = ptr->child[TRIEINDEX(text[i])];
    }
    if (i < strlen(text) ) {
        return 0;
    }

    /* try to free child */
    for (i = 0; i < TRIE_MAX_SLOT; i++) {
        if (ptr->child[i] != NULL) {
            freeable = 0;
            break;
        }
    }
    if (freeable == 1) {
         pptr->child[TRIEINDEX(text[strlen(text) - 2])] = NULL;
         freeTrie(ptr);
    } else {
        ptr->type = UNCOM;
    }
    return 0;
}

/* ********************* */
/*   init/destroy        */
/* ********************* */
//user code


