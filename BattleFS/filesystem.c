#include "filesystem.h"
#include "compression.h"
#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>

static FileIndex *fi = NULL;

const char* basename(const char* filepath) {
    const char* p1 = strrchr(filepath, '\\');
    const char* p2 = strrchr(filepath, '/');
    if (p1 || p2) return (p1 > p2 ? p1 : p2) + 1;
    return filepath;
}

int is_already_compressed(const char *filename, const char *comp_dir) {
    char comp_path[512];
    snprintf(comp_path, sizeof(comp_path), "%s/%s.lzw", comp_dir, filename);
    struct stat st;
    return stat(comp_path, &st) == 0;
}

void filesystem_init() {
    if (fi) fileindex_free(fi);
    fi = fileindex_create();
    printf("Sistema limpio.\n");
}

void filesystem_create(const char *filepath, const char *comp_dir) {
    const char *file = basename(filepath);
    if (is_already_compressed(file, comp_dir)) {
        printf("Ya existe comprimido: %s.lzw (omitido)\n", file);
        return;
    }
    FILE *f = fopen(filepath, "rb");
    if (!f) { printf("No se pudo abrir %s\n", filepath); return; }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    if (sz <= 0) { fclose(f); printf("Archivo vacío o no legible: %s\n", filepath); return; }

    unsigned char *buf = (unsigned char*)malloc(sz);
    if (!buf) { fclose(f); printf("Sin memoria para %s\n", filepath); return; }
    size_t read = fread(buf, 1, sz, f);
    fclose(f);

    if ((long)read != sz) { free(buf); printf("Error al leer %s\n", filepath); return; }

    unsigned char *comp = NULL;
    int comp_sz = lzw_compress(buf, sz, &comp);

    if (!comp || comp_sz <= 0) { free(buf); printf("Error al comprimir %s\n", filepath); return; }

    char comp_path[512];
    snprintf(comp_path, sizeof(comp_path), "%s/%s.lzw", comp_dir, file);
    FILE *cf = fopen(comp_path, "wb");
    if (!cf) { printf("No se pudo guardar comprimido en %s\n", comp_path); free(buf); free(comp); return; }
    fwrite(comp, 1, comp_sz, cf);
    fclose(cf);

    fileindex_insert(fi, file, sz, comp_sz, comp);

    free(buf);
    free(comp);

    int percent = sz > 0 ? (100 * (sz - comp_sz)) / sz : 0;
    printf("Comprimido y guardado: %s -> %s.lzw (Ahorro: %d%%)\n", file, file, percent);
}

typedef struct {
    char filepath[512];
    char comp_dir[512];
} CompressTask;

void* compress_file_thread(void* arg) {
    CompressTask* task = (CompressTask*)arg;
    filesystem_create(task->filepath, task->comp_dir);
    free(task);
    return NULL;
}

void filesystem_create_all_threads(const char *folder_path, const char *comp_dir, int max_threads) {
    DIR *dir;
    struct dirent *entry;
    pthread_t threads[16];
    int thread_count = 0;
    int total_files = 0;

    dir = opendir(folder_path);
    if (!dir) {
        printf("No se pudo abrir la carpeta: %s\n", folder_path);
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
#ifdef _DIRENT_HAVE_D_TYPE
        if (entry->d_type != DT_REG) continue;
#endif
        size_t len = strlen(entry->d_name);
        if (len > 4 && strcmp(entry->d_name + len - 4, ".lzw") == 0)
            continue;

        char filepath[512];
        snprintf(filepath, sizeof(filepath), "%s/%s", folder_path, entry->d_name);

        CompressTask* task = malloc(sizeof(CompressTask));
        strncpy(task->filepath, filepath, sizeof(task->filepath)-1);
        task->filepath[sizeof(task->filepath)-1] = '\0';
        strncpy(task->comp_dir, comp_dir, sizeof(task->comp_dir)-1);
        task->comp_dir[sizeof(task->comp_dir)-1] = '\0';

        pthread_create(&threads[thread_count], NULL, compress_file_thread, task);
        thread_count++;
        total_files++;

        if (thread_count >= max_threads) {
            for (int i = 0; i < thread_count; i++)
                pthread_join(threads[i], NULL);
            thread_count = 0;
        }
    }
    closedir(dir);

    for (int i = 0; i < thread_count; i++)
        pthread_join(threads[i], NULL);

    printf("Completada la compresión multihilo de %d archivos.\n", total_files);
}

