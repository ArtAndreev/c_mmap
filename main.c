#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>

#define BUFFER_SIZE 8

enum {
    SUCCESS = 0,
    FILE_ERROR,
    MEMORY_ERROR,
    ARGUMENT_ERROR
};

typedef struct Word {
    char* word;
    int frequency;
} Word;

int process_words(int file_desc, size_t file_size);
int read_text(char* mapped_file, size_t file_size, Word** words, size_t* array_len, size_t* words_count);
int read_word(char* begin, char* end, Word** words, size_t* words_count, size_t* array_len);
int add_word(char* word, Word** words, size_t* words_count, size_t* array_len);
int expand(Word** array, size_t* array_len);
int create_word(Word* word, const char* from);
void sort_words(Word** words, size_t words_count);
int cmp_words(const void* left, const void* right);
void print_words(Word* words, size_t words_count);
void free_words(Word* words, size_t words_count);

//
// Program receives one filename with text.
//

int main(int argc, char** argv) {
    if (argc != 2) {
        fprintf(stderr, "No file entered or more than 1 document entered. Try again.\n");
        return FILE_ERROR;
    }

    int file_desc = open(argv[1], O_RDONLY);
    if (file_desc == -1) {
        fprintf(stderr, "No file found or access error. Try again.\n");
        return FILE_ERROR;
    }

    struct stat file_stats;
    stat(argv[1], &file_stats);
    size_t file_size = (size_t)file_stats.st_size;

    return process_words(file_desc, file_size);
}

int process_words(int file_desc, size_t file_size) {
    size_t array_len = BUFFER_SIZE;
    Word* words = malloc(array_len * sizeof(Word));
    if (!words) {
        fprintf(stderr, "Memory allocation error. Try again.\n");
        return MEMORY_ERROR;
    }
    size_t words_count = 0;

    char* mapped_file = mmap(0, file_size, PROT_READ, MAP_PRIVATE, file_desc, 0);
    close(file_desc);
    if (mapped_file == MAP_FAILED) {
        fprintf(stderr, "Memory mapping error. Try again.\n");
        return MEMORY_ERROR;
    }

    int error_code = SUCCESS;
    if ((error_code = read_text(mapped_file, file_size, &words, &array_len, &words_count)) != SUCCESS) {
        munmap(mapped_file, file_size);
        free_words(words, words_count);
        return error_code;
    }

    munmap(mapped_file, file_size);

    sort_words(&words, words_count);

    print_words(words, words_count);

    free_words(words, words_count);

    return SUCCESS;
}

int read_text(char* mapped_file, size_t file_size, Word** words, size_t* array_len, size_t* words_count) {
    char* begin = mapped_file;
    char* end = mapped_file + 1;
    char symbol = 0;
    printf("%zu", file_size);
    for (size_t i = 0; i < file_size; i++) {
        symbol = mapped_file[i];
        if (isspace(symbol) || ispunct(symbol) || symbol == '\0' || symbol == EOF) {
            begin++;
            end++;
            continue;
        }
        if (isspace(*(end)) || ispunct(*(end)) || *end == '\0' || *end == EOF) {
            if (read_word(begin, end, words, words_count, array_len) != SUCCESS)
                return MEMORY_ERROR;
            begin = end;
        }
        end++;
    }

    return SUCCESS;
}

int read_word(char* begin, char* end, Word** words, size_t* words_count, size_t* array_len) {
    char* temp = malloc(end - begin + 1);
    if (!temp) {
        fprintf(stderr, "Memory allocation error. Try again.\n");
        return MEMORY_ERROR;
    }
    memcpy(temp, begin, end - begin);

    temp[end - begin] = '\0';
    printf("%s\n", temp);

    int error_code = SUCCESS;
    error_code = add_word(temp, words, words_count, array_len);

    free(temp);

    return error_code;
}

int add_word(char* word, Word** words, size_t* words_count, size_t* array_len) {
    int already_exists = 0;
    for (size_t i = 0; i < *words_count; i++) {
        strcmp(word, (*words)[i].word);
        if (strcmp(word, (*words)[i].word) == 0) {
            already_exists = 1;
            (*words)[i].frequency++;
            break;
        }
    }

    if (!already_exists) {
        int error_code = SUCCESS;
        if (*words_count == *array_len) {
            error_code = expand(words, array_len);
        }
        if (error_code != SUCCESS)
            return error_code;

        error_code = create_word(&(*words)[*words_count], word);
        if (error_code != SUCCESS)
            return error_code;
        (*words_count)++;
    }

    return SUCCESS;
}

int expand(Word** array, size_t* array_len) {
    Word* temp = realloc(*array, (*array_len) * sizeof(Word) * 2);
    if (!temp) {
        fprintf(stderr, "Memory reallocation error. Try again.\n");
        return MEMORY_ERROR;
    }
    *array = temp;
    *array_len *= 2;

    return SUCCESS;
}

int create_word(Word* word, const char* from) {
    if (!word) {
        return ARGUMENT_ERROR;
    }
    word->word = malloc(strlen(from) + 1);
    if (!word->word) {
        fprintf(stderr, "Memory allocation error. Try again.\n");
        return MEMORY_ERROR;
    }

    memcpy(word->word, from, strlen(from) + 1);
    word->frequency = 1;

    return SUCCESS;
}

void sort_words(Word** words, size_t words_count) {
    qsort(*words, words_count, sizeof(Word), cmp_words);
}

int cmp_words(const void* left, const void* right) {
    const Word* l_word = left;
    const Word* r_word = right;
    if (l_word->frequency > r_word->frequency) return -1;
    if (l_word->frequency == r_word->frequency) return 0;
    return 1;
}

void print_words(Word* words, size_t words_count) {
    printf("Total: %zu words\n", words_count);
    for (size_t i = 0; i < words_count; i++) {
        printf("\"%s\" - %d time(-s)\n", words[i].word, words[i].frequency);
    }
}

void free_words(Word* words, size_t words_count) {
    for (size_t i = 0; i < words_count; i++)
        free(words[i].word);

    free(words);
}