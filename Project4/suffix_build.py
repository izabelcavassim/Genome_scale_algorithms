# Suffix array construction algorithm
#from radix_sort import radix_sort


# Constructing the array in the naive way: (On * n log n)
def build_array_naive(string):
	#string = string.strip('\n')
	zero_index = sorted(range(len(string)), key=lambda i: string[i:]) # sorting suffixes by alphabethic order
	return zero_index
