#include "../include/logica.h"
#include <cstring>
#include <cctype>
#include <cstdlib>

static void my_strlwr(char* str) {
    for (; *str; ++str) *str = (char)tolower((unsigned char)*str);
}

static bool contieneSubcadena(const char* cadena, const char* subcadena) {
    char c1[200], c2[200];
    strncpy(c1, cadena, sizeof(c1) - 1); c1[sizeof(c1) - 1] = '\0';
    strncpy(c2, subcadena, sizeof(c2) - 1); c2[sizeof(c2) - 1] = '\0';
    my_strlwr(c1);
    my_strlwr(c2);
    return strstr(c1, c2) != nullptr;
}

void inicializarSistema(SistemaTaller* s, const char* nombre, const char* rif, const char* direccion, const char* telefono) {
    strncpy(s->taller.nombre, nombre, sizeof(s->taller.nombre));
    strncpy(s->taller.rif, rif, sizeof(s->taller.rif));
    strncpy(s->taller.direccion, direccion, sizeof(s->taller.direccion));
    strncpy(s->taller.telefono, telefono, sizeof(s->taller.telefono));

    s->capacidadVehiculos = 4;
    s->numVehiculos = 0;
    s->vehiculos = new Vehiculo[s->capacidadVehiculos];

    s->capacidadMecanicos = 4;
    s->numMecanicos = 0;
    s->mecanicos = new Mecanico[s->capacidadMecanicos];

    s->capacidadRepuestos = 4;
    s->numRepuestos = 0;
    s->repuestos = new Repuesto[s->capacidadRepuestos];

    s->capacidadOrdenes = 4;
    s->numOrdenes = 0;
    s->ordenes = new OrdenServicio[s->capacidadOrdenes];

    s->siguienteIdVehiculo = 1;
    s->siguienteIdMecanico = 1;
    s->siguienteIdRepuesto = 1;
    s->siguienteIdOrden = 1;
}

void liberarSistema(SistemaTaller* s) {
    delete[] s->vehiculos;
    delete[] s->mecanicos;
    delete[] s->repuestos;

    for (int i = 0; i < s->numOrdenes; ++i) {
        delete[] s->ordenes[i].detalles;
    }
    delete[] s->ordenes;
}

Vehiculo* agregarVehiculo(SistemaTaller* s, Vehiculo datos) {
    if (datos.anio < 1950 || datos.anio > 2027 || datos.kilometraje < 0) return nullptr;
    if (buscarVehiculoPorPlaca(s, datos.placa) != nullptr) return nullptr;

    if (s->numVehiculos == s->capacidadVehiculos) {
        int nuevaCap = s->capacidadVehiculos * 2;
        Vehiculo* temp = new Vehiculo[nuevaCap];
        for (int i = 0; i < s->numVehiculos; ++i) temp[i] = s->vehiculos[i];
        delete[] s->vehiculos;
        s->vehiculos = temp;
        s->capacidadVehiculos = nuevaCap;
    }

    datos.id = s->siguienteIdVehiculo++;
    s->vehiculos[s->numVehiculos] = datos;
    return &s->vehiculos[s->numVehiculos++];
}

Vehiculo* buscarVehiculoPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numVehiculos; ++i) {
        if (s->vehiculos[i].id == id) return &s->vehiculos[i];
    }
    return nullptr;
}

Vehiculo* buscarVehiculoPorPlaca(SistemaTaller* s, const char* placa) {
    for (int i = 0; i < s->numVehiculos; ++i) {
        if (strcmp(s->vehiculos[i].placa, placa) == 0) return &s->vehiculos[i];
    }
    return nullptr;
}

Vehiculo** buscarVehiculosPorPropietario(SistemaTaller* s, const char* subcadena, int* cantidad) {
    int coincidencias = 0;
    for (int i = 0; i < s->numVehiculos; ++i) {
        if (contieneSubcadena(s->vehiculos[i].nombrePropietario, subcadena)) coincidencias++;
    }

    *cantidad = coincidencias;
    if (coincidencias == 0) return nullptr;

    Vehiculo** resultado = new Vehiculo*[coincidencias];
    int idx = 0;
    for (int i = 0; i < s->numVehiculos; ++i) {
        if (contieneSubcadena(s->vehiculos[i].nombrePropietario, subcadena)) {
            resultado[idx++] = &s->vehiculos[i];
        }
    }
    return resultado;
}

