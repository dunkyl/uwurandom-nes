#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<inttypes.h>

#include<neslib.h>

int nes_copy(char* dst, const char* src, size_t len) {
    memcpy(dst, src, len);
    return 0;
}

#define COPY_STR(dst, src, len) nes_copy(dst, src, len)

#define EFAULT 14

static uint16_t rand_state = 13;

void get_random_bytes(void* buf, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        rand_state = rand_state * 1105245 + 12345;
        ((char*)buf)[i] = (char)(rand_state >> 8);
    }
}

typedef struct uwu_markov_choice uwu_markov_choice;
struct uwu_markov_choice {
    size_t next_ngram;
    uint16_t cumulative_probability;
    char next_char;
};

typedef struct {
    uwu_markov_choice* choices;
    int total_probability;
} uwu_markov_ngram;

// Stores the state for a Markov chain operation.
typedef struct {
    size_t prev_ngram; /* previous ngram */
    size_t remaining_chars; /* number of remaining characters */
    uwu_markov_ngram* ngrams /* ngram table */;
} uwu_markov_state;

// Stores the state for a "print string" operation.
typedef struct {
    char* string; /* pointer to string */
    size_t remaining_chars; /* number of remaining characters */
} uwu_print_string_state;

// Stores the state for a "repeat this character" operation.
typedef struct {
    char character;
    size_t remaining_chars; /* number of remaining times to repeat the character */
} uwu_repeat_character_state;

// Because we're asked to read a certain amount of data at a time, and we
// can't control what that amount is, we need a way to store what we're
// currently doing. This is done using opcodes--currently "repeat character",
// "print string", and the most complicated one, "print using Markov chain".
// An op is executed repeatedly as many times as we need characters.
// Once it's run out of characters, move on to the next one.
// If we're all out of ops, generate a new program.

typedef union {
    uwu_markov_state markov;
    uwu_print_string_state print_string;
    uwu_repeat_character_state repeat_character;
} uwu_op_state;

typedef enum {
    UWU_NULL,
    UWU_MARKOV,
    UWU_PRINT_STRING,
    UWU_REPEAT_CHARACTER
} UWU_OPCODE;

typedef struct {
    UWU_OPCODE opcode;
    uwu_op_state state;
    void* op_data;
} uwu_op;

#define MAX_OPS 2

typedef struct {
    uwu_op ops[MAX_OPS];
    size_t current_op;
    uint16_t prev_op;
    bool print_space;
    unsigned int* rng_buf;
    size_t rng_idx;
} uwu_state;

// static struct uwu_markov_choice catnonsense_ngram0_choices[] = {
//     {.next_ngram = 0, .cumulative_probability = 2, .next_char = 'a'},
//     {.next_ngram = 1, .cumulative_probability = 3, .next_char = 'm'}
// };
// static struct uwu_markov_choice catnonsense_ngram1_choices[] = {
//     {.next_ngram = 7, .cumulative_probability = 3, .next_char = 'r'},
//     {.next_ngram = 6, .cumulative_probability = 4, .next_char = 'e'}
// };
// static struct uwu_markov_choice catnonsense_ngram2_choices[] = {
//     {.next_ngram = 8, .cumulative_probability = 1, .next_char = 'y'}
// };
// static struct uwu_markov_choice catnonsense_ngram3_choices[] = {
//     {.next_ngram = 9, .cumulative_probability = 1, .next_char = 'w'}
// };
// static struct uwu_markov_choice catnonsense_ngram4_choices[] = {
//     {.next_ngram = 9, .cumulative_probability = 1, .next_char = 'w'}
// };
// static struct uwu_markov_choice catnonsense_ngram5_choices[] = {
//     {.next_ngram = 21, .cumulative_probability = 6, .next_char = 'm'},
//     {.next_ngram = 22, .cumulative_probability = 7, .next_char = 'n'},
//     {.next_ngram = 23, .cumulative_probability = 8, .next_char = 'p'}
// };
// static struct uwu_markov_choice catnonsense_ngram6_choices[] = {
//     {.next_ngram = 4, .cumulative_probability = 3, .next_char = 'o'},
//     {.next_ngram = 5, .cumulative_probability = 5, .next_char = 'w'}
// };
// static struct uwu_markov_choice catnonsense_ngram7_choices[] = {
//     {.next_ngram = 16, .cumulative_probability = 15, .next_char = 'o'},
//     {.next_ngram = 12, .cumulative_probability = 24, .next_char = 'a'},
//     {.next_ngram = 18, .cumulative_probability = 28, .next_char = 'r'},
//     {.next_ngram = 19, .cumulative_probability = 29, .next_char = 'w'},
//     {.next_ngram = 13, .cumulative_probability = 30, .next_char = 'e'}
// };
// static struct uwu_markov_choice catnonsense_ngram8_choices[] = {
//     {.next_ngram = 26, .cumulative_probability = 1, .next_char = 'a'}
// };
// static struct uwu_markov_choice catnonsense_ngram9_choices[] = {
//     {.next_ngram = 21, .cumulative_probability = 22, .next_char = 'm'},
//     {.next_ngram = 24, .cumulative_probability = 32, .next_char = 'r'},
//     {.next_ngram = 22, .cumulative_probability = 36, .next_char = 'n'},
//     {.next_ngram = 25, .cumulative_probability = 37, .next_char = 'w'},
//     {.next_ngram = 23, .cumulative_probability = 38, .next_char = 'p'}
// };
// static struct uwu_markov_choice catnonsense_ngram10_choices[] = {
//     {.next_ngram = 11, .cumulative_probability = 1, .next_char = 'u'}
// };
// static struct uwu_markov_choice catnonsense_ngram11_choices[] = {
//     {.next_ngram = 20, .cumulative_probability = 1, .next_char = 'r'}
// };
// static struct uwu_markov_choice catnonsense_ngram12_choices[] = {
//     {.next_ngram = 3, .cumulative_probability = 1, .next_char = 'o'}
// };
// static struct uwu_markov_choice catnonsense_ngram13_choices[] = {
//     {.next_ngram = 4, .cumulative_probability = 1, .next_char = 'o'}
// };
// static struct uwu_markov_choice catnonsense_ngram14_choices[] = {
//     {.next_ngram = 7, .cumulative_probability = 1, .next_char = 'r'},
//     {.next_ngram = 6, .cumulative_probability = 2, .next_char = 'e'}
// };
// static struct uwu_markov_choice catnonsense_ngram15_choices[] = {
//     {.next_ngram = 8, .cumulative_probability = 1, .next_char = 'y'}
// };
// static struct uwu_markov_choice catnonsense_ngram16_choices[] = {
//     {.next_ngram = 9, .cumulative_probability = 1, .next_char = 'w'}
// };
// static struct uwu_markov_choice catnonsense_ngram17_choices[] = {
//     {.next_ngram = 10, .cumulative_probability = 1, .next_char = 'p'}
// };
// static struct uwu_markov_choice catnonsense_ngram18_choices[] = {
//     {.next_ngram = 18, .cumulative_probability = 7, .next_char = 'r'},
//     {.next_ngram = 14, .cumulative_probability = 10, .next_char = 'm'},
//     {.next_ngram = 16, .cumulative_probability = 13, .next_char = 'o'},
//     {.next_ngram = 17, .cumulative_probability = 14, .next_char = 'p'}
// };
// static struct uwu_markov_choice catnonsense_ngram19_choices[] = {
//     {.next_ngram = 21, .cumulative_probability = 1, .next_char = 'm'}
// };
// static struct uwu_markov_choice catnonsense_ngram20_choices[] = {
//     {.next_ngram = 18, .cumulative_probability = 1, .next_char = 'r'}
// };
// static struct uwu_markov_choice catnonsense_ngram21_choices[] = {
//     {.next_ngram = 7, .cumulative_probability = 17, .next_char = 'r'},
//     {.next_ngram = 6, .cumulative_probability = 30, .next_char = 'e'}
// };
// static struct uwu_markov_choice catnonsense_ngram22_choices[] = {
//     {.next_ngram = 8, .cumulative_probability = 1, .next_char = 'y'}
// };
// static struct uwu_markov_choice catnonsense_ngram23_choices[] = {
//     {.next_ngram = 11, .cumulative_probability = 1, .next_char = 'u'}
// };
// static struct uwu_markov_choice catnonsense_ngram24_choices[] = {
//     {.next_ngram = 14, .cumulative_probability = 7, .next_char = 'm'},
//     {.next_ngram = 15, .cumulative_probability = 10, .next_char = 'n'}
// };
// static struct uwu_markov_choice catnonsense_ngram25_choices[] = {
//     {.next_ngram = 25, .cumulative_probability = 3, .next_char = 'w'},
//     {.next_ngram = 21, .cumulative_probability = 4, .next_char = 'm'}
// };
// static struct uwu_markov_choice catnonsense_ngram26_choices[] = {
//     {.next_ngram = 0, .cumulative_probability = 4, .next_char = 'a'},
//     {.next_ngram = 2, .cumulative_probability = 5, .next_char = 'n'},
//     {.next_ngram = 1, .cumulative_probability = 6, .next_char = 'm'}
// };

