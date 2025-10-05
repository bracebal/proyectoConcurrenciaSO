#ifndef UTILIDADES_H
#define UTILIDADES_H

#include "autopista.h"

// --- Prototipos de funciones ---

// Funciones del menú
void mostrar_menu(ParametrosSimulacion* params);
void modificar_parametros(ParametrosSimulacion* params);

// Funciones auxiliares
void ver_consumo_recursos();
double tiempo_actual_preciso();
const char* tipo_a_texto(TipoVehiculo t);
const char* dir_a_texto(Direccion d);

#endif // UTILIDADES_H