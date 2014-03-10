#ifndef TRIE_H
#define TRIE_H

struct trienode *allocTrie(char ch);
void freeTrie(struct trienode *node);
int searchTrie(struct trienode *root, char *text);
int insertTrie(struct trienode *root, char *text);
int deleteTrie(struct trienode *root, char *text);

#endif