// static uwu_markov_ngram catnonsense_ngrams[] = {
//     {.choices = catnonsense_ngram0_choices, .total_probability = 3}, // aa
//     {.choices = catnonsense_ngram1_choices, .total_probability = 4}, // am
//     {.choices = catnonsense_ngram2_choices, .total_probability = 1}, // an
//     {.choices = catnonsense_ngram3_choices, .total_probability = 1}, // ao
//     {.choices = catnonsense_ngram4_choices, .total_probability = 1}, // eo
//     {.choices = catnonsense_ngram5_choices, .total_probability = 8}, // ew
//     {.choices = catnonsense_ngram6_choices, .total_probability = 5}, // me
//     {.choices = catnonsense_ngram7_choices, .total_probability = 30}, // mr
//     {.choices = catnonsense_ngram8_choices, .total_probability = 1}, // ny
//     {.choices = catnonsense_ngram9_choices, .total_probability = 38}, // ow
//     {.choices = catnonsense_ngram10_choices, .total_probability = 1}, // pp
//     {.choices = catnonsense_ngram11_choices, .total_probability = 1}, // pu
//     {.choices = catnonsense_ngram12_choices, .total_probability = 1}, // ra
//     {.choices = catnonsense_ngram13_choices, .total_probability = 1}, // re
//     {.choices = catnonsense_ngram14_choices, .total_probability = 2}, // rm
//     {.choices = catnonsense_ngram15_choices, .total_probability = 1}, // rn
//     {.choices = catnonsense_ngram16_choices, .total_probability = 1}, // ro
//     {.choices = catnonsense_ngram17_choices, .total_probability = 1}, // rp
//     {.choices = catnonsense_ngram18_choices, .total_probability = 14}, // rr
//     {.choices = catnonsense_ngram19_choices, .total_probability = 1}, // rw
//     {.choices = catnonsense_ngram20_choices, .total_probability = 1}, // ur
//     {.choices = catnonsense_ngram21_choices, .total_probability = 30}, // wm
//     {.choices = catnonsense_ngram22_choices, .total_probability = 1}, // wn
//     {.choices = catnonsense_ngram23_choices, .total_probability = 1}, // wp
//     {.choices = catnonsense_ngram24_choices, .total_probability = 10}, // wr
//     {.choices = catnonsense_ngram25_choices, .total_probability = 4}, // ww
//     {.choices = catnonsense_ngram26_choices, .total_probability = 6} // ya
// };
static struct uwu_markov_choice keysmash_ngram0_choices[] = {
    {.next_ngram = 1, .cumulative_probability = 4, .next_char = 'a'},
    {.next_ngram = 10, .cumulative_probability = 5, .next_char = 'k'},
    {.next_ngram = 3, .cumulative_probability = 6, .next_char = 'd'},
    {.next_ngram = 11, .cumulative_probability = 7, .next_char = 'l'},
    {.next_ngram = 7, .cumulative_probability = 8, .next_char = 'h'}
};
static struct uwu_markov_choice keysmash_ngram1_choices[] = {
    {.next_ngram = 7, .cumulative_probability = 7, .next_char = 'h'},
    {.next_ngram = 9, .cumulative_probability = 13, .next_char = 'j'},
    {.next_ngram = 5, .cumulative_probability = 18, .next_char = 'f'},
    {.next_ngram = 11, .cumulative_probability = 21, .next_char = 'l'},
    {.next_ngram = 16, .cumulative_probability = 24, .next_char = 'u'},
    {.next_ngram = 6, .cumulative_probability = 26, .next_char = 'g'},
    {.next_ngram = 3, .cumulative_probability = 28, .next_char = 'd'},
    {.next_ngram = 0, .cumulative_probability = 29, .next_char = ';'},
    {.next_ngram = 14, .cumulative_probability = 30, .next_char = 'r'}
};
static struct uwu_markov_choice keysmash_ngram2_choices[] = {
    {.next_ngram = 10, .cumulative_probability = 1, .next_char = 'k'},
    {.next_ngram = 1, .cumulative_probability = 2, .next_char = 'a'},
    {.next_ngram = 7, .cumulative_probability = 3, .next_char = 'h'}
};
static struct uwu_markov_choice keysmash_ngram3_choices[] = {
    {.next_ngram = 5, .cumulative_probability = 7, .next_char = 'f'},
    {.next_ngram = 7, .cumulative_probability = 12, .next_char = 'h'},
    {.next_ngram = 15, .cumulative_probability = 13, .next_char = 's'},
    {.next_ngram = 6, .cumulative_probability = 14, .next_char = 'g'},
    {.next_ngram = 10, .cumulative_probability = 15, .next_char = 'k'},
    {.next_ngram = 9, .cumulative_probability = 16, .next_char = 'j'},
    {.next_ngram = 2, .cumulative_probability = 17, .next_char = 'b'}
};
static struct uwu_markov_choice keysmash_ngram4_choices[] = {
    {.next_ngram = 6, .cumulative_probability = 1, .next_char = 'g'}
};
static struct uwu_markov_choice keysmash_ngram5_choices[] = {
    {.next_ngram = 6, .cumulative_probability = 12, .next_char = 'g'},
    {.next_ngram = 10, .cumulative_probability = 16, .next_char = 'k'},
    {.next_ngram = 9, .cumulative_probability = 19, .next_char = 'j'},
    {.next_ngram = 11, .cumulative_probability = 21, .next_char = 'l'},
    {.next_ngram = 7, .cumulative_probability = 23, .next_char = 'h'},
    {.next_ngram = 3, .cumulative_probability = 25, .next_char = 'd'},
    {.next_ngram = 1, .cumulative_probability = 26, .next_char = 'a'}
};
static struct uwu_markov_choice keysmash_ngram6_choices[] = {
    {.next_ngram = 1, .cumulative_probability = 8, .next_char = 'a'},
    {.next_ngram = 7, .cumulative_probability = 14, .next_char = 'h'},
    {.next_ngram = 0, .cumulative_probability = 18, .next_char = ';'},
    {.next_ngram = 9, .cumulative_probability = 22, .next_char = 'j'},
    {.next_ngram = 11, .cumulative_probability = 25, .next_char = 'l'},
    {.next_ngram = 2, .cumulative_probability = 27, .next_char = 'b'},
    {.next_ngram = 5, .cumulative_probability = 29, .next_char = 'f'},
    {.next_ngram = 3, .cumulative_probability = 30, .next_char = 'd'},
    {.next_ngram = 15, .cumulative_probability = 31, .next_char = 's'},
    {.next_ngram = 10, .cumulative_probability = 32, .next_char = 'k'},
    {.next_ngram = 16, .cumulative_probability = 33, .next_char = 'u'},
    {.next_ngram = 12, .cumulative_probability = 34, .next_char = 'n'}
};
static struct uwu_markov_choice keysmash_ngram7_choices[] = {
    {.next_ngram = 6, .cumulative_probability = 7, .next_char = 'g'},
    {.next_ngram = 9, .cumulative_probability = 11, .next_char = 'j'},
    {.next_ngram = 5, .cumulative_probability = 14, .next_char = 'f'},
    {.next_ngram = 10, .cumulative_probability = 17, .next_char = 'k'},
    {.next_ngram = 1, .cumulative_probability = 20, .next_char = 'a'},
    {.next_ngram = 3, .cumulative_probability = 23, .next_char = 'd'},
    {.next_ngram = 8, .cumulative_probability = 25, .next_char = 'i'},
    {.next_ngram = 14, .cumulative_probability = 27, .next_char = 'r'},
    {.next_ngram = 0, .cumulative_probability = 28, .next_char = ';'},
    {.next_ngram = 12, .cumulative_probability = 29, .next_char = 'n'},
    {.next_ngram = 7, .cumulative_probability = 30, .next_char = 'h'},
    {.next_ngram = 16, .cumulative_probability = 31, .next_char = 'u'}
};
static struct uwu_markov_choice keysmash_ngram8_choices[] = {
    {.next_ngram = 16, .cumulative_probability = 1, .next_char = 'u'}
};
static struct uwu_markov_choice keysmash_ngram9_choices[] = {
    {.next_ngram = 7, .cumulative_probability = 5, .next_char = 'h'},
    {.next_ngram = 3, .cumulative_probability = 9, .next_char = 'd'},
    {.next_ngram = 10, .cumulative_probability = 12, .next_char = 'k'},
    {.next_ngram = 5, .cumulative_probability = 15, .next_char = 'f'},
    {.next_ngram = 1, .cumulative_probability = 17, .next_char = 'a'},
    {.next_ngram = 14, .cumulative_probability = 18, .next_char = 'r'},
    {.next_ngram = 4, .cumulative_probability = 19, .next_char = 'e'},
    {.next_ngram = 13, .cumulative_probability = 20, .next_char = 'o'},
    {.next_ngram = 11, .cumulative_probability = 21, .next_char = 'l'},
    {.next_ngram = 6, .cumulative_probability = 22, .next_char = 'g'}
};
static struct uwu_markov_choice keysmash_ngram10_choices[] = {
    {.next_ngram = 1, .cumulative_probability = 6, .next_char = 'a'},
    {.next_ngram = 5, .cumulative_probability = 10, .next_char = 'f'},
    {.next_ngram = 6, .cumulative_probability = 13, .next_char = 'g'},
    {.next_ngram = 9, .cumulative_probability = 16, .next_char = 'j'},
    {.next_ngram = 15, .cumulative_probability = 17, .next_char = 's'},
    {.next_ngram = 3, .cumulative_probability = 18, .next_char = 'd'},
    {.next_ngram = 11, .cumulative_probability = 19, .next_char = 'l'},
    {.next_ngram = 7, .cumulative_probability = 20, .next_char = 'h'}
};
static struct uwu_markov_choice keysmash_ngram11_choices[] = {
    {.next_ngram = 10, .cumulative_probability = 4, .next_char = 'k'},
    {.next_ngram = 0, .cumulative_probability = 6, .next_char = ';'},
    {.next_ngram = 15, .cumulative_probability = 7, .next_char = 's'},
    {.next_ngram = 6, .cumulative_probability = 8, .next_char = 'g'},
    {.next_ngram = 5, .cumulative_probability = 9, .next_char = 'f'},
    {.next_ngram = 3, .cumulative_probability = 10, .next_char = 'd'},
    {.next_ngram = 1, .cumulative_probability = 11, .next_char = 'a'}
};
static struct uwu_markov_choice keysmash_ngram12_choices[] = {
    {.next_ngram = 6, .cumulative_probability = 1, .next_char = 'g'},
    {.next_ngram = 7, .cumulative_probability = 2, .next_char = 'h'}
};
static struct uwu_markov_choice keysmash_ngram13_choices[] = {
    {.next_ngram = 6, .cumulative_probability = 1, .next_char = 'g'}
};
static struct uwu_markov_choice keysmash_ngram14_choices[] = {
    {.next_ngram = 6, .cumulative_probability = 4, .next_char = 'g'},
    {.next_ngram = 1, .cumulative_probability = 6, .next_char = 'a'},
    {.next_ngram = 7, .cumulative_probability = 7, .next_char = 'h'}
};
static struct uwu_markov_choice keysmash_ngram15_choices[] = {
    {.next_ngram = 3, .cumulative_probability = 1, .next_char = 'd'},
    {.next_ngram = 10, .cumulative_probability = 2, .next_char = 'k'}
};
static struct uwu_markov_choice keysmash_ngram16_choices[] = {
    {.next_ngram = 14, .cumulative_probability = 3, .next_char = 'r'},
    {.next_ngram = 1, .cumulative_probability = 4, .next_char = 'a'},
    {.next_ngram = 5, .cumulative_probability = 5, .next_char = 'f'},
    {.next_ngram = 9, .cumulative_probability = 6, .next_char = 'j'},
    {.next_ngram = 4, .cumulative_probability = 7, .next_char = 'e'}
};

