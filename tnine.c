#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* =========================================
 *                 Constants
 * ========================================= */

/**
 *  @brief defines the maximum number of characters in one line
 *  @note width is assumed WITHOUT new line character...
 */
#define MAX_LINE_WIDTH 100
/** @brief alias for the maximum line width + termination character */
#define MAX_STR_LEN MAX_LINE_WIDTH + 1
/** @brief the maximum number of phone items supported */
#define MAX_PHONE_ITEMS 256
#define MAX_PHONE_ITEMS_FILE_SIZE (MAX_LINE_WIDTH + sizeof('\0')) * MAX_PHONE_ITEMS

#define todo() \
    printf("Not yet implemented [%s, %d]", __func__, __LINE__); \
    exit(EXIT_FAILURE);

#define _stringify_dispatch(x) #x
#define stringify(x) _stringify_dispatch(x)

// todo:
#define OUT

/* =========================================
 *                   Error
 * ========================================= */

/** @brief error alias (should be of the same type as the main's return type) */
typedef int Error;

#define ERROR_NONE (Error)0
#define ERROR_INVALID_NUMBER_OF_ARGS (Error)-1
#define ERORR_INVALID_NUMBER_ARG (Error)-2
#define ERROR_FILE_OPEN (Error)-3
#define ERROR_FILE_TOO_LARGE (Error)-4
#define ERROR_FILE_SIZE_MISMATCH (Error)-5
#define ERROR_SCANNING_LINE (Error)-6
#define ERROR_LINE_TOO_LARGE (Error)-7

#define register_error(error, error_msg) \
    case error: \
        printf("[%s]: %s", stringify(error), error_msg); \
        return

static inline void on_exit_error(Error error) {
    printf("\x1b[31m[Error]:\x1b[0m ");
    switch (error) {
        register_error(ERROR_INVALID_NUMBER_OF_ARGS, "The number of arguments passed to the tnine.exe is either too small or too large\nThe possible arguments are: [optional]-s [optional]#number_to_be_searched_for\n");
        register_error(ERORR_INVALID_NUMBER_ARG, "The argument [optional]#number_to_be_searched_for is not in valid number format!\n");
        register_error(ERROR_FILE_SIZE_MISMATCH, "The number of lines expected and the number of given lines is invalid!\n");
    }
}

/** @note it is fine to 'exit' the application intermittently if fatal error occurs, the program does not heap alloc at all */
#define do_exit(error) \
    do { \
        printf("[Error]: %d\n", error); \
        exit(error); \
    } while(0);
// todo: error messages
#define or_exit(error) \
    do { \
        Error __err = (Error)(error); \
        if (__err != ERROR_NONE) { \
            do_exit(__err); \
        } \
    } while(0)

/* =========================================
 *                  Strings
 * ========================================= */

typedef struct {
    size_t size;
    char string[MAX_STR_LEN];
} Sized_String;

typedef char* String_View;

static inline int char_is_number(char c) {
    return c >= '0' && c <= '9';
}

static inline int string_is_number(char* str) {
    for (size_t i = 0; str[i] != '\0' && i < MAX_STR_LEN; i++) {
        if (!char_is_number(str[i])) {
            return 1; // note: to match the strcmp and other string related functions returning 0 on success, we will follow
        }
    }
    return 0;
}

#define is_lower(c) (c) >= 'a' && (c) <= 'z'

static inline char to_lower(char c) {
    if (is_lower(c)) {
        return c;
    }
    return c + 32;
}

/* =========================================
 *                   File
 * ========================================= */

typedef struct _File_Content {
    size_t actual_size;
    char content[MAX_PHONE_ITEMS_FILE_SIZE];
} File_Content;

/* =========================================
 *                 SysArgs
 * ========================================= */

typedef int Optional_Sys_Args_Footprint;

#define bit(x) (1 << (x))
#define OPTIONAL_SYS_ARG_FOOTPRINT_SEARCH bit(1)
#define OPTIONAL_SYS_ARG_FOOTPRINT_NUMBER bit(2)
typedef struct _Sys_Args {
    /** @brief we will store the optional arguments here, then later in the program we may determine whether or not to use the associated parameter inside the algorithm */
    Optional_Sys_Args_Footprint optionals;
    /**
      * @note assuming the line width inside the text file can be 100, then the keyboard input should match the string
      * @brief this is the number that can be submitted by the user
      */
    Sized_String keyboard_input;
} Sys_Args;

static Sys_Args validate_sys_args(int argc, char** argv) {
    // todo: is there a possibility that the first argument wont be defined ??
    or_exit(argc < 4 ? ERROR_NONE : ERROR_INVALID_NUMBER_OF_ARGS);
    Sys_Args args = {0};
    if (argc > 1) {
        int current_arg = 1; // skip the first argument
        /* check for optional parameter (-s) */
        if (strcmp(argv[current_arg], "-s") == 0) {
            args.optionals |= OPTIONAL_SYS_ARG_FOOTPRINT_SEARCH;
            current_arg++;
            if (argc == current_arg) {
                return;
            }
        }
        /* check for optional parameter (#number) */
        if (string_is_number(argv[current_arg]) == 0) {
            args.optionals |= OPTIONAL_SYS_ARG_FOOTPRINT_NUMBER;
            // determine the length of the string
            args.keyboard_input.size = strlen(argv[current_arg]);
            memcpy(&args.keyboard_input.string[0], argv[current_arg], args.keyboard_input.size);
        } else {
            do_exit(ERORR_INVALID_NUMBER_ARG);
        }
    }
    return args;
}

