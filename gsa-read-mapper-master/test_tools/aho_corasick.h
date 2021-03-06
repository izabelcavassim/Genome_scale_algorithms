#ifndef AHO_CORASICK_H
#define AHO_CORASICK_H

#include "trie.h"
#include <stddef.h>

// matching callbacks
typedef void (*ac_callback_func)(int string_label, size_t index, void * data);

void aho_corasick_match(const char *text, size_t n, struct trie *patterns,
                        ac_callback_func callback, void * callback_data);


#endif
