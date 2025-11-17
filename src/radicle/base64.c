#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

#include <base64.h>
#include <util.h>

static const char* base64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static const uint8_t map_base64[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,62,-1,-1,-1,63,
    52,53,54,55,56,57,58,59, 60,61,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6,  7, 8, 9,10,11,12,13,14,
    15,16,17,18,19,20,21,22, 23,24,25,-1,-1,-1,-1,-1,
    -1,26,27,28,29,30,31,32, 33,34,35,36,37,38,39,40,
    41,42,43,44,45,46,47,48, 49,50,51,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
};

uint8_t* decode_base64 (const char* str, int max_ret_len) {
    // Skip leading spaces.
    while (*str && rad_is_space(*str))
        str++;
    // Skip and count leading 'A's.
    int zeroes = 0;
    int length = 0;
    while (*str == 'A') {
        zeroes++;
        if (zeroes > max_ret_len) return 0;
        str++;
    }
    // Allocate enough space in big-endian base256 representation.
    int size = strlen(str) * 750 /1000 + 1; // log(64) / log(256), rounded up.
    uint8_t* b256 = malloc(size);
    // Process the characters.
    static_assert(sizeof(map_base64) == 256); // guarantee not out of range
    while (*str && !rad_is_space(*str)) {
        // Decode base64 character
        int carry = map_base64[(uint8_t)*str];
        if (carry == -1)  // Invalid b64 character
            return 0;
        int i = 0;
        for (uint8_t* it = b256+size-1; (carry != 0 || i < length) && (it != b256-1); --it, ++i) {
            carry += 64 * (*it);
            *it = carry % 256;
            carry /= 256;
        }
        assert(carry == 0);
        length = i;
        if (length + zeroes > max_ret_len) return 0;
        str++;
    }
    // Skip trailing spaces.
    while (rad_is_space(*str))
        str++;
    if (*str != 0)
        return 0;
    // Skip leading zeroes in b256.
    uint8_t* it = b256 + (size - length);

    // Copy result into output vector.
    //vch.reserve(zeroes + (b256.end() - it));
    //vch.assign(zeroes, 0x00);
    //while (it != b256.end())
    //vch.push_back(*(it++));
    return it;
}
