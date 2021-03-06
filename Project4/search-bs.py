from suffix_build import build_array_naive
from sys import argv

def binary_search(pat, text, suffix_array):
    '''
    A suffix array based search function for a given pattern (pattern) in a given text (text) using
    the suffix array build by the function build_array_naive
    '''
    m = len(pat)  # lenght of the pattern
    n = len(text + '$')  # length of the text

    # Starting binary search:
    l = 0  # left index
    r = n  # right index

    # Find left most interval of the suffix array
    while (l < r):

        mid = (l + r) / 2  # middle array

        if pat > text[suffix_array[mid]:(suffix_array[mid] + len(pat))]:
            l = mid + 1
        else:
            r = mid

    # Find the right most interval of the suffix array
    s = l
    r = n
    while (l < r):
        mid = (l + r) / 2
        if pat < text[suffix_array[mid]:(suffix_array[mid] + len(pat))]:
            r = mid
        else:
            l = mid + 1

    if r < max(suffix_array): # dealing with the indexing in python
        zero_indexed = suffix_array[s:r]
        one_indexed = [x + 1 for x in zero_indexed]
 
        return(sorted(one_indexed[::-1]))
    else:
        zero_indexed = suffix_array[s:r+1]
        one_indexed = [x + 1 for x in zero_indexed]

        return(sorted(one_indexed[::-1]))

text = open(argv[1], 'r').read()
# Constructing the suffix array before search
suffix_array = build_array_naive(text + '$')
pattern = argv[2]

print(binary_search(pattern, text, suffix_array))
