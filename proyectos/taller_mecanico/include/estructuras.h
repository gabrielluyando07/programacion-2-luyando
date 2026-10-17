#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

struct Taller {
    char nombre[100];
    char rif[20];
    char direccion[200];
    char telefono[20];
};

struct Vehiculo {
    int id;
    char placa[10];
    char marca[50];
    char modelo[50];
    int anio;
    int kilometraje;
    char nombrePropietario[100];
    char cedulaPropietario[20];
    char telefonoPropietario[20];
    char fechaRegistro[11];
};

struct Mecanico {
    int id;
    char nombre[100];
    char cedula[20];
    char especialidad[20]; // "MOTOR", "FRENOS", "ELECTRICIDAD", "SUSPENSION", "GENERAL"
    char fechaIngreso[11];
};

struct Repuesto {
    int id;
    char codigo[20];
    char nombre[100];
    char marca[50];
    float precio;
    int stock;
    int stockMinimo;
    char fechaRegistro[11];
};

struct DetalleOrden {
    int idRepuesto;
    int cantidad;
    float precioUnitario;
};

struct OrdenServicio {
    int id;
    int idVehiculo;
    int idMecanico;
    char fechaApertura[11];
    char fechaCierre[11];
    char estado[12]; // "ABIERTA", "CERRADA", "ANULADA"
    char descripcionFalla[200];
    int kilometrajeIngreso;
    float manoObra;
    
    DetalleOrden* detalles;
    int numDetalles;
    int capacidadDetalles;
};

struct SistemaTaller {
    Taller taller;

    Vehiculo* vehiculos;
    int numVehiculos;
    int capacidadVehiculos;

    Mecanico* mecanicos;
    int numMecanicos;
    int capacidadMecanicos;

    Repuesto* repuestos;
    int numRepuestos;
    int capacidadRepuestos;

    OrdenServicio* ordenes;
    int numOrdenes;
    int capacidadOrdenes;

    int siguienteIdVehiculo;
    int siguienteIdMecanico;
    int siguienteIdRepuesto;
    int siguienteIdOrden;
};

enum ResultadoOperacion {
    EXITO = 0,
    ERROR_NO_ENCONTRADO,
    ERROR_DUPLICADO,
    ERROR_DATOS_INVALIDOS,
    ERROR_TIENE_ORDENES,
    ERROR_ORDEN_CERRADA_O_ANULADA,
    ERROR_STOCK_INSUFICIENTE,
    ERROR_LIMITE_ALCANZADO,
    ERROR_CANCELADO
};

#endif // ESTRUCTURAS_H
