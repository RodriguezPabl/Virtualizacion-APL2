#include "utils.h"
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

bool archivo_valido(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    return st.st_size > 0 && S_ISREG(st.st_mode);
}

void limpiar_fifo(const char* path) {
    if (access(path, F_OK) == 0) {
        unlink(path);
    }
}

void obtener_timestamp(char* buffer, int size) {
    time_t ahora = time(NULL);
    struct tm* tm_info = localtime(&ahora);
    strftime(buffer, size, "%d/%m/%Y %H:%M:%S", tm_info);
}