static uwu_markov_ngram keysmash_ngrams[] = {
    {.choices = keysmash_ngram0_choices, .total_probability = 8}, // ;
    {.choices = keysmash_ngram1_choices, .total_probability = 30}, // a
    {.choices = keysmash_ngram2_choices, .total_probability = 3}, // b
    {.choices = keysmash_ngram3_choices, .total_probability = 17}, // d
    {.choices = keysmash_ngram4_choices, .total_probability = 1}, // e
    {.choices = keysmash_ngram5_choices, .total_probability = 26}, // f
    {.choices = keysmash_ngram6_choices, .total_probability = 34}, // g
    {.choices = keysmash_ngram7_choices, .total_probability = 31}, // h
    {.choices = keysmash_ngram8_choices, .total_probability = 1}, // i
    {.choices = keysmash_ngram9_choices, .total_probability = 22}, // j
    {.choices = keysmash_ngram10_choices, .total_probability = 20}, // k
    {.choices = keysmash_ngram11_choices, .total_probability = 11}, // l
    {.choices = keysmash_ngram12_choices, .total_probability = 2}, // n
    {.choices = keysmash_ngram13_choices, .total_probability = 1}, // o
    {.choices = keysmash_ngram14_choices, .total_probability = 7}, // r
    {.choices = keysmash_ngram15_choices, .total_probability = 2}, // s
    {.choices = keysmash_ngram16_choices, .total_probability = 7} // u
};
// static struct uwu_markov_choice scrunkly_ngram0_choices[] = {
//     {.next_ngram = 34, .cumulative_probability = 500, .next_char = 'n'},
//     {.next_ngram = 37, .cumulative_probability = 867, .next_char = 'w'},
//     {.next_ngram = 31, .cumulative_probability = 1067, .next_char = 'd'}
// };
// static struct uwu_markov_choice scrunkly_ngram1_choices[] = {
//     {.next_ngram = 43, .cumulative_probability = 1, .next_char = 'o'}
// };
// static struct uwu_markov_choice scrunkly_ngram2_choices[] = {
//     {.next_ngram = 46, .cumulative_probability = 60, .next_char = 'u'},
//     {.next_ngram = 45, .cumulative_probability = 107, .next_char = 'r'},
//     {.next_ngram = 44, .cumulative_probability = 147, .next_char = 'a'}
// };
// static struct uwu_markov_choice scrunkly_ngram3_choices[] = {
//     {.next_ngram = 51, .cumulative_probability = 1, .next_char = 'o'}
// };
// static struct uwu_markov_choice scrunkly_ngram4_choices[] = {
//     {.next_ngram = 61, .cumulative_probability = 1, .next_char = 'l'}
// };
// static struct uwu_markov_choice scrunkly_ngram5_choices[] = {
//     {.next_ngram = 64, .cumulative_probability = 1, .next_char = 'a'}
// };
// static struct uwu_markov_choice scrunkly_ngram6_choices[] = {
//     {.next_ngram = 77, .cumulative_probability = 200, .next_char = 't'},
//     {.next_ngram = 70, .cumulative_probability = 300, .next_char = 'c'},
//     {.next_ngram = 68, .cumulative_probability = 325, .next_char = ','},
//     {.next_ngram = 67, .cumulative_probability = 349, .next_char = '!'},
//     {.next_ngram = 69, .cumulative_probability = 364, .next_char = '.'}
// };
// static struct uwu_markov_choice scrunkly_ngram7_choices[] = {
//     {.next_ngram = 81, .cumulative_probability = 2, .next_char = 'i'},
//     {.next_ngram = 79, .cumulative_probability = 3, .next_char = 'd'}
// };
// static struct uwu_markov_choice scrunkly_ngram8_choices[] = {
//     {.next_ngram = 88, .cumulative_probability = 4, .next_char = 'i'},
//     {.next_ngram = 89, .cumulative_probability = 5, .next_char = 'o'}
// };
// static struct uwu_markov_choice scrunkly_ngram9_choices[] = {
//     {.next_ngram = 95, .cumulative_probability = 1, .next_char = 'i'},
//     {.next_ngram = 94, .cumulative_probability = 2, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram10_choices[] = {
//     {.next_ngram = 107, .cumulative_probability = 50, .next_char = 'o'},
//     {.next_ngram = 98, .cumulative_probability = 81, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram11_choices[] = {
//     {.next_ngram = 135, .cumulative_probability = 535, .next_char = 'c'},
//     {.next_ngram = 140, .cumulative_probability = 835, .next_char = 'p'},
//     {.next_ngram = 139, .cumulative_probability = 1100, .next_char = 'o'},
//     {.next_ngram = 138, .cumulative_probability = 1200, .next_char = 'm'},
//     {.next_ngram = 136, .cumulative_probability = 1271, .next_char = 'h'}
// };
// static struct uwu_markov_choice scrunkly_ngram12_choices[] = {
//     {.next_ngram = 149, .cumulative_probability = 262, .next_char = 'h'},
//     {.next_ngram = 152, .cumulative_probability = 362, .next_char = 'o'},
//     {.next_ngram = 150, .cumulative_probability = 453, .next_char = 'i'},
//     {.next_ngram = 147, .cumulative_probability = 478, .next_char = 'a'}
// };
// static struct uwu_markov_choice scrunkly_ngram13_choices[] = {
//     {.next_ngram = 165, .cumulative_probability = 11, .next_char = 'h'},
//     {.next_ngram = 166, .cumulative_probability = 19, .next_char = 'i'}
// };
// static struct uwu_markov_choice scrunkly_ngram14_choices[] = {
//     {.next_ngram = 173, .cumulative_probability = 1, .next_char = 'a'}
// };
// static struct uwu_markov_choice scrunkly_ngram15_choices[] = {
//     {.next_ngram = 12, .cumulative_probability = 190, .next_char = 't'},
//     {.next_ngram = 0, .cumulative_probability = 326, .next_char = 'a'},
//     {.next_ngram = 11, .cumulative_probability = 454, .next_char = 's'},
//     {.next_ngram = 8, .cumulative_probability = 572, .next_char = 'l'},
//     {.next_ngram = 2, .cumulative_probability = 646, .next_char = 'c'},
//     {.next_ngram = 14, .cumulative_probability = 684, .next_char = 'y'},
//     {.next_ngram = 5, .cumulative_probability = 722, .next_char = 'h'},
//     {.next_ngram = 13, .cumulative_probability = 757, .next_char = 'w'},
//     {.next_ngram = 1, .cumulative_probability = 790, .next_char = 'b'},
//     {.next_ngram = 10, .cumulative_probability = 821, .next_char = 'n'},
//     {.next_ngram = 6, .cumulative_probability = 849, .next_char = 'i'}
// };
// static struct uwu_markov_choice scrunkly_ngram16_choices[] = {
//     {.next_ngram = 15, .cumulative_probability = 31, .next_char = ' '},
//     {.next_ngram = 16, .cumulative_probability = 60, .next_char = '!'}
// };
// static struct uwu_markov_choice scrunkly_ngram17_choices[] = {
//     {.next_ngram = 17, .cumulative_probability = 777, .next_char = ','},
//     {.next_ngram = 26, .cumulative_probability = 965, .next_char = 't'},
//     {.next_ngram = 18, .cumulative_probability = 1098, .next_char = 'a'},
//     {.next_ngram = 25, .cumulative_probability = 1227, .next_char = 's'},
//     {.next_ngram = 23, .cumulative_probability = 1322, .next_char = 'l'},
//     {.next_ngram = 20, .cumulative_probability = 1387, .next_char = 'c'},
//     {.next_ngram = 24, .cumulative_probability = 1425, .next_char = 'n'},
//     {.next_ngram = 28, .cumulative_probability = 1462, .next_char = 'y'},
//     {.next_ngram = 22, .cumulative_probability = 1498, .next_char = 'i'},
//     {.next_ngram = 19, .cumulative_probability = 1531, .next_char = 'b'},
//     {.next_ngram = 21, .cumulative_probability = 1560, .next_char = 'h'},
//     {.next_ngram = 27, .cumulative_probability = 1585, .next_char = 'w'}
// };
// static struct uwu_markov_choice scrunkly_ngram18_choices[] = {
//     {.next_ngram = 37, .cumulative_probability = 1, .next_char = 'w'}
// };
// static struct uwu_markov_choice scrunkly_ngram19_choices[] = {
//     {.next_ngram = 43, .cumulative_probability = 1, .next_char = 'o'}
// };
// static struct uwu_markov_choice scrunkly_ngram20_choices[] = {
//     {.next_ngram = 45, .cumulative_probability = 1, .next_char = 'r'}
// };
// static struct uwu_markov_choice scrunkly_ngram21_choices[] = {
//     {.next_ngram = 64, .cumulative_probability = 1, .next_char = 'a'}
// };
// static struct uwu_markov_choice scrunkly_ngram22_choices[] = {
//     {.next_ngram = 67, .cumulative_probability = 7, .next_char = '!'},
//     {.next_ngram = 68, .cumulative_probability = 13, .next_char = ','},
//     {.next_ngram = 69, .cumulative_probability = 18, .next_char = '.'}
// };
// static struct uwu_markov_choice scrunkly_ngram23_choices[] = {
//     {.next_ngram = 89, .cumulative_probability = 59, .next_char = 'o'},
//     {.next_ngram = 88, .cumulative_probability = 95, .next_char = 'i'}
// };
// static struct uwu_markov_choice scrunkly_ngram24_choices[] = {
//     {.next_ngram = 98, .cumulative_probability = 1, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram25_choices[] = {
//     {.next_ngram = 135, .cumulative_probability = 65, .next_char = 'c'},
//     {.next_ngram = 139, .cumulative_probability = 100, .next_char = 'o'},
//     {.next_ngram = 136, .cumulative_probability = 129, .next_char = 'h'}
// };
// static struct uwu_markov_choice scrunkly_ngram26_choices[] = {
//     {.next_ngram = 149, .cumulative_probability = 38, .next_char = 'h'},
//     {.next_ngram = 150, .cumulative_probability = 47, .next_char = 'i'}
// };
// static struct uwu_markov_choice scrunkly_ngram27_choices[] = {
//     {.next_ngram = 165, .cumulative_probability = 1, .next_char = 'h'}
// };
// static struct uwu_markov_choice scrunkly_ngram28_choices[] = {
//     {.next_ngram = 173, .cumulative_probability = 1, .next_char = 'a'}
// };
// static struct uwu_markov_choice scrunkly_ngram29_choices[] = {
//     {.next_ngram = 12, .cumulative_probability = 222, .next_char = 't'},
//     {.next_ngram = 11, .cumulative_probability = 365, .next_char = 's'},
//     {.next_ngram = 0, .cumulative_probability = 496, .next_char = 'a'},
//     {.next_ngram = 8, .cumulative_probability = 583, .next_char = 'l'},
//     {.next_ngram = 2, .cumulative_probability = 644, .next_char = 'c'},
//     {.next_ngram = 13, .cumulative_probability = 684, .next_char = 'w'},
//     {.next_ngram = 6, .cumulative_probability = 720, .next_char = 'i'},
//     {.next_ngram = 1, .cumulative_probability = 754, .next_char = 'b'},
//     {.next_ngram = 5, .cumulative_probability = 787, .next_char = 'h'},
//     {.next_ngram = 10, .cumulative_probability = 818, .next_char = 'n'},
//     {.next_ngram = 14, .cumulative_probability = 843, .next_char = 'y'}
// };
// static struct uwu_markov_choice scrunkly_ngram30_choices[] = {
//     {.next_ngram = 29, .cumulative_probability = 638, .next_char = ' '},
//     {.next_ngram = 30, .cumulative_probability = 1271, .next_char = '.'}
// };
// static struct uwu_markov_choice scrunkly_ngram31_choices[] = {
//     {.next_ngram = 51, .cumulative_probability = 1, .next_char = 'o'}
// };
// static struct uwu_markov_choice scrunkly_ngram32_choices[] = {
//     {.next_ngram = 87, .cumulative_probability = 1, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram33_choices[] = {
//     {.next_ngram = 97, .cumulative_probability = 1, .next_char = 't'}
// };
// static struct uwu_markov_choice scrunkly_ngram34_choices[] = {
//     {.next_ngram = 103, .cumulative_probability = 5, .next_char = 'd'},
//     {.next_ngram = 98, .cumulative_probability = 6, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram35_choices[] = {
//     {.next_ngram = 122, .cumulative_probability = 1, .next_char = ' '},
//     {.next_ngram = 123, .cumulative_probability = 2, .next_char = 'p'}
// };
// static struct uwu_markov_choice scrunkly_ngram36_choices[] = {
//     {.next_ngram = 143, .cumulative_probability = 1, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram37_choices[] = {
//     {.next_ngram = 168, .cumulative_probability = 300, .next_char = 'w'},
//     {.next_ngram = 161, .cumulative_probability = 400, .next_char = ' '},
//     {.next_ngram = 162, .cumulative_probability = 443, .next_char = '!'},
//     {.next_ngram = 164, .cumulative_probability = 473, .next_char = '.'},
//     {.next_ngram = 163, .cumulative_probability = 500, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram38_choices[] = {
//     {.next_ngram = 170, .cumulative_probability = 17, .next_char = '!'},
//     {.next_ngram = 172, .cumulative_probability = 34, .next_char = '.'},
//     {.next_ngram = 171, .cumulative_probability = 50, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram39_choices[] = {
//     {.next_ngram = 16, .cumulative_probability = 29, .next_char = '!'},
//     {.next_ngram = 15, .cumulative_probability = 33, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram40_choices[] = {
//     {.next_ngram = 17, .cumulative_probability = 1, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram41_choices[] = {
//     {.next_ngram = 30, .cumulative_probability = 7, .next_char = '.'},
//     {.next_ngram = 29, .cumulative_probability = 9, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram42_choices[] = {
//     {.next_ngram = 89, .cumulative_probability = 2, .next_char = 'o'},
//     {.next_ngram = 87, .cumulative_probability = 3, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram43_choices[] = {
//     {.next_ngram = 113, .cumulative_probability = 1, .next_char = 'i'}
// };
// static struct uwu_markov_choice scrunkly_ngram44_choices[] = {
//     {.next_ngram = 34, .cumulative_probability = 1, .next_char = 'n'},
//     {.next_ngram = 33, .cumulative_probability = 2, .next_char = 'm'},
//     {.next_ngram = 36, .cumulative_probability = 3, .next_char = 't'}
// };
// static struct uwu_markov_choice scrunkly_ngram45_choices[] = {
//     {.next_ngram = 131, .cumulative_probability = 5, .next_char = 'u'},
//     {.next_ngram = 128, .cumulative_probability = 9, .next_char = 'i'}
// };
// static struct uwu_markov_choice scrunkly_ngram46_choices[] = {
//     {.next_ngram = 159, .cumulative_probability = 1, .next_char = 't'}
// };
// static struct uwu_markov_choice scrunkly_ngram47_choices[] = {
//     {.next_ngram = 2, .cumulative_probability = 2, .next_char = 'c'},
//     {.next_ngram = 3, .cumulative_probability = 3, .next_char = 'd'},
//     {.next_ngram = 6, .cumulative_probability = 4, .next_char = 'i'},
//     {.next_ngram = 11, .cumulative_probability = 5, .next_char = 's'}
// };
// static struct uwu_markov_choice scrunkly_ngram48_choices[] = {
//     {.next_ngram = 41, .cumulative_probability = 36, .next_char = '.'},
//     {.next_ngram = 39, .cumulative_probability = 69, .next_char = '!'},
//     {.next_ngram = 40, .cumulative_probability = 100, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram49_choices[] = {
//     {.next_ngram = 50, .cumulative_probability = 1, .next_char = 'l'}
// };
// static struct uwu_markov_choice scrunkly_ngram50_choices[] = {
//     {.next_ngram = 87, .cumulative_probability = 1, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram51_choices[] = {
//     {.next_ngram = 118, .cumulative_probability = 2, .next_char = 'r'},
//     {.next_ngram = 117, .cumulative_probability = 4, .next_char = 'o'},
//     {.next_ngram = 120, .cumulative_probability = 5, .next_char = 'u'}
// };
// static struct uwu_markov_choice scrunkly_ngram52_choices[] = {
//     {.next_ngram = 12, .cumulative_probability = 6, .next_char = 't'},
//     {.next_ngram = 11, .cumulative_probability = 10, .next_char = 's'},
//     {.next_ngram = 8, .cumulative_probability = 13, .next_char = 'l'},
//     {.next_ngram = 2, .cumulative_probability = 16, .next_char = 'c'},
//     {.next_ngram = 0, .cumulative_probability = 18, .next_char = 'a'},
//     {.next_ngram = 13, .cumulative_probability = 20, .next_char = 'w'},
//     {.next_ngram = 1, .cumulative_probability = 22, .next_char = 'b'},
//     {.next_ngram = 7, .cumulative_probability = 24, .next_char = 'k'},
//     {.next_ngram = 9, .cumulative_probability = 25, .next_char = 'm'}
// };
// static struct uwu_markov_choice scrunkly_ngram53_choices[] = {
//     {.next_ngram = 16, .cumulative_probability = 157, .next_char = '!'},
//     {.next_ngram = 15, .cumulative_probability = 208, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram54_choices[] = {
//     {.next_ngram = 17, .cumulative_probability = 1, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram55_choices[] = {
//     {.next_ngram = 30, .cumulative_probability = 80, .next_char = '.'},
//     {.next_ngram = 29, .cumulative_probability = 101, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram56_choices[] = {
//     {.next_ngram = 92, .cumulative_probability = 1, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram57_choices[] = {
//     {.next_ngram = 108, .cumulative_probability = 400, .next_char = 'p'},
//     {.next_ngram = 98, .cumulative_probability = 500, .next_char = ' '},
//     {.next_ngram = 101, .cumulative_probability = 536, .next_char = '.'},
//     {.next_ngram = 99, .cumulative_probability = 569, .next_char = '!'},
//     {.next_ngram = 100, .cumulative_probability = 600, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram58_choices[] = {
//     {.next_ngram = 121, .cumulative_probability = 1, .next_char = 'w'}
// };
// static struct uwu_markov_choice scrunkly_ngram59_choices[] = {
//     {.next_ngram = 141, .cumulative_probability = 1, .next_char = 't'}
// };
// static struct uwu_markov_choice scrunkly_ngram60_choices[] = {
//     {.next_ngram = 0, .cumulative_probability = 1, .next_char = 'a'}
// };
// static struct uwu_markov_choice scrunkly_ngram61_choices[] = {
//     {.next_ngram = 90, .cumulative_probability = 1, .next_char = 'u'}
// };
// static struct uwu_markov_choice scrunkly_ngram62_choices[] = {
//     {.next_ngram = 53, .cumulative_probability = 74, .next_char = '!'},
//     {.next_ngram = 54, .cumulative_probability = 139, .next_char = ','},
//     {.next_ngram = 55, .cumulative_probability = 200, .next_char = '.'}
// };
// static struct uwu_markov_choice scrunkly_ngram63_choices[] = {
//     {.next_ngram = 87, .cumulative_probability = 1, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram64_choices[] = {
//     {.next_ngram = 35, .cumulative_probability = 1, .next_char = 'p'}
// };
// static struct uwu_markov_choice scrunkly_ngram65_choices[] = {
//     {.next_ngram = 52, .cumulative_probability = 1200, .next_char = ' '},
//     {.next_ngram = 57, .cumulative_probability = 1400, .next_char = 'n'},
//     {.next_ngram = 56, .cumulative_probability = 1500, .next_char = 'm'},
//     {.next_ngram = 53, .cumulative_probability = 1537, .next_char = '!'},
//     {.next_ngram = 54, .cumulative_probability = 1569, .next_char = ','},
//     {.next_ngram = 55, .cumulative_probability = 1600, .next_char = '.'}
// };
// static struct uwu_markov_choice scrunkly_ngram66_choices[] = {
//     {.next_ngram = 130, .cumulative_probability = 1, .next_char = 'o'}
// };
// static struct uwu_markov_choice scrunkly_ngram67_choices[] = {
//     {.next_ngram = 16, .cumulative_probability = 25, .next_char = '!'},
//     {.next_ngram = 15, .cumulative_probability = 38, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram68_choices[] = {
//     {.next_ngram = 17, .cumulative_probability = 1, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram69_choices[] = {
//     {.next_ngram = 30, .cumulative_probability = 17, .next_char = '.'},
//     {.next_ngram = 29, .cumulative_probability = 25, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram70_choices[] = {
//     {.next_ngram = 44, .cumulative_probability = 1, .next_char = 'a'}
// };
// static struct uwu_markov_choice scrunkly_ngram71_choices[] = {
//     {.next_ngram = 49, .cumulative_probability = 1, .next_char = 'd'}
// };
// static struct uwu_markov_choice scrunkly_ngram72_choices[] = {
//     {.next_ngram = 57, .cumulative_probability = 400, .next_char = 'n'},
//     {.next_ngram = 55, .cumulative_probability = 443, .next_char = '.'},
//     {.next_ngram = 54, .cumulative_probability = 479, .next_char = ','},
//     {.next_ngram = 53, .cumulative_probability = 500, .next_char = '!'}
// };
// static struct uwu_markov_choice scrunkly_ngram73_choices[] = {
//     {.next_ngram = 80, .cumulative_probability = 1, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram74_choices[] = {
//     {.next_ngram = 93, .cumulative_probability = 1, .next_char = 'b'},
//     {.next_ngram = 97, .cumulative_probability = 2, .next_char = 't'}
// };
// static struct uwu_markov_choice scrunkly_ngram75_choices[] = {
//     {.next_ngram = 106, .cumulative_probability = 6, .next_char = 'k'},
//     {.next_ngram = 105, .cumulative_probability = 7, .next_char = 'g'}
// };
// static struct uwu_markov_choice scrunkly_ngram76_choices[] = {
//     {.next_ngram = 127, .cumulative_probability = 1, .next_char = 'y'}
// };
// static struct uwu_markov_choice scrunkly_ngram77_choices[] = {
//     {.next_ngram = 154, .cumulative_probability = 300, .next_char = 't'},
//     {.next_ngram = 143, .cumulative_probability = 600, .next_char = ' '},
//     {.next_ngram = 146, .cumulative_probability = 706, .next_char = '.'},
//     {.next_ngram = 155, .cumulative_probability = 806, .next_char = 'y'},
//     {.next_ngram = 145, .cumulative_probability = 905, .next_char = ','},
//     {.next_ngram = 144, .cumulative_probability = 1000, .next_char = '!'}
// };
// static struct uwu_markov_choice scrunkly_ngram78_choices[] = {
//     {.next_ngram = 160, .cumulative_probability = 1, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram79_choices[] = {
//     {.next_ngram = 48, .cumulative_probability = 1, .next_char = 'b'}
// };
// static struct uwu_markov_choice scrunkly_ngram80_choices[] = {
//     {.next_ngram = 52, .cumulative_probability = 1, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram81_choices[] = {
//     {.next_ngram = 77, .cumulative_probability = 3, .next_char = 't'},
//     {.next_ngram = 74, .cumulative_probability = 4, .next_char = 'm'}
// };
// static struct uwu_markov_choice scrunkly_ngram82_choices[] = {
//     {.next_ngram = 91, .cumulative_probability = 6, .next_char = 'y'},
//     {.next_ngram = 87, .cumulative_probability = 7, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram83_choices[] = {
//     {.next_ngram = 112, .cumulative_probability = 39, .next_char = '.'},
//     {.next_ngram = 110, .cumulative_probability = 72, .next_char = '!'},
//     {.next_ngram = 111, .cumulative_probability = 100, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram84_choices[] = {
//     {.next_ngram = 158, .cumulative_probability = 1, .next_char = 's'}
// };
// static struct uwu_markov_choice scrunkly_ngram85_choices[] = {
//     {.next_ngram = 169, .cumulative_probability = 1, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram86_choices[] = {
//     {.next_ngram = 11, .cumulative_probability = 1, .next_char = 's'}
// };
// static struct uwu_markov_choice scrunkly_ngram87_choices[] = {
//     {.next_ngram = 52, .cumulative_probability = 550, .next_char = ' '},
//     {.next_ngram = 53, .cumulative_probability = 569, .next_char = '!'},
//     {.next_ngram = 55, .cumulative_probability = 586, .next_char = '.'},
//     {.next_ngram = 54, .cumulative_probability = 600, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram88_choices[] = {
//     {.next_ngram = 77, .cumulative_probability = 3, .next_char = 't'},
//     {.next_ngram = 73, .cumulative_probability = 4, .next_char = 'k'},
//     {.next_ngram = 78, .cumulative_probability = 5, .next_char = 'v'},
//     {.next_ngram = 74, .cumulative_probability = 6, .next_char = 'm'}
// };
// static struct uwu_markov_choice scrunkly_ngram89_choices[] = {
//     {.next_ngram = 117, .cumulative_probability = 100, .next_char = 'o'},
//     {.next_ngram = 109, .cumulative_probability = 150, .next_char = ' '},
//     {.next_ngram = 110, .cumulative_probability = 169, .next_char = '!'},
//     {.next_ngram = 111, .cumulative_probability = 187, .next_char = ','},
//     {.next_ngram = 112, .cumulative_probability = 200, .next_char = '.'}
// };
// static struct uwu_markov_choice scrunkly_ngram90_choices[] = {
//     {.next_ngram = 157, .cumulative_probability = 1, .next_char = 'n'}
// };
// static struct uwu_markov_choice scrunkly_ngram91_choices[] = {
//     {.next_ngram = 169, .cumulative_probability = 300, .next_char = ' '},
//     {.next_ngram = 170, .cumulative_probability = 373, .next_char = '!'},
//     {.next_ngram = 172, .cumulative_probability = 439, .next_char = '.'},
//     {.next_ngram = 171, .cumulative_probability = 500, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram92_choices[] = {
//     {.next_ngram = 12, .cumulative_probability = 1, .next_char = 't'}
// };
// static struct uwu_markov_choice scrunkly_ngram93_choices[] = {
//     {.next_ngram = 42, .cumulative_probability = 1, .next_char = 'l'}
// };
// static struct uwu_markov_choice scrunkly_ngram94_choices[] = {
//     {.next_ngram = 58, .cumulative_probability = 1, .next_char = 'o'}
// };
// static struct uwu_markov_choice scrunkly_ngram95_choices[] = {
//     {.next_ngram = 76, .cumulative_probability = 1, .next_char = 'p'}
// };
// static struct uwu_markov_choice scrunkly_ngram96_choices[] = {
//     {.next_ngram = 115, .cumulative_probability = 1, .next_char = 'l'}
// };
// static struct uwu_markov_choice scrunkly_ngram97_choices[] = {
//     {.next_ngram = 151, .cumulative_probability = 100, .next_char = 'l'},
//     {.next_ngram = 155, .cumulative_probability = 200, .next_char = 'y'},
//     {.next_ngram = 146, .cumulative_probability = 238, .next_char = '.'},
//     {.next_ngram = 145, .cumulative_probability = 273, .next_char = ','},
//     {.next_ngram = 144, .cumulative_probability = 300, .next_char = '!'}
// };
// static struct uwu_markov_choice scrunkly_ngram98_choices[] = {
//     {.next_ngram = 12, .cumulative_probability = 2, .next_char = 't'},
//     {.next_ngram = 4, .cumulative_probability = 3, .next_char = 'f'}
// };
// static struct uwu_markov_choice scrunkly_ngram99_choices[] = {
//     {.next_ngram = 16, .cumulative_probability = 26, .next_char = '!'},
//     {.next_ngram = 15, .cumulative_probability = 33, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram100_choices[] = {
//     {.next_ngram = 17, .cumulative_probability = 1, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram101_choices[] = {
//     {.next_ngram = 30, .cumulative_probability = 25, .next_char = '.'},
//     {.next_ngram = 29, .cumulative_probability = 36, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram102_choices[] = {
//     {.next_ngram = 32, .cumulative_probability = 1, .next_char = 'l'}
// };
// static struct uwu_markov_choice scrunkly_ngram103_choices[] = {
//     {.next_ngram = 47, .cumulative_probability = 1, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram104_choices[] = {
//     {.next_ngram = 60, .cumulative_probability = 1, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram105_choices[] = {
//     {.next_ngram = 62, .cumulative_probability = 1, .next_char = 'e'},
//     {.next_ngram = 63, .cumulative_probability = 2, .next_char = 'l'}
// };
// static struct uwu_markov_choice scrunkly_ngram106_choices[] = {
//     {.next_ngram = 82, .cumulative_probability = 7, .next_char = 'l'},
//     {.next_ngram = 85, .cumulative_probability = 9, .next_char = 'y'},
//     {.next_ngram = 84, .cumulative_probability = 10, .next_char = 'u'},
//     {.next_ngram = 83, .cumulative_probability = 11, .next_char = 'o'}
// };
// static struct uwu_markov_choice scrunkly_ngram107_choices[] = {
//     {.next_ngram = 121, .cumulative_probability = 1, .next_char = 'w'}
// };
// static struct uwu_markov_choice scrunkly_ngram108_choices[] = {
//     {.next_ngram = 125, .cumulative_probability = 1, .next_char = 's'}
// };
// static struct uwu_markov_choice scrunkly_ngram109_choices[] = {
//     {.next_ngram = 0, .cumulative_probability = 1, .next_char = 'a'},
//     {.next_ngram = 6, .cumulative_probability = 2, .next_char = 'i'},
//     {.next_ngram = 8, .cumulative_probability = 3, .next_char = 'l'},
//     {.next_ngram = 1, .cumulative_probability = 4, .next_char = 'b'},
//     {.next_ngram = 9, .cumulative_probability = 5, .next_char = 'm'}
// };
// static struct uwu_markov_choice scrunkly_ngram110_choices[] = {
//     {.next_ngram = 16, .cumulative_probability = 105, .next_char = '!'},
//     {.next_ngram = 15, .cumulative_probability = 136, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram111_choices[] = {
//     {.next_ngram = 17, .cumulative_probability = 1, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram112_choices[] = {
//     {.next_ngram = 30, .cumulative_probability = 104, .next_char = '.'},
//     {.next_ngram = 29, .cumulative_probability = 141, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram113_choices[] = {
//     {.next_ngram = 75, .cumulative_probability = 1, .next_char = 'n'}
// };
// static struct uwu_markov_choice scrunkly_ngram114_choices[] = {
//     {.next_ngram = 81, .cumulative_probability = 1, .next_char = 'i'}
// };
// static struct uwu_markov_choice scrunkly_ngram115_choices[] = {
//     {.next_ngram = 86, .cumulative_probability = 1, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram116_choices[] = {
//     {.next_ngram = 106, .cumulative_probability = 1, .next_char = 'k'}
// };
// static struct uwu_markov_choice scrunkly_ngram117_choices[] = {
//     {.next_ngram = 119, .cumulative_probability = 300, .next_char = 't'},
//     {.next_ngram = 114, .cumulative_probability = 500, .next_char = 'k'},
//     {.next_ngram = 117, .cumulative_probability = 600, .next_char = 'o'},
//     {.next_ngram = 109, .cumulative_probability = 700, .next_char = ' '},
//     {.next_ngram = 112, .cumulative_probability = 776, .next_char = '.'},
//     {.next_ngram = 110, .cumulative_probability = 841, .next_char = '!'},
//     {.next_ngram = 111, .cumulative_probability = 900, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram118_choices[] = {
//     {.next_ngram = 129, .cumulative_probability = 1, .next_char = 'n'}
// };
// static struct uwu_markov_choice scrunkly_ngram119_choices[] = {
//     {.next_ngram = 153, .cumulative_probability = 1, .next_char = 's'}
// };
// static struct uwu_markov_choice scrunkly_ngram120_choices[] = {
//     {.next_ngram = 156, .cumulative_probability = 1, .next_char = 'b'}
// };
// static struct uwu_markov_choice scrunkly_ngram121_choices[] = {
//     {.next_ngram = 161, .cumulative_probability = 2, .next_char = ' '},
//     {.next_ngram = 167, .cumulative_probability = 3, .next_char = 'm'}
// };
// static struct uwu_markov_choice scrunkly_ngram122_choices[] = {
//     {.next_ngram = 10, .cumulative_probability = 1, .next_char = 'n'}
// };
// static struct uwu_markov_choice scrunkly_ngram123_choices[] = {
//     {.next_ngram = 127, .cumulative_probability = 1, .next_char = 'y'}
// };
// static struct uwu_markov_choice scrunkly_ngram124_choices[] = {
//     {.next_ngram = 130, .cumulative_probability = 1, .next_char = 'o'}
// };
// static struct uwu_markov_choice scrunkly_ngram125_choices[] = {
//     {.next_ngram = 142, .cumulative_probability = 1, .next_char = 'y'}
// };
// static struct uwu_markov_choice scrunkly_ngram126_choices[] = {
//     {.next_ngram = 157, .cumulative_probability = 1, .next_char = 'n'}
// };
// static struct uwu_markov_choice scrunkly_ngram127_choices[] = {
//     {.next_ngram = 169, .cumulative_probability = 100, .next_char = ' '},
//     {.next_ngram = 170, .cumulative_probability = 141, .next_char = '!'},
//     {.next_ngram = 171, .cumulative_probability = 177, .next_char = ','},
//     {.next_ngram = 172, .cumulative_probability = 200, .next_char = '.'}
// };
// static struct uwu_markov_choice scrunkly_ngram128_choices[] = {
//     {.next_ngram = 75, .cumulative_probability = 1, .next_char = 'n'},
//     {.next_ngram = 74, .cumulative_probability = 2, .next_char = 'm'}
// };
// static struct uwu_markov_choice scrunkly_ngram129_choices[] = {
//     {.next_ngram = 102, .cumulative_probability = 1, .next_char = 'a'}
// };
// static struct uwu_markov_choice scrunkly_ngram130_choices[] = {
//     {.next_ngram = 116, .cumulative_probability = 1, .next_char = 'n'},
//     {.next_ngram = 113, .cumulative_probability = 2, .next_char = 'i'}
// };
// static struct uwu_markov_choice scrunkly_ngram131_choices[] = {
//     {.next_ngram = 157, .cumulative_probability = 1, .next_char = 'n'}
// };
// static struct uwu_markov_choice scrunkly_ngram132_choices[] = {
//     {.next_ngram = 16, .cumulative_probability = 8, .next_char = '!'},
//     {.next_ngram = 15, .cumulative_probability = 11, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram133_choices[] = {
//     {.next_ngram = 17, .cumulative_probability = 1, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram134_choices[] = {
//     {.next_ngram = 30, .cumulative_probability = 13, .next_char = '.'},
//     {.next_ngram = 29, .cumulative_probability = 17, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram135_choices[] = {
//     {.next_ngram = 45, .cumulative_probability = 1, .next_char = 'r'}
// };
// static struct uwu_markov_choice scrunkly_ngram136_choices[] = {
//     {.next_ngram = 66, .cumulative_probability = 1, .next_char = 'r'}
// };
// static struct uwu_markov_choice scrunkly_ngram137_choices[] = {
//     {.next_ngram = 77, .cumulative_probability = 2, .next_char = 't'},
//     {.next_ngram = 72, .cumulative_probability = 3, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram138_choices[] = {
//     {.next_ngram = 96, .cumulative_probability = 1, .next_char = 'o'}
// };
// static struct uwu_markov_choice scrunkly_ngram139_choices[] = {
//     {.next_ngram = 109, .cumulative_probability = 2, .next_char = ' '},
//     {.next_ngram = 117, .cumulative_probability = 3, .next_char = 'o'}
// };
// static struct uwu_markov_choice scrunkly_ngram140_choices[] = {
//     {.next_ngram = 126, .cumulative_probability = 2, .next_char = 'u'},
//     {.next_ngram = 124, .cumulative_probability = 3, .next_char = 'r'}
// };
// static struct uwu_markov_choice scrunkly_ngram141_choices[] = {
//     {.next_ngram = 143, .cumulative_probability = 1, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram142_choices[] = {
//     {.next_ngram = 169, .cumulative_probability = 300, .next_char = ' '},
//     {.next_ngram = 172, .cumulative_probability = 341, .next_char = '.'},
//     {.next_ngram = 171, .cumulative_probability = 376, .next_char = ','},
//     {.next_ngram = 170, .cumulative_probability = 400, .next_char = '!'}
// };
// static struct uwu_markov_choice scrunkly_ngram143_choices[] = {
//     {.next_ngram = 12, .cumulative_probability = 2, .next_char = 't'},
//     {.next_ngram = 0, .cumulative_probability = 4, .next_char = 'a'},
//     {.next_ngram = 2, .cumulative_probability = 5, .next_char = 'c'},
//     {.next_ngram = 11, .cumulative_probability = 6, .next_char = 's'}
// };
// static struct uwu_markov_choice scrunkly_ngram144_choices[] = {
//     {.next_ngram = 16, .cumulative_probability = 85, .next_char = '!'},
//     {.next_ngram = 15, .cumulative_probability = 122, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram145_choices[] = {
//     {.next_ngram = 17, .cumulative_probability = 1, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram146_choices[] = {
//     {.next_ngram = 30, .cumulative_probability = 101, .next_char = '.'},
//     {.next_ngram = 29, .cumulative_probability = 144, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram147_choices[] = {
//     {.next_ngram = 35, .cumulative_probability = 1, .next_char = 'p'}
// };
// static struct uwu_markov_choice scrunkly_ngram148_choices[] = {
//     {.next_ngram = 59, .cumulative_probability = 200, .next_char = 's'},
//     {.next_ngram = 53, .cumulative_probability = 238, .next_char = '!'},
//     {.next_ngram = 55, .cumulative_probability = 271, .next_char = '.'},
//     {.next_ngram = 54, .cumulative_probability = 300, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram149_choices[] = {
//     {.next_ngram = 65, .cumulative_probability = 1, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram150_choices[] = {
//     {.next_ngram = 72, .cumulative_probability = 1, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram151_choices[] = {
//     {.next_ngram = 87, .cumulative_probability = 1, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram152_choices[] = {
//     {.next_ngram = 117, .cumulative_probability = 3, .next_char = 'o'},
//     {.next_ngram = 109, .cumulative_probability = 4, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram153_choices[] = {
//     {.next_ngram = 137, .cumulative_probability = 1, .next_char = 'i'}
// };
// static struct uwu_markov_choice scrunkly_ngram154_choices[] = {
//     {.next_ngram = 151, .cumulative_probability = 1, .next_char = 'l'}
// };
// static struct uwu_markov_choice scrunkly_ngram155_choices[] = {
//     {.next_ngram = 169, .cumulative_probability = 100, .next_char = ' '},
//     {.next_ngram = 171, .cumulative_probability = 138, .next_char = ','},
//     {.next_ngram = 170, .cumulative_probability = 169, .next_char = '!'},
//     {.next_ngram = 172, .cumulative_probability = 200, .next_char = '.'}
// };
// static struct uwu_markov_choice scrunkly_ngram156_choices[] = {
//     {.next_ngram = 42, .cumulative_probability = 1, .next_char = 'l'}
// };
// static struct uwu_markov_choice scrunkly_ngram157_choices[] = {
//     {.next_ngram = 106, .cumulative_probability = 4, .next_char = 'k'},
//     {.next_ngram = 105, .cumulative_probability = 7, .next_char = 'g'},
//     {.next_ngram = 104, .cumulative_probability = 8, .next_char = 'f'}
// };
// static struct uwu_markov_choice scrunkly_ngram158_choices[] = {
//     {.next_ngram = 134, .cumulative_probability = 34, .next_char = '.'},
//     {.next_ngram = 132, .cumulative_probability = 67, .next_char = '!'},
//     {.next_ngram = 133, .cumulative_probability = 100, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram159_choices[] = {
//     {.next_ngram = 148, .cumulative_probability = 1, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram160_choices[] = {
//     {.next_ngram = 52, .cumulative_probability = 1, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram161_choices[] = {
//     {.next_ngram = 12, .cumulative_probability = 2, .next_char = 't'},
//     {.next_ngram = 8, .cumulative_probability = 3, .next_char = 'l'},
//     {.next_ngram = 11, .cumulative_probability = 4, .next_char = 's'},
//     {.next_ngram = 13, .cumulative_probability = 5, .next_char = 'w'},
//     {.next_ngram = 6, .cumulative_probability = 6, .next_char = 'i'}
// };
// static struct uwu_markov_choice scrunkly_ngram162_choices[] = {
//     {.next_ngram = 16, .cumulative_probability = 24, .next_char = '!'},
//     {.next_ngram = 15, .cumulative_probability = 43, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram163_choices[] = {
//     {.next_ngram = 17, .cumulative_probability = 1, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram164_choices[] = {
//     {.next_ngram = 30, .cumulative_probability = 4, .next_char = '.'},
//     {.next_ngram = 29, .cumulative_probability = 5, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram165_choices[] = {
//     {.next_ngram = 65, .cumulative_probability = 1, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram166_choices[] = {
//     {.next_ngram = 71, .cumulative_probability = 1, .next_char = 'd'}
// };
// static struct uwu_markov_choice scrunkly_ngram167_choices[] = {
//     {.next_ngram = 94, .cumulative_probability = 1, .next_char = 'e'}
// };
// static struct uwu_markov_choice scrunkly_ngram168_choices[] = {
//     {.next_ngram = 161, .cumulative_probability = 3, .next_char = ' '},
//     {.next_ngram = 168, .cumulative_probability = 5, .next_char = 'w'}
// };
// static struct uwu_markov_choice scrunkly_ngram169_choices[] = {
//     {.next_ngram = 12, .cumulative_probability = 2, .next_char = 't'},
//     {.next_ngram = 11, .cumulative_probability = 4, .next_char = 's'},
//     {.next_ngram = 3, .cumulative_probability = 6, .next_char = 'd'},
//     {.next_ngram = 0, .cumulative_probability = 8, .next_char = 'a'},
//     {.next_ngram = 7, .cumulative_probability = 9, .next_char = 'k'},
//     {.next_ngram = 13, .cumulative_probability = 10, .next_char = 'w'}
// };
// static struct uwu_markov_choice scrunkly_ngram170_choices[] = {
//     {.next_ngram = 16, .cumulative_probability = 5, .next_char = '!'},
//     {.next_ngram = 15, .cumulative_probability = 7, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram171_choices[] = {
//     {.next_ngram = 17, .cumulative_probability = 1, .next_char = ','}
// };
// static struct uwu_markov_choice scrunkly_ngram172_choices[] = {
//     {.next_ngram = 30, .cumulative_probability = 51, .next_char = '.'},
//     {.next_ngram = 29, .cumulative_probability = 65, .next_char = ' '}
// };
// static struct uwu_markov_choice scrunkly_ngram173_choices[] = {
//     {.next_ngram = 38, .cumulative_probability = 1, .next_char = 'y'}
// };

