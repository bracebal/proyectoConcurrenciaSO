#ifndef UTILIDADES_H
#define UTILIDADES_H

#include "central.h"

// --- Prototipos de funciones de utilidades ---

// Funciones del menú
void mostrar_menu(ParametrosSimulacion* params);
void modificar_parametros(ParametrosSimulacion* params);

// Funciones auxiliares
void ver_consumo_recursos();
const char* estado_a_texto(EstadoTelefono estado);

#endif // UTILIDADES_H