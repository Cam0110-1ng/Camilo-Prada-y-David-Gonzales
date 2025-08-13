#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "filesystem.h"

#define MAX_CMD 512
#define MAX_DIR 512

char base_dir[MAX_DIR] = "";
char comp_dir[MAX_DIR] = "comprimidos";

void print_help() {
    printf("Comandos:\n");
    printf("init                   - Inicializa el sistema de archivos limpio\n");
    printf("create <archivo>       - Comprime y guarda un archivo en el sistema\n");
    printf("create_all             - Comprime todos los archivos del directorio base usando 16 hilos\n");
    printf("read <archivo>         - Descomprime y muestra el archivo en vivo\n");
    printf("delete <archivo>       - Elimina un archivo del sistema\n");
    printf("list                   - Muestra los nombres de archivos ordenados alfabéticamente\n");
    printf("save <nombre>          - Guarda el sistema en un archivo binario\n");
    printf("load <nombre>          - Carga un sistema desde archivo binario\n");
    printf("exit                   - Cierra el programa\n");
}

void ensure_directories() {
#ifdef _WIN32
    mkdir(comp_dir);
#else
    mkdir(comp_dir, 0777);
#endif
}

void get_full_path(char *dest, const char *dirname, const char *filename) {
    snprintf(dest, MAX_CMD, "%s/%s", dirname, filename);
}

int main() {
    char cmd[MAX_CMD], path[MAX_CMD];

    ensure_directories();
    filesystem_init();

    printf("Ingrese el directorio base de los archivos (ejemplo: C:\\Users\\user\\Desktop\\archivos):\n> ");
    if (fgets(base_dir, sizeof(base_dir), stdin)) {
        size_t len = strlen(base_dir);
        if (len > 0 && base_dir[len-1] == '\n') base_dir[len-1] = '\0';
    }

    printf("Directorio base: %s\n", base_dir);
    print_help();

    while (1) {
        printf("battleFS> ");
        if (!fgets(cmd, sizeof(cmd), stdin)) break;

        char *op = strtok(cmd, " \n");
        clock_t start = clock();
        if (!op) continue;

        if (!strcmp(op, "init")) {
            filesystem_init();
        }
        else if (!strcmp(op, "create")) {
            char *archivo = strtok(NULL, " \n");
            if (archivo) {
                get_full_path(path, base_dir, archivo);
                filesystem_create(path, comp_dir);
            } else printf("Falta nombre de archivo\n");
        }
        else if (!strcmp(op, "create_all")) {
            filesystem_create_all_threads(base_dir, comp_dir, 16);
        }
        else if (!strcmp(op, "read")) {
            char *archivo = strtok(NULL, " \n");
            if (archivo) {
                filesystem_read_in_console(archivo, comp_dir);
            } else {
                printf("Falta nombre de archivo\n");
            }
        }
        else if (!strcmp(op, "delete")) {
            char *archivo = strtok(NULL, " \n");
            if (archivo) filesystem_delete(archivo);
            else printf("Falta nombre de archivo\n");
        }
        else if (!strcmp(op, "list")) {
            filesystem_list();
        }
        else if (!strcmp(op, "save")) {
            char *nombre = strtok(NULL, " \n");
            if (nombre) filesystem_save(nombre);
            else printf("Falta nombre de archivo\n");
        }
        else if (!strcmp(op, "load")) {
            char *nombre = strtok(NULL, " \n");
            if (nombre) filesystem_load(nombre);
            else printf("Falta nombre de archivo\n");
        }
        else if (!strcmp(op, "exit")) {
            break;
        }
        else {
            print_help();
        }
        clock_t end = clock();
        double elapsed = (double)(end - start) / CLOCKS_PER_SEC;
        printf("[Tiempo] %.4f segundos\n", elapsed);
    }
    filesystem_close();
    return 0;
}