/* =========================================
 *                   Phone
 * ========================================= */

typedef struct _Phone_Item {
    char name[MAX_STR_LEN];
    char number[MAX_STR_LEN];
} Phone_Item;

typedef struct _Phone_Registry {
    size_t num_items;
    struct _Phone_Item items[MAX_PHONE_ITEMS];
} Phone_Registry;

static inline int _parse_read_line(OUT char* out_buff) {
    char ch = 0;
    int num_chars = 0;
    while ((ch = getchar()) != EOF && ch != '\n' && num_chars < MAX_LINE_WIDTH) {
        out_buff[num_chars++] = ch;
    }
    out_buff[num_chars] = '\0';
    return (ch == EOF && num_chars == 0) ? 1 : 0;
}

static void parse_file_contents(OUT Phone_Registry* out_registry) {
    for (; out_registry->num_items < MAX_PHONE_ITEMS; out_registry->num_items++) { // note: this size cannot exceed since we shall have already checked the file size when reading
        // read name
        char* name = out_registry->items[out_registry->num_items].name;
        if (_parse_read_line(OUT name) == 1) { // this means that we have hit the EOF but we have to make sure that it was just a blank line and not a real name, in that case the file is incomplete
            if (strlen(name) == 0) {
                break;
            }
            do_exit(ERROR_FILE_SIZE_MISMATCH);
        }
        // read number
        char* number = out_registry->items[out_registry->num_items].number;
        if (_parse_read_line(OUT number) == 1) {
            out_registry->num_items++;
            break;
        }
    }
}

// should return 0 on match and 1 on mismatch
// todo: make the type more explicit since there is this inversed logic of 0 being 'true' and 1 being 'false'
typedef int (*Substring_Match_Function)(char, char);

/* @note technically we could merge the 'default_match' and 'is_in_t9_range' but there would be no difference... ?? */
static int check_substring_match(Sized_String needle, String_View haystack, Substring_Match_Function match_function) {
    size_t i = 0;
    for (; haystack[i] != '\0' && i < needle.size; i++) {
        if (match_function(haystack[i], needle.string[i])) {
            return 1;
        }
    }
    return needle.size == i ? 0 : 1;
}

static int default_match(char c1, char c2) {
    return to_lower(c1) == to_lower(c2) ? 0 : 1;
}

static int is_in_t9_range(char ch, char num) {
    ch = to_lower(ch);
    switch (num) {
    case '0':
        return ch == '+' ? 0 : 1;
    case '1':
        return 1;
    case '9':
        return ch >= 'w' && ch <= 'z' ? 0 : 1;
    default:
        char lower = ((int)(num - '0') - 2) * 3 + 'a';
        char upper = ((int)(num - '0') - 1) * 3 + 'a' - 1;
        int ret = ch >= lower && ch <= upper ? 0 : 1;
        return ret;
    }
}

static void match(Sized_String phone, Phone_Registry* registry, OUT Phone_Registry* out_matches) {
    for (size_t i = 0; i < registry->num_items; i++) {
        char* curr_number = registry->items[i].number;
        char phone_placeholder = phone.string[0];
        for (size_t j = 0; curr_number[j] != '\0'; j++) {
            if (curr_number[j] == phone_placeholder && (check_substring_match(phone, &curr_number[j], default_match) == 0)) {
                memcpy(&out_matches->items[out_matches->num_items++], &registry->items[i], sizeof(Phone_Item));
                break;
            }
        }
        char* curr_name = &registry->items[i].name[0];
        for (size_t j = 0; curr_name[j] != '\0'; j++) {
            if ((is_in_t9_range(curr_name[j], phone_placeholder) == 0) && (check_substring_match(phone, &curr_name[j], is_in_t9_range) == 0)) {
                memcpy(&out_matches->items[out_matches->num_items++], &registry->items[i], sizeof(Phone_Item));
                break;
            }
        }
    }
}

static void match_ex(Phone_Registry* registry, OUT Phone_Registry* out_matches) {
    todo();
}

static void registry_match(Sys_Args* args, Phone_Registry* registry, OUT Phone_Registry* out_matches) {
    if ((args->optionals & OPTIONAL_SYS_ARG_FOOTPRINT_NUMBER) == 0) {
        // the number itself is not set, meaning we can return immedeatly
        memcpy(out_matches, registry, sizeof(Phone_Registry));
        return;
    }
    if (args->optionals & OPTIONAL_SYS_ARG_FOOTPRINT_SEARCH > 0) {
        // optional search enabled
        match_ex(registry, OUT out_matches);
        return;
    }
    match(args->keyboard_input, registry, OUT out_matches);
}

static void inline print_match(Phone_Item* item) {
    printf("%s\n%s\n", item->name, item->number);
}

static void inline print_matches(Phone_Registry* restrict registry) {
    for (size_t i = 0; i < registry->num_items; i++) {
        print_match(&registry->items[i]);
    }
}

int main(int argc, char** argv) {
    /* ensure the validity of sysargs */
    Sys_Args args = validate_sys_args(argc, argv);

    /* parse file */
    Phone_Registry registry = {0};
    parse_file_contents(&registry);

    printf("[Debug_Info]: List of parsed registers:\n");
    for (size_t i = 0; i < registry.num_items; i++) {
        printf(
            "\tName: %s\n\tNumber: %s\n", registry.items[i].name, registry.items[i].number);
    }

    /* scan for matches */
    Phone_Registry matches = {0};
    registry_match(&args, &registry, &matches);

    /* print matches */
    print_matches(&matches);

    return ERROR_NONE;
}