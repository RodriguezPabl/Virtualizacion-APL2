#ifndef BUFFER_H_INCLUDED
#define BUFFER_H_INCLUDED

#include <pthread.h>

void inicializar_buffer(int capacidad, const char *dir);
void liberar_buffer();
void agregar_archivo(const char *archivo);
char *tomar_archivo();
void marcar_productores_terminados();
void registrar_paquete(int sucursal, float peso);
void mostrar_resumen();

#endif // BUFFER_H_INCLUDED
