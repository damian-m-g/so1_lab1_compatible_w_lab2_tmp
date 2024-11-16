/**
 * @file main.c
 * @brief Entry point of the system
 */

#include "expose_metrics.h"
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <string.h>

//! \brief Lowest array index.
#define LOWEST_ARR_INDEX 0
//! \brief Once the daemon thread running the HTTP server is set to end, wait for a moment so it ends properly.
#define WAITING_TIME_FOR_EXPOSE_METRICS_THREAD_TO_END 500000 // us
//! \brief Total amount of key-value pairs in the JSON config file.
#define N_JSON_ENTRIES 6
/**
 * \brief JSON config file default values for key-value pairs.
 * - 0: update_interval (in seconds, only integer).
 * - 1: cpu (take or not metric).
 * - 2: mem (take or not metric).
 * - 3: hdd (take or not metric).
 * - 4: net (take or not metric).
 * - 5: procs (take or not metric).
 */
#define JSON_ENTRIES_DEF_VAL {1, 1, 1, 1, 1, 1}

/* GLOBAL VARIABLES */
static pthread_t tid;
//! \brief Definido y asignado en "expose_metrics.c".
extern unsigned char g_status[G_STATUS_N_METRICS_TRACKED];
//! \brief Configuration data array.
unsigned char config[N_JSON_ENTRIES] = JSON_ENTRIES_DEF_VAL;

/* FUNCTIONS PROTOTYPE */
//! \brief Handler ante syscalls SIGINT y SIGTERM.
void handle_sigint_and_sigterm(int sig);
//! \brief Handler ante syscall SIGUSR1; interpretada como petición de status.
void handle_sigusr1(int sig, siginfo_t *info, void *context);
//! \brief Register the handlers for different signals.
void register_signal_handlers(void);
//! \brief Method only called if a path to certain config file was provided to this program. Sets the config glob var.
void set_configuration(char* path_to_config_file);

/* FUNCTIONS DECLARATION */
void handle_sigint_and_sigterm(int sig)
{
    // Unused arg
    (void)sig;
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

void handle_sigusr1(int sig, siginfo_t *info, void *context)
{
    // Args ignored
    (void)context;
    (void)sig;

    // Get the process id of the one trying to get this program status
    __pid_t caller_pid = info->si_pid;

    // Construct the status data, which gets encoded into an int
    int encoded_data = (int)g_status[0] | (int)g_status[1] << 8 | (int)g_status[2] << 16 | (int)g_status[3] << 24;

    // Send the data back
    union sigval status;
    status.sival_int = encoded_data;
    if (sigqueue(caller_pid, SIGUSR1, status) == -1)
    {
        perror("ERROR: Status return to calling process through SIGUSR1 unable to perform");
    }
}

void register_signal_handlers(void)
{
    // Registro de signals handler para salida limpia
    signal(SIGINT, handle_sigint_and_sigterm);
    signal(SIGTERM, handle_sigint_and_sigterm);
    // Registro de singal handler para obtención de status
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_restorer = NULL;
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("ERROR: signal subscription failed");
    }
}

void set_configuration(char* path_to_config_file)
{
    // Open the file, and check for success
    FILE *file = fopen(path_to_config_file, "r");
    if (!file)
    {
        perror("ERROR: Can't open config file");
        return;
    }

    // Prepare placeholders, 5 string holders of 6 chars max (counting null termination char) to hold "true" or "false"
    char temp[5][6];
    if (fscanf(
               file,
               "{ \"update_interval\": %hhu, \"metrics\": { \"cpu\": %[truefals], \"mem\": %[truefals], "
               "\"hdd\": %[truefals], \"net\": %[truefals], \"procs\": %[truefals] } } ",
               config, temp[0], temp[1], temp[2], temp[3], temp[4]
              ) != 6)
    {
        fprintf(stderr, "ERROR: Config file wrongly parsed.\n");
        fclose(file);
        return;
    }

    // Convert the remaining parsed values to useful ones
    for (int i = LOWEST_ARR_INDEX; i < 5; i++)
    {
        if (strcmp(temp[i], "true") == 0)
        {
            config[i + 1] = 1;
        }
        else if (strcmp(temp[i], "false") == 0)
        {
            config[i + 1] = 0;
        }
    }

    fclose(file);
}

//! \brief Main function of the program.
int main(int argc, char* argv[])
{
    if (argc > 1)
    {
        // Potentially a path to a JSON configuration file was passed
        set_configuration(argv[1]);
    }

    // Creamos un hilo para exponer las métricas vía HTTP
    if (pthread_create(&tid, NULL, expose_metrics, NULL) != 0)
    {
        fprintf(stderr, "Error al crear el hilo del servidor HTTP\n");
        return EXIT_FAILURE;
    }

    init_metrics();
    register_signal_handlers();

    // Bucle principal para actualizar las métricas cada segundo, only required ones
    while (true)
    {
        if (config[1]) update_cpu_gauge();
        if (config[2]) update_memory_gauges();
        if (config[3]) update_disk_gauges();
        if (config[4]) update_network_gauges();
        if (config[5]) update_processes_gauge();
        sleep(config[0]);
    }

    return EXIT_SUCCESS;
}