// static uwu_markov_ngram scrunkly_ngrams[] = {
//     {.choices = scrunkly_ngram0_choices, .total_probability = 1067}, //  a
//     {.choices = scrunkly_ngram1_choices, .total_probability = 1}, //  b
//     {.choices = scrunkly_ngram2_choices, .total_probability = 147}, //  c
//     {.choices = scrunkly_ngram3_choices, .total_probability = 1}, //  d
//     {.choices = scrunkly_ngram4_choices, .total_probability = 1}, //  f
//     {.choices = scrunkly_ngram5_choices, .total_probability = 1}, //  h
//     {.choices = scrunkly_ngram6_choices, .total_probability = 364}, //  i
//     {.choices = scrunkly_ngram7_choices, .total_probability = 3}, //  k
//     {.choices = scrunkly_ngram8_choices, .total_probability = 5}, //  l
//     {.choices = scrunkly_ngram9_choices, .total_probability = 2}, //  m
//     {.choices = scrunkly_ngram10_choices, .total_probability = 81}, //  n
//     {.choices = scrunkly_ngram11_choices, .total_probability = 1271}, //  s
//     {.choices = scrunkly_ngram12_choices, .total_probability = 478}, //  t
//     {.choices = scrunkly_ngram13_choices, .total_probability = 19}, //  w
//     {.choices = scrunkly_ngram14_choices, .total_probability = 1}, //  y
//     {.choices = scrunkly_ngram15_choices, .total_probability = 849}, // ! 
//     {.choices = scrunkly_ngram16_choices, .total_probability = 60}, // !!
//     {.choices = scrunkly_ngram17_choices, .total_probability = 1585}, // ,,
//     {.choices = scrunkly_ngram18_choices, .total_probability = 1}, // ,a
//     {.choices = scrunkly_ngram19_choices, .total_probability = 1}, // ,b
//     {.choices = scrunkly_ngram20_choices, .total_probability = 1}, // ,c
//     {.choices = scrunkly_ngram21_choices, .total_probability = 1}, // ,h
//     {.choices = scrunkly_ngram22_choices, .total_probability = 18}, // ,i
//     {.choices = scrunkly_ngram23_choices, .total_probability = 95}, // ,l
//     {.choices = scrunkly_ngram24_choices, .total_probability = 1}, // ,n
//     {.choices = scrunkly_ngram25_choices, .total_probability = 129}, // ,s
//     {.choices = scrunkly_ngram26_choices, .total_probability = 47}, // ,t
//     {.choices = scrunkly_ngram27_choices, .total_probability = 1}, // ,w
//     {.choices = scrunkly_ngram28_choices, .total_probability = 1}, // ,y
//     {.choices = scrunkly_ngram29_choices, .total_probability = 843}, // . 
//     {.choices = scrunkly_ngram30_choices, .total_probability = 1271}, // ..
//     {.choices = scrunkly_ngram31_choices, .total_probability = 1}, // ad
//     {.choices = scrunkly_ngram32_choices, .total_probability = 1}, // al
//     {.choices = scrunkly_ngram33_choices, .total_probability = 1}, // am
//     {.choices = scrunkly_ngram34_choices, .total_probability = 6}, // an
//     {.choices = scrunkly_ngram35_choices, .total_probability = 2}, // ap
//     {.choices = scrunkly_ngram36_choices, .total_probability = 1}, // at
//     {.choices = scrunkly_ngram37_choices, .total_probability = 500}, // aw
//     {.choices = scrunkly_ngram38_choices, .total_probability = 50}, // ay
//     {.choices = scrunkly_ngram39_choices, .total_probability = 33}, // b!
//     {.choices = scrunkly_ngram40_choices, .total_probability = 1}, // b,
//     {.choices = scrunkly_ngram41_choices, .total_probability = 9}, // b.
//     {.choices = scrunkly_ngram42_choices, .total_probability = 3}, // bl
//     {.choices = scrunkly_ngram43_choices, .total_probability = 1}, // bo
//     {.choices = scrunkly_ngram44_choices, .total_probability = 3}, // ca
//     {.choices = scrunkly_ngram45_choices, .total_probability = 9}, // cr
//     {.choices = scrunkly_ngram46_choices, .total_probability = 1}, // cu
//     {.choices = scrunkly_ngram47_choices, .total_probability = 5}, // d 
//     {.choices = scrunkly_ngram48_choices, .total_probability = 100}, // db
//     {.choices = scrunkly_ngram49_choices, .total_probability = 1}, // dd
//     {.choices = scrunkly_ngram50_choices, .total_probability = 1}, // dl
//     {.choices = scrunkly_ngram51_choices, .total_probability = 5}, // do
//     {.choices = scrunkly_ngram52_choices, .total_probability = 25}, // e 
//     {.choices = scrunkly_ngram53_choices, .total_probability = 208}, // e!
//     {.choices = scrunkly_ngram54_choices, .total_probability = 1}, // e,
//     {.choices = scrunkly_ngram55_choices, .total_probability = 101}, // e.
//     {.choices = scrunkly_ngram56_choices, .total_probability = 1}, // em
//     {.choices = scrunkly_ngram57_choices, .total_probability = 600}, // en
//     {.choices = scrunkly_ngram58_choices, .total_probability = 1}, // eo
//     {.choices = scrunkly_ngram59_choices, .total_probability = 1}, // es
//     {.choices = scrunkly_ngram60_choices, .total_probability = 1}, // f 
//     {.choices = scrunkly_ngram61_choices, .total_probability = 1}, // fl
//     {.choices = scrunkly_ngram62_choices, .total_probability = 200}, // ge
//     {.choices = scrunkly_ngram63_choices, .total_probability = 1}, // gl
//     {.choices = scrunkly_ngram64_choices, .total_probability = 1}, // ha
//     {.choices = scrunkly_ngram65_choices, .total_probability = 1600}, // he
//     {.choices = scrunkly_ngram66_choices, .total_probability = 1}, // hr
//     {.choices = scrunkly_ngram67_choices, .total_probability = 38}, // i!
//     {.choices = scrunkly_ngram68_choices, .total_probability = 1}, // i,
//     {.choices = scrunkly_ngram69_choices, .total_probability = 25}, // i.
//     {.choices = scrunkly_ngram70_choices, .total_probability = 1}, // ic
//     {.choices = scrunkly_ngram71_choices, .total_probability = 1}, // id
//     {.choices = scrunkly_ngram72_choices, .total_probability = 500}, // ie
//     {.choices = scrunkly_ngram73_choices, .total_probability = 1}, // ik
//     {.choices = scrunkly_ngram74_choices, .total_probability = 2}, // im
//     {.choices = scrunkly_ngram75_choices, .total_probability = 7}, // in
//     {.choices = scrunkly_ngram76_choices, .total_probability = 1}, // ip
//     {.choices = scrunkly_ngram77_choices, .total_probability = 1000}, // it
//     {.choices = scrunkly_ngram78_choices, .total_probability = 1}, // iv
//     {.choices = scrunkly_ngram79_choices, .total_probability = 1}, // kd
//     {.choices = scrunkly_ngram80_choices, .total_probability = 1}, // ke
//     {.choices = scrunkly_ngram81_choices, .total_probability = 4}, // ki
//     {.choices = scrunkly_ngram82_choices, .total_probability = 7}, // kl
//     {.choices = scrunkly_ngram83_choices, .total_probability = 100}, // ko
//     {.choices = scrunkly_ngram84_choices, .total_probability = 1}, // ku
//     {.choices = scrunkly_ngram85_choices, .total_probability = 1}, // ky
//     {.choices = scrunkly_ngram86_choices, .total_probability = 1}, // l 
//     {.choices = scrunkly_ngram87_choices, .total_probability = 600}, // le
//     {.choices = scrunkly_ngram88_choices, .total_probability = 6}, // li
//     {.choices = scrunkly_ngram89_choices, .total_probability = 200}, // lo
//     {.choices = scrunkly_ngram90_choices, .total_probability = 1}, // lu
//     {.choices = scrunkly_ngram91_choices, .total_probability = 500}, // ly
//     {.choices = scrunkly_ngram92_choices, .total_probability = 1}, // m 
//     {.choices = scrunkly_ngram93_choices, .total_probability = 1}, // mb
//     {.choices = scrunkly_ngram94_choices, .total_probability = 1}, // me
//     {.choices = scrunkly_ngram95_choices, .total_probability = 1}, // mi
//     {.choices = scrunkly_ngram96_choices, .total_probability = 1}, // mo
//     {.choices = scrunkly_ngram97_choices, .total_probability = 300}, // mt
//     {.choices = scrunkly_ngram98_choices, .total_probability = 3}, // n 
//     {.choices = scrunkly_ngram99_choices, .total_probability = 33}, // n!
//     {.choices = scrunkly_ngram100_choices, .total_probability = 1}, // n,
//     {.choices = scrunkly_ngram101_choices, .total_probability = 36}, // n.
//     {.choices = scrunkly_ngram102_choices, .total_probability = 1}, // na
//     {.choices = scrunkly_ngram103_choices, .total_probability = 1}, // nd
//     {.choices = scrunkly_ngram104_choices, .total_probability = 1}, // nf
//     {.choices = scrunkly_ngram105_choices, .total_probability = 2}, // ng
//     {.choices = scrunkly_ngram106_choices, .total_probability = 11}, // nk
//     {.choices = scrunkly_ngram107_choices, .total_probability = 1}, // no
//     {.choices = scrunkly_ngram108_choices, .total_probability = 1}, // np
//     {.choices = scrunkly_ngram109_choices, .total_probability = 5}, // o 
//     {.choices = scrunkly_ngram110_choices, .total_probability = 136}, // o!
//     {.choices = scrunkly_ngram111_choices, .total_probability = 1}, // o,
//     {.choices = scrunkly_ngram112_choices, .total_probability = 141}, // o.
//     {.choices = scrunkly_ngram113_choices, .total_probability = 1}, // oi
//     {.choices = scrunkly_ngram114_choices, .total_probability = 1}, // ok
//     {.choices = scrunkly_ngram115_choices, .total_probability = 1}, // ol
//     {.choices = scrunkly_ngram116_choices, .total_probability = 1}, // on
//     {.choices = scrunkly_ngram117_choices, .total_probability = 900}, // oo
//     {.choices = scrunkly_ngram118_choices, .total_probability = 1}, // or
//     {.choices = scrunkly_ngram119_choices, .total_probability = 1}, // ot
//     {.choices = scrunkly_ngram120_choices, .total_probability = 1}, // ou
//     {.choices = scrunkly_ngram121_choices, .total_probability = 3}, // ow
//     {.choices = scrunkly_ngram122_choices, .total_probability = 1}, // p 
//     {.choices = scrunkly_ngram123_choices, .total_probability = 1}, // pp
//     {.choices = scrunkly_ngram124_choices, .total_probability = 1}, // pr
//     {.choices = scrunkly_ngram125_choices, .total_probability = 1}, // ps
//     {.choices = scrunkly_ngram126_choices, .total_probability = 1}, // pu
//     {.choices = scrunkly_ngram127_choices, .total_probability = 200}, // py
//     {.choices = scrunkly_ngram128_choices, .total_probability = 2}, // ri
//     {.choices = scrunkly_ngram129_choices, .total_probability = 1}, // rn
//     {.choices = scrunkly_ngram130_choices, .total_probability = 2}, // ro
//     {.choices = scrunkly_ngram131_choices, .total_probability = 1}, // ru
//     {.choices = scrunkly_ngram132_choices, .total_probability = 11}, // s!
//     {.choices = scrunkly_ngram133_choices, .total_probability = 1}, // s,
//     {.choices = scrunkly_ngram134_choices, .total_probability = 17}, // s.
//     {.choices = scrunkly_ngram135_choices, .total_probability = 1}, // sc
//     {.choices = scrunkly_ngram136_choices, .total_probability = 1}, // sh
//     {.choices = scrunkly_ngram137_choices, .total_probability = 3}, // si
//     {.choices = scrunkly_ngram138_choices, .total_probability = 1}, // sm
//     {.choices = scrunkly_ngram139_choices, .total_probability = 3}, // so
//     {.choices = scrunkly_ngram140_choices, .total_probability = 3}, // sp
//     {.choices = scrunkly_ngram141_choices, .total_probability = 1}, // st
//     {.choices = scrunkly_ngram142_choices, .total_probability = 400}, // sy
//     {.choices = scrunkly_ngram143_choices, .total_probability = 6}, // t 
//     {.choices = scrunkly_ngram144_choices, .total_probability = 122}, // t!
//     {.choices = scrunkly_ngram145_choices, .total_probability = 1}, // t,
//     {.choices = scrunkly_ngram146_choices, .total_probability = 144}, // t.
//     {.choices = scrunkly_ngram147_choices, .total_probability = 1}, // ta
//     {.choices = scrunkly_ngram148_choices, .total_probability = 300}, // te
//     {.choices = scrunkly_ngram149_choices, .total_probability = 1}, // th
//     {.choices = scrunkly_ngram150_choices, .total_probability = 1}, // ti
//     {.choices = scrunkly_ngram151_choices, .total_probability = 1}, // tl
//     {.choices = scrunkly_ngram152_choices, .total_probability = 4}, // to
//     {.choices = scrunkly_ngram153_choices, .total_probability = 1}, // ts
//     {.choices = scrunkly_ngram154_choices, .total_probability = 1}, // tt
//     {.choices = scrunkly_ngram155_choices, .total_probability = 200}, // ty
//     {.choices = scrunkly_ngram156_choices, .total_probability = 1}, // ub
//     {.choices = scrunkly_ngram157_choices, .total_probability = 8}, // un
//     {.choices = scrunkly_ngram158_choices, .total_probability = 100}, // us
//     {.choices = scrunkly_ngram159_choices, .total_probability = 1}, // ut
//     {.choices = scrunkly_ngram160_choices, .total_probability = 1}, // ve
//     {.choices = scrunkly_ngram161_choices, .total_probability = 6}, // w 
//     {.choices = scrunkly_ngram162_choices, .total_probability = 43}, // w!
//     {.choices = scrunkly_ngram163_choices, .total_probability = 1}, // w,
//     {.choices = scrunkly_ngram164_choices, .total_probability = 5}, // w.
//     {.choices = scrunkly_ngram165_choices, .total_probability = 1}, // wh
//     {.choices = scrunkly_ngram166_choices, .total_probability = 1}, // wi
//     {.choices = scrunkly_ngram167_choices, .total_probability = 1}, // wm
//     {.choices = scrunkly_ngram168_choices, .total_probability = 5}, // ww
//     {.choices = scrunkly_ngram169_choices, .total_probability = 10}, // y 
//     {.choices = scrunkly_ngram170_choices, .total_probability = 7}, // y!
//     {.choices = scrunkly_ngram171_choices, .total_probability = 1}, // y,
//     {.choices = scrunkly_ngram172_choices, .total_probability = 65}, // y.
//     {.choices = scrunkly_ngram173_choices, .total_probability = 1} // ya
// };

