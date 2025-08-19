// laboratorio2_bmp_explicado.c
// Programa didáctico para cargar un BMP de 24 bits, convertirlo a grises o aplicar convolución, y guardar el resultado.
// Cada parte está explicada de manera exhaustiva y detallada.
// COMENTADO Y EXPLICADO EN EXTREMO DETALLE SEGÚN SOLICITUD

// --- INCLUSIÓN DE LIBRERÍAS ESTÁNDAR ---
// Incluimos las librerías estándar de C necesarias para el funcionamiento del programa.
#include <stdio.h>      // Para entrada y salida estándar: printf, scanf, fopen, fclose, fread, fwrite
#include <stdlib.h>     // Para manejo de memoria dinámica: malloc, free, exit
#include <stdint.h>     // Para tipos de datos de tamaño fijo: uint8_t, uint16_t, uint32_t
#include <string.h>     // Para funciones de manipulación de cadenas y memoria: memcpy
#include <time.h>       // Para medir tiempo de ejecución: clock, CLOCKS_PER_SEC

// --- DEFINICIÓN DE ESTRUCTURAS PARA BMP ---
// #pragma pack(push, 1) fuerza al compilador a no añadir padding entre los campos de las estructuras.
// Esto es esencial porque el formato BMP requiere que los bytes estén alineados exactamente como están definidos.

// Estructura del encabezado de archivo BMP (14 bytes)
#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;      // Identificador de tipo de archivo. Debe ser 'BM' (0x4D42 en hexadecimal).
    uint32_t bfSize;      // Tamaño total del archivo en bytes, incluyendo todos los encabezados y los datos de píxeles.
    uint16_t bfReserved1; // Campo reservado. Debe ser 0 según la especificación BMP.
    uint16_t bfReserved2; // Segundo campo reservado. También debe ser 0.
    uint32_t bfOffBits;   // Offset (desplazamiento) en bytes desde el inicio del archivo hasta el comienzo de los datos de la imagen.
} BITMAPFILEHEADER;

// Estructura del encabezado de información BMP (BITMAPINFOHEADER, 40 bytes)
typedef struct {
    uint32_t biSize;          // Tamaño de esta cabecera de información. Debe ser 40 para BITMAPINFOHEADER.
    int32_t  biWidth;         // Ancho de la imagen en píxeles.
    int32_t  biHeight;        // Alto de la imagen en píxeles. Si es positivo, la imagen se almacena de abajo hacia arriba (bottom-up).
    uint16_t biPlanes;        // Número de planos. Siempre debe ser 1.
    uint16_t biBitCount;      // Número de bits por píxel. Debe ser 24 para imágenes de 24 bits (este programa solo soporta 24 bpp).
    uint32_t biCompression;   // Tipo de compresión. 0 (BI_RGB) significa sin compresión.
    uint32_t biSizeImage;     // Tamaño de los datos de la imagen en bytes. Puede ser 0 si biCompression es 0.
    int32_t  biXPelsPerMeter; // Resolución horizontal en píxeles por metro. Opcional, puede ser 0.
    int32_t  biYPelsPerMeter; // Resolución vertical en píxeles por metro. Opcional, puede ser 0.
    uint32_t biClrUsed;       // Número de colores usados en la paleta. 0 indica todos.
    uint32_t biClrImportant;  // Número de colores importantes. 0 indica todos.
} BITMAPINFOHEADER;
#pragma pack(pop) // Se vuelve a la alineación por defecto de la plataforma

// Estructura para representar un píxel en formato BMP 24 bits (BGR: Blue, Green, Red)
typedef struct {
    uint8_t b; // Canal azul (Blue), ocupa 1 byte
    uint8_t g; // Canal verde (Green), ocupa 1 byte
    uint8_t r; // Canal rojo (Red), ocupa 1 byte
} BGR;

// FUNCIÓN clampi
// Función que limita un valor entero al rango [0, 255]. Es fundamental para evitar desbordamientos y valores inválidos en los canales de color.
// Parámetro: v -> valor entero a limitar.
// Retorno: valor limitado entre 0 y 255.
uint8_t clampi(int v) {
    if (v < 0) {
        return 0; // Si es menor que 0, devuelve 0.
    }
    if (v > 255) {
        return 255; // Si es mayor que 255, devuelve 255.
    }
    return (uint8_t)v; // Si está en el rango [0,255], lo devuelve tal cual, convertido a uint8_t.
}