// Nueva función: descomprime y muestra en consola
void filesystem_read_in_console(const char *filename, const char *comp_dir) {
    char comp_path[512];
    snprintf(comp_path, sizeof(comp_path), "%s/%s.lzw", comp_dir, filename);
    FILE *cf = fopen(comp_path, "rb");
    if (!cf) {
        printf("No existe archivo comprimido: %s\n", comp_path);
        return;
    }
    fseek(cf, 0, SEEK_END);
    long comp_sz = ftell(cf);
    fseek(cf, 0, SEEK_SET);

    if (comp_sz <= 0) {
        printf("El archivo comprimido está vacío: %s\n", comp_path);
        fclose(cf);
        return;
    }

    unsigned char *comp_data = malloc(comp_sz);
    if (!comp_data) {
        printf("Sin memoria para el archivo comprimido.\n");
        fclose(cf);
        return;
    }
    size_t read_sz = fread(comp_data, 1, comp_sz, cf);
    fclose(cf);

    if (read_sz != comp_sz) {
        printf("Error al leer datos del archivo comprimido.\n");
        free(comp_data);
        return;
    }

    unsigned char *decomp = NULL;
    int decomp_sz = lzw_decompress(comp_data, comp_sz, &decomp);

    if (decomp_sz <= 0 || !decomp) {
        printf("Error al descomprimir: %s\n", filename);
        free(comp_data);
        if (decomp) free(decomp);
        return;
    }

    printf("\n---- Archivo '%s' descomprimido ----\n", filename);
    // Muestra como texto. Si hay bytes nulos, puede verse raro.
    for (int i = 0; i < decomp_sz; ++i) {
        putchar(decomp[i]);
    }
    printf("\n--------- Fin ---------\n");

    free(comp_data);
    free(decomp);
}

void filesystem_delete(const char *filename) {
    if (!fi || fi->count == 0) {
        printf("No hay archivos en el sistema.\n");
        return;
    }
    int idx = -1;
    for (int i = 0; i < fi->count; i++) {
        if (strcmp(fi->entries[i].name, filename) == 0) {
            idx = i;
            break;
        }
    }
    if (idx == -1) {
        printf("Archivo no encontrado en el sistema: %s\n", filename);
        return;
    }
    FileEntry *e = &fi->entries[idx];
    free(e->data);
    for (int i = idx; i < fi->count - 1; i++)
        fi->entries[i] = fi->entries[i+1];
    fi->count--;

    printf("Archivo eliminado del sistema: %s\n", filename);

    char comp_path[512];
    snprintf(comp_path, sizeof(comp_path), "comprimidos/%s.lzw", filename);
    remove(comp_path);
}

