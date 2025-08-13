// tree.h
// Cabecera para las estructuras y funciones del �ndice de archivos (FileIndex)

#ifndef TREE_H
#define TREE_H

// Estructura que representa un archivo en el �ndice
typedef struct {
    char name[256];               // Nombre del archivo (sin ruta)
    long size_original;           // Tama�o original del archivo en bytes
    int size_compressed;          // Tama�o comprimido en bytes
    unsigned char *data;          // Buffer con los datos comprimidos (reservado por malloc)
} FileEntry;

// Estructura �ndice de archivos, almacenamiento din�mico
typedef struct {
    FileEntry *entries;           // Array din�mico de archivos
    int count;                    // Cantidad actual de archivos
    int capacity;                 // Capacidad m�xima del array (crece autom�ticamente)
} FileIndex;

// Crea el �ndice de archivos con capacidad inicial
FileIndex* fileindex_create();

// Inserta un archivo en el �ndice, reservando memoria y copiando datos
void fileindex_insert(FileIndex *fi, const char *name, long size_original, int size_compressed, const unsigned char *compressed_data);

// Libera toda la memoria asociada al �ndice y los archivos
void fileindex_free(FileIndex *fi);

#endif