// FUNCIÓN row_padding_24
// Calcula cuántos bytes de relleno (padding) necesita cada fila de píxeles para que su tamaño sea múltiplo de 4 bytes, según la especificación BMP.
// Parámetro: width -> ancho de la imagen en píxeles.
// Retorno: número de bytes de padding por fila.
int row_padding_24(int width) {
    int row_bytes = width * 3; // Cada píxel ocupa 3 bytes (B, G, R).
    int resto = row_bytes % 4; // Calcula el resto al dividir row_bytes entre 4.
    if (resto == 0) {
        return 0; // Si ya es múltiplo de 4, no necesita padding.
    } else {
        return 4 - resto; // Si no, necesita 4 - resto bytes de padding.
    }
}

// FUNCIÓN cargar_bmp24
// Carga en memoria una imagen BMP de 24 bits, validando su formato y leyendo los datos de los píxeles.
// Parámetros:
//   path      -> ruta del archivo BMP a leer.
//   out_w     -> puntero donde se almacenará el ancho de la imagen.
//   out_h     -> puntero donde se almacenará el alto de la imagen.
//   out_pixels-> puntero donde se almacenará el buffer de píxeles leídos (memoria dinámica).
// Retorno: 1 si tuvo éxito, 0 si hubo error.
int load_bmp24(const char* path, int* out_w, int* out_h, BGR** out_pixels) {
    FILE* f = fopen(path, "rb"); // Abre el archivo para lectura en modo binario.
    if (f == NULL) {
        printf("No se pudo abrir el archivo: %s\n", path);
        return 0; // Error al abrir.
    }

    BITMAPFILEHEADER file_header; // Variable para almacenar el encabezado de archivo BMP.
    BITMAPINFOHEADER info_header; // Variable para almacenar el encabezado de información BMP.

    // Lee el encabezado de archivo. Debe leerse toda la estructura.
    if (fread(&file_header, sizeof(file_header), 1, f) != 1) {
        printf("No se pudo leer la cabecera del archivo.\n");
        fclose(f);
        return 0;
    }
    // Lee el encabezado de información.
    if (fread(&info_header, sizeof(info_header), 1, f) != 1) {
        printf("No se pudo leer la cabecera de información.\n");
        fclose(f);
        return 0;
    }

    // Validación: verifica que el archivo es un BMP válido de 24 bits sin compresión.
    if (file_header.bfType != 0x4D42) { // 'BM' en hexadecimal es 0x4D42.
        printf("El archivo no es un BMP válido (no empieza con 'BM').\n");
        fclose(f);
        return 0;
    }
   
    // Verifica que sea un bmp de maximo 24 bits
    if (info_header.biBitCount != 24 || info_header.biCompression != 0) {
        printf("Solo se soportan BMP de 24 bits sin compresión.\n");
        fclose(f);
        return 0;
    }

    // Obtiene el ancho y el alto de la imagen.
    int width = info_header.biWidth; // Ancho en píxeles.
    int height = info_header.biHeight > 0 ? info_header.biHeight : -info_header.biHeight; // El alto puede ser negativo.
    int is_bottom_up = (info_header.biHeight > 0) ? 1 : 0; // Si es positivo, la imagen se almacena de abajo hacia arriba.

    // Reserva memoria dinámica para almacenar todos los píxeles de la imagen.
    BGR* pixels = (BGR*)malloc(width * height * sizeof(BGR));
    if (pixels == NULL) {
        printf("No hay suficiente memoria para cargar la imagen.\n");
        fclose(f);
        return 0;
    }

    // Posiciona el puntero del archivo al inicio de los datos de la imagen según el offset bfOffBits.
    fseek(f, file_header.bfOffBits, SEEK_SET);

    // Calcula el padding necesario para cada fila.
    int pad = row_padding_24(width);

    // Bucle para leer cada fila de la imagen.
    // Si es bottom-up, la primera fila leída es la última en memoria.
    for (int y = 0; y < height; y++) {
        int fila_destino = is_bottom_up ? (height - 1 - y) : y; // Calcula a qué fila de memoria corresponde la fila leída.
        BGR* fila = pixels + fila_destino * width;              // Apunta al inicio de la fila en el buffer de píxeles.
        if (fread(fila, sizeof(BGR), width, f) != (size_t)width) {
            printf("Error leyendo los datos de la imagen.\n");
            free(pixels);
            fclose(f);
            return 0;
        }
        fseek(f, pad, SEEK_CUR); // Salta el padding al final de la fila.
    }

    fclose(f); // Cierra el archivo.
    *out_w = width;         // Devuelve el ancho.
    *out_h = height;        // Devuelve el alto.
    *out_pixels = pixels;   // Devuelve el buffer de píxeles.
    return 1; // Éxito.
}
// FIN FUNCIÓN row_padding_24


