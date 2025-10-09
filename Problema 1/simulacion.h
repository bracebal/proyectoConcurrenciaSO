#ifndef SIMULACION_H
#define SIMULACION_H

#include "central.h"

// --- Prototipos de funciones ---

void iniciar_simulacion(ParametrosSimulacion* params);
void* hilo_telefono(void* args);
void* hilo_central(void* args); // <-- Hilo central está de vuelta

#endif // SIMULACION_H