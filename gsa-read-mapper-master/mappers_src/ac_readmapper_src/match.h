#ifndef MATCH_H
#define MATCH_H

#include <stddef.h>

// matching callbacks
typedef void (*match_callback_func)(size_t index, void * data);

void naive_exact_match(const char *text, size_t n,
                       const char *pattern, size_t m,
                       match_callback_func callback, void *callback_data);
void boyer_moore_horspool(const char *text, size_t n,
                          const char *pattern, size_t m,
                          match_callback_func callback, void *callback_data);
void knuth_morris_pratt(const char *text, size_t n,
                        const char *pattern, size_t m,
                        match_callback_func callback, void *callback_data);
void knuth_morris_pratt_r(const char *text, size_t n,
                          const char *pattern, size_t m,
                          match_callback_func callback, void *callback_data);

#endif
