#include "expose_metrics.h"

/** Mutex para sincronización de hilos */
pthread_mutex_t lock;

/** Métrica de Prometheus para el uso de CPU */
static prom_gauge_t* cpu_usage_metric;
/** Métrica de Prometheus para el uso de memoria */
static prom_gauge_t* memory_metrics[N_MEM_METRICS];
/** Métrica de Prometheus para el uso de disco */
static prom_gauge_t* disk_metrics[N_DISK_METRICS];
/** Métrica de Prometheus para el uso de disco */
static prom_gauge_t* network_metrics[N_NET_METRICS];
/** Métrica de Prometheus para el conteo de procesos */
static prom_gauge_t* processes_count[N_PROC_COUNT];
/** Configuration data array. Gets value in main.c */
extern unsigned char config[];
/** Estado general del programa (métricas) para reporte via SIGUSR1
 * 0 - cpu_usage_percentage
 * 1 - memory_used_percentage
 * 2 - sectors_read_rate
 * 3 - sectors_written_rate
 */
unsigned char g_status[G_STATUS_N_METRICS_TRACKED] = {0};

void update_cpu_gauge()
{
    double usage = get_cpu_usage();
    if (usage >= 0)
    {
        // Trackeo interno de estado general
        g_status[0] = (unsigned char)usage;
        // Trackeo del propio Prometheus
        pthread_mutex_lock(&lock);
        prom_gauge_set(cpu_usage_metric, usage, NULL);
        pthread_mutex_unlock(&lock);
    }
    else
    {
        fprintf(stderr, "Error al obtener el uso de CPU\n");
    }
}

void update_memory_gauges()
{
    double* usage = get_memory_usage();
    if (usage != NULL)
    {
        // Trackeo interno de estado general
        g_status[1] = (unsigned char)usage[3];
        // Trackeo del propio Prometheus
        for (int i = 0; i < N_MEM_METRICS; i++)
        {
            pthread_mutex_lock(&lock);
            prom_gauge_set(memory_metrics[i], usage[i], NULL);
            pthread_mutex_unlock(&lock);
        }
    }
    else
    {
        fprintf(stderr, "Error al obtener el uso de memoria\n");
    }
}

void update_disk_gauges()
{
    double* usage = get_disk_usage();
    if (usage != NULL)
    {
        // Trackeo interno de estado general; puede haber overflow, asi que se implementan medidas
        g_status[2] = usage[0] >= 255 ? 255 : (unsigned char)usage[0];
        g_status[3] = usage[1] >= 255 ? 255 : (unsigned char)usage[1];
        // Trackeo del propio Prometheus
        for (int i = 0; i < N_DISK_METRICS; i++)
        {
            pthread_mutex_lock(&lock);
            prom_gauge_set(disk_metrics[i], usage[i], NULL);
            pthread_mutex_unlock(&lock);
        }
    }
    else
    {
        fprintf(stderr, "Error al obtener el uso del disco duro\n");
    }
}

void update_network_gauges()
{
    double* usage = get_network_usage();
    if (usage != NULL)
    {
        for (int i = 0; i < N_NET_METRICS; i++)
        {
            pthread_mutex_lock(&lock);
            prom_gauge_set(network_metrics[i], usage[i], NULL);
            pthread_mutex_unlock(&lock);
        }
    }
    else
    {
        fprintf(stderr, "Error al obtener el uso de networking\n");
    }
}

void update_processes_gauge()
{
    double* usage = get_processes_usage();
    if (usage != NULL)
    {
        for (int i = 0; i < N_PROC_COUNT; i++)
        {
            pthread_mutex_lock(&lock);
            prom_gauge_set(processes_count[i], usage[i], NULL);
            pthread_mutex_unlock(&lock);
        }
    }
    else
    {
        fprintf(stderr, "Error al obtener el uso de procesos\n");
    }
}

void* expose_metrics(void* arg)
{
    (void)arg; // Argumento no utilizado

    // Aseguramos que el manejador HTTP esté adjunto al registro por defecto
    promhttp_set_active_collector_registry(NULL);

    // Iniciamos el servidor HTTP en el puerto 8000
    struct MHD_Daemon* daemon = promhttp_start_daemon(MHD_USE_SELECT_INTERNALLY, 8000, NULL, NULL);
    if (daemon == NULL)
    {
        fprintf(stderr, "Error al iniciar el servidor HTTP\n");
        return NULL;
    }

    // Mantenemos el servidor en ejecución
    while (1)
    {
        sleep(1);
    }

    // Nunca debería llegar aquí
    MHD_stop_daemon(daemon);
    return NULL;
}