// FUNCIÓN save_bmp24
// Guarda una imagen en memoria en formato BMP de 24 bits en disco.
// Parámetros:
//   path   -> ruta o nombre del archivo BMP de salida.
//   width  -> ancho de la imagen en píxeles.
//   height -> alto de la imagen en píxeles.
//   pixels -> buffer con los píxeles a guardar.
// Retorno: 1 si tuvo éxito, 0 si hubo error.
int save_bmp24(const char* path, int width, int height, const BGR* pixels) {
    FILE* f = fopen(path, "wb"); // Abre el archivo para escritura en modo binario.
    if (f == NULL) {
        printf("No se pudo crear el archivo de salida: %s\n", path);
        return 0;
    }

    int pad = row_padding_24(width); // Calcula el padding por fila.
    uint32_t row_bytes = width * 3 + pad; // Número de bytes por fila (píxeles + padding).
    uint32_t img_bytes = row_bytes * height; // Tamaño total de los datos de imagen.

    // Prepara los encabezados BMP para el archivo de salida.
    BITMAPFILEHEADER file_header;
    BITMAPINFOHEADER info_header;

    // Completa los campos del encabezado de archivo BMP.
    file_header.bfType = 0x4D42; // 'BM'
    file_header.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + img_bytes; // Tamaño total del archivo.
    file_header.bfReserved1 = 0; // Reservado, siempre 0.
    file_header.bfReserved2 = 0; // Reservado, siempre 0.
    file_header.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); // Offset donde empiezan los datos de la imagen.

    // Completa los campos del encabezado de información BMP.
    info_header.biSize = sizeof(BITMAPINFOHEADER); // Debe ser 40.
    info_header.biWidth = width;                   // Ancho.
    info_header.biHeight = -height;                // Negativo para top-down (la primera fila en memoria es la primera en archivo).
    info_header.biPlanes = 1;                      // Siempre 1.
    info_header.biBitCount = 24;                   // 24 bits por píxel.
    info_header.biCompression = 0;                 // Sin compresión.
    info_header.biSizeImage = img_bytes;           // Tamaño de los datos de imagen.
    info_header.biXPelsPerMeter = 0;               // Resolución horizontal (opcional).
    info_header.biYPelsPerMeter = 0;               // Resolución vertical (opcional).
    info_header.biClrUsed = 0;                     // N° de colores usados, 0 para todos.
    info_header.biClrImportant = 0;                // N° de colores importantes, 0 para todos.

    // Escribe los encabezados en el archivo de salida.
    fwrite(&file_header, sizeof(file_header), 1, f);
    fwrite(&info_header, sizeof(info_header), 1, f);

    // Escribe cada fila de píxeles, añadiendo el padding al final de cada una.
    uint8_t zeroes[3] = {0,0,0}; // Buffer de 3 bytes en cero para el padding (máximo posible).
    for (int y = 0; y < height; y++) {
        const BGR* fila = pixels + y * width;        // Apunta al inicio de la fila en el buffer.
        fwrite(fila, sizeof(BGR), width, f);         // Escribe los píxeles.
        fwrite(zeroes, 1, pad, f);                   // Escribe el padding necesario.
    }

    fclose(f); // Cierra el archivo de salida.
    return 1;  // Éxito.
}

// --- FUNCIÓN to_grayscale ---
// Convierte una imagen de color a escala de grises, modificando el buffer recibido.
// El valor de gris se calcula usando la luminancia perceptual del ojo humano.
// Parámetros:
//   pixels -> buffer de píxeles (modificado en sitio).
//   width  -> ancho de la imagen en píxeles.
//   height -> alto de la imagen en píxeles.
void to_grayscale(BGR* pixels, int width, int height) {
    for (int i = 0; i < width * height; i++) {
        // Fórmula de luminancia ponderada: 0.299*R + 0.587*G + 0.114*B, aproximada con enteros.
        int gray = (int)(pixels[i].r * 299 + pixels[i].g * 587 + pixels[i].b * 114) / 1000;
        uint8_t g = clampi(gray); // Limita el resultado a [0,255].
        pixels[i].r = g; // Asigna el gris al canal rojo.
        pixels[i].g = g; // Asigna el gris al canal verde.
        pixels[i].b = g; // Asigna el gris al canal azul.
    }
}

