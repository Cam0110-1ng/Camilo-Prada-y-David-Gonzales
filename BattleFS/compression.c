// compression.c
// Implementación didáctica del algoritmo LZW para compresión y descompresión de archivos binarios en BattleFS.

#include "compression.h"
#include <stdlib.h>
#include <string.h>

#define MAX_DICT_SIZE 4096

typedef struct {
    unsigned short code;
    unsigned char *data;
    int length;
} DictEntry;

int lzw_compress(const unsigned char *input, long input_size, unsigned char **output) {
    if (!input || input_size <= 0) return 0;
    unsigned char *out = (unsigned char*)malloc(input_size * 2);
    if (!out) return 0;
    int out_pos = 0;

    DictEntry dict[MAX_DICT_SIZE];
    int dict_size = 256;
    for (int i = 0; i < 256; i++) {
        dict[i].code = i;
        dict[i].data = (unsigned char*)malloc(1);
        dict[i].data[0] = (unsigned char)i;
        dict[i].length = 1;
    }

    int prefix_len = 1;
    unsigned char prefix[1024];
    prefix[0] = input[0];

    for (long i = 1; i <= input_size; i++) {
        if (i < input_size)
            prefix[prefix_len] = input[i];
        int found = -1;
        for (int j = 0; j < dict_size; j++) {
            if (dict[j].length == prefix_len + (i < input_size ? 1 : 0) &&
                memcmp(dict[j].data, prefix, prefix_len + (i < input_size ? 1 : 0)) == 0) {
                found = j;
                break;
            }
        }
        if (found != -1 && i < input_size) {
            prefix_len++;
        } else {
            int code = -1;
            for (int j = 0; j < dict_size; j++) {
                if (dict[j].length == prefix_len &&
                    memcmp(dict[j].data, prefix, prefix_len) == 0) {
                    code = dict[j].code;
                    break;
                }
            }
            out[out_pos++] = (code >> 8) & 0xFF;
            out[out_pos++] = code & 0xFF;

            if (dict_size < MAX_DICT_SIZE && i < input_size) {
                dict[dict_size].length = prefix_len + 1;
                dict[dict_size].data = (unsigned char*)malloc(prefix_len + 1);
                memcpy(dict[dict_size].data, prefix, prefix_len + 1);
                dict[dict_size].code = dict_size;
                dict_size++;
            }
            prefix[0] = input[i];
            prefix_len = 1;
        }
    }

    for (int i = 0; i < dict_size; i++)
        free(dict[i].data);

    *output = out;
    return out_pos;
}

// --- DESCOMPRESIÓN CORREGIDA USANDO LÓGICA DE TU AMIGO ---
int lzw_decompress(const unsigned char *input, long input_size, unsigned char **output) {
    if (!input || input_size <= 0) return 0;
    unsigned char *out = (unsigned char*)malloc(input_size * 8);
    if (!out) return 0;
    int out_pos = 0;

    // Diccionario de pares (prefix, character)
    typedef struct {
        int prefix;
        unsigned char character;
    } DictEntry;

    DictEntry dict[MAX_DICT_SIZE];
    for (int i = 0; i < 256; i++) {
        dict[i].prefix = -1;
        dict[i].character = (unsigned char)i;
    }
    int dict_size = 256;

    // Leer los códigos (2 bytes por código)
    int num_codes = input_size / 2;
    unsigned short *codes = (unsigned short*)malloc(sizeof(unsigned short) * num_codes);
    for (int i = 0; i < num_codes; i++) {
        codes[i] = (input[2*i] << 8) | input[2*i+1];
    }

    int old = codes[0];
    out[out_pos++] = dict[old].character;

    for (int i = 1; i < num_codes; i++) {
        int code = codes[i];

        // Reconstruir la secuencia correspondiente al código actual
        unsigned char stack[MAX_DICT_SIZE];
        int stack_pos = 0;

        int current = code;
        if (code >= dict_size) {
            // Caso especial: el código aún no existe
            stack[stack_pos++] = dict[old].character;
            current = old;
        }
        while (current != -1) {
            stack[stack_pos++] = dict[current].character;
            current = dict[current].prefix;
        }
        // Escribir la secuencia reconstruida en orden correcto
        for (int j = stack_pos - 1; j >= 0; j--) {
            out[out_pos++] = stack[j];
        }

        // Agregar nueva entrada al diccionario
        if (dict_size < MAX_DICT_SIZE) {
            dict[dict_size].prefix = old;
            dict[dict_size].character = stack[stack_pos - 1];
            dict_size++;
        }

        old = code;
    }
    free(codes);

    *output = out;
    return out_pos;
}
