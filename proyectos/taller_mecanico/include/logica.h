#ifndef LOGICA_H
#define LOGICA_H

#include "estructuras.h"

// Inicialización y liberación
void inicializarSistema(SistemaTaller* s, const char* nombre, const char* rif, const char* direccion, const char* telefono);
void liberarSistema(SistemaTaller* s);

// Módulo Vehículos
Vehiculo* agregarVehiculo(SistemaTaller* s, Vehiculo datos);
Vehiculo* buscarVehiculoPorID(SistemaTaller* s, int id);
Vehiculo* buscarVehiculoPorPlaca(SistemaTaller* s, const char* placa);
Vehiculo** buscarVehiculosPorPropietario(SistemaTaller* s, const char* subcadena, int* cantidad);
Vehiculo** listarVehiculos(SistemaTaller* s, int* cantidad);
bool actualizarVehiculo(SistemaTaller* s, int id, Vehiculo datos);
ResultadoOperacion eliminarVehiculo(SistemaTaller* s, int id);

// Módulo Mecánicos
Mecanico* agregarMecanico(SistemaTaller* s, Mecanico datos);
Mecanico* buscarMecanicoPorID(SistemaTaller* s, int id);
Mecanico* buscarMecanicoPorCedula(SistemaTaller* s, const char* cedula);
Mecanico** listarMecanicos(SistemaTaller* s, int* cantidad);
int contarOrdenesAbiertasMecanico(SistemaTaller* s, int idMecanico);

// Módulo Repuestos
Repuesto* agregarRepuesto(SistemaTaller* s, Repuesto datos);
Repuesto* buscarRepuestoPorID(SistemaTaller* s, int id);
Repuesto* buscarRepuestoPorCodigo(SistemaTaller* s, const char* codigo);
Repuesto** buscarRepuestosPorNombreOCodigo(SistemaTaller* s, const char* subcadena, int* cantidad);
Repuesto** listarRepuestos(SistemaTaller* s, int* cantidad);
bool registrarEntradaMercancia(SistemaTaller* s, int id, int cantidad);
bool cambiarPrecioRepuesto(SistemaTaller* s, int id, float nuevoPrecio);
ResultadoOperacion eliminarRepuesto(SistemaTaller* s, int id);

// Módulo Órdenes de Servicio
OrdenServicio* abrirOrden(SistemaTaller* s, int idVehiculo, int idMecanico, const char* fechaApertura, const char* descripcionFalla, int kilometrajeIngreso, ResultadoOperacion* resultado);
ResultadoOperacion agregarRepuestoAOrden(SistemaTaller* s, int idOrden, int idRepuesto, int cantidad);
ResultadoOperacion quitarRepuestoDeOrden(SistemaTaller* s, int idOrden, int idRepuesto);
ResultadoOperacion cerrarOrden(SistemaTaller* s, int idOrden, float manoObra, const char* fechaCierre);
ResultadoOperacion anularOrden(SistemaTaller* s, int idOrden);
float calcularTotalOrden(const OrdenServicio* orden);
OrdenServicio* buscarOrdenPorID(SistemaTaller* s, int id);
OrdenServicio** listarOrdenesPorEstado(SistemaTaller* s, const char* estado, int* cantidad);
OrdenServicio** listarOrdenesPorVehiculo(SistemaTaller* s, int idVehiculo, int* cantidad);
OrdenServicio** listarTodasOrdenes(SistemaTaller* s, int* cantidad);

#endif // LOGICA_H
