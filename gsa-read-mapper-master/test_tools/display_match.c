
#include "fasta.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

#define MAX_LINE_SIZE 10000
#define BUFFER_SIZE 1024

static void parse_sam_line(const char *line_buffer, char *read_name_buffer,
                           char *ref_name_buffer, int *match_index,
                           char *cigar_buffer, char *pattern_buffer) {
    int no_matched = sscanf(
        line_buffer, "%s %*d %s %d %*d %s * %*d %*d %s %*s\n", read_name_buffer,
        ref_name_buffer, match_index, cigar_buffer, pattern_buffer);
    if (no_matched != 5) {
        fprintf(stderr, "Couldn't parse line \"%s\"\n", line_buffer);
        exit(EXIT_FAILURE);
    }
}

static const char *cigar_alignment(const char *cigar, const char *pattern,
                                   const char *matched_seq,
                                   char *pattern_buffer, char *match_buffer) {
    int count;
    char op;
    while (*cigar) {
        int no_chars_scanned;
        int matched_tokens =
            sscanf(cigar, "%d%c%n", &count, &op, &no_chars_scanned);
        if (matched_tokens != 2)
            break;
        cigar += no_chars_scanned;
        switch (op) {
        case '=':
        case 'X':
        case 'M':
            // match
            for (int i = 0; i < count; i++) {
                *(pattern_buffer++) = *(pattern++);
                *(match_buffer++) = *(matched_seq++);
            }
            break;

        case 'I':
            // insertion
            for (int i = 0; i < count; i++) {
                *(match_buffer++) = '-';
                *(pattern_buffer++) = *(pattern++);
            }
            break;

        case 'D':
            // deletion
            for (int i = 0; i < count; i++) {
                *(match_buffer++) = *(matched_seq++);
                *(pattern_buffer++) = '-';
            }
            break;

        default:
            fprintf(stderr, "Unknown CIGAR code '%c'\n", op);
            exit(1);
        }
    }

    *pattern_buffer = *match_buffer = '\0';
    return matched_seq;
}

static void print_flanking_dots(const char *start, const char *end) {
    for (const char *s = start; s < end && *s; s++) {
        putchar('.');
    }
}

static void print_flanking(const char *start, const char *end) {
    for (const char *s = start; s < end && *s; s++) {
        putchar(*s);
    }
}

static void print_pattern(const char *pattern_buffer,
                          const char *match_buffer) {
    for (const char *cp = pattern_buffer, *mc = match_buffer; *cp; cp++, mc++) {
        if (*cp == *mc) {
            // match
            printf("\033[1m%c\033[0m", *cp);
        } else {
            // mismatch
            printf("\033[1;91m%c\033[0m", *cp);
        }
    }
}

static void print_match(const char *pattern_buffer, const char *match_buffer) {
    for (const char *cp = pattern_buffer, *mc = match_buffer; *cp; cp++, mc++) {
        if (*cp == *mc) {
            // match
            printf("\033[1m%c\033[0m", *mc);
        } else {
            // mismatch
            printf("\033[1;91m%c\033[0m", *mc);
        }
    }
}

static void display_match(const char *pattern, const char *cigar,
                          const char *ref_seq_name, int match_index,
                          int flanking_seq_length,
                          struct fasta_records *ref_genome) {
    const char *ref_seq = 0;
    size_t ref_seq_size = 0;
    for (size_t i = 0; i < ref_genome->names->used; i++) {
        if (strcmp(ref_seq_name, ref_genome->names->strings[i]) == 0) {
            ref_seq = ref_genome->sequences->strings[i];
            ref_seq_size = ref_genome->seq_sizes->sizes[i];
        }
    }

    if (ref_seq == 0) {
        fprintf(stderr, "Didn't find the reference sequence %s.\n",
                ref_seq_name);
        exit(EXIT_FAILURE);
    }
    assert(ref_seq != 0); // to quite static analyser
    if ((size_t)match_index >= ref_seq_size) {
        fprintf(stderr,
                "The matching index is larger than the sequence length.\n");
        exit(EXIT_FAILURE);
    }

    const char *matched_seq =
        ref_seq + (size_t)match_index - 1; // -1 for zero-termination
    size_t pattern_length = strlen(pattern);
    char pattern_buffer[2 * pattern_length];
    bzero(pattern_buffer, sizeof(pattern_buffer));
    char match_buffer[2 * pattern_length];
    bzero(match_buffer, sizeof(match_buffer));
    const char *match_end =
        cigar_alignment(cigar, pattern, matched_seq, (char *)&pattern_buffer,
                        (char *)&match_buffer);

    printf("...");
    print_flanking_dots(
        ref_seq + MIN(match_index - 1, match_index - 1 - flanking_seq_length),
        matched_seq);
    print_pattern(pattern_buffer, match_buffer);
    print_flanking_dots(match_end, MIN(ref_seq + ref_seq_size,
                                       match_end + flanking_seq_length));
    printf("...");
    putchar('\n');

    printf("...");
    print_flanking(
        ref_seq + MIN(match_index - 1, match_index - 1 - flanking_seq_length),
        matched_seq);
    print_match(pattern_buffer, match_buffer);
    print_flanking(match_end, MIN(ref_seq + ref_seq_size,
                                  match_end + flanking_seq_length));
    printf("...");
    putchar('\n');
}

int main(int argc, const char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s reference.fa matches.sam\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *ref_filename = argv[1];
    const char *sam_filename = argv[2];
    int flanking_seq_length = 5;

    FILE *fasta_file = fopen(ref_filename, "r");
    if (!fasta_file) {
        fprintf(stderr, "Could not open %s.\n", ref_filename);
        return EXIT_FAILURE;
    }
    FILE *sam_file = fopen(sam_filename, "r");
    if (!sam_file) {
        fprintf(stderr, "Could not open %s.\n", sam_filename);
        return EXIT_FAILURE;
    }

    struct fasta_records *ref_genome = empty_fasta_records();
    if (0 != read_fasta_records(ref_genome, fasta_file)) {
        fprintf(stderr, "Errors reading reference genome.\n");
        return EXIT_FAILURE;
    }

    char read_name_buffer[BUFFER_SIZE];
    char ref_name_buffer[BUFFER_SIZE];
    int match_index;
    char cigar_buffer[BUFFER_SIZE];
    char pattern_buffer[BUFFER_SIZE];
    char line_buffer[MAX_LINE_SIZE];

    while (fgets(line_buffer, MAX_LINE_SIZE, sam_file) != 0) {
        parse_sam_line(line_buffer, (char *)&read_name_buffer,
                       (char *)ref_name_buffer, &match_index,
                       (char *)cigar_buffer, (char *)pattern_buffer);

        printf("Match: %s [at %d] %s %s\n", ref_name_buffer, match_index,
               pattern_buffer, cigar_buffer);
        display_match(pattern_buffer, cigar_buffer, ref_name_buffer,
                      match_index, flanking_seq_length, ref_genome);
        putchar('\n');
    }

    delete_fasta_records(ref_genome);

    return EXIT_SUCCESS;
}
