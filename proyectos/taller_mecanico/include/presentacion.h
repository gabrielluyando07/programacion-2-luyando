#ifndef PRESENTACION_H
#define PRESENTACION_H

#include "estructuras.h"

void menuPrincipal(SistemaTaller* s);
void menuVehiculos(SistemaTaller* s);
void menuMecanicos(SistemaTaller* s);
void menuRepuestos(SistemaTaller* s);
void menuOrdenes(SistemaTaller* s);
void mostrarDetalleOrden(SistemaTaller* s, const OrdenServicio* orden);

#endif // PRESENTACION_H
