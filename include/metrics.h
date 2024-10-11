/**
 * @file metrics.h
 * @brief Funciones para obtener el uso de CPU y memoria desde el sistema de archivos /proc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BUFFER_SIZE 256

/**
 * @brief Obtiene datos de la memoria principal desde /proc/meminfo.
 *
 * Lee los valores de memoria total y disponible desde /proc/meminfo, y calcula
 * la memoria siendo utilizada (con su porcentaje asociado).
 *
 * @return Un puntero a array de 4 elementos double, donde cada uno representa respectivamente:
 *   0: memoria total
 *   1: memoria usada
 *   2: memoria disponible
 *   3: porcentaje de memoria disponible (rounded)
 * Devuelve NULL en caso de error.
 */
double* get_memory_usage();

/**
 * @brief Obtiene el porcentaje de uso de CPU desde /proc/stat.
 *
 * Lee los tiempos de CPU desde /proc/stat y calcula el porcentaje de uso de CPU
 * en un intervalo de tiempo.
 *
 * @return Uso de CPU como porcentaje (0.0 a 100.0), o -1.0 en caso de error.
 */
double get_cpu_usage();

/**
 * @brief Obtiene datos de uso del disco duro desde /proc/diskstats.
 *
 * Ubica el disco duro único de la laptop, y obtiene la cantida de lecturas
 * y el tiempo que le llevó realizarlas; tanto lo mismo para las escrituras.
 *
 * @return Un puntero a array de 2 elementos double, 2 promedios:
 *   0: Lecturas por segundo.
 *   1: Escrituras por segundo.
 * Devuelve NULL en caso de error.
 */
double* get_disk_usage();

/**
 * @brief Obtiene datos de uso de networking de /proc/net/dev.
 *
 * La pandemia de Cordyceps ha generado el levantamiento de pandillas hackers
 * que utilizan el medio aereo para realizar sus ataques. Es por esta razón que
 * en nuestra base nos vemos obligados a no utilizar la red wireless!
 * Ubica la NIC Ethernet de la laptop, y obtiene información de TX y RX.
 *
 * @return Un puntero a array de 6 elementos double:
 *   0: Bytes RX.
 *   1: Paquetes con errores RX.
 *   2: Paquetes descartados en RX.
 *   3: Bytes TX.
 *   4: Paquetes con errores TX.
 *   5: Paquetes descartados en TX.
 * Devuelve NULL en caso de error.
 */
double* get_network_usage();
