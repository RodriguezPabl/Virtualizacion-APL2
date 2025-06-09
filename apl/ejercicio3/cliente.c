#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "utils.h"

#define FIFO_IMPRESION "/tmp/cola_impresion"
#define FIFO_PRIVADO_TEMPLATE "/tmp/FIFO_%d" //saca
#define BUFFER_SIZE 1024

void mostrar_ayuda() {
    printf("Uso: ./cliente -a <archivo_a_imprimir>\n");
    printf("Opciones:\n");
    printf("  -a, --archivo     Archivo a enviar a la cola de impresión (obligatorio)\n");
    printf("  -h, --help        Muestra esta ayuda\n");
}

int main(int argc, char* argv[]) {
    if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        mostrar_ayuda();
        return 0;
    }

    if (argc != 3 || (strcmp(argv[1], "-a") != 0 && strcmp(argv[1], "--archivo") != 0)) {
        mostrar_ayuda();
        return 1;
    }

    char* archivo = argv[2];
    pid_t pid = getpid();

    if (!archivo_valido(archivo)) {
        fprintf(stderr, "ERROR: El archivo '%s' no es válido o está vacío.\n", archivo);
        return 1;
    }

    // Crear FIFO privado
    char fifo_privado[BUFFER_SIZE];
    snprintf(fifo_privado, sizeof(fifo_privado), "/tmp/FIFO_%d", pid);
    limpiar_fifo(fifo_privado);
    if (mkfifo(fifo_privado, 0666) == -1) {
        perror("Error creando FIFO privado");
        return 1;
    }

    // Abrir FIFO de impresión
    int fifo_impresion = open(FIFO_IMPRESION, O_WRONLY);
    if (fifo_impresion == -1) {
        perror("Error abriendo FIFO de impresión (¿está corriendo el servidor?)");
        unlink(fifo_privado);
        return 1;
    }

    // Enviar mensaje al servidor
    char mensaje[BUFFER_SIZE];
    snprintf(mensaje, sizeof(mensaje), "%d:%s", pid, archivo);
    write(fifo_impresion, mensaje, strlen(mensaje));
    close(fifo_impresion);

    // Esperar respuesta
    int fifo_respuesta = open(fifo_privado, O_RDONLY);
    if (fifo_respuesta == -1) {
        perror("Error abriendo FIFO de respuesta");
        unlink(fifo_privado);
        return 1;
    }

    char respuesta[BUFFER_SIZE];
    ssize_t leido = read(fifo_respuesta, respuesta, sizeof(respuesta) - 1);
    if (leido > 0) {
        respuesta[leido] = '\0';
        printf("Servidor: %s", respuesta);
    } else {
        fprintf(stderr, "Error: no se recibió respuesta del servidor\n");
    }

    close(fifo_respuesta);
    unlink(fifo_privado);
    return 0;
}
