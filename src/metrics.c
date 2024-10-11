#include "metrics.h"

double* get_memory_usage()
{
    FILE* fp;
    char buffer[BUFFER_SIZE];
    unsigned long long total_mem = 0, free_mem = 0;

    // Abrir el archivo /proc/meminfo
    fp = fopen("/proc/meminfo", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/meminfo");
        return NULL;
    }

    // Leer los valores de memoria total y disponible
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "MemTotal: %llu kB", &total_mem) == 1)
        {
            continue; // MemTotal encontrado
        }
        if (sscanf(buffer, "MemAvailable: %llu kB", &free_mem) == 1)
        {
            break; // MemAvailable encontrado, podemos dejar de leer
        }
    }

    fclose(fp);

    // Verificar si se encontraron ambos valores
    if (total_mem == 0 || free_mem == 0)
    {
        fprintf(stderr, "Error al leer la información de memoria desde /proc/meminfo\n");
        return NULL;
    }

    // Calcular aquello a retornar
    static double metrics[4];
    metrics[0] = (double)total_mem;
    long long unsigned used_mem = total_mem - free_mem;
    metrics[1] = (double)used_mem;
    metrics[2] = (double)free_mem;
    double mem_usage_percent = ((double)used_mem / total_mem) * 100;
    metrics[3] = mem_usage_percent;

    return metrics;
}

double get_cpu_usage()
{
    static unsigned long long prev_user = 0, prev_nice = 0, prev_system = 0, prev_idle = 0, prev_iowait = 0,
                              prev_irq = 0, prev_softirq = 0, prev_steal = 0;
    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    unsigned long long totald, idled;
    double cpu_usage_percent;

    // Abrir el archivo /proc/stat
    FILE* fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/stat");
        return -1.0;
    }

    char buffer[BUFFER_SIZE * 4];
    if (fgets(buffer, sizeof(buffer), fp) == NULL)
    {
        perror("Error al leer /proc/stat");
        fclose(fp);
        return -1.0;
    }
    fclose(fp);

    // Analizar los valores de tiempo de CPU
    int ret = sscanf(buffer, "cpu  %llu %llu %llu %llu %llu %llu %llu %llu", &user, &nice, &system, &idle, &iowait,
                     &irq, &softirq, &steal);
    if (ret < 8)
    {
        fprintf(stderr, "Error al parsear /proc/stat\n");
        return -1.0;
    }

    // Calcular las diferencias entre las lecturas actuales y anteriores
    unsigned long long prev_idle_total = prev_idle + prev_iowait;
    unsigned long long idle_total = idle + iowait;

    unsigned long long prev_non_idle = prev_user + prev_nice + prev_system + prev_irq + prev_softirq + prev_steal;
    unsigned long long non_idle = user + nice + system + irq + softirq + steal;

    unsigned long long prev_total = prev_idle_total + prev_non_idle;
    unsigned long long total = idle_total + non_idle;

    totald = total - prev_total;
    idled = idle_total - prev_idle_total;

    if (totald == 0)
    {
        fprintf(stderr, "Totald es cero, no se puede calcular el uso de CPU!\n");
        return -1.0;
    }

    // Calcular el porcentaje de uso de CPU
    cpu_usage_percent = ((double)(totald - idled) / (double)totald) * 100.0;

    // Actualizar los valores anteriores para la siguiente lectura
    prev_user = user;
    prev_nice = nice;
    prev_system = system;
    prev_idle = idle;
    prev_iowait = iowait;
    prev_irq = irq;
    prev_softirq = softirq;
    prev_steal = steal;

    return cpu_usage_percent;
}

double* get_disk_usage()
{
    FILE* fp;
    char buffer[BUFFER_SIZE];
    unsigned long long sectors_read = 0, time_spent_reading = 0, sectors_written = 0, time_spent_writting = 0;

    // Abrir el archivo /proc/diskstats
    fp = fopen("/proc/diskstats", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/diskstats");
        return NULL;
    }

    // Leer los valores de disco de interés
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "%*u       %*u sda %*u %*u %llu %llu %*u %*u %llu %llu",
            &sectors_read, &time_spent_reading, &sectors_written, &time_spent_writting) == 4)
        {
            break; // Datos de sda encontrados, podemos dejar de leer
        }
    }

    fclose(fp);

    // Verificar si se encontraron los valores
    if (sectors_read == 0 || time_spent_reading == 0 || sectors_written == 0 || time_spent_writting == 0)
    {
        fprintf(stderr, "Error al leer la información de disco duro desde /proc/diskstats\n");
        return NULL;
    }

    // Calcular aquello a retornar
    static double metrics[2];
    metrics[0] = (double)sectors_read / time_spent_reading;
    metrics[1] = (double)sectors_written / time_spent_writting;

    return metrics;
}

double* get_network_usage()
{
    FILE* fp;
    char buffer[BUFFER_SIZE];
    unsigned long long rx_bytes = 0, rx_errors = 0, rx_packets_dropped = 0,
        tx_bytes = 0, tx_errors = 0, tx_packets_dropped = 0;

    // Abrir el archivo /proc/net/dev
    fp = fopen("/proc/net/dev", "r");
    if (fp == NULL)
    {
        perror("Error al abrir /proc/net/dev");
        return NULL;
    }

    // Leer los valores de networking de interés
    int data_read = 0;
    while (fgets(buffer, sizeof(buffer), fp) != NULL)
    {
        if (sscanf(buffer, "en%*s %llu %*u %llu %llu %*u %*u %*u %*u %llu %*u %llu %llu",
            &rx_bytes, &rx_errors, &rx_packets_dropped, &tx_bytes, &tx_errors, &tx_packets_dropped) == 6)
        {
            data_read = 1;
            break; // Datos de sda encontrados, podemos dejar de leer
        }
    }

    fclose(fp);

    // Verificar si se encontraron los valores
    if(data_read == 0)
    {
        fprintf(stderr, "Error al leer la información de networking desde /proc/net/dev\n");
        return NULL;
    }

    // Construir aquello a retornar
    static double metrics[6];
    // Typecast explícito a *double* ya que los gauges de libprom trabajan con ese tipo de valor
    metrics[0] = (double)rx_bytes;
    metrics[1] = (double)rx_errors;
    metrics[2] = (double)rx_packets_dropped;
    metrics[3] = (double)tx_bytes;
    metrics[4] = (double)tx_errors;
    metrics[5] = (double)tx_packets_dropped;

    return metrics;
}