// --- FUNCIÓN convolve3x3_gray ---
// Aplica una convolución 3x3 sobre una imagen en escala de grises.
// Parámetros:
//   src     -> buffer de entrada (debe estar en escala de grises).
//   dst     -> buffer de salida (donde se guardará el resultado).
//   width   -> ancho de la imagen en píxeles.
//   height  -> alto de la imagen en píxeles.
//   k       -> kernel 3x3 de enteros (matriz de convolución).
//   divisor -> divisor para normalizar el resultado (suma de los valores del kernel, si es 0 se usa 1).
//   offset  -> valor a sumar al resultado final (bias).
void convolve3x3_gray(const BGR* src, BGR* dst, int width, int height, const int k[3][3], int divisor, int offset) {
    if (divisor == 0) {
        divisor = 1; // Para evitar división por cero.
    }

    // Recorre cada píxel de la imagen de salida.
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int suma = 0; // Acumulador para el valor convolucionado del píxel actual.
            // Recorre la vecindad 3x3 centrada en (x, y).
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {
                    int px = x + kx; // Coordenada x del vecino.
                    int py = y + ky; // Coordenada y del vecino.
                    // Verifica si el vecino está dentro de los límites de la imagen.
                    if (px >= 0 && px < width && py >= 0 && py < height) {
                        int kernel_val = k[ky+1][kx+1]; // Valor correspondiente del kernel.
                        const BGR* p = &src[py * width + px]; // Puntero al píxel vecino.
                        suma += p->r * kernel_val; // Multiplica el valor de gris (r) por el kernel y suma.
                    }
                }
            }
            suma = suma / divisor + offset; // Normaliza y suma el offset.
            uint8_t g = clampi(suma); // Limita a [0,255].
            dst[y * width + x].r = g; // Asigna el resultado al canal rojo.
            dst[y * width + x].g = g; // Asigna el resultado al canal verde.
            dst[y * width + x].b = g; // Asigna el resultado al canal azul.
        }
    }
}

// FUNCIÓN read_kernel 
// Solicita al usuario (por consola) que ingrese los 9 valores del kernel 3x3 y el offset.
// Parámetros:
//   k       -> matriz 3x3 donde se almacenará el kernel ingresado.
//   divisor -> puntero donde se almacenará la suma de los valores del kernel (usada como divisor).
//   offset  -> puntero donde se almacenará el offset/bias ingresado por el usuario.
void read_kernel(int k[3][3], int* divisor, int* offset) {
    printf("Ingrese los 9 enteros del kernel 3x3, fila por fila (separados por espacio):\n");
    *divisor = 0; // Inicializa el divisor (suma del kernel).
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (scanf("%d", &k[i][j]) != 1) { // Lee cada valor del kernel.
                printf("Entrada inválida.\n");
                exit(1); // Sale si hay error de entrada.
            }
            *divisor += k[i][j]; // Acumula el valor en el divisor.
        }
    }
    if (*divisor == 0) *divisor = 1; // Si la suma es 0, usa 1 para evitar división por cero.
    printf("Ingrese offset/bias (por lo general 0, o 128 para bordes): ");
    if (scanf("%d", offset) != 1) { // Lee el offset.
        printf("Entrada inválida.\n");
        exit(1); // Sale si hay error de entrada.
    }
}

// --- FUNCIÓN print_menu ---
// Imprime el menú principal del programa en consola.
void print_menu(void) {
    printf("\n--- Menu ---\n");
    printf("1) Convertir a escala de grises y guardar (output_gray.bmp)\n");
    printf("2) Convolucion 3x3 (pide kernel) y guardar (output_conv.bmp)\n");
    printf("0) Salir\n");
    printf("Opcion: ");
}

