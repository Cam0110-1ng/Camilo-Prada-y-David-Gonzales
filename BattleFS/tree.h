// tree.h
// Cabecera para las estructuras y funciones del índice de archivos (FileIndex)

#ifndef TREE_H
#define TREE_H

// Estructura que representa un archivo en el índice
typedef struct {
    char name[256];               // Nombre del archivo (sin ruta)
    long size_original;           // Tamaño original del archivo en bytes
    int size_compressed;          // Tamaño comprimido en bytes
    unsigned char *data;          // Buffer con los datos comprimidos (reservado por malloc)
} FileEntry;

// Estructura índice de archivos, almacenamiento dinámico
typedef struct {
    FileEntry *entries;           // Array dinámico de archivos
    int count;                    // Cantidad actual de archivos
    int capacity;                 // Capacidad máxima del array (crece automáticamente)
} FileIndex;

// Crea el índice de archivos con capacidad inicial
FileIndex* fileindex_create();

// Inserta un archivo en el índice, reservando memoria y copiando datos
void fileindex_insert(FileIndex *fi, const char *name, long size_original, int size_compressed, const unsigned char *compressed_data);

// Libera toda la memoria asociada al índice y los archivos
void fileindex_free(FileIndex *fi);

#endif