#define CREATE_PRINT_STRING(printed_string) {\
    .opcode = UWU_PRINT_STRING,\
    .state = {\
        .print_string = {\
            .string = (printed_string),\
            .remaining_chars = sizeof(printed_string) - 1\
        }\
    }\
}

#define CREATE_REPEAT_CHARACTER(repeated_character, num_repetitions) {\
    .opcode = UWU_REPEAT_CHARACTER,\
    .state = {\
        .repeat_character = {\
            .character = (repeated_character),\
            .remaining_chars = (num_repetitions)\
        }\
    }\
}


typedef struct {
    size_t len;
    char* string;
} string_with_len;

#define STRING_WITH_LEN(literal) {.len = (sizeof(literal) - 1), .string = literal}

static string_with_len actions[] = {
    STRING_WITH_LEN("*tilts head*"),
    STRING_WITH_LEN("*twitches ears slightly*"),
    STRING_WITH_LEN("*purrs*"),
    STRING_WITH_LEN("*falls asleep*"),
    STRING_WITH_LEN("*sits on ur keyboard*"),
    STRING_WITH_LEN("*nuzzles*"),
    // STRING_WITH_LEN("*stares at u*"),
    // STRING_WITH_LEN("*points towards case of monster zero ultra*"),
    // STRING_WITH_LEN("*sneezes*"),
    // STRING_WITH_LEN("*plays with yarn*"),
    // STRING_WITH_LEN("*eats all ur doritos*"),
    // STRING_WITH_LEN("*lies down on a random surface*")
};

