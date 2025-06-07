#pragma once
#include <semaphore.h>

#define SHM_NAME "/ahorcado_shm"
#define SEM_SERVER_READY "/sem_server_ready"
#define SEM_CLIENT_READY "/sem_client_ready"
#define SEM_TURNO "/sem_turno"
#define SEM_END "/sem_end"
#define SEM_UNICO_CLIENTE "/sem_unico_cliente"
#define SEM_UNICO_SERVIDOR "/sem_unico_servidor"

#define MAX_FRASE 256
#define MAX_NICK 32

struct Juego {
    char frase_original[MAX_FRASE];
    char frase_visible[MAX_FRASE];
    char letras_usadas[27];
    char nickname[MAX_NICK];
    int intentos_restantes;
    int gano; // 1 = ganó, 0 = perdió, -1 = en curso
    int en_partida;
    long tiempo_inicio;
    long tiempo_fin;
};
