#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include "utils.h"

#define FIFO_PATH "/tmp/cola_impresion"
#define LOG_PATH "/tmp/impresiones.log"

int fifo_fd = -1;
int fifo_write_dummy = -1;
FILE* log_file = NULL;

void finalizar(int signo) {
    (void)signo;

    if (fifo_fd != -1) close(fifo_fd);
    if (fifo_write_dummy != -1) close(fifo_write_dummy);
    if (log_file != NULL) fclose(log_file);

    unlink(FIFO_PATH);

    printf("Servidor finalizado correctamente.\n");
    exit(EXIT_SUCCESS);
}

void mostrar_ayuda() {
    printf("Uso: ./servidor -i <cantidad_trabajos>\n");
    printf("Opciones:\n");
    printf("  -i, --impresiones   Cantidad de archivos a procesar (obligatorio)\n");
    printf("  -h, --help          Muestra esta ayuda\n");
}

int main(int argc, char* argv[]) {
    int cantidad = -1;

    for (int i = 1; i < argc; i++) {
        if ((strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--impresiones") == 0) && i + 1 < argc) {
            cantidad = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            mostrar_ayuda();
            return 0;
        }
    }

    if (cantidad <= 0) {
        fprintf(stderr, "Error: Debe especificar una cantidad válida de impresiones con -i\n");
        return 1;
    }

    signal(SIGINT, finalizar);
    signal(SIGTERM, finalizar);

    unlink(FIFO_PATH);
    if (mkfifo(FIFO_PATH, 0666) == -1) {
        perror("Error creando FIFO de impresión");
        return 1;
    }

    fifo_fd = open(FIFO_PATH, O_RDONLY);
    if (fifo_fd == -1) {
        perror("Error abriendo FIFO de impresión (lectura)");
        return 1;
    }

    
    fifo_write_dummy = open(FIFO_PATH, O_WRONLY);
    if (fifo_write_dummy == -1) {
        perror("Error abriendo FIFO dummy writer");
        return 1;
    }

    log_file = fopen(LOG_PATH, "w");
    if (!log_file) {
        perror("Error abriendo archivo de log");
        finalizar(0);
    }

    char buffer[512];
    int trabajos_recibidos = 0;

    while (trabajos_recibidos < cantidad) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = read(fifo_fd, buffer, sizeof(buffer) - 1);
        if (bytes <= 0) continue;

        buffer[bytes] = '\0';

        char* token = strtok(buffer, ":");
        if (!token) continue;
        int pid = atoi(token);

        char* path = strtok(NULL, "\n");
        if (!path) continue;

        FILE* archivo = fopen(path, "r");
        char* mensaje;

        if (!archivo) {
            mensaje = "Error: archivo no encontrado.\n";
        } else {
            fseek(archivo, 0, SEEK_END);
            long size = ftell(archivo);
            rewind(archivo);

            if (size == 0) {
                mensaje = "Error: archivo vacío.\n";
            } else {
                char timestamp[64];
                obtener_timestamp(timestamp, sizeof(timestamp));
                printf("PID %d imprimió el archivo %s el día %s\n", pid, path, timestamp);
                fflush(stdout);

                fprintf(log_file, "PID %d imprimió el archivo %s el día %s\n", pid, path, timestamp);

                char ch;
                while ((ch = fgetc(archivo)) != EOF) {
                    fputc(ch, log_file);
                }
                fputc('\n', log_file);
                fflush(log_file);
                mensaje = "Impresión completada.\n";
            }

            fclose(archivo);
        }

        char fifo_cliente[64];
        snprintf(fifo_cliente, sizeof(fifo_cliente), "/tmp/FIFO_%d", pid);
        int fd_cliente = open(fifo_cliente, O_WRONLY);
        if (fd_cliente != -1) {
            write(fd_cliente, mensaje, strlen(mensaje));
            close(fd_cliente);
        }

        trabajos_recibidos++;
    }

    finalizar(0);
    return 0;
}