// const size_t RAND_SIZE = 128;
#define RAND_SIZE 16
static unsigned int uwu_random_int(uwu_state* state) {
    if (state->rng_idx >= RAND_SIZE) {
        get_random_bytes(state->rng_buf, RAND_SIZE * sizeof(unsigned int));
        state->rng_idx = 0;
    }
    unsigned int rand_value = state->rng_buf[state->rng_idx];
    state->rng_idx++;
    return rand_value;
}

// Pick a random program from the list of programs and write it to the ops list
static void
generate_new_ops(uwu_state* state) {
    // init to 0 in case get_random_bytes fails
    uint16_t random = uwu_random_int(state);

    static uwu_op null_op = {
        .opcode = UWU_NULL
    };

    static const int NUM_OPS = 10;

    if (state->prev_op == UINT16_MAX) {
        random %= NUM_OPS;
    } else {
        // don't repeat previous op
        random %= NUM_OPS - 1;
        if (random >= state->prev_op) {
            random += 1;
        }
    }

    state->prev_op = random;

    uwu_op* ops = state->ops;
    switch (random) {
        case 0: { // uwu
            uwu_op op = CREATE_PRINT_STRING("uwu");
            ops[0] = op;
            ops[1] = null_op;
            break;
        }
        case 1: { // catgirl nonsense
            // random = uwu_random_int(state);
            // uwu_op op1 = CREATE_PRINT_STRING("mr");
            // uwu_op op2 = {
            //     .opcode = UWU_MARKOV,
            //     .state = {
            //         .markov = {
            //             .prev_ngram = 7, // mr
            //             .remaining_chars = (random % 125) + 25,
            //             .ngrams = catnonsense_ngrams
            //         }
            //     }
            // };
            // uwu_op op3 = CREATE_PRINT_STRING("nya");
            // ops[0] = op1;
            // ops[1] = op2;
            // ops[2] = op3;
            // ops[3] = null_op;
            // break;
            [[fallthrough]];
        }
        case 2: { // nyaaaaaaa
            random = uwu_random_int(state);
            uwu_op op1 = CREATE_PRINT_STRING("ny");
            uwu_op op2 = CREATE_REPEAT_CHARACTER('a', (random % 7) + 1);
            ops[0] = op1;
            ops[1] = op2;
            ops[2] = null_op;
            break;
        }
        case 3: { // >/////<
            random = uwu_random_int(state);
            uwu_op op1 = CREATE_PRINT_STRING(">");
            uwu_op op2 = CREATE_REPEAT_CHARACTER('/', (random % 4) + 3);
            uwu_op op3 = CREATE_PRINT_STRING("<");
            ops[0] = op1;
            ops[1] = op2;
            ops[2] = op3;
            ops[3] = null_op;
            break;
        }
        case 4: { // :3
            uwu_op op = CREATE_PRINT_STRING(":3");
            ops[0] = op;
            ops[1] = null_op;
            break;
        }
        case 5: { // actions
            random = uwu_random_int(state);
            string_with_len action = actions[random % (sizeof(actions) / sizeof(string_with_len))];
            uwu_op op = {
                .opcode = UWU_PRINT_STRING,
                .state = {
                    .print_string = {
                        .string = action.string,
                        .remaining_chars = action.len
                    }
                }
            };
            ops[0] = op;
            ops[1] = null_op;
            break;
        }
        case 6: { // keyboard mash
            random = uwu_random_int(state);
            uwu_op op = {
                .opcode = UWU_MARKOV,
                .state = {
                    .markov = {
                        .prev_ngram = random % (sizeof(keysmash_ngrams) / sizeof(uwu_markov_ngram)),
                        .remaining_chars = (random % 125) + 25,
                        .ngrams = keysmash_ngrams
                    }
                }
            };
            ops[0] = op;
            ops[1] = null_op;
            break;
        }
        case 7: { // screaming
            random = uwu_random_int(state);
            uwu_op op = CREATE_REPEAT_CHARACTER('A', (random % 12) + 5);
            ops[0] = op;
            ops[1] = null_op;
            break;
        }
        case 8: { // aww the scrunkly :)
            // random = uwu_random_int(state);
            // uwu_op op1 = CREATE_PRINT_STRING("aw");
            // uwu_op op2 = {
            //     .opcode = UWU_MARKOV,
            //     .state = {
            //         .markov = {
            //             .prev_ngram = 37, // aw
            //             .remaining_chars = (random % 75) + 25,
            //             .ngrams = scrunkly_ngrams
            //         }
            //     }
            // };
            // ops[0] = op1;
            // ops[1] = op2;
            // ops[2] = null_op;
            // break;
            [[fallthrough]];
        }
        case 9: { // owo
            uwu_op op = CREATE_PRINT_STRING("owo");
            ops[0] = op;
            ops[1] = null_op;
            break;
        }
    }
}