void filesystem_list() {
    if (!fi || fi->count == 0) {
        printf("No hay archivos en el sistema.\n");
        return;
    }
    FileEntry *sorted = malloc(sizeof(FileEntry) * fi->count);
    if (!sorted) {
        printf("Sin memoria para ordenar archivos.\n");
        return;
    }
    memcpy(sorted, fi->entries, sizeof(FileEntry) * fi->count);

    int compare_fileentry(const void *a, const void *b) {
        const FileEntry *fa = (const FileEntry *)a;
        const FileEntry *fb = (const FileEntry *)b;
        return strcmp(fa->name, fb->name);
    }

    qsort(sorted, fi->count, sizeof(FileEntry), compare_fileentry);

    printf("+----------------------+------------+------------+--------+\n");
    printf("| %-20s | %-10s | %-10s | %-6s |\n", "Archivo", "Original", "Comprimido", "Ahorro");
    printf("+----------------------+------------+------------+--------+\n");
    for (int i = 0; i < fi->count; i++) {
        FileEntry *e = &sorted[i];
        int percent = e->size_original > 0 ? (100 * (e->size_original - e->size_compressed)) / e->size_original : 0;
        printf("| %-20s | %10ld | %10d | %5d%% |\n", e->name, e->size_original, e->size_compressed, percent);
    }
    printf("+----------------------+------------+------------+--------+\n");

    free(sorted);
}

void filesystem_load(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("No se pudo abrir %s\n", filename);
        return;
    }

    fseek(f, 0, SEEK_END);
    long total_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (total_size > 1024L * 1024L * 1024L) {
        printf("El archivo binario es demasiado grande para cargar (%.2f MB).\n", total_size / (1024.0 * 1024.0));
        fclose(f);
        return;
    }

    int count = 0;
    if (fread(&count, sizeof(int), 1, f) != 1 || count < 0) {
        printf("Error al leer la cantidad de archivos.\n");
        fclose(f);
        return;
    }

    filesystem_init();

    for (int i = 0; i < count; i++) {
        int namelen = 0;
        if (fread(&namelen, sizeof(int), 1, f) != 1 || namelen <= 0 || namelen > 255) {
            printf("Error al leer el nombre del archivo.\n");
            fclose(f);
            return;
        }
        char name[256];
        if (fread(name, 1, namelen, f) != namelen) {
            printf("Error al leer el nombre del archivo.\n");
            fclose(f);
            return;
        }
        long orig = 0;
        int comp_sz = 0;
        if (fread(&orig, sizeof(long), 1, f) != 1 ||
            fread(&comp_sz, sizeof(int), 1, f) != 1 ||
            comp_sz <= 0 ||
            (long)comp_sz > total_size) {
            printf("Error al leer tamaños del archivo %s.\n", name);
            fclose(f);
            return;
        }
        long pos = ftell(f);
        if (pos + comp_sz > total_size) {
            printf("El archivo binario está corrupto o demasiado grande (%s).\n", name);
            fclose(f);
            return;
        }
        unsigned char *data = (unsigned char*)malloc(comp_sz);
        if (!data) {
            printf("Sin memoria para datos de %s.\n", name);
            fclose(f);
            return;
        }
        if (fread(data, 1, comp_sz, f) != comp_sz) {
            printf("Error al leer datos de %s.\n", name);
            free(data);
            fclose(f);
            return;
        }
        fileindex_insert(fi, name, orig, comp_sz, data);
        free(data);
    }
    fclose(f);
    printf("Sistema cargado de %s\n", filename);
}

void filesystem_save(const char *filename) {
    if (!fi || fi->count == 0) {
        printf("No hay archivos para guardar.\n");
        return;
    }
    FILE *f = fopen(filename, "wb");
    if (!f) { printf("No se pudo abrir %s\n", filename); return; }
    fwrite(&fi->count, sizeof(int), 1, f);
    for (int i = 0; i < fi->count; i++) {
        FileEntry *e = &fi->entries[i];
        int namelen = strlen(e->name) + 1;
        fwrite(&namelen, sizeof(int), 1, f);
        fwrite(e->name, 1, namelen, f);
        fwrite(&e->size_original, sizeof(long), 1, f);
        fwrite(&e->size_compressed, sizeof(int), 1, f);
        fwrite(e->data, 1, e->size_compressed, f);
    }
    fclose(f);
    printf("Sistema guardado en %s\n", filename);
}

void filesystem_close() {
    if (fi) fileindex_free(fi);
    fi = NULL;
}
