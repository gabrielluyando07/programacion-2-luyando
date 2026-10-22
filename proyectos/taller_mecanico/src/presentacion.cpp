#include "../include/presentacion.h"
#include "../include/logica.h"
#include "../include/estructuras.h"
#include <iostream>
#include <iomanip>
#include <limits>
#include <cstring>

using namespace std;

static bool solicitarConfirmacion(const char* mensaje) {
    char resp;
    cout << mensaje << " (S/N): ";
    cin >> resp;
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
    return (resp == 'S' || resp == 's');
}

static void leerCadena(const char* prompt, char* destino, int tam) {
    cout << prompt;
    cin.getline(destino, tam);
}

static int leerInt(const char* prompt) {
    int val;
    while (true) {
        cout << prompt;
        if (cin >> val) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return val;
        }
        cout << "ERROR: Entrada inválida. Ingrese un número entero." << endl;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

static float leerFloat(const char* prompt) {
    float val;
    while (true) {
        cout << prompt;
        if (cin >> val) {
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return val;
        }
        cout << "ERROR: Entrada inválida. Ingrese un número válido." << endl;
        cin.clear();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
}

void mostrarVehiculo(const Vehiculo* v) {
    cout << left << setw(4) << v->id << " | "
         << setw(10) << v->placa << " | "
         << setw(18) << (string(v->marca) + " " + v->modelo) << " | "
         << setw(5) << v->anio << " | "
         << setw(8) << v->kilometraje << " | "
         << v->nombrePropietario << endl;
}

void mostrarListaVehiculos(Vehiculo** vehiculos, int cantidad) {
    if (!vehiculos || cantidad == 0) {
        cout << "No hay vehículos para mostrar." << endl;
        return;
    }
    cout << left << setw(4) << "ID" << " | "
         << setw(10) << "Placa" << " | "
         << setw(18) << "Vehículo" << " | "
         << setw(5) << "Año" << " | "
         << setw(8) << "Km" << " | "
         << "Propietario" << endl;
    cout << string(65, '-') << endl;
    for (int i = 0; i < cantidad; ++i) {
        mostrarVehiculo(vehiculos[i]);
    }
    cout << "Total de vehículos: " << cantidad << endl;
}

void mostrarDetalleOrden(SistemaTaller* s, const OrdenServicio* orden) {
    Vehiculo* v = buscarVehiculoPorID(s, orden->idVehiculo);
    Mecanico* m = buscarMecanicoPorID(s, orden->idMecanico);

    cout << "\n==============================================================" << endl;
    cout << "                    ORDEN DE SERVICIO #" << orden->id << endl;
    cout << "==============================================================" << endl;
    cout << "Estado     : " << left << setw(15) << orden->estado 
         << "Apertura: " << orden->fechaApertura << endl;
    cout << "              " << setw(15) << " " 
         << "Cierre  : " << (strlen(orden->fechaCierre) > 0 ? orden->fechaCierre : "N/A") << endl;
    cout << "Vehiculo   : " << (v ? v->placa : "N/A") << " - " 
         << (v ? v->marca : "") << " " << (v ? v->modelo : "") << " " 
         << (v ? to_string(v->anio) : "") << " (" << orden->kilometrajeIngreso << " km)" << endl;
    cout << "Dueño      : " << (v ? v->nombrePropietario : "N/A") << endl;
    cout << "Mecanico   : " << (m ? m->nombre : "N/A") << " (" << (m ? m->especialidad : "") << ")" << endl;
    cout << "Falla      : " << orden->descripcionFalla << endl;
    cout << "--------------------------------------------------------------" << endl;
    cout << left << setw(10) << "Código" << " | " 
         << setw(25) << "Repuesto" << " | " 
         << right << setw(5) << "Cant" << " | " 
         << setw(8) << "P.Unit" << " | " 
         << setw(8) << "Total" << endl;
    cout << "--------------------------------------------------------------" << endl;

    float totalRepuestos = 0.0f;
    for (int i = 0; i < orden->numDetalles; ++i) {
        Repuesto* r = buscarRepuestoPorID(s, orden->detalles[i].idRepuesto);
        float subtotal = orden->detalles[i].cantidad * orden->detalles[i].precioUnitario;
        totalRepuestos += subtotal;

        cout << left << setw(10) << (r ? r->codigo : "N/A") << " | "
             << setw(25) << (r ? r->nombre : "N/A") << " | "
             << right << setw(5) << orden->detalles[i].cantidad << " | "
             << setw(8) << fixed << setprecision(2) << orden->detalles[i].precioUnitario << " | "
             << setw(8) << subtotal << endl;
    }
    cout << "--------------------------------------------------------------" << endl;
    cout << right << setw(48) << "Repuestos   : " << setw(8) << fixed << setprecision(2) << totalRepuestos << endl;
    cout << right << setw(48) << "Mano de obra: " << setw(8) << orden->manoObra << endl;
    cout << right << setw(48) << "TOTAL       : " << setw(8) << calcularTotalOrden(orden) << endl;
    cout << "==============================================================\n" << endl;
}

void menuVehiculos(SistemaTaller* s) {
    int opc;
    do {
        cout << "\n--- GESTION DE VEHICULOS ---" << endl;
        cout << "1. Registrar vehiculo" << endl;
        cout << "2. Buscar vehiculo por ID / Placa" << endl;
        cout << "3. Buscar vehiculo por Propietario" << endl;
        cout << "4. Actualizar vehiculo" << endl;
        cout << "5. Eliminar vehiculo" << endl;
        cout << "6. Listar vehiculos" << endl;
        cout << "0. Volver al menu principal" << endl;
        opc = leerInt("Seleccione una opción: ");

        if (opc == 1) {
            Vehiculo v;
            leerCadena("Ingrese placa: ", v.placa, sizeof(v.placa));
            if (strcmp(v.placa, "CANCELAR") == 0) { cout << "Operación cancelada." << endl; continue; }

            if (buscarVehiculoPorPlaca(s, v.placa) != nullptr) {
                cout << "ERROR: La placa '" << v.placa << "' ya está registrada." << endl;
                continue;
            }

            leerCadena("Ingrese marca: ", v.marca, sizeof(v.marca));
            if (strcmp(v.marca, "CANCELAR") == 0) { cout << "Operación cancelada." << endl; continue; }

            leerCadena("Ingrese modelo: ", v.modelo, sizeof(v.modelo));
            v.anio = leerInt("Ingrese año (1950-2027): ");
            v.kilometraje = leerInt("Ingrese kilometraje: ");
            leerCadena("Ingrese nombre del propietario: ", v.nombrePropietario, sizeof(v.nombrePropietario));
            leerCadena("Ingrese cédula del propietario: ", v.cedulaPropietario, sizeof(v.cedulaPropietario));
            leerCadena("Ingrese teléfono del propietario: ", v.telefonoPropietario, sizeof(v.telefonoPropietario));
            leerCadena("Ingrese fecha de registro (YYYY-MM-DD): ", v.fechaRegistro, sizeof(v.fechaRegistro));

            if (solicitarConfirmacion("¿Desea registrar este vehículo?")) {
                if (agregarVehiculo(s, v)) cout << "Vehículo registrado con éxito." << endl;
                else cout << "ERROR: No se pudo registrar el vehículo (datos inválidos)." << endl;
            } else cout << "Registro cancelado." << endl;

        } else if (opc == 3) {
            char sub[100];
            leerCadena("Ingrese nombre o parte del nombre del propietario: ", sub, sizeof(sub));
            int cant;
            Vehiculo** res = buscarVehiculosPorPropietario(s, sub, &cant);
            mostrarListaVehiculos(res, cant);
            delete[] res;

        } else if (opc == 5) {
            int id = leerInt("Ingrese ID del vehículo a eliminar: ");
            if (solicitarConfirmacion("¿Está seguro de eliminar el vehículo?")) {
                ResultadoOperacion res = eliminarVehiculo(s, id);
                if (res == EXITO) cout << "Vehículo eliminado con éxito." << endl;
                else if (res == ERROR_TIENE_ORDENES) cout << "ERROR: El vehículo tiene órdenes registradas y no puede eliminarse." << endl;
                else cout << "ERROR: Vehículo no encontrado." << endl;
            }

        } else if (opc == 6) {
            int cant;
            Vehiculo** res = listarVehiculos(s, &cant);
            mostrarListaVehiculos(res, cant);
            delete[] res;
        }
    } while (opc != 0);
}

void menuMecanicos(SistemaTaller* s) {
    int opc;
    do {
        cout << "\n--- GESTION DE MECANICOS ---" << endl;
        cout << "1. Registrar mecanico" << endl;
        cout << "2. Listar mecanicos" << endl;
        cout << "0. Volver" << endl;
        opc = leerInt("Seleccione una opcion: ");

        if (opc == 1) {
            Mecanico m;
            leerCadena("Ingrese cedula: ", m.cedula, sizeof(m.cedula));
            leerCadena("Ingrese nombre: ", m.nombre, sizeof(m.nombre));
            leerCadena("Ingrese especialidad (MOTOR/FRENOS/ELECTRICIDAD/SUSPENSION/GENERAL): ", m.especialidad, sizeof(m.especialidad));
            leerCadena("Ingrese fecha de ingreso (YYYY-MM-DD): ", m.fechaIngreso, sizeof(m.fechaIngreso));

            if (solicitarConfirmacion("¿Guardar mecanico?")) {
                if (agregarMecanico(s, m)) cout << "Mecanico registrado exitosamente." << endl;
                else cout << "ERROR: Cedula duplicada o especialidad invalida." << endl;
            }

        } else if (opc == 2) {
            int cant;
            Mecanico** res = listarMecanicos(s, &cant);
            if (!res || cant == 0) cout << "No hay mecanicos registrados." << endl;
            else {
                cout << left << setw(4) << "ID" << " | " << setw(12) << "Cedula" << " | " << setw(20) << "Nombre" << " | " << setw(14) << "Especialidad" << " | Ordenes Abiertas" << endl;
                cout << string(65, '-') << endl;
                for (int i = 0; i < cant; ++i) {
                    int abiertas = contarOrdenesAbiertasMecanico(s, res[i]->id);
                    cout << left << setw(4) << res[i]->id << " | " << setw(12) << res[i]->cedula << " | " << setw(20) << res[i]->nombre << " | " << setw(14) << res[i]->especialidad << " | " << abiertas << endl;
                }
            }
            delete[] res;
        }
    } while (opc != 0);
}

void menuRepuestos(SistemaTaller* s) {
    int opc;
    do {
        cout << "\n--- INVENTARIO DE REPUESTOS ---" << endl;
        cout << "1. Registrar repuesto" << endl;
        cout << "2. Registrar entrada de mercancia" << endl;
        cout << "3. Cambiar precio" << endl;
        cout << "4. Listar repuestos" << endl;
        cout << "5. Eliminar repuesto" << endl;
        cout << "0. Volver" << endl;
        opc = leerInt("Seleccione una opcion: ");

        if (opc == 1) {
            Repuesto r;
            leerCadena("Ingrese codigo: ", r.codigo, sizeof(r.codigo));
            leerCadena("Ingrese nombre: ", r.nombre, sizeof(r.nombre));
            leerCadena("Ingrese marca: ", r.marca, sizeof(r.marca));
            r.precio = leerFloat("Ingrese precio: ");
            r.stock = leerInt("Ingrese stock inicial: ");
            r.stockMinimo = leerInt("Ingrese stock minimo: ");
            leerCadena("Ingrese fecha de registro (YYYY-MM-DD): ", r.fechaRegistro, sizeof(r.fechaRegistro));

            if (solicitarConfirmacion("¿Registrar repuesto?")) {
                if (agregarRepuesto(s, r)) cout << "Repuesto registrado." << endl;
                else cout << "ERROR: Codigo duplicado o datos invalidos." << endl;
            }

        } else if (opc == 2) {
            int id = leerInt("Ingrese ID del repuesto: ");
            int cant = leerInt("Ingrese cantidad que ingresa: ");
            if (registrarEntradaMercancia(s, id, cant)) cout << "Mercancia ingresada con exito." << endl;
            else cout << "ERROR: Repuesto no encontrado o cantidad invalida." << endl;

        } else if (opc == 3) {
            int id = leerInt("Ingrese ID del repuesto: ");
            float np = leerFloat("Ingrese nuevo precio: ");
            if (cambiarPrecioRepuesto(s, id, np)) cout << "Precio actualizado." << endl;
            else cout << "ERROR: Repuesto no encontrado o precio invalido." << endl;

        } else if (opc == 4) {
            int cant;
            Repuesto** res = listarRepuestos(s, &cant);
            if (!res || cant == 0) cout << "No hay repuestos registrados." << endl;
            else {
                cout << left << setw(4) << "ID" << " | " << setw(10) << "Codigo" << " | " << setw(25) << "Nombre" << " | " << setw(8) << "Precio" << " | " << setw(6) << "Stock" << " | Alerta" << endl;
                cout << string(70, '-') << endl;
                for (int i = 0; i < cant; ++i) {
                    string alerta = (res[i]->stock <= res[i]->stockMinimo) ? "BAJO STOCK" : "OK";
                    cout << left << setw(4) << res[i]->id << " | " << setw(10) << res[i]->codigo << " | " << setw(25) << res[i]->nombre << " | " << setw(8) << fixed << setprecision(2) << res[i]->precio << " | " << setw(6) << res[i]->stock << " | " << alerta << endl;
                }
            }
            delete[] res;

        } else if (opc == 5) {
            int id = leerInt("Ingrese ID de repuesto a eliminar: ");
            if (solicitarConfirmacion("¿Eliminar repuesto?")) {
                ResultadoOperacion res = eliminarRepuesto(s, id);
                if (res == EXITO) cout << "Repuesto eliminado." << endl;
                else if (res == ERROR_TIENE_ORDENES) cout << "ERROR: El repuesto esta registrado en ordenes y no puede eliminarse." << endl;
                else cout << "ERROR: Repuesto no encontrado." << endl;
            }
        }
    } while (opc != 0);
}

void menuOrdenes(SistemaTaller* s) {
    int opc;
    do {
        cout << "\n--- ORDENES DE SERVICIO ---" << endl;
        cout << "1. Abrir orden" << endl;
        cout << "2. Agregar repuesto a orden" << endl;
        cout << "3. Quitar repuesto de orden" << endl;
        cout << "4. Cerrar orden" << endl;
        cout << "5. Anular orden" << endl;
        cout << "6. Ver detalle de orden" << endl;
        cout << "7. Listar órdenes" << endl;
        cout << "0. Volver al menu principal" << endl;
        opc = leerInt("Seleccione una opción: ");

        if (opc == 1) {
            int idV = leerInt("ID Vehiculo: ");
            int idM = leerInt("ID Mecanico: ");
            char fecha[11], falla[200];
            leerCadena("Fecha apertura (YYYY-MM-DD): ", fecha, sizeof(fecha));
            leerCadena("Descripción de falla: ", falla, sizeof(falla));
            int km = leerInt("Kilometraje de ingreso: ");

            ResultadoOperacion res;
            OrdenServicio* o = abrirOrden(s, idV, idM, fecha, falla, km, &res);
            if (o) cout << "Orden de Servicio #" << o->id << " abierta con éxito." << endl;
            else {
                if (res == ERROR_DUPLICADO) cout << "ERROR: El vehiculo ya tiene una orden ABIERTA." << endl;
                else if (res == ERROR_LIMITE_ALCANZADO) cout << "ERROR: El mecanico ya tiene 3 ordenes abiertas." << endl;
                else if (res == ERROR_DATOS_INVALIDOS) cout << "ERROR: Kilometraje menor al actual del vehiculo." << endl;
                else cout << "ERROR: Vehiculo o Mecanico no encontrado." << endl;
            }

        } else if (opc == 2) {
            int idO = leerInt("ID Orden: ");
            int idR = leerInt("ID Repuesto: ");
            int cant = leerInt("Cantidad: ");
            ResultadoOperacion res = agregarRepuestoAOrden(s, idO, idR, cant);
            if (res == EXITO) cout << "Repuesto agregado a la orden." << endl;
            else if (res == ERROR_STOCK_INSUFICIENTE) cout << "ERROR: Stock insuficiente." << endl;
            else cout << "ERROR: Orden o repuesto no encontrado / orden no abierta." << endl;

        } else if (opc == 3) {
            int idO = leerInt("ID Orden: ");
            int idR = leerInt("ID Repuesto: ");
            ResultadoOperacion res = quitarRepuestoDeOrden(s, idO, idR);
            if (res == EXITO) cout << "Repuesto quitado de la orden y reincorporado al stock." << endl;
            else cout << "ERROR: Operacion fallida." << endl;

        } else if (opc == 4) {
            int idO = leerInt("ID Orden: ");
            float mo = leerFloat("Monto mano de obra: ");
            char fecha[11];
            leerCadena("Fecha cierre (YYYY-MM-DD): ", fecha, sizeof(fecha));
            if (solicitarConfirmacion("¿Cerrar orden?")) {
                ResultadoOperacion res = cerrarOrden(s, idO, mo, fecha);
                if (res == EXITO) cout << "Orden CERRADA exitosamente." << endl;
                else cout << "ERROR: No se pudo cerrar la orden." << endl;
            }

        } else if (opc == 5) {
            int idO = leerInt("ID Orden: ");
            if (solicitarConfirmacion("¿Está seguro de anular la orden?")) {
                ResultadoOperacion res = anularOrden(s, idO);
                if (res == EXITO) cout << "Orden ANULADA y stock revertido." << endl;
                else cout << "ERROR: No se pudo anular." << endl;
            }

        } else if (opc == 6) {
            int idO = leerInt("ID Orden: ");
            OrdenServicio* o = buscarOrdenPorID(s, idO);
            if (o) mostrarDetalleOrden(s, o);
            else cout << "ERROR: Orden no encontrada." << endl;

        } else if (opc == 7) {
            int cant;
            OrdenServicio** list = listarTodasOrdenes(s, &cant);
            if (!list || cant == 0) cout << "No hay ordenes registradas." << endl;
            else {
                for (int i = 0; i < cant; ++i) mostrarDetalleOrden(s, list[i]);
            }
            delete[] list;
        }
    } while (opc != 0);
}

void menuPrincipal(SistemaTaller* s) {
    int opc;
    do {
        cout << "\n==========================================" << endl;
        cout << "      SISTEMA DE GESTION DE TALLER        " << endl;
        cout << " Taller: " << s->taller.nombre << endl;
        cout << "==========================================" << endl;
        cout << "1. Gestion de Vehiculos" << endl;
        cout << "2. Gestion de Mecanicos" << endl;
        cout << "3. Inventario de Repuestos" << endl;
        cout << "4. Ordenes de Servicio" << endl;
        cout << "0. Salir" << endl;
        opc = leerInt("Seleccione una opcion: ");

        switch (opc) {
            case 1: menuVehiculos(s); break;
            case 2: menuMecanicos(s); break;
            case 3: menuRepuestos(s); break;
            case 4: menuOrdenes(s); break;
            case 0:
                if (!solicitarConfirmacion("¿Desea salir del programa? (Los datos no guardados se perderan)")) {
                    opc = -1;
                }
                break;
            default: cout << "Opcion invalida." << endl;
        }
    } while (opc != 0);
}