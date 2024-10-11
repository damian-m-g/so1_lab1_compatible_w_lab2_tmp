/**
 * @file main.c
 * @brief Entry point of the system
 */

#include "expose_metrics.h"
#include <stdbool.h>
#include <signal.h>
#include <pthread.h>

#define SLEEP_TIME 1
#define WAITING_TIME_FOR_EXPOSE_METRICS_THREAD_TO_END 500000 // us

static pthread_t tid;

// Handler ante syscalls SIGINT y SIGTERM.
void handle_sigint_and_sigterm(int sig) {
    // Cierre de archivo temporal, usado por update_processes_gauge() en caso de que existe
    if (access(TEMP_PROC_METRICS_FILE, F_OK) == 0)
    {
        remove(TEMP_PROC_METRICS_FILE);
    }
    // Destrucción de mutex y terminación de thread del servidor Prometheus
    destroy_mutex();
    pthread_cancel(tid);
    // pthread_timedjoin_np() no existe en mi ver usada de glibc, por lo que uso un tiempo estandar para esperar por tid
    usleep(WAITING_TIME_FOR_EXPOSE_METRICS_THREAD_TO_END);
    exit(EXIT_SUCCESS);
}

int main(int argc, char* argv[])
{
    // Creamos un hilo para exponer las métricas vía HTTP
    if (pthread_create(&tid, NULL, expose_metrics, NULL) != 0)
    {
        fprintf(stderr, "Error al crear el hilo del servidor HTTP\n");
        return EXIT_FAILURE;
    }

    init_metrics();

    // Registro de signals handler para salida limpia
    signal(SIGINT, handle_sigint_and_sigterm);
    signal(SIGTERM, handle_sigint_and_sigterm);

    // Bucle principal para actualizar las métricas cada segundo
    while (true)
    {
        update_cpu_gauge();
        update_memory_gauges();
        update_disk_gauges();
        update_network_gauges();
        update_processes_gauge();
        sleep(SLEEP_TIME);
    }

    return EXIT_SUCCESS;
}
