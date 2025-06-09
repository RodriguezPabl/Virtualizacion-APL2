/*############# INTEGRANTES ###############
###     Justiniano, Máximo              ###
###     Mallia, Leandro                 ###
###     Maudet, Alejandro               ###
###     Naspleda, Julián                ###
###     Rodriguez, Pablo                ###
#########################################*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <getopt.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "buffer.h"
#include "productor.h"
#include "consumidor.h"

#define MAX_PATH 4096

char directorio[MAX_PATH];
int generadores = 0, consumidores = 0, total_paquetes = 0;

pthread_t *threads_gen = NULL;
pthread_t *threads_con = NULL;

void mostrar_ayuda() {
    printf("Uso: ./ejercicio2 -d <directorio> -g <generadores> -c <consumidores> -p <paquetes>\n");
}

void limpiar_directorio(const char *path) {
    DIR *dir = opendir(path);
    if (!dir) return;

    struct dirent *entry;
    char filepath[MAX_PATH];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        size_t len = strlen(entry->d_name);
        if (len > 4 && strcmp(entry->d_name + len - 4, ".paq") == 0) {
            snprintf(filepath, MAX_PATH - 1, "%s/%s", path, entry->d_name);
            filepath[MAX_PATH - 1] = '\0';
            remove(filepath);
        }
    }
    closedir(dir);

    char procesados[MAX_PATH];
    snprintf(procesados, MAX_PATH - 1, "%s/procesados", path);
    procesados[MAX_PATH - 1] = '\0';
    mkdir(procesados, 0777);
}

void manejar_senal(int sig) {
    liberar_buffer();
    printf("\nProceso terminado con se�al %d. Recursos liberados.\n", sig);
    exit(EXIT_SUCCESS);
}

int main(int argc, char *argv[]) {
    int opt;
    static struct option opciones_largas[] = {
        {"directorio", required_argument, 0, 'd'},
        {"generadores", required_argument, 0, 'g'},
        {"consumidores", required_argument, 0, 'c'},
        {"paquetes", required_argument, 0, 'p'},
        {"help", no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };

    while ((opt = getopt_long(argc, argv, "d:g:c:p:h", opciones_largas, NULL)) != -1) {
        switch (opt) {
            case 'd': strncpy(directorio, optarg, MAX_PATH); break;
            case 'g': generadores = atoi(optarg); break;
            case 'c': consumidores = atoi(optarg); break;
            case 'p': total_paquetes = atoi(optarg); break;
            case 'h': mostrar_ayuda(); exit(EXIT_SUCCESS);
            default: mostrar_ayuda(); exit(EXIT_FAILURE);
        }
    }

    if (!directorio[0] || generadores <= 0 || consumidores <= 0 || total_paquetes <= 0) {
        mostrar_ayuda();
        return EXIT_FAILURE;
    }

    signal(SIGINT, manejar_senal);
    signal(SIGTERM, manejar_senal);

    limpiar_directorio(directorio);
    inicializar_buffer(10, directorio);

    threads_gen = malloc(sizeof(pthread_t) * generadores);
    threads_con = malloc(sizeof(pthread_t) * consumidores);

    for (int i = 0; i < generadores; ++i) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads_gen[i], NULL, thread_productor, id);
    }
    for (int i = 0; i < consumidores; ++i) {
        int *id = malloc(sizeof(int));
        *id = i;
        pthread_create(&threads_con[i], NULL, thread_consumidor, id);
    }

    for (int i = 0; i < generadores; ++i) pthread_join(threads_gen[i], NULL);
    marcar_productores_terminados();
    for (int i = 0; i < consumidores; ++i) pthread_join(threads_con[i], NULL);

    mostrar_resumen();
    liberar_buffer();

    free(threads_gen);
    free(threads_con);
    return 0;
}
