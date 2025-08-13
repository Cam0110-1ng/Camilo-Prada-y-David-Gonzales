// tree.c
// Implementaci�n de las funciones para manejar el �ndice din�mico de archivos comprimidos en BattleFS.
// Este �ndice permite almacenar informaci�n sobre los archivos comprimidos, acceder a ellos y liberar memoria correctamente.

#include "tree.h"
#include <stdlib.h>
#include <string.h>

/**
 * Crea un nuevo FileIndex (�ndice de archivos) con una capacidad inicial.
 * Este �ndice es un array din�mico de FileEntry, donde cada entrada almacena los datos comprimidos y metadatos de un archivo.
 * La capacidad inicial es 16 archivos, pero se expande autom�ticamente si es necesario.
 *
 * @return Un puntero a la estructura FileIndex creada (reservada con malloc).
 */
FileIndex* fileindex_create() {
    // Reserva memoria para la estructura FileIndex
    FileIndex* fi = (FileIndex*)malloc(sizeof(FileIndex));
    fi->count = 0;          // Inicialmente no hay archivos en el �ndice
    fi->capacity = 16;      // Capacidad inicial del array din�mico
    // Reserva memoria para el array de entradas de archivos (FileEntry)
    fi->entries = (FileEntry*)malloc(sizeof(FileEntry) * fi->capacity);
    return fi;
}

/**
 * Inserta un archivo comprimido en el �ndice din�mico.
 * Si la capacidad actual del array se alcanza, se duplica (realloc) para admitir m�s archivos.
 * Copia el nombre, tama�o original, tama�o comprimido y los datos comprimidos al �ndice.
 *
 * @param fi                Puntero al FileIndex donde se va a insertar el archivo.
 * @param name              Nombre del archivo (sin ruta).
 * @param size_original     Tama�o original del archivo en bytes.
 * @param size_compressed   Tama�o del archivo comprimido en bytes.
 * @param compressed_data   Puntero a los datos comprimidos (buffer).
 */
void fileindex_insert(FileIndex *fi, const char *name, long size_original, int size_compressed, const unsigned char *compressed_data) {
    // Si el array est� lleno, se duplica la capacidad para admitir m�s archivos
    if (fi->count >= fi->capacity) {
        fi->capacity *= 2;
        fi->entries = (FileEntry*)realloc(fi->entries, sizeof(FileEntry) * fi->capacity);
    }
    // Copia el nombre del archivo (protegido para no exceder el tama�o de name)
    strncpy(fi->entries[fi->count].name, name, sizeof(fi->entries[fi->count].name)-1);
    fi->entries[fi->count].name[sizeof(fi->entries[fi->count].name)-1] = '\0'; // Asegura terminaci�n nula
    // Copia los metadatos de tama�o
    fi->entries[fi->count].size_original = size_original;
    fi->entries[fi->count].size_compressed = size_compressed;
    // Reserva memoria para los datos comprimidos y los copia desde el buffer
    fi->entries[fi->count].data = (unsigned char*)malloc(size_compressed);
    memcpy(fi->entries[fi->count].data, compressed_data, size_compressed);
    // Incrementa el contador de archivos en el �ndice
    fi->count++;
}

/**
 * Libera toda la memoria reservada para el �ndice de archivos, incluyendo:
 * - Los buffers de cada archivo comprimido
 * - El array de FileEntry
 * - La estructura FileIndex en s� misma
 *
 * @param fi    Puntero al FileIndex que se desea liberar.
 */
void fileindex_free(FileIndex *fi) {
    if (!fi) return;
    // Libera la memoria reservada para los datos comprimidos de cada archivo
    for (int i = 0; i < fi->count; i++) {
        free(fi->entries[i].data);
    }
    // Libera el array de entradas
    free(fi->entries);
    // Libera la estructura principal
    free(fi);
}