Vehiculo** listarVehiculos(SistemaTaller* s, int* cantidad) {
    *cantidad = s->numVehiculos;
    if (s->numVehiculos == 0) return nullptr;

    Vehiculo** resultado = new Vehiculo*[s->numVehiculos];
    for (int i = 0; i < s->numVehiculos; ++i) {
        resultado[i] = &s->vehiculos[i];
    }
    return resultado;
}

bool actualizarVehiculo(SistemaTaller* s, int id, Vehiculo datos) {
    Vehiculo* v = buscarVehiculoPorID(s, id);
    if (!v) return false;
    if (datos.anio < 1950 || datos.anio > 2027) return false;

    strncpy(v->marca, datos.marca, sizeof(v->marca));
    strncpy(v->modelo, datos.modelo, sizeof(v->modelo));
    v->anio = datos.anio;
    strncpy(v->nombrePropietario, datos.nombrePropietario, sizeof(v->nombrePropietario));
    strncpy(v->cedulaPropietario, datos.cedulaPropietario, sizeof(v->cedulaPropietario));
    strncpy(v->telefonoPropietario, datos.telefonoPropietario, sizeof(v->telefonoPropietario));

    return true;
}

ResultadoOperacion eliminarVehiculo(SistemaTaller* s, int id) {
    int idx = -1;
    for (int i = 0; i < s->numVehiculos; ++i) {
        if (s->vehiculos[i].id == id) { idx = i; break; }
    }
    if (idx == -1) return ERROR_NO_ENCONTRADO;

    for (int i = 0; i < s->numOrdenes; ++i) {
        if (s->ordenes[i].idVehiculo == id) return ERROR_TIENE_ORDENES;
    }

    for (int i = idx; i < s->numVehiculos - 1; ++i) {
        s->vehiculos[i] = s->vehiculos[i + 1];
    }
    s->numVehiculos--;
    return EXITO;
}

Mecanico* agregarMecanico(SistemaTaller* s, Mecanico datos) {
    if (buscarMecanicoPorCedula(s, datos.cedula) != nullptr) return nullptr;
    if (strcmp(datos.especialidad, "MOTOR") != 0 && strcmp(datos.especialidad, "FRENOS") != 0 &&
        strcmp(datos.especialidad, "ELECTRICIDAD") != 0 && strcmp(datos.especialidad, "SUSPENSION") != 0 &&
        strcmp(datos.especialidad, "GENERAL") != 0) return nullptr;

    if (s->numMecanicos == s->capacidadMecanicos) {
        int nuevaCap = s->capacidadMecanicos * 2;
        Mecanico* temp = new Mecanico[nuevaCap];
        for (int i = 0; i < s->numMecanicos; ++i) temp[i] = s->mecanicos[i];
        delete[] s->mecanicos;
        s->mecanicos = temp;
        s->capacidadMecanicos = nuevaCap;
    }

    datos.id = s->siguienteIdMecanico++;
    s->mecanicos[s->numMecanicos] = datos;
    return &s->mecanicos[s->numMecanicos++];
}

Mecanico* buscarMecanicoPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numMecanicos; ++i) {
        if (s->mecanicos[i].id == id) return &s->mecanicos[i];
    }
    return nullptr;
}

Mecanico* buscarMecanicoPorCedula(SistemaTaller* s, const char* cedula) {
    for (int i = 0; i < s->numMecanicos; ++i) {
        if (strcmp(s->mecanicos[i].cedula, cedula) == 0) return &s->mecanicos[i];
    }
    return nullptr;
}

Mecanico** listarMecanicos(SistemaTaller* s, int* cantidad) {
    *cantidad = s->numMecanicos;
    if (s->numMecanicos == 0) return nullptr;

    Mecanico** res = new Mecanico*[s->numMecanicos];
    for (int i = 0; i < s->numMecanicos; ++i) res[i] = &s->mecanicos[i];
    return res;
}

int contarOrdenesAbiertasMecanico(SistemaTaller* s, int idMecanico) {
    int cont = 0;
    for (int i = 0; i < s->numOrdenes; ++i) {
        if (s->ordenes[i].idMecanico == idMecanico && strcmp(s->ordenes[i].estado, "ABIERTA") == 0) {
            cont++;
        }
    }
    return cont;
}

