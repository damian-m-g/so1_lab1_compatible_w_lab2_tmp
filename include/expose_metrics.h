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

//! \brief Used to hold each line read from files.
#define BUFFER_SIZE 256
//! \brief Number of memory metrics.
#define N_MEM_METRICS 4
//! \brief Number of hard disk metrics.
#define N_DISK_METRICS 2
//! \brief Number of network metrics.
#define N_NET_METRICS 6
//! \brief Number of processes metrics.
#define N_PROC_COUNT 2
//! \brief Number of metrics tracked by a general status.
#define G_STATUS_N_METRICS_TRACKED 4

/**
 * @brief Actualiza la métrica de uso de CPU.
 */
void update_cpu_gauge(void);

/**
 * @brief Actualiza las métricas de uso de memoria.
 */
void update_memory_gauges(void);

/**
 * @brief Actualiza las métrica de uso del disco duro.
 */
void update_disk_gauges(void);

/**
 * @brief Actualiza las métrica de uso de red.
 */
void update_network_gauges(void);

/**
 * @brief Actualiza las métrica de uso de procesos.
 */
void update_processes_gauge(void);

/**
 * @brief Función del hilo para exponer las métricas vía HTTP en el puerto 8000.
 * @param arg Argumento no utilizado.
 * @return NULL
 */
void* expose_metrics(void* arg);

/**
 * @brief Inicializar mutex y métricas.
 */
int init_metrics(void);

/**
 * @brief Destructor de mutex
 */
void destroy_mutex(void);
