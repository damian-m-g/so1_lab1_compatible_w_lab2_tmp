/**
 * @file expose_metrics.h
 * @brief Programa para leer el uso de CPU y memoria y exponerlos como métricas de Prometheus.
 */

#include "metrics.h"
#include <errno.h>
#include <libprom/prom.h>
#include <libprom/promhttp.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> // Para sleep

#define BUFFER_SIZE 256
#define N_MEM_METRICS 4
#define N_DISK_METRICS 2
#define N_NET_METRICS 6
#define N_PROC_COUNT 2

/**
 * @brief Actualiza la métrica de uso de CPU.
 */
void update_cpu_gauge();

/**
 * @brief Actualiza las métricas de uso de memoria.
 */
void update_memory_gauges();

/**
 * @brief Actualiza las métrica de uso del disco duro.
 */
void update_disk_gauges();

/**
 * @brief Actualiza las métrica de uso de red.
 */
void update_network_gauges();

/**
 * @brief Actualiza las métrica de uso de procesos.
 */
void update_processes_gauge();

/**
 * @brief Función del hilo para exponer las métricas vía HTTP en el puerto 8000.
 * @param arg Argumento no utilizado.
 * @return NULL
 */
void* expose_metrics(void* arg);

/**
 * @brief Inicializar mutex y métricas.
 */
int init_metrics();

/**
 * @brief Destructor de mutex
 */
void destroy_mutex();