// --- FUNCIÓN PRINCIPAL main ---
// Controla el flujo principal del programa, mostrando el menú, solicitando opciones y gestionando el procesamiento de imágenes.
int main() {
    char filename[512]; // Buffer para almacenar la ruta/nombre del archivo BMP a cargar.
    int w = 0, h = 0;   // Variables para almacenar el ancho y el alto de la imagen cargada.
    BGR* img = NULL;    // Puntero al buffer de imagen cargada en memoria.
    int opcion;         // Variable para almacenar la opción seleccionada del menú.

    // Bucle principal del programa (se repite hasta que el usuario elija salir).
    do {
        print_menu(); // Muestra el menú principal.
        if (scanf("%d", &opcion) != 1) break; // Lee la opción del usuario.

        // Si el usuario selecciona una opción de procesamiento (1 o 2), solicita el archivo BMP.
        if (opcion == 1 || opcion == 2) {
            printf("Ingrese la ruta o nombre del archivo BMP (ejemplo: C:\\\\imagenes\\\\foto.bmp): ");
            scanf("%511s", filename); // Lee la ruta del archivo BMP.

            if (img) { // Si ya hay una imagen cargada, libera la memoria antes de cargar una nueva.
                free(img);
                img = NULL;
            }

            if (!load_bmp24(filename, &w, &h, &img)) { // Carga la imagen BMP de 24 bits.
                printf("Fallo al cargar el BMP.\n");
                continue; // Si falla, vuelve a mostrar el menú.
            }
            printf("Imagen cargada: %dx%d\n", w, h); // Muestra dimensiones de la imagen cargada.
        }

        // Opción 1: Convertir la imagen a escala de grises.
        if (opcion == 1) {
            clock_t inicio = clock(); // Marca el tiempo de inicio.
            BGR* temp = (BGR*)malloc(w * h * sizeof(BGR)); // Reserva buffer temporal para procesar la imagen.
            if (!temp) { printf("Sin memoria.\n"); break; }
            memcpy(temp, img, w * h * sizeof(BGR)); // Copia la imagen original al buffer temporal.
            to_grayscale(temp, w, h); // Convierte la imagen a escala de grises.
            int ok = save_bmp24("output_gray.bmp", w, h, temp); // Guarda la imagen resultante.
            clock_t fin = clock(); // Marca el tiempo de finalización.
            double segundos = (double)(fin - inicio) / CLOCKS_PER_SEC; // Calcula duración.
            if (!ok) {
                printf("No se pudo guardar output_gray.bmp\n");
            } else {
                printf("Guardado output_gray.bmp\n");
            }
            printf("Tiempo de ejecución: %.4f segundos\n", segundos); // Muestra el tiempo de procesamiento.
            free(temp); // Libera el buffer temporal.
        }
        // Opción 2: Aplicar convolución 3x3 a la imagen (previamente convertida a grises).
        else if (opcion == 2) {
            int k[3][3], divisor, offset; // Kernel 3x3, divisor y offset para la convolución.
            read_kernel(k, &divisor, &offset); // Solicita el kernel y el offset al usuario.
            clock_t inicio = clock(); // Tiempo de inicio.
            BGR* gray = (BGR*)malloc(w * h * sizeof(BGR)); // Buffer para imagen en grises.
            BGR* out  = (BGR*)malloc(w * h * sizeof(BGR)); // Buffer para imagen resultante.
            if (!gray || !out) { printf("Sin memoria.\n"); free(gray); free(out); break; }
            memcpy(gray, img, w * h * sizeof(BGR)); // Copia la imagen original a gray.
            to_grayscale(gray, w, h); // Convierte la imagen a grises.
            convolve3x3_gray(gray, out, w, h, k, divisor, offset); // Aplica la convolución.
            int ok = save_bmp24("output_conv.bmp", w, h, out); // Guarda la imagen procesada.
            clock_t fin = clock(); // Tiempo de fin.
            double segundos = (double)(fin - inicio) / CLOCKS_PER_SEC; // Duración.
            if (!ok) {
                printf("No se pudo guardar output_conv.bmp\n");
            } else {
                printf("Guardado output_conv.bmp\n");
            }
            printf("Tiempo de ejecución: %.4f segundos\n", segundos);
            free(gray); // Libera el buffer de grises.
            free(out);  // Libera el buffer de salida.
        }
        // Opción 0: Salir del programa.
        else if (opcion == 0) {
            printf("Saliendo.\n");
        }
        // Otras opciones: no válidas.
        else {
            printf("Opción no válida.\n");
        }
    } while (opcion != 0); // El bucle termina si la opción es 0 (salir).

    if (img) free(img); // Libera la memoria de la imagen cargada si queda alguna.
    return 0; // Fin del programa.
}

/* --- Kernels de ejemplo para pruebas (copiar/pegar en consola) ---

Sobel X (divisor 1):
-1 0 1
-2 0 2
-1 0 1
0

Sobel Y (divisor 1):
-1 -2 -1
 0  0  0
 1  2  1
0

Laplaciano (8-neighbors, divisor 1):
-1 -1 -1
-1  8 -1
-1 -1 -1
0

Blur 3x3 (divisor 9):
1 1 1
1 1 1
1 1 1
0

Sharpen:
 0 -1  0
-1  5 -1
 0 -1  0
0

*/