// Execute an operation once. Returns the number of characters written, or a negative value on error.
static int exec_op(uwu_state* state, char* buf, size_t len) {
    uwu_op* op = &state->ops[state->current_op];
    switch (op->opcode) {
        case UWU_PRINT_STRING: {
            char* string = op->state.print_string.string;
            size_t remaining = op->state.print_string.remaining_chars;

            if (remaining == 0) return 0;

            size_t num_chars_to_copy = remaining > len ? len : remaining;

            int result = COPY_STR(buf, string, num_chars_to_copy);
            // TODO: advance state in failure case
            if (result) return -EFAULT;

            // Advance state by number of characters copied;
            op->state.print_string.string += num_chars_to_copy;
            op->state.print_string.remaining_chars -= num_chars_to_copy;

            return num_chars_to_copy;
        }

        case UWU_MARKOV: {
            size_t ngram_index = op->state.markov.prev_ngram;
            uwu_markov_ngram* ngrams = op->state.markov.ngrams;
            size_t remaining = op->state.markov.remaining_chars;

            size_t num_chars_to_copy = remaining > len ? len : remaining;

            for (size_t i = 0; i < num_chars_to_copy; ++i) {
                uwu_markov_ngram ngram = ngrams[ngram_index];
                unsigned int random = uwu_random_int(state);
                random %= ngram.total_probability;
                int j = 0;
                while (true) {
                    uwu_markov_choice choice = ngram.choices[j];
                    size_t cumulative_probability = choice.cumulative_probability;
                    if (random < cumulative_probability) {
                        ngram_index = choice.next_ngram;
                        int result = COPY_STR(buf + i, &choice.next_char, 1);
                        if (result) return -EFAULT;
                        break;
                    }
                    j++;
                }
            }

            op->state.markov.prev_ngram = ngram_index;
            op->state.markov.remaining_chars -= num_chars_to_copy;

            return num_chars_to_copy;
        }

        case UWU_REPEAT_CHARACTER: {
            char* c = &op->state.repeat_character.character;
            for (size_t i = 0; i < len; ++i) {
                if (op->state.repeat_character.remaining_chars == 0) {
                    // Out of characters. Return the number of characters thus written.
                    return i;
                }
                int result = COPY_STR(buf + i, c, 1);
                if (result) return -EFAULT;
                op->state.repeat_character.remaining_chars--;
            }
            return len;
        }

        default:
            return 0;
    }
}

