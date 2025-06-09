#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>


bool archivo_valido(const char* path);


void limpiar_fifo(const char* path);


void obtener_timestamp(char* buffer, int size);

#endif
