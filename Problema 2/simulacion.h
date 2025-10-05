// Nombre del archivo: simulacion.h
#ifndef SIMULACION_H
#define SIMULACION_H

#include "autopista.h"

// --- Prototipos de funciones de simulación ---

void iniciar_simulacion(ParametrosSimulacion* params);
void* hilo_vehiculo(void* args);
void* hilo_reloj(void* args);

#endif // SIMULACION_H