int init_metrics()
{
    // Inicializamos el mutex
    if (pthread_mutex_init(&lock, NULL) != 0)
    {
        fprintf(stderr, "Error al inicializar el mutex\n");
        return EXIT_FAILURE;
    }

    // Inicializamos el registro de coleccionistas de Prometheus
    if (pcr_default_init() != 0)
    {
        fprintf(stderr, "Error al inicializar el registro de Prometheus\n");
        return EXIT_FAILURE;
    }

    /* CREACIÓN DE MÉTRICAS */

    // Creamos la métrica para el uso de CPU, if required
    if (config[1])
    {
        cpu_usage_metric = prom_gauge_new("cpu_usage_percentage", "Porcentaje de uso de CPU", 0, NULL);
        if (cpu_usage_metric == NULL)
        {
            fprintf(stderr, "Error al crear la métrica de uso de CPU\n");
            return EXIT_FAILURE;
        }
    }

    // Creamos las métricas para el uso de memoria, if required
    if (config[2])
    {
        memory_metrics[0] = prom_gauge_new("memory_total", "Memoria total", 0, NULL);
        memory_metrics[1] = prom_gauge_new("memory_used", "Memoria en uso", 0, NULL);
        memory_metrics[2] = prom_gauge_new("memory_free", "Memoria libre", 0, NULL);
        memory_metrics[3] = prom_gauge_new("memory_used_percentage", "Porcentaje de memoria en uso", 0, NULL);
        // Chequear que todo haya ido bien
        for (int i = 0; i < N_MEM_METRICS; i++)
        {
            if (memory_metrics[i] == NULL)
            {
                fprintf(stderr, "Error al crear las métricas de uso de memoria\n");
                return EXIT_FAILURE;
            }
        }
    }

    // Creamos las métricas para el uso del disco duro, if required
    if (config[3])
    {
        disk_metrics[0] = prom_gauge_new("sectors_read_rate", "Sectores (512 KB c/u) de HDD leidos p/s", 0, NULL);
        disk_metrics[1] = prom_gauge_new("sectors_written_rate", "Sectores (512 KB c/u) de HDD escritos p/s", 0, NULL);
        // Chequear que todo haya ido bien
        for (int i = 0; i < N_DISK_METRICS; i++)
        {
            if (disk_metrics[i] == NULL)
            {
                fprintf(stderr, "Error al crear las métricas de uso del disco duro\n");
                return EXIT_FAILURE;
            }
        }
    }

    // Creamos las métricas para el uso de networking, if required
    if (config[4])
    {
        network_metrics[0] = prom_gauge_new("rx_bytes", "RX Bytes", 0, NULL);
        network_metrics[1] = prom_gauge_new("rx_errors", "RX packets with errors", 0, NULL);
        network_metrics[2] = prom_gauge_new("rx_packets_dropped", "RX packets dropped", 0, NULL);
        network_metrics[3] = prom_gauge_new("tx_bytes", "TX Bytes", 0, NULL);
        network_metrics[4] = prom_gauge_new("tx_errors", "TX packets with errors", 0, NULL);
        network_metrics[5] = prom_gauge_new("tx_packets_dropped", "TX packets dropped", 0, NULL);
        // Chequear que todo haya ido bien
        for (int i = 0; i < N_NET_METRICS; i++)
        {
            if (network_metrics[i] == NULL)
            {
                fprintf(stderr, "Error al crear las métricas de uso de networking\n");
                return EXIT_FAILURE;
            }
        }
    }

    // Creamos las métricas relacionadas a los procesos del sistema, if required
    if (config[5])
    {
        processes_count[0] = prom_gauge_new("existing_processes", "Procesos existentes en el sistema", 0, NULL);
        processes_count[1] = prom_gauge_new("running_processes", "Procesos actualmente corriendo en el sistema", 0, NULL);
        // Chequear que todo haya ido bien
        for (int i = 0; i < N_PROC_COUNT; i++)
        {
            if (processes_count[i] == NULL)
            {
                fprintf(stderr, "Error al crear las métricas de uso de procesos\n");
                return EXIT_FAILURE;
            }
        }
    }

    /* REGISTRO DE MÉTRICAS */

    // Registramos las métricas en el registro por defecto, para el uso de CPU, if required
    if (config[1])
    {
        if (pcr_must_register_metric(cpu_usage_metric) == NULL)
        {
            fprintf(stderr, "Error al registrar la métrica de CPU\n");
            return EXIT_FAILURE;
        }
    }

    // Registramos las métricas en el registro por defecto, para el uso de memoria, if required
    if (config[2])
    {
        for (int i = 0; i < N_MEM_METRICS; i++)
        {
            if (pcr_must_register_metric(memory_metrics[i]) == NULL)
            {
                fprintf(stderr, "Error al registrar las métricas de memoria\n");
                return EXIT_FAILURE;
            }
        }
    }

    // Registramos las métricas en el registro por defecto, para el uso del disco duro, if required
    if (config[3])
    {
        for (int i = 0; i < N_DISK_METRICS; i++)
        {
            if (pcr_must_register_metric(disk_metrics[i]) == NULL)
            {
                fprintf(stderr, "Error al registrar las métricas del disco duro\n");
                return EXIT_FAILURE;
            }
        }
    }

    // Registramos las métricas en el registro por defecto, para el uso de networking, if required
    if (config[4])
    {
        for (int i = 0; i < N_NET_METRICS; i++)
        {
            if (pcr_must_register_metric(network_metrics[i]) == NULL)
            {
                fprintf(stderr, "Error al registrar las métricas de networking\n");
                return EXIT_FAILURE;
            }
        }
    }

    // Registramos las métricas en el registro por defecto, para procesos del sistema, if required
    if (config[5])
    {
        for (int i = 0; i < N_PROC_COUNT; i++)
        {
            if (pcr_must_register_metric(processes_count[i]) == NULL)
            {
                fprintf(stderr, "Error al registrar las métricas de uso de procesos\n");
                return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;
}

void destroy_mutex()
{
    pthread_mutex_destroy(&lock);
}
