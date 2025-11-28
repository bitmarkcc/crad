#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>

#include <base58.h>
#include <util.h>


static const char* base58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const uint8_t map_base58[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
    -1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
    22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
    -1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
    47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
};

uint8_t* decode_base58 (const char* str, int max_ret_len) {
    // Skip leading spaces.
    while (*str && rad_is_space(*str))
        str++;
    // Skip and count leading '1's.
    int zeroes = 0;
    int length = 0;
    while (*str == '1') {
        zeroes++;
        if (zeroes > max_ret_len) return 0;
        str++;
    }
    // Allocate enough space in big-endian base256 representation.
    int size = strlen(str) * 733 /1000 + 1; // log(58) / log(256), rounded up.
    uint8_t* b256 = malloc(size);
    // Process the characters.
    static_assert(sizeof(map_base58) == 256); // guarantee not out of range
    while (*str && !rad_is_space(*str)) {
        // Decode base58 character
        int carry = map_base58[(uint8_t)*str];
        if (carry == -1)  // Invalid b58 character
            return 0;
        int i = 0;
        for (uint8_t* it = b256+size-1; (carry != 0 || i < length) && (it != b256-1); --it, ++i) {
            carry += 58 * (*it);
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

char* encode_base58 (const uint8_t* inp, size_t len) {
    const uint8_t* end = inp+len;
    // Skip & count leading zeroes.
    int zeroes = 0;
    int length = 0;
    while (inp != end && *inp == 0) {
        inp++;
        zeroes++;
    }
    int inp_size = len-zeroes;
    // Allocate enough space in big-endian base58 representation.
    int size = inp_size * 138 / 100 + 1; // log(256) / log(58), rounded up.
    
    uint8_t* b58 = malloc(size);
    memset(b58,0,size);
    // Process the bytes.
    while (inp != end) {
        int carry = *inp;
	int i = 0;
        // Apply "b58 = b58 * 256 + ch".
        for (uint8_t* it = b58+size-1; (carry != 0 || i < length) && (it != b58-1); it--, i++) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
	    
        }
        assert(carry == 0);
	length = i;
	inp++;
    }
    
    // Skip leading zeroes in base58 result.
    uint8_t* it = b58 + (size - length);
    while (it != b58+size && *it == 0) {
        it++;
    }

    char* str = malloc(zeroes+(b58+size-it)+1);
    memset(str,'1',zeroes);
    str += zeroes;
    *str = 'z';
    char* str_it = str+1;
    while (it != b58+size) {
	*str_it++ = base58[*(it++)];
    }
    *str_it = 0;
    free(b58);
    return str;
}
