// filesystem.h
// Cabecera de funciones del sistema de archivos comprimido de BattleFS
// Declara los prototipos para las operaciones principales sobre archivos comprimidos
// filesystem.h
// Cabecera de funciones del sistema de archivos comprimido BattleFS
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

void filesystem_init();
void filesystem_create(const char *filepath, const char *comp_dir);
void filesystem_create_all_threads(const char *folder_path, const char *comp_dir, int max_threads);
// Nueva función: muestra el archivo descomprimido en consola
void filesystem_read_in_console(const char *filename, const char *comp_dir);
void filesystem_delete(const char *filename);
void filesystem_list();
void filesystem_save(const char *filename);
void filesystem_load(const char *filename);
void filesystem_close();

#endif
