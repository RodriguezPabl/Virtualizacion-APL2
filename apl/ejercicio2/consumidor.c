#include "buffer.h"
#include "consumidor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

extern char directorio[];

void *thread_consumidor(void *arg) {
    free(arg);

    while (1) {
        char *archivo = tomar_archivo();
        if (!archivo) break;

        FILE *fp = fopen(archivo, "r");
        if (!fp) {
            free(archivo);
            continue;
        }

        int id_paquete, destino;
        float peso;
        if (fscanf(fp, "%d;%f;%d", &id_paquete, &peso, &destino) == 3) {
            registrar_paquete(destino, peso);
        }
        fclose(fp);

        char destino_path[512];
        snprintf(destino_path, sizeof(destino_path), "%s/procesados/%s", directorio, basename(archivo));
        rename(archivo, destino_path);
        free(archivo);
    }
    return NULL;
}
