#ifndef RADICLE_UTIL_H
#define RADICLE_UTIL_H

#include <stdint.h>
#include <stdbool.h>
#include <json-c/json.h>

#include <set.h>

#define RAD_BUFSIZ 256
#define RAD_BUFSIZ2 512

typedef struct {
    size_t n_keys;
    char** keys;
    json_object** values;
} StrJsonMap;

StrJsonMap str_json_map_new(size_t n);

char* rad_strcpy (char* out, const char* inp, int from, int len);

void rad_rstrip_nl (char* str);

char* rad_strip (char c, const char* str);

char* rad_hex_to_uchar (const char* hex);

bool rad_is_space (char c);

bool rad_get_input (char* str, size_t bufsiz);

char* rad_to_lower (const char* str);

char* rad_remove_space_json (const char* str);

void rad_assert_equal (const uint8_t* a, const uint8_t* b, size_t n);

int rad_push_array (size_t* pn, void** arr, size_t m, void* pelement); 

char* time_offset (int offset);

int exec_command (const char* file, char* const argv []);
int exec_command_inp (const char* file, char* const argv [], const char* inp);

char* get_password ();

char* rad_substr (const char* str, int start, int len);

char* rad_indent_str (const char* str);

uint8_t* rad_reverse_bytes (const uint8_t* bytes, size_t len);

char* rad_str_with_line_size (const char* str, size_t n);

char* rad_email_get_user (const char* emailaddr);
char* rad_email_get_domain (const char* emailaddr);

char* rad_str_remove_spaces (const char* str);

int rad_dir_list_recursive (const char* basepath, const char* filepath, SimpleSet* files);
char* rad_basename_dirname (const char* path);
bool rad_line_in_file (const char* line, const char* filename);

#endif