Repuesto* agregarRepuesto(SistemaTaller* s, Repuesto datos) {
    if (datos.precio <= 0 || datos.stock < 0 || datos.stockMinimo < 0) return nullptr;
    if (buscarRepuestoPorCodigo(s, datos.codigo) != nullptr) return nullptr;

    if (s->numRepuestos == s->capacidadRepuestos) {
        int nuevaCap = s->capacidadRepuestos * 2;
        Repuesto* temp = new Repuesto[nuevaCap];
        for (int i = 0; i < s->numRepuestos; ++i) temp[i] = s->repuestos[i];
        delete[] s->repuestos;
        s->repuestos = temp;
        s->capacidadRepuestos = nuevaCap;
    }

    datos.id = s->siguienteIdRepuesto++;
    s->repuestos[s->numRepuestos] = datos;
    return &s->repuestos[s->numRepuestos++];
}

Repuesto* buscarRepuestoPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numRepuestos; ++i) {
        if (s->repuestos[i].id == id) return &s->repuestos[i];
    }
    return nullptr;
}

Repuesto* buscarRepuestoPorCodigo(SistemaTaller* s, const char* codigo) {
    for (int i = 0; i < s->numRepuestos; ++i) {
        if (strcmp(s->repuestos[i].codigo, codigo) == 0) return &s->repuestos[i];
    }
    return nullptr;
}

Repuesto** buscarRepuestosPorNombreOCodigo(SistemaTaller* s, const char* subcadena, int* cantidad) {
    int count = 0;
    for (int i = 0; i < s->numRepuestos; ++i) {
        if (contieneSubcadena(s->repuestos[i].nombre, subcadena) || contieneSubcadena(s->repuestos[i].codigo, subcadena)) {
            count++;
        }
    }
    *cantidad = count;
    if (count == 0) return nullptr;

    Repuesto** res = new Repuesto*[count];
    int idx = 0;
    for (int i = 0; i < s->numRepuestos; ++i) {
        if (contieneSubcadena(s->repuestos[i].nombre, subcadena) || contieneSubcadena(s->repuestos[i].codigo, subcadena)) {
            res[idx++] = &s->repuestos[i];
        }
    }
    return res;
}

Repuesto** listarRepuestos(SistemaTaller* s, int* cantidad) {
    *cantidad = s->numRepuestos;
    if (s->numRepuestos == 0) return nullptr;

    Repuesto** res = new Repuesto*[s->numRepuestos];
    for (int i = 0; i < s->numRepuestos; ++i) res[i] = &s->repuestos[i];
    return res;
}

bool registrarEntradaMercancia(SistemaTaller* s, int id, int cantidad) {
    if (cantidad <= 0) return false;
    Repuesto* r = buscarRepuestoPorID(s, id);
    if (!r) return false;
    r->stock += cantidad;
    return true;
}

bool cambiarPrecioRepuesto(SistemaTaller* s, int id, float nuevoPrecio) {
    if (nuevoPrecio <= 0) return false;
    Repuesto* r = buscarRepuestoPorID(s, id);
    if (!r) return false;
    r->precio = nuevoPrecio;
    return true;
}

ResultadoOperacion eliminarRepuesto(SistemaTaller* s, int id) {
    int idx = -1;
    for (int i = 0; i < s->numRepuestos; ++i) {
        if (s->repuestos[i].id == id) { idx = i; break; }
    }
    if (idx == -1) return ERROR_NO_ENCONTRADO;

    for (int i = 0; i < s->numOrdenes; ++i) {
        for (int j = 0; j < s->ordenes[i].numDetalles; ++j) {
            if (s->ordenes[i].detalles[j].idRepuesto == id) return ERROR_TIENE_ORDENES;
        }
    }

    for (int i = idx; i < s->numRepuestos - 1; ++i) {
        s->repuestos[i] = s->repuestos[i + 1];
    }
    s->numRepuestos--;
    return EXITO;
}

