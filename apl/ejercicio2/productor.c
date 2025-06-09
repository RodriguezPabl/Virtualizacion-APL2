/*############# INTEGRANTES ###############
###     Justiniano, Máximo              ###
###     Mallia, Leandro                 ###
###     Maudet, Alejandro               ###
###     Naspleda, Julián                ###
###     Rodriguez, Pablo                ###
#########################################*/

#include "productor.h"
#include "buffer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

extern int total_paquetes;
extern char directorio[];
extern pthread_mutex_t mutex_cantidad;
extern int cantidad_generada;

void *thread_productor(void *arg) {
    int id = *(int *)arg;
    free(arg);
    srand(time(NULL) ^ (id << 8));

    while (1) {
        pthread_mutex_lock(&mutex_cantidad);
        if (cantidad_generada >= total_paquetes) {
            pthread_mutex_unlock(&mutex_cantidad);
            break;
        }
        int id_paquete = ++cantidad_generada;
        pthread_mutex_unlock(&mutex_cantidad);

        float peso = ((float)rand() / RAND_MAX) * 300.0f;
        int destino = rand() % 50 + 1;

        char nombre_archivo[256];
        snprintf(nombre_archivo, sizeof(nombre_archivo), "%s/%d.paq", directorio, id_paquete);
        FILE *fp = fopen(nombre_archivo, "w");
        if (!fp) continue;
        fprintf(fp, "%d;%.2f;%d\n", id_paquete, peso, destino);
        fclose(fp);

        agregar_archivo(strdup(nombre_archivo));
        usleep(100000); // simula trabajo
    }
    return NULL;
}

pthread_mutex_t mutex_cantidad = PTHREAD_MUTEX_INITIALIZER;
int cantidad_generada = 0;
