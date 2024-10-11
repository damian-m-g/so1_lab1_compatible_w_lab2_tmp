/**
 * @file metrics.h
 * @brief Funciones para obtener el uso de CPU y memoria desde el sistema de archivos /proc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#define BUFFER_SIZE 256

/**
 * @brief Obtiene datos de la memoria principal desde /proc/meminfo.
 *
 * Lee los valores de memoria total y disponible desde /proc/meminfo, y calcula
 * la memoria siendo utilizada (con su porcentaje asociado).
 *
 * @return Un puntero a array de 4 elementos long long unsigned, donde cada uno
 * representa respectivamente:
 *   0: memoria total
 *   1: memoria usada
 *   2: memoria disponible
 *   3: porcentaje de memoria disponible (rounded)
 * Devuelve NULL en caso de error.
 */
long long unsigned* get_memory_usage();

/**
 * @brief Obtiene el porcentaje de uso de CPU desde /proc/stat.
 *
 * Lee los tiempos de CPU desde /proc/stat y calcula el porcentaje de uso de CPU
 * en un intervalo de tiempo.
 *
 * @return Uso de CPU como porcentaje (0.0 a 100.0), o -1.0 en caso de error.
 */
double get_cpu_usage();
