#include "expose_metrics.h"

/** Mutex para sincronización de hilos */
pthread_mutex_t lock;

/** Métrica de Prometheus para el uso de CPU */
static prom_gauge_t* cpu_usage_metric;

/** Métrica de Prometheus para el uso de memoria */
static prom_gauge_t* memory_metrics[N_MEM_METRICS];

void update_cpu_gauge()
{
    double usage = get_cpu_usage();
    if (usage >= 0)
    {
        pthread_mutex_lock(&lock);
        prom_gauge_set(cpu_usage_metric, usage, NULL);
        pthread_mutex_unlock(&lock);
    }
    else
    {
        fprintf(stderr, "Error al obtener el uso de CPU\n");
    }
}

void update_memory_gauge()
{
    double* usage = get_memory_usage();
    if (usage != NULL)
    {
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

    // Creamos la métrica para el uso de CPU
    cpu_usage_metric = prom_gauge_new("cpu_usage_percentage", "Porcentaje de uso de CPU", 0, NULL);
    if (cpu_usage_metric == NULL)
    {
        fprintf(stderr, "Error al crear la métrica de uso de CPU\n");
        return EXIT_FAILURE;
    }

    // Creamos las métricas para el uso de memoria
    memory_metrics[0] = prom_gauge_new("memory_total", "Memoria total", 0, NULL);
    memory_metrics[1] = prom_gauge_new("memory_used", "Memoria en uso", 0, NULL);
    memory_metrics[2] = prom_gauge_new("memory_free", "Memoria libre", 0, NULL);
    memory_metrics[3] = prom_gauge_new("memory_free_percentage", "Porcentaje de memoria libre", 0, NULL);
    // Chequear que todo haya ido bien
    for (int i = 0; i < N_MEM_METRICS; i++)
    {
        if (memory_metrics[i] == NULL)
        {
            fprintf(stderr, "Error al crear las métricas de uso de memoria\n");
            return EXIT_FAILURE;
        }
    }

    // Registramos las métricas en el registro por defecto, para el uso de memoria
    for (int i = 0; i < N_MEM_METRICS; i++)
    {
        if (pcr_must_register_metric(memory_metrics[i]) == NULL)
        {
            fprintf(stderr, "Error al registrar las métricas de memoria\n");
            return EXIT_FAILURE;
        }
    }
    // Registramos las métricas en el registro por defecto, para el uso de CPU
    if (pcr_must_register_metric(cpu_usage_metric) == NULL)
    {
        fprintf(stderr, "Error al registrar la métrica de CPU\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void destroy_mutex()
{
    pthread_mutex_destroy(&lock);
}
