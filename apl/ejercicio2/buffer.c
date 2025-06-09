/*############# INTEGRANTES ###############
###     Justiniano, Máximo              ###
###     Mallia, Leandro                 ###
###     Maudet, Alejandro               ###
###     Naspleda, Julián                ###
###     Rodriguez, Pablo                ###
#########################################*/

#include "buffer.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_BUFFER 100
#define MAX_PATH 4096

static char *buffer[MAX_BUFFER];
static int capacidad = 0, inicio = 0, fin = 0, cantidad = 0;
static int productores_terminados = 0;
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static sem_t hay_espacio, hay_datos;
static char directorio[MAX_PATH];
static float pesos[51] = {0};
static int conteos[51] = {0};

void inicializar_buffer(int cap, const char *dir) {
    capacidad = cap;
    sem_init(&hay_espacio, 0, cap);
    sem_init(&hay_datos, 0, 0);
    strncpy(directorio, dir, MAX_PATH);
}

void liberar_buffer() {
    for (int i = 0; i < cantidad; ++i) {
        free(buffer[(inicio + i) % capacidad]);
    }
    sem_destroy(&hay_espacio);
    sem_destroy(&hay_datos);
    pthread_mutex_destroy(&mutex);
}

void agregar_archivo(const char *archivo) {
    sem_wait(&hay_espacio);
    pthread_mutex_lock(&mutex);
    buffer[fin] = strdup(archivo);
    fin = (fin + 1) % capacidad;
    cantidad++;
    pthread_mutex_unlock(&mutex);
    sem_post(&hay_datos);
}

char *tomar_archivo() {
    sem_wait(&hay_datos);
    pthread_mutex_lock(&mutex);
    if (cantidad == 0 && productores_terminados) {
        pthread_mutex_unlock(&mutex);
        sem_post(&hay_datos); // liberar por otros
        return NULL;
    }
    char *archivo = buffer[inicio];
    inicio = (inicio + 1) % capacidad;
    cantidad--;
    pthread_mutex_unlock(&mutex);
    sem_post(&hay_espacio);
    return archivo;
}

void marcar_productores_terminados() {
    pthread_mutex_lock(&mutex);
    productores_terminados = 1;
    pthread_mutex_unlock(&mutex);
    for (int i = 0; i < capacidad; ++i) sem_post(&hay_datos);
}

void registrar_paquete(int sucursal, float peso) {
    if (sucursal < 1 || sucursal > 50) return;
    pthread_mutex_lock(&mutex);
    pesos[sucursal] += peso;
    conteos[sucursal]++;
    pthread_mutex_unlock(&mutex);
}

void mostrar_resumen() {
    printf("\nResumen de procesamiento:\n");
    for (int i = 1; i <= 50; ++i) {
        if (conteos[i] > 0) {
            printf("Sucursal %d: %d paquetes, Peso total: %.2f kg\n", i, conteos[i], pesos[i]);
        }
    }
}