static const char SPACE = ' ';

// Fill the given buffer with UwU
int write_chars(uwu_state* state, char* buf, size_t n) {
    size_t total_written = 0;
    while (total_written < n) {
        if (state->print_space) {
            int result = COPY_STR(buf + total_written, &SPACE, 1);
            if (result) return -EFAULT;
            total_written++;
            state->print_space = false;
            continue;
        }

        size_t chars_written = exec_op(state, buf + total_written, n - total_written);
        if (chars_written < 0) return chars_written;

        if (chars_written == 0) {
            state->current_op++;
            if (state->current_op >= MAX_OPS || state->ops[state->current_op].opcode == UWU_NULL) {
                // regenerate ops
                generate_new_ops(state);
                state->print_space = true;
                state->current_op = 0;
            }
        } else {
            total_written += chars_written;
        }
    }
    return 0;
}

void vprint(int x, int y, const char* text, size_t len) {
    vram_adr(NTADR_A(x, y));
    for (size_t i = 0; i < len; ++i) {
        vram_put(text[i]);
    }
}

void vprints(int x, int y, const char* text) {
    vram_adr(NTADR_A(x, y));
    size_t len = strlen(text);
    for (size_t i = 0; i < len; ++i) {
        vram_put(text[i]);
    }
}

// void vprint_wrap(int x, int y, const char* text, size_t len, size_t max_line_len) {
//     int line = y;
//     size_t remaing = len;
//     while (remaing) {
//         size_t num_chars = remaing > max_line_len ? max_line_len : remaing;
//         vprint(x, line, text, num_chars);
//         text += num_chars;
//         remaing -= num_chars;
//         ++line;
//     }
// }

static char dialog_buf[128] = {0};
// static char dialog_buf2[28] = {0};

static uwu_state state;

void clear_dialog() {
    memset(dialog_buf, 0, sizeof(dialog_buf));
}

void write_uwu_line(int x, int y, int len) {
    write_chars(&state, dialog_buf, len);
    vprint(x, y, dialog_buf, len);
}

enum GameState{
    GAME_STATE_TEXT_SCROLL,
    GAME_STATE_TEXT_WAIT,
} GameState;

int main() {

    unsigned int rng_buf[RAND_SIZE] = {0};
    state.prev_op = UINT16_MAX;
    state.current_op = 0;
    state.rng_buf = rng_buf;
    state.rng_idx = RAND_SIZE;
    generate_new_ops(&state);

    static const char palette[15] = {0x0c, 0x05, 0x15, 0x35};

    // splash
    ppu_off();
    oam_clear();
    pal_bg(palette);

    vprints(7, 5, "~ UWURANDOM  NES ~");
    vprints(10, 7, "by Dunkyl \x81\x81");
    vprints(15, 11, "\x08\x09");
    vprints(15, 12, "\x18\x19");

    ppu_on_all(); 
    oam_clear();

    for (int i = 0; i < 60; ++i) {
        ppu_wait_nmi();
    }
    

    ppu_off(); // screen off
    vram_fill(0, 0x2000);
    pal_bg(palette);

    // cat
    vram_adr(NTADR_A(15, 10));
    vram_put(0x86);
    vram_put(0x87);
    vram_adr(NTADR_A(15, 11));
    vram_put(0x96);
    vram_put(0x97);
    vram_adr(NTADR_A(15, 12));
    vram_put(0xA6);
    vram_put(0xA7);

    // text box
    vram_adr(NTADR_A(2, 22));
    vram_put(0x82);
    for (int i = 0; i < 26; ++i) {
        vram_put(0x83);
    }
    vram_put(0x85);
    for (int i = 0; i < 4; ++i) {
        vram_adr(NTADR_A(2, 23+i));
        vram_put(0x84);
        vram_adr(NTADR_A(29, 23+i));
        vram_put(0x94);
    }
    vram_adr(NTADR_A(2, 27));
    vram_put(0x92);
    for (int i = 0; i < 26; ++i) {
        vram_put(0x93);
    }
    vram_put(0x95);

    ppu_on_all();
    oam_clear();
    pal_spr(palette);

    char time = 0;
    int big_time = 0;

    char game_state = GAME_STATE_TEXT_SCROLL;
    size_t text_scroll = 0;
    #define UWUN 103
    clear_dialog();
    write_chars(&state, dialog_buf, UWUN);

    char dialog_len = strlen(dialog_buf);;

    #define MAX_FAST_TEXT 24
    char fast_text_bufc[MAX_FAST_TEXT] = {0};
    uint8_t fast_text_bufx[MAX_FAST_TEXT] = {0};
    uint8_t fast_text_bufy[MAX_FAST_TEXT] = {0};
    size_t fast_text_index = 0;

    while (true) {
        
        ppu_wait_nmi();
        oam_clear();
        char pad1 = pad_poll(0);

        

        if (game_state == GAME_STATE_TEXT_WAIT) {
            if (time < 30) {
                oam_spr(226, 208,0x80, 0); 
            } else {
                oam_spr(226, 208,0x90, 0); 
            }
            if (pad1 & PAD_A) {
                game_state = GAME_STATE_TEXT_SCROLL;
                text_scroll = 0;
                ppu_off();
                clear_dialog();
                fast_text_index = 0;
                vram_adr(NTADR_A(3, 23));
                vram_write(dialog_buf, 26);
                vram_adr(NTADR_A(3, 24));
                vram_write(dialog_buf, 26);
                vram_adr(NTADR_A(3, 25));
                vram_write(dialog_buf, 26);
                vram_adr(NTADR_A(3, 26));
                vram_write(dialog_buf, 26);
                write_chars(&state, dialog_buf, UWUN);
                dialog_len = strlen(dialog_buf);
                ppu_on_all();
            }
        } else {
            if (text_scroll == dialog_len) {
                game_state = GAME_STATE_TEXT_WAIT;
                text_scroll = 0;
            } else {
                // ppu_off();
                char cursorx = 3 + text_scroll;
                char cursory = 23;
                while (cursorx > 28) {
                    cursorx -= 26;
                    cursory += 1;
                }
                fast_text_bufc[fast_text_index] = dialog_buf[text_scroll];
                fast_text_bufx[fast_text_index] = cursorx;
                fast_text_bufy[fast_text_index] = cursory;
                ++fast_text_index;
                ++text_scroll;
                // reset fast text and place all characters as bg tiles
                if (fast_text_index == MAX_FAST_TEXT) {
                    fast_text_index = 0;
                    ppu_off();
                    for (size_t i = 0; i < MAX_FAST_TEXT; ++i) {
                        vram_adr(NTADR_A(fast_text_bufx[i], fast_text_bufy[i]));
                        vram_put(fast_text_bufc[i]);
                    }
                    ppu_on_all();
                }
            }
        }
        
        for (size_t i = 0; i < fast_text_index; ++i) {
            oam_spr(fast_text_bufx[i]<<3, (fast_text_bufy[i]<<3)-1 + (fast_text_index%3), fast_text_bufc[i], 0); 
        }

        ++time;
        if (time == 60) {
            time = 0;
            ++big_time;
        }

        
        if (game_state == GAME_STATE_TEXT_SCROLL) {
            oam_spr(17<<3, (10<<3)-1, 0x8E, 0);
            oam_spr(18<<3, (10<<3)-1, 0x8F, 0);
            oam_spr(17<<3, (11<<3)-1, 0x9E, 0);
            // oam_spr(16<<3, (11<<3)-1, 0x9F, 0);
            if (time%10 < 5) { // mouth open
                oam_spr(15<<3, (11<<3)-1, 0x98, 0);
                oam_spr(16<<3, (11<<3)-1, 0x99, 0);
                // oam_spr((15<<3)+2, (13<<3)-1, 0xB8, OAM_FLIP_V); //kbd
                // oam_spr((16<<3)+2, (13<<3)-1, 0xB9, OAM_FLIP_V);
            } else {
                // oam_spr((15<<3)+2, (13<<3)+1, 0xB8, OAM_FLIP_V);//kbd
                // oam_spr((16<<3)+2, (13<<3)+1, 0xB9, OAM_FLIP_V);
            }
        } else {
            // oam_spr((15<<3)+2, (13<<3)+4, 0xB8, OAM_FLIP_V);//kbd
            // oam_spr((16<<3)+2, (13<<3)+4, 0xB9, OAM_FLIP_V);
            if (big_time % 6 == 2) { // blink hold
                oam_spr(15<<3, (10<<3)-1, 0x8A, 0);
                oam_spr(16<<3, (10<<3)-1, 0x8B, 0);
                oam_spr(15<<3, (11<<3)-1, 0x9A, 0);
                oam_spr(16<<3, (11<<3)-1, 0x9B, 0);
            } else if (big_time % 6 == 1) {
                if (time > 50) { // blink start
                    oam_spr(15<<3, (10<<3)-1, 0x8C, 0);
                    oam_spr(16<<3, (10<<3)-1, 0x8D, 0);
                    oam_spr(15<<3, (11<<3)-1, 0x9C, 0);
                    oam_spr(16<<3, (11<<3)-1, 0x9D, 0);
                }
            }
        }
    }
}