OrdenServicio* abrirOrden(SistemaTaller* s, int idVehiculo, int idMecanico, const char* fechaApertura, const char* descripcionFalla, int kilometrajeIngreso, ResultadoOperacion* resultado) {
    Vehiculo* v = buscarVehiculoPorID(s, idVehiculo);
    if (!v) { *resultado = ERROR_NO_ENCONTRADO; return nullptr; }

    Mecanico* m = buscarMecanicoPorID(s, idMecanico);
    if (!m) { *resultado = ERROR_NO_ENCONTRADO; return nullptr; }

    for (int i = 0; i < s->numOrdenes; ++i) {
        if (s->ordenes[i].idVehiculo == idVehiculo && strcmp(s->ordenes[i].estado, "ABIERTA") == 0) {
            *resultado = ERROR_DUPLICADO; return nullptr;
        }
    }

    if (contarOrdenesAbiertasMecanico(s, idMecanico) >= 3) {
        *resultado = ERROR_LIMITE_ALCANZADO; return nullptr;
    }

    if (kilometrajeIngreso < v->kilometraje) {
        *resultado = ERROR_DATOS_INVALIDOS; return nullptr;
    }

    v->kilometraje = kilometrajeIngreso;

    if (s->numOrdenes == s->capacidadOrdenes) {
        int nuevaCap = s->capacidadOrdenes * 2;
        OrdenServicio* temp = new OrdenServicio[nuevaCap];
        for (int i = 0; i < s->numOrdenes; ++i) temp[i] = s->ordenes[i];
        delete[] s->ordenes;
        s->ordenes = temp;
        s->capacidadOrdenes = nuevaCap;
    }

    OrdenServicio& o = s->ordenes[s->numOrdenes];
    o.id = s->siguienteIdOrden++;
    o.idVehiculo = idVehiculo;
    o.idMecanico = idMecanico;
    strncpy(o.fechaApertura, fechaApertura, sizeof(o.fechaApertura));
    o.fechaCierre[0] = '\0';
    strcpy(o.estado, "ABIERTA");
    strncpy(o.descripcionFalla, descripcionFalla, sizeof(o.descripcionFalla));
    o.kilometrajeIngreso = kilometrajeIngreso;
    o.manoObra = 0.0f;

    o.capacidadDetalles = 2;
    o.numDetalles = 0;
    o.detalles = new DetalleOrden[o.capacidadDetalles];

    *resultado = EXITO;
    return &s->ordenes[s->numOrdenes++];
}

ResultadoOperacion agregarRepuestoAOrden(SistemaTaller* s, int idOrden, int idRepuesto, int cantidad) {
    if (cantidad <= 0) return ERROR_DATOS_INVALIDOS;

    OrdenServicio* o = buscarOrdenPorID(s, idOrden);
    if (!o) return ERROR_NO_ENCONTRADO;
    if (strcmp(o->estado, "ABIERTA") != 0) return ERROR_ORDEN_CERRADA_O_ANULADA;

    Repuesto* r = buscarRepuestoPorID(s, idRepuesto);
    if (!r) return ERROR_NO_ENCONTRADO;
    if (r->stock < cantidad) return ERROR_STOCK_INSUFFICIENTE;

    for (int i = 0; i < o->numDetalles; ++i) {
        if (o->detalles[i].idRepuesto == idRepuesto) {
            o->detalles[i].cantidad += cantidad;
            r->stock -= cantidad;
            return EXITO;
        }
    }

    if (o->numDetalles == o->capacidadDetalles) {
        int nuevaCap = o->capacidadDetalles * 2;
        DetalleOrden* temp = new DetalleOrden[nuevaCap];
        for (int i = 0; i < o->numDetalles; ++i) temp[i] = o->detalles[i];
        delete[] o->detalles;
        o->detalles = temp;
        o->capacidadDetalles = nuevaCap;
    }

    o->detalles[o->numDetalles].idRepuesto = idRepuesto;
    o->detalles[o->numDetalles].cantidad = cantidad;
    o->detalles[o->numDetalles].precioUnitario = r->precio;
    o->numDetalles++;

    r->stock -= cantidad;
    return EXITO;
}

ResultadoOperacion quitarRepuestoDeOrden(SistemaTaller* s, int idOrden, int idRepuesto) {
    OrdenServicio* o = buscarOrdenPorID(s, idOrden);
    if (!o) return ERROR_NO_ENCONTRADO;
    if (strcmp(o->estado, "ABIERTA") != 0) return ERROR_ORDEN_CERRADA_O_ANULADA;

    int idx = -1;
    for (int i = 0; i < o->numDetalles; ++i) {
        if (o->detalles[i].idRepuesto == idRepuesto) { idx = i; break; }
    }
    if (idx == -1) return ERROR_NO_ENCONTRADO;

    Repuesto* r = buscarRepuestoPorID(s, idRepuesto);
    if (r) {
        r->stock += o->detalles[idx].cantidad;
    }

    for (int i = idx; i < o->numDetalles - 1; ++i) {
        o->detalles[i] = o->detalles[i + 1];
    }
    o->numDetalles--;
    return EXITO;
}

ResultadoOperacion cerrarOrden(SistemaTaller* s, int idOrden, float manoObra, const char* fechaCierre) {
    if (manoObra < 0) return ERROR_DATOS_INVALIDOS;

    OrdenServicio* o = buscarOrdenPorID(s, idOrden);
    if (!o) return ERROR_NO_ENCONTRADO;
    if (strcmp(o->estado, "ABIERTA") != 0) return ERROR_ORDEN_CERRADA_O_ANULADA;

    if (o->numDetalles == 0 && manoObra == 0) return ERROR_DATOS_INVALIDOS;

    o->manoObra = manoObra;
    // CORREGIDO AQUÍ: se usa o->fechaCierre en lugar de o.fechaCierre
    strncpy(o->fechaCierre, fechaCierre, sizeof(o->fechaCierre));
    strcpy(o->estado, "CERRADA");
    return EXITO;
}

ResultadoOperacion anularOrden(SistemaTaller* s, int idOrden) {
    OrdenServicio* o = buscarOrdenPorID(s, idOrden);
    if (!o) return ERROR_NO_ENCONTRADO;
    if (strcmp(o->estado, "ANULADA") == 0) return ERROR_ORDEN_CERRADA_O_ANULADA;

    for (int i = 0; i < o->numDetalles; ++i) {
        Repuesto* r = buscarRepuestoPorID(s, o->detalles[i].idRepuesto);
        if (r) r->stock += o->detalles[i].cantidad;
    }

    strcpy(o->estado, "ANULADA");
    return EXITO;
}

float calcularTotalOrden(const OrdenServicio* orden) {
    float totalRepuestos = 0.0f;
    for (int i = 0; i < orden->numDetalles; ++i) {
        totalRepuestos += orden->detalles[i].cantidad * orden->detalles[i].precioUnitario;
    }
    return totalRepuestos + orden->manoObra;
}

OrdenServicio* buscarOrdenPorID(SistemaTaller* s, int id) {
    for (int i = 0; i < s->numOrdenes; ++i) {
        if (s->ordenes[i].id == id) return &s->ordenes[i];
    }
    return nullptr;
}

OrdenServicio** listarOrdenesPorEstado(SistemaTaller* s, const char* estado, int* cantidad) {
    int count = 0;
    for (int i = 0; i < s->numOrdenes; ++i) {
        if (strcmp(s->ordenes[i].estado, estado) == 0) count++;
    }
    *cantidad = count;
    if (count == 0) return nullptr;

    OrdenServicio** res = new OrdenServicio*[count];
    int idx = 0;
    for (int i = 0; i < s->numOrdenes; ++i) {
        if (strcmp(s->ordenes[i].estado, estado) == 0) res[idx++] = &s->ordenes[i];
    }
    return res;
}

OrdenServicio** listarOrdenesPorVehiculo(SistemaTaller* s, int idVehiculo, int* cantidad) {
    int count = 0;
    for (int i = 0; i < s->numOrdenes; ++i) {
        if (s->ordenes[i].idVehiculo == idVehiculo) count++;
    }
    *cantidad = count;
    if (count == 0) return nullptr;

    OrdenServicio** res = new OrdenServicio*[count];
    int idx = 0;
    for (int i = 0; i < s->numOrdenes; ++i) {
        if (s->ordenes[i].idVehiculo == idVehiculo) res[idx++] = &s->ordenes[i];
    }
    return res;
}

OrdenServicio** listarTodasOrdenes(SistemaTaller* s, int* cantidad) {
    *cantidad = s->numOrdenes;
    if (s->numOrdenes == 0) return nullptr;

    OrdenServicio** res = new OrdenServicio*[s->numOrdenes];
    for (int i = 0; i < s->numOrdenes; ++i) res[i] = &s->ordenes[i];
    return res;
}