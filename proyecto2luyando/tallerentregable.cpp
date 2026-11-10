// =====================================================================
//  PROYECTO 2: TALLER MECANICO CON PERSISTENCIA Y ACCESO ALEATORIO
//  Programacion 2 - 2026-C
//
//  Compilar:  g++ -std=c++17 -Wall -o taller taller.cpp
//  Ejecutar:  ./taller           
//            
// =====================================================================

#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include <ctime>
#include <filesystem>

using namespace std;
namespace fs = std::filesystem;

// =====================================================================
// PARTE 0: CONSTANTES, ESTRUCTURAS Y UTILIDADES
// =====================================================================


const char* const CARPETA_DATOS  = "datos";
const char* const RUTA_TALLER    = "datos/taller.bin";
const char* const RUTA_VEHICULOS = "datos/vehiculos.bin";
const char* const RUTA_MECANICOS = "datos/mecanicos.bin";
const char* const RUTA_REPUESTOS = "datos/repuestos.bin";
const char* const RUTA_ORDENES   = "datos/ordenes.bin";


const int CANTIDAD_ARCHIVOS = 5;
const char* const TODOS_LOS_ARCHIVOS[CANTIDAD_ARCHIVOS] = {
    RUTA_TALLER, RUTA_VEHICULOS, RUTA_MECANICOS, RUTA_REPUESTOS, RUTA_ORDENES
};

// ---------- Limites del sistema ----------
const int MAX_LINEAS           = 20;  
const int MAX_RESULTADOS       = 50;  
const int MAX_ORDENES_ABIERTAS = 3;   
const int TOP_REPUESTOS        = 10;  
const int MAX_PROBLEMAS        = 100; 
const int ANIO_MINIMO          = 1950;
const int ANIO_MAXIMO          = 2027;

const int CANTIDAD_ESPECIALIDADES = 5;
const char* const ESPECIALIDADES[CANTIDAD_ESPECIALIDADES] = {
    "MOTOR", "FRENOS", "ELECTRICIDAD", "SUSPENSION", "GENERAL"
};

// ---------- Estados de una orden ----------
const char* const ESTADO_ABIERTA = "ABIERTA";
const char* const ESTADO_CERRADA = "CERRADA";
const char* const ESTADO_ANULADA = "ANULADA";

// ---------- Header de cada archivo de entidades ----------
struct ArchivoHeader {
    int cantidadRegistros; 
    int proximoID;          
    int registrosActivos;  
    int version;           
};

// ---------- Estructuras que se guardan en disco ----------


struct Taller {
    char   nombre[100];
    char   rif[20];
    char   direccion[200];
    char   telefono[20];
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;
};

struct Vehiculo {
    int    id;
    char   placa[10];
    char   marca[50];
    char   modelo[50];
    int    anio;
    int    kilometraje;
    char   nombrePropietario[100];
    char   cedulaPropietario[20];
    char   telefonoPropietario[20];

    bool   eliminado;
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;
};

struct Mecanico {
    int    id;
    char   nombre[100];
    char   cedula[20];
    char   especialidad[20];
    char   fechaIngreso[11];

    bool   eliminado;          // true = dado de baja
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;
};

struct Repuesto {
    int    id;
    char   codigo[20];
    char   nombre[100];
    char   marca[50];
    float  precio;
    int    stock;
    int    stockMinimo;

    bool   eliminado;
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;
};


struct LineaDetalle {
    int   idRepuesto;
    int   cantidad;
    float precioUnitario;   // precio historico: el que tenia el repuesto al agregarlo
};

struct OrdenServicio {
    int    id;
    int    idVehiculo;
    int    idMecanico;
    char   fechaApertura[11];
    char   fechaCierre[11];
    char   estado[12];           // "ABIERTA", "CERRADA" o "ANULADA"
    char   descripcionFalla[200];
    int    kilometrajeIngreso;
    float  manoObra;

    // ---- DETALLE DE LA ORDEN ----
    int          numLineas;              
    LineaDetalle lineas[MAX_LINEAS];     

    bool   eliminado;           
    time_t fechaCreacion;
    time_t fechaUltimaModificacion;
};


enum Resultado {
    OK = 0,

    // --- Errores de archivo (persistencia) ---
    ERR_ARCHIVO,                
    ERR_ARCHIVO_CORRUPTO,       
    ERR_NO_EXISTE,               
    ERR_ELIMINADO,               

    // --- Errores de validacion de datos ---
    ERR_CAMPO_VACIO,             
    ERR_PLACA_INVALIDA,
    ERR_PLACA_DUPLICADA,
    ERR_ANIO_INVALIDO,
    ERR_KM_INVALIDO,
    ERR_CEDULA_INVALIDA,
    ERR_CEDULA_DUPLICADA,
    ERR_ESPECIALIDAD_INVALIDA,
    ERR_FECHA_INVALIDA,
    ERR_CODIGO_DUPLICADO,
    ERR_PRECIO_INVALIDO,
    ERR_STOCK_INVALIDO,
    ERR_STOCK_MINIMO_INVALIDO,
    ERR_CANTIDAD_INVALIDA,
    ERR_MANO_OBRA_INVALIDA,

    // --- La entidad pedida no existe / esta eliminada ---
    ERR_VEHICULO_NO_EXISTE,
    ERR_VEHICULO_ELIMINADO,
    ERR_MECANICO_NO_EXISTE,
    ERR_MECANICO_ELIMINADO,
    ERR_REPUESTO_NO_EXISTE,
    ERR_REPUESTO_ELIMINADO,
    ERR_ORDEN_NO_EXISTE,

    // --- Reglas de negocio ---
    ERR_VEHICULO_CON_ORDENES,    
    ERR_MECANICO_CON_ABIERTAS,   
    ERR_REPUESTO_EN_USO,         
    ERR_VEHICULO_YA_TIENE_ABIERTA,
    ERR_MECANICO_LLENO,          
    ERR_KM_MENOR,
    ERR_ESTADO_INCORRECTO,       
    ERR_STOCK_INSUFICIENTE,
    ERR_LIMITE_LINEAS,           
    ERR_LINEA_NO_EXISTE,         
    ERR_ORDEN_VACIA              
};


struct InfoError {
    int  idRelacionado;   
    int  cantidad;        
    int  solicitado;      
    char texto[100];      

// ---------- Estructuras de apoyo para mostrar datos ----------


struct MecanicoVista {
    Mecanico mecanico;
    int      ordenesAbiertas;
};

struct OrdenVista {
    OrdenServicio orden;
    char  placa[10];
    char  marca[50];
    char  modelo[50];
    int   anio;
    char  nombrePropietario[100];
    char  nombreMecanico[100];
    char  especialidad[20];
    bool  mecanicoDeBaja;
    float totalRepuestos;
    float total;
};

struct LineaVista {
    char  codigo[20];
    char  nombre[100];
    int   cantidad;
    float precioUnitario;
    float subtotal;
};

struct RepuestoUso {         
    char codigo[20];
    char nombre[100];
    int  unidades;
};

struct FacturacionMecanico {  
    char  nombre[100];
    bool  dadoDeBaja;
    int   ordenesCerradas;
    float manoObra;
    float repuestos;
};

struct ReporteIntegridad {
    int  vehiculos, mecanicos, repuestos, ordenes;   
    int  problemas;                                  
    int  guardados;                                  
    char mensajes[MAX_PROBLEMAS][160];
};




void copiarTexto(char* destino, int tam, const char* origen) {
    int i = 0;
    while (i < tam - 1 && origen[i] != '\0') {
        destino[i] = origen[i];
        i++;
    }
    destino[i] = '\0';
}

bool textoVacio(const char* t) {
    while (*t != '\0') {
        if (!isspace((unsigned char)*t)) return false;
        t++;
    }
    return true;
}

void aMayusculas(char* t) {
    for (; *t != '\0'; t++) *t = (char)toupper((unsigned char)*t);
}


bool contieneIgnorandoMayusculas(const char* texto, const char* sub) {
    int n = (int)strlen(texto);
    int m = (int)strlen(sub);
    if (m == 0) return true;
    for (int i = 0; i + m <= n; i++) {
        bool igual = true;
        for (int j = 0; j < m; j++) {
            if (tolower((unsigned char)texto[i + j]) != tolower((unsigned char)sub[j])) {
                igual = false;
                break;
            }
        }
        if (igual) return true;
    }
    return false;
}


void fechaActual(char destino[11]) {
    time_t ahora = time(nullptr);
    tm* f = localtime(&ahora);
    char temporal[40];
    snprintf(temporal, sizeof(temporal), "%04d-%02d-%02d", (f->tm_year + 1900) % 10000, f->tm_mon + 1, f->tm_mday);
    copiarTexto(destino, 11, temporal);
}

bool esFechaValida(const char* f) {
    if (strlen(f) != 10) return false;
    for (int i = 0; i < 10; i++) {
        if (i == 4 || i == 7) {
            if (f[i] != '-') return false;
        } else if (!isdigit((unsigned char)f[i])) {
            return false;
        }
    }
    int anio = 0, mes = 0, dia = 0;
    sscanf(f, "%d-%d-%d", &anio, &mes, &dia);
    if (anio < 1900 || anio > 2100) return false;
    if (mes < 1 || mes > 12) return false;
    int diasDelMes[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    bool bisiesto = (anio % 4 == 0 && anio % 100 != 0) || (anio % 400 == 0);
    int maximo = diasDelMes[mes - 1];
    if (mes == 2 && bisiesto) maximo = 29;
    return dia >= 1 && dia <= maximo;
}


bool esCedulaValida(const char* c) {
    int inicio = 0;
    if (strlen(c) >= 2 && c[1] == '-') {
        char p = (char)toupper((unsigned char)c[0]);
        if (p == 'V' || p == 'E' || p == 'J') inicio = 2;
    }
    if (c[inicio] == '\0') return false;   // debe haber al menos un digito
    for (int i = inicio; c[i] != '\0'; i++) {
        if (!isdigit((unsigned char)c[i])) return false;
    }
    return true;
}


bool esPlacaValida(const char* p) {
    int n = (int)strlen(p);
    if (n < 6 || n > 7) return false;
    for (int i = 0; i < n; i++) {
        if (!isalnum((unsigned char)p[i])) return false;
    }
    return true;
}

bool esEspecialidadValida(const char* e) {
    for (int i = 0; i < CANTIDAD_ESPECIALIDADES; i++) {
        if (strcmp(e, ESPECIALIDADES[i]) == 0) return true;
    }
    return false;
}

bool esAnioValido(int anio) {
    return anio >= ANIO_MINIMO && anio <= ANIO_MAXIMO;
}

// =====================================================================
// PARTE 1: CAPA DE PERSISTENCIA
// Solo lee y escribe registros "crudos" en los archivos binarios.
// No aplica reglas de negocio, no filtra eliminados y no usa cin/cout.
// =====================================================================


static long ultimaPosicionLeida = -1;   
static char rutaConError[200]   = "";   


Resultado errorArchivo(const char* ruta) {
    copiarTexto(rutaConError, 200, ruta);
    return ERR_ARCHIVO;
}


long posicionDeID(size_t tamRegistro, int id) {
    return (long)sizeof(ArchivoHeader) + (long)(id - 1) * (long)tamRegistro;
}


bool inicializarArchivo(const char* ruta) {
    {
        ifstream existente(ruta, ios::binary);
        if (existente) return true;
    }
    ofstream nuevo(ruta, ios::binary);
    if (!nuevo) {
        errorArchivo(ruta);
        return false;
    }
    ArchivoHeader h;
    h.cantidadRegistros = 0;
    h.proximoID = 1;
    h.registrosActivos = 0;
    h.version = 1;
    nuevo.write(reinterpret_cast<const char*>(&h), sizeof(h));
    if (!nuevo) {
        errorArchivo(ruta);
        return false;
    }
    return true;
}


bool leerHeader(const char* ruta, ArchivoHeader &h) {
    ifstream archivo(ruta, ios::binary);
    if (!archivo) {
        errorArchivo(ruta);
        return false;
    }
    archivo.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (!archivo) {
        errorArchivo(ruta);
        return false;
    }
    return true;
}


bool escribirHeader(const char* ruta, const ArchivoHeader &h) {
    fstream archivo(ruta, ios::in | ios::out | ios::binary);   // in|out para NO borrar el archivo
    if (!archivo) {
        errorArchivo(ruta);
        return false;
    }
    archivo.seekp(0);
    archivo.write(reinterpret_cast<const char*>(&h), sizeof(h));
    if (!archivo) {
        errorArchivo(ruta);
        return false;
    }
    return true;
}


bool contarRegistrosEnArchivo(const char* ruta, size_t tamRegistro, int &cantidad, long &sobrante) {
    ifstream archivo(ruta, ios::binary | ios::ate);   // ate: empieza al final
    if (!archivo) {
        errorArchivo(ruta);
        return false;
    }
    long tamano = (long)archivo.tellg();
    long util = tamano - (long)sizeof(ArchivoHeader);
    if (util < 0) util = 0;
    cantidad = (int)(util / (long)tamRegistro);
    sobrante = util % (long)tamRegistro;
    return true;
}


bool existeTaller() {
    ifstream archivo(RUTA_TALLER, ios::binary | ios::ate);
    if (!archivo) return false;
    return (long)archivo.tellg() >= (long)sizeof(Taller);
}

// taller.bin: un unico registro, SIN header
Resultado leerTaller(Taller &t) {
    ifstream archivo(RUTA_TALLER, ios::binary);
    if (!archivo) return errorArchivo(RUTA_TALLER);
    archivo.read(reinterpret_cast<char*>(&t), sizeof(Taller));
    if (!archivo) return errorArchivo(RUTA_TALLER);
    return OK;
}

Resultado guardarTaller(Taller &t) {
    t.fechaCreacion = time(nullptr);
    t.fechaUltimaModificacion = t.fechaCreacion;
    ofstream archivo(RUTA_TALLER, ios::binary);
    if (!archivo) return errorArchivo(RUTA_TALLER);
    archivo.write(reinterpret_cast<const char*>(&t), sizeof(Taller));
    if (!archivo) return errorArchivo(RUTA_TALLER);
    return OK;
}

bool crearCarpeta(const char* ruta) {
    error_code ec;
    fs::create_directories(ruta, ec);
    if (ec) {
        errorArchivo(ruta);
        return false;
    }
    return true;
}


bool copiarArchivo(const char* origen, const char* destino) {
    ifstream entrada(origen, ios::binary);
    if (!entrada) {
        errorArchivo(origen);
        return false;
    }
    ofstream salida(destino, ios::binary);
    if (!salida) {
        errorArchivo(destino);
        return false;
    }
    char bloque[4096];
    while (entrada.read(bloque, sizeof(bloque)) || entrada.gcount() > 0) {
        salida.write(bloque, entrada.gcount());
        if (!salida) {
            errorArchivo(destino);
            return false;
        }
    }
    return true;
}

// ---------- Las 5 operaciones por cada entidad ----------


// ---------- Vehiculo (archivo vehiculos.bin) ----------


Resultado guardarVehiculo(Vehiculo &x) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return ERR_ARCHIVO;
    if (h.proximoID != h.cantidadRegistros + 1) return ERR_ARCHIVO_CORRUPTO;

    x.id = h.proximoID;
    x.eliminado = false;
    x.fechaCreacion = time(nullptr);
    x.fechaUltimaModificacion = x.fechaCreacion;

    fstream archivo(RUTA_VEHICULOS, ios::in | ios::out | ios::binary);
    if (!archivo) return errorArchivo(RUTA_VEHICULOS);
    archivo.seekp(posicionDeID(sizeof(Vehiculo), x.id));
    archivo.write(reinterpret_cast<const char*>(&x), sizeof(Vehiculo));
    if (!archivo) return errorArchivo(RUTA_VEHICULOS);
    archivo.close();

    h.cantidadRegistros++;
    h.registrosActivos++;
    h.proximoID++;
    if (!escribirHeader(RUTA_VEHICULOS, h)) return ERR_ARCHIVO;
    return OK;
}


Resultado leerVehiculoPorID(int id, Vehiculo &x) {
    ifstream archivo(RUTA_VEHICULOS, ios::binary);
    if (!archivo) return errorArchivo(RUTA_VEHICULOS);
    ArchivoHeader h;
    archivo.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (!archivo) return errorArchivo(RUTA_VEHICULOS);
    if (id < 1 || id > h.cantidadRegistros) return ERR_NO_EXISTE;

    ultimaPosicionLeida = posicionDeID(sizeof(Vehiculo), id);
    archivo.seekg(ultimaPosicionLeida);
    archivo.read(reinterpret_cast<char*>(&x), sizeof(Vehiculo));
    if (!archivo) return errorArchivo(RUTA_VEHICULOS);
    return OK;
}


Resultado sobrescribirVehiculo(Vehiculo &x) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return ERR_ARCHIVO;
    if (x.id < 1 || x.id > h.cantidadRegistros) return ERR_NO_EXISTE;

    x.fechaUltimaModificacion = time(nullptr);
    fstream archivo(RUTA_VEHICULOS, ios::in | ios::out | ios::binary);
    if (!archivo) return errorArchivo(RUTA_VEHICULOS);
    archivo.seekp(posicionDeID(sizeof(Vehiculo), x.id));
    archivo.write(reinterpret_cast<const char*>(&x), sizeof(Vehiculo));
    if (!archivo) return errorArchivo(RUTA_VEHICULOS);
    return OK;
}


Resultado marcarVehiculoEliminado(int id) {
    Vehiculo x;
    Resultado r = leerVehiculoPorID(id, x);
    if (r != OK) return r;
    if (x.eliminado) return ERR_ELIMINADO;

    x.eliminado = true;
    r = sobrescribirVehiculo(x);
    if (r != OK) return r;

    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return ERR_ARCHIVO;
    h.registrosActivos--;
    if (!escribirHeader(RUTA_VEHICULOS, h)) return ERR_ARCHIVO;
    return OK;
}

.
Resultado contarVehiculosActivos(int &cantidad) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return ERR_ARCHIVO;
    cantidad = h.registrosActivos;
    return OK;
}

// ---------- Mecanico (archivo mecanicos.bin) ----------


Resultado guardarMecanico(Mecanico &x) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return ERR_ARCHIVO;
    if (h.proximoID != h.cantidadRegistros + 1) return ERR_ARCHIVO_CORRUPTO;

    x.id = h.proximoID;
    x.eliminado = false;
    x.fechaCreacion = time(nullptr);
    x.fechaUltimaModificacion = x.fechaCreacion;

    fstream archivo(RUTA_MECANICOS, ios::in | ios::out | ios::binary);
    if (!archivo) return errorArchivo(RUTA_MECANICOS);
    archivo.seekp(posicionDeID(sizeof(Mecanico), x.id));
    archivo.write(reinterpret_cast<const char*>(&x), sizeof(Mecanico));
    if (!archivo) return errorArchivo(RUTA_MECANICOS);
    archivo.close();

    h.cantidadRegistros++;
    h.registrosActivos++;
    h.proximoID++;
    if (!escribirHeader(RUTA_MECANICOS, h)) return ERR_ARCHIVO;
    return OK;
}


Resultado leerMecanicoPorID(int id, Mecanico &x) {
    ifstream archivo(RUTA_MECANICOS, ios::binary);
    if (!archivo) return errorArchivo(RUTA_MECANICOS);
    ArchivoHeader h;
    archivo.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (!archivo) return errorArchivo(RUTA_MECANICOS);
    if (id < 1 || id > h.cantidadRegistros) return ERR_NO_EXISTE;

    ultimaPosicionLeida = posicionDeID(sizeof(Mecanico), id);
    archivo.seekg(ultimaPosicionLeida);
    archivo.read(reinterpret_cast<char*>(&x), sizeof(Mecanico));
    if (!archivo) return errorArchivo(RUTA_MECANICOS);
    return OK;
}


Resultado sobrescribirMecanico(Mecanico &x) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return ERR_ARCHIVO;
    if (x.id < 1 || x.id > h.cantidadRegistros) return ERR_NO_EXISTE;

    x.fechaUltimaModificacion = time(nullptr);
    fstream archivo(RUTA_MECANICOS, ios::in | ios::out | ios::binary);
    if (!archivo) return errorArchivo(RUTA_MECANICOS);
    archivo.seekp(posicionDeID(sizeof(Mecanico), x.id));
    archivo.write(reinterpret_cast<const char*>(&x), sizeof(Mecanico));
    if (!archivo) return errorArchivo(RUTA_MECANICOS);
    return OK;
}


Resultado marcarMecanicoEliminado(int id) {
    Mecanico x;
    Resultado r = leerMecanicoPorID(id, x);
    if (r != OK) return r;
    if (x.eliminado) return ERR_ELIMINADO;

    x.eliminado = true;
    r = sobrescribirMecanico(x);
    if (r != OK) return r;

    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return ERR_ARCHIVO;
    h.registrosActivos--;
    if (!escribirHeader(RUTA_MECANICOS, h)) return ERR_ARCHIVO;
    return OK;
}


Resultado contarMecanicosActivos(int &cantidad) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return ERR_ARCHIVO;
    cantidad = h.registrosActivos;
    return OK;
}

// ---------- Repuesto (archivo repuestos.bin) ----------


Resultado guardarRepuesto(Repuesto &x) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return ERR_ARCHIVO;
    if (h.proximoID != h.cantidadRegistros + 1) return ERR_ARCHIVO_CORRUPTO;

    x.id = h.proximoID;
    x.eliminado = false;
    x.fechaCreacion = time(nullptr);
    x.fechaUltimaModificacion = x.fechaCreacion;

    fstream archivo(RUTA_REPUESTOS, ios::in | ios::out | ios::binary);
    if (!archivo) return errorArchivo(RUTA_REPUESTOS);
    archivo.seekp(posicionDeID(sizeof(Repuesto), x.id));
    archivo.write(reinterpret_cast<const char*>(&x), sizeof(Repuesto));
    if (!archivo) return errorArchivo(RUTA_REPUESTOS);
    archivo.close();

    h.cantidadRegistros++;
    h.registrosActivos++;
    h.proximoID++;
    if (!escribirHeader(RUTA_REPUESTOS, h)) return ERR_ARCHIVO;
    return OK;
}


Resultado leerRepuestoPorID(int id, Repuesto &x) {
    ifstream archivo(RUTA_REPUESTOS, ios::binary);
    if (!archivo) return errorArchivo(RUTA_REPUESTOS);
    ArchivoHeader h;
    archivo.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (!archivo) return errorArchivo(RUTA_REPUESTOS);
    if (id < 1 || id > h.cantidadRegistros) return ERR_NO_EXISTE;

    ultimaPosicionLeida = posicionDeID(sizeof(Repuesto), id);
    archivo.seekg(ultimaPosicionLeida);
    archivo.read(reinterpret_cast<char*>(&x), sizeof(Repuesto));
    if (!archivo) return errorArchivo(RUTA_REPUESTOS);
    return OK;
}


Resultado sobrescribirRepuesto(Repuesto &x) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return ERR_ARCHIVO;
    if (x.id < 1 || x.id > h.cantidadRegistros) return ERR_NO_EXISTE;

    x.fechaUltimaModificacion = time(nullptr);
    fstream archivo(RUTA_REPUESTOS, ios::in | ios::out | ios::binary);
    if (!archivo) return errorArchivo(RUTA_REPUESTOS);
    archivo.seekp(posicionDeID(sizeof(Repuesto), x.id));
    archivo.write(reinterpret_cast<const char*>(&x), sizeof(Repuesto));
    if (!archivo) return errorArchivo(RUTA_REPUESTOS);
    return OK;
}

Resultado marcarRepuestoEliminado(int id) {
    Repuesto x;
    Resultado r = leerRepuestoPorID(id, x);
    if (r != OK) return r;
    if (x.eliminado) return ERR_ELIMINADO;

    x.eliminado = true;
    r = sobrescribirRepuesto(x);
    if (r != OK) return r;

    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return ERR_ARCHIVO;
    h.registrosActivos--;
    if (!escribirHeader(RUTA_REPUESTOS, h)) return ERR_ARCHIVO;
    return OK;
}


Resultado contarRepuestosActivos(int &cantidad) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return ERR_ARCHIVO;
    cantidad = h.registrosActivos;
    return OK;
}

// ---------- OrdenServicio (archivo ordenes.bin) ----------


Resultado guardarOrden(OrdenServicio &x) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return ERR_ARCHIVO;
    if (h.proximoID != h.cantidadRegistros + 1) return ERR_ARCHIVO_CORRUPTO;

    x.id = h.proximoID;
    x.eliminado = false;
    x.fechaCreacion = time(nullptr);
    x.fechaUltimaModificacion = x.fechaCreacion;

    fstream archivo(RUTA_ORDENES, ios::in | ios::out | ios::binary);
    if (!archivo) return errorArchivo(RUTA_ORDENES);
    archivo.seekp(posicionDeID(sizeof(OrdenServicio), x.id));
    archivo.write(reinterpret_cast<const char*>(&x), sizeof(OrdenServicio));
    if (!archivo) return errorArchivo(RUTA_ORDENES);
    archivo.close();

    h.cantidadRegistros++;
    h.registrosActivos++;
    h.proximoID++;
    if (!escribirHeader(RUTA_ORDENES, h)) return ERR_ARCHIVO;
    return OK;
}


Resultado leerOrdenPorID(int id, OrdenServicio &x) {
    ifstream archivo(RUTA_ORDENES, ios::binary);
    if (!archivo) return errorArchivo(RUTA_ORDENES);
    ArchivoHeader h;
    archivo.read(reinterpret_cast<char*>(&h), sizeof(h));
    if (!archivo) return errorArchivo(RUTA_ORDENES);
    if (id < 1 || id > h.cantidadRegistros) return ERR_NO_EXISTE;

    ultimaPosicionLeida = posicionDeID(sizeof(OrdenServicio), id);
    archivo.seekg(ultimaPosicionLeida);
    archivo.read(reinterpret_cast<char*>(&x), sizeof(OrdenServicio));
    if (!archivo) return errorArchivo(RUTA_ORDENES);
    return OK;
}


Resultado sobrescribirOrden(OrdenServicio &x) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return ERR_ARCHIVO;
    if (x.id < 1 || x.id > h.cantidadRegistros) return ERR_NO_EXISTE;

    x.fechaUltimaModificacion = time(nullptr);
    fstream archivo(RUTA_ORDENES, ios::in | ios::out | ios::binary);
    if (!archivo) return errorArchivo(RUTA_ORDENES);
    archivo.seekp(posicionDeID(sizeof(OrdenServicio), x.id));
    archivo.write(reinterpret_cast<const char*>(&x), sizeof(OrdenServicio));
    if (!archivo) return errorArchivo(RUTA_ORDENES);
    return OK;
}


Resultado marcarOrdenEliminado(int id) {
    OrdenServicio x;
    Resultado r = leerOrdenPorID(id, x);
    if (r != OK) return r;
    if (x.eliminado) return ERR_ELIMINADO;

    x.eliminado = true;
    r = sobrescribirOrden(x);
    if (r != OK) return r;

    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return ERR_ARCHIVO;
    h.registrosActivos--;
    if (!escribirHeader(RUTA_ORDENES, h)) return ERR_ARCHIVO;
    return OK;
}


Resultado contarOrdenesActivas(int &cantidad) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return ERR_ARCHIVO;
    cantidad = h.registrosActivos;
    return OK;
}

// =====================================================================
// PARTE 2: CAPA DE LOGICA
// Aplica las reglas de negocio. Usa la persistencia para leer y escribir.
// NO usa cin/cout y NINGUNA funcion devuelve un puntero a un registro:
// los datos se entregan por referencia (copias) y el resultado es un
// valor de tipo Resultado que dice si salio bien o por que fallo.
//
// Patron de las funciones:  Resultado nombre(parametros..., InfoError &info)
//   - devuelve OK si todo salio bien
//   - si falla devuelve el motivo y deja datos extra en 'info'
// =====================================================================



void limpiarInfo(InfoError &info) {
    memset(&info, 0, sizeof(info));
}

long depuracionUltimaPosicion() { return ultimaPosicionLeida; }
const char* depuracionArchivoConError() { return rutaConError; }

Resultado campoVacio(const char* campo, InfoError &info) {
    copiarTexto(info.texto, 100, campo);
    return ERR_CAMPO_VACIO;
}


Resultado cargarVehiculoActivo(int id, Vehiculo &v, InfoError &info) {
    Resultado r = leerVehiculoPorID(id, v);
    if (r == ERR_NO_EXISTE) {
        info.idRelacionado = id;
        return ERR_VEHICULO_NO_EXISTE;
    }
    if (r != OK) return r;
    if (v.eliminado) {
        info.idRelacionado = id;
        copiarTexto(info.texto, 100, v.placa);
        return ERR_VEHICULO_ELIMINADO;
    }
    return OK;
}

Resultado cargarMecanicoActivo(int id, Mecanico &m, InfoError &info) {
    Resultado r = leerMecanicoPorID(id, m);
    if (r == ERR_NO_EXISTE) {
        info.idRelacionado = id;
        return ERR_MECANICO_NO_EXISTE;
    }
    if (r != OK) return r;
    if (m.eliminado) {
        info.idRelacionado = id;
        copiarTexto(info.texto, 100, m.nombre);
        return ERR_MECANICO_ELIMINADO;
    }
    return OK;
}

Resultado cargarRepuestoActivo(int id, Repuesto &rep, InfoError &info) {
    Resultado r = leerRepuestoPorID(id, rep);
    if (r == ERR_NO_EXISTE) {
        info.idRelacionado = id;
        return ERR_REPUESTO_NO_EXISTE;
    }
    if (r != OK) return r;
    if (rep.eliminado) {
        info.idRelacionado = id;
        copiarTexto(info.texto, 100, rep.nombre);
        return ERR_REPUESTO_ELIMINADO;
    }
    return OK;
}

Resultado cargarOrden(int id, OrdenServicio &o, InfoError &info) {
    Resultado r = leerOrdenPorID(id, o);
    if (r == ERR_NO_EXISTE) {
        info.idRelacionado = id;
        return ERR_ORDEN_NO_EXISTE;
    }
    return r;
}

Resultado contarOrdenesDeVehiculo(int idVehiculo, int &cantidad) {
    ArchivoHeader h;
    cantidad = 0;
    if (!leerHeader(RUTA_ORDENES, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        OrdenServicio o;
        Resultado r = leerOrdenPorID(i, o);
        if (r != OK) return r;
        if (o.idVehiculo == idVehiculo) cantidad++;
    }
    return OK;
}


Resultado buscarOrdenAbiertaDeVehiculo(int idVehiculo, int &idOrden) {
    ArchivoHeader h;
    idOrden = 0;
    if (!leerHeader(RUTA_ORDENES, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        OrdenServicio o;
        Resultado r = leerOrdenPorID(i, o);
        if (r != OK) return r;
        if (o.idVehiculo == idVehiculo && strcmp(o.estado, ESTADO_ABIERTA) == 0) {
            idOrden = o.id;
            return OK;
        }
    }
    return OK;
}

Resultado contarOrdenesAbiertasDeMecanico(int idMecanico, int &cantidad) {
    ArchivoHeader h;
    cantidad = 0;
    if (!leerHeader(RUTA_ORDENES, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        OrdenServicio o;
        Resultado r = leerOrdenPorID(i, o);
        if (r != OK) return r;
        if (o.idMecanico == idMecanico && strcmp(o.estado, ESTADO_ABIERTA) == 0) cantidad++;
    }
    return OK;
}


Resultado buscarOrdenConRepuesto(int idRepuesto, int &idOrden) {
    ArchivoHeader h;
    idOrden = 0;
    if (!leerHeader(RUTA_ORDENES, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        OrdenServicio o;
        Resultado r = leerOrdenPorID(i, o);
        if (r != OK) return r;
        for (int j = 0; j < o.numLineas && j < MAX_LINEAS; j++) {
            if (o.lineas[j].idRepuesto == idRepuesto) {
                idOrden = o.id;
                return OK;
            }
        }
    }
    return OK;
}

// ---------------------------------------------------------------------
// TALLER
// ---------------------------------------------------------------------


Resultado iniciarSistema() {
    if (!crearCarpeta(CARPETA_DATOS)) return ERR_ARCHIVO;
    if (!inicializarArchivo(RUTA_VEHICULOS)) return ERR_ARCHIVO;
    if (!inicializarArchivo(RUTA_MECANICOS)) return ERR_ARCHIVO;
    if (!inicializarArchivo(RUTA_REPUESTOS)) return ERR_ARCHIVO;
    if (!inicializarArchivo(RUTA_ORDENES)) return ERR_ARCHIVO;
    return OK;
}

bool tallerRegistrado() {
    return existeTaller();
}

Resultado registrarTaller(Taller &t, InfoError &info) {
    limpiarInfo(info);
    if (textoVacio(t.nombre)) return campoVacio("nombre del taller", info);
    if (textoVacio(t.rif)) return campoVacio("RIF", info);
    if (textoVacio(t.direccion)) return campoVacio("direccion", info);
    if (textoVacio(t.telefono)) return campoVacio("telefono", info);
    return guardarTaller(t);
}

Resultado obtenerTaller(Taller &t) {
    return leerTaller(t);
}

// ---------------------------------------------------------------------
// VEHICULOS
// ---------------------------------------------------------------------

Resultado buscarVehiculoActivoPorPlaca(const char* placa, Vehiculo &salida) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        Vehiculo v;
        Resultado r = leerVehiculoPorID(i, v);
        if (r != OK) return r;
        if (!v.eliminado && strcmp(v.placa, placa) == 0) {
            salida = v;
            return OK;
        }
    }
    return ERR_VEHICULO_NO_EXISTE;
}


Resultado validarCamposVehiculo(const Vehiculo &v, InfoError &info) {
    if (textoVacio(v.marca)) return campoVacio("marca", info);
    if (textoVacio(v.modelo)) return campoVacio("modelo", info);
    if (!esAnioValido(v.anio)) {
        info.cantidad = v.anio;
        return ERR_ANIO_INVALIDO;
    }
    if (textoVacio(v.nombrePropietario)) return campoVacio("nombre del propietario", info);
    if (!esCedulaValida(v.cedulaPropietario)) {
        copiarTexto(info.texto, 100, v.cedulaPropietario);
        return ERR_CEDULA_INVALIDA;
    }
    if (textoVacio(v.telefonoPropietario)) return campoVacio("telefono del propietario", info);
    return OK;
}


Resultado agregarVehiculo(Vehiculo &datos, InfoError &info) {
    limpiarInfo(info);
    aMayusculas(datos.placa);
    aMayusculas(datos.cedulaPropietario);

    if (!esPlacaValida(datos.placa)) {
        copiarTexto(info.texto, 100, datos.placa);
        return ERR_PLACA_INVALIDA;
    }
    Resultado r = validarCamposVehiculo(datos, info);
    if (r != OK) return r;
    if (datos.kilometraje < 0) {
        info.cantidad = datos.kilometraje;
        return ERR_KM_INVALIDO;
    }

  
    Vehiculo otro;
    r = buscarVehiculoActivoPorPlaca(datos.placa, otro);
    if (r == OK) {
        info.idRelacionado = otro.id;
        copiarTexto(info.texto, 100, datos.placa);
        return ERR_PLACA_DUPLICADA;
    }
    if (r != ERR_VEHICULO_NO_EXISTE) return r;   // error de archivo

    return guardarVehiculo(datos);
}


Resultado obtenerVehiculoPorID(int id, Vehiculo &v, InfoError &info) {
    limpiarInfo(info);
    return cargarVehiculoActivo(id, v, info);
}

Resultado obtenerVehiculoPorPlaca(const char* placa, Vehiculo &v, InfoError &info) {
    limpiarInfo(info);
    char buscada[10];
    copiarTexto(buscada, 10, placa);
    aMayusculas(buscada);
    Resultado r = buscarVehiculoActivoPorPlaca(buscada, v);
    if (r == ERR_VEHICULO_NO_EXISTE) copiarTexto(info.texto, 100, buscada);
    return r;
}


Resultado buscarVehiculosPorPropietario(const char* subcadena, Vehiculo resultados[],
                                        int &cantidad, bool &hayMas, InfoError &info) {
    limpiarInfo(info);
    cantidad = 0;
    hayMas = false;
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        Vehiculo v;
        Resultado r = leerVehiculoPorID(i, v);
        if (r != OK) return r;
        if (v.eliminado) continue;
        if (!contieneIgnorandoMayusculas(v.nombrePropietario, subcadena)) continue;
        if (cantidad < MAX_RESULTADOS) {
            resultados[cantidad] = v;
            cantidad++;
        } else {
            hayMas = true;
            break;
        }
    }
    return OK;
}

Resultado listarVehiculos(Vehiculo resultados[], int &cantidad, bool &hayMas, InfoError &info) {
    limpiarInfo(info);
    cantidad = 0;
    hayMas = false;
    ArchivoHeader h;
    if (!leerHeader(RUTA_VEHICULOS, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        Vehiculo v;
        Resultado r = leerVehiculoPorID(i, v);
        if (r != OK) return r;
        if (v.eliminado) continue;
        if (cantidad < MAX_RESULTADOS) {
            resultados[cantidad] = v;
            cantidad++;
        } else {
            hayMas = true;
            break;
        }
    }
    return OK;
}


Resultado actualizarVehiculo(int id, const Vehiculo &datos, InfoError &info) {
    limpiarInfo(info);
    Vehiculo v;
    Resultado r = cargarVehiculoActivo(id, v, info);
    if (r != OK) return r;

    Vehiculo nuevo = datos;
    aMayusculas(nuevo.cedulaPropietario);
    r = validarCamposVehiculo(nuevo, info);
    if (r != OK) return r;

    copiarTexto(v.marca, 50, nuevo.marca);
    copiarTexto(v.modelo, 50, nuevo.modelo);
    v.anio = nuevo.anio;
    copiarTexto(v.nombrePropietario, 100, nuevo.nombrePropietario);
    copiarTexto(v.cedulaPropietario, 20, nuevo.cedulaPropietario);
    copiarTexto(v.telefonoPropietario, 20, nuevo.telefonoPropietario);
    return sobrescribirVehiculo(v);
}

Resultado eliminarVehiculo(int id, InfoError &info) {
    limpiarInfo(info);
    Vehiculo v;
    Resultado r = cargarVehiculoActivo(id, v, info);
    if (r != OK) return r;

    int ordenes = 0;
    r = contarOrdenesDeVehiculo(id, ordenes);
    if (r != OK) return r;
    if (ordenes > 0) {
        copiarTexto(info.texto, 100, v.placa);
        info.cantidad = ordenes;
        return ERR_VEHICULO_CON_ORDENES;
    }
    return marcarVehiculoEliminado(id);
}

// ---------------------------------------------------------------------
// MECANICOS
// ---------------------------------------------------------------------

Resultado registrarMecanico(Mecanico &datos, InfoError &info) {
    limpiarInfo(info);
    aMayusculas(datos.cedula);
    aMayusculas(datos.especialidad);

    if (textoVacio(datos.nombre)) return campoVacio("nombre", info);
    if (!esCedulaValida(datos.cedula)) {
        copiarTexto(info.texto, 100, datos.cedula);
        return ERR_CEDULA_INVALIDA;
    }
    if (!esEspecialidadValida(datos.especialidad)) {
        copiarTexto(info.texto, 100, datos.especialidad);
        return ERR_ESPECIALIDAD_INVALIDA;
    }
    if (!esFechaValida(datos.fechaIngreso)) {
        copiarTexto(info.texto, 100, datos.fechaIngreso);
        return ERR_FECHA_INVALIDA;
    }

    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        Mecanico otro;
        Resultado r = leerMecanicoPorID(i, otro);
        if (r != OK) return r;
        if (!otro.eliminado && strcmp(otro.cedula, datos.cedula) == 0) {
            copiarTexto(info.texto, 100, datos.cedula);
            info.idRelacionado = otro.id;
            return ERR_CEDULA_DUPLICADA;
        }
    }
    return guardarMecanico(datos);
}

Resultado obtenerMecanicoPorID(int id, Mecanico &m, InfoError &info) {
    limpiarInfo(info);
    Resultado r = leerMecanicoPorID(id, m);
    if (r == ERR_NO_EXISTE) {
        info.idRelacionado = id;
        return ERR_MECANICO_NO_EXISTE;
    }
    return r;
}

Resultado listarMecanicos(MecanicoVista resultados[], int &cantidad, bool &hayMas, InfoError &info) {
    limpiarInfo(info);
    cantidad = 0;
    hayMas = false;
    ArchivoHeader h;
    if (!leerHeader(RUTA_MECANICOS, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        if (cantidad >= MAX_RESULTADOS) {
            hayMas = true;
            break;
        }
        Mecanico m;
        Resultado r = leerMecanicoPorID(i, m);
        if (r != OK) return r;
        int abiertas = 0;
        r = contarOrdenesAbiertasDeMecanico(i, abiertas);
        if (r != OK) return r;
        resultados[cantidad].mecanico = m;
        resultados[cantidad].ordenesAbiertas = abiertas;
        cantidad++;
    }
    return OK;
}

Resultado darDeBajaMecanico(int id, InfoError &info) {
    limpiarInfo(info);
    Mecanico m;
    Resultado r = cargarMecanicoActivo(id, m, info);
    if (r != OK) return r;

    int abiertas = 0;
    r = contarOrdenesAbiertasDeMecanico(id, abiertas);
    if (r != OK) return r;
    if (abiertas > 0) {
        copiarTexto(info.texto, 100, m.nombre);
        info.cantidad = abiertas;
        return ERR_MECANICO_CON_ABIERTAS;
    }
    return marcarMecanicoEliminado(id);
}

// ---------------------------------------------------------------------
// REPUESTOS
// ---------------------------------------------------------------------

Resultado buscarRepuestoActivoPorCodigo(const char* codigo, Repuesto &salida) {
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        Repuesto rep;
        Resultado r = leerRepuestoPorID(i, rep);
        if (r != OK) return r;
        if (!rep.eliminado && strcmp(rep.codigo, codigo) == 0) {
            salida = rep;
            return OK;
        }
    }
    return ERR_REPUESTO_NO_EXISTE;
}

Resultado registrarRepuesto(Repuesto &datos, InfoError &info) {
    limpiarInfo(info);
    aMayusculas(datos.codigo);

    if (textoVacio(datos.codigo)) return campoVacio("codigo", info);
    if (textoVacio(datos.nombre)) return campoVacio("nombre", info);
    if (textoVacio(datos.marca)) return campoVacio("marca", info);
    if (datos.precio <= 0) {
        snprintf(info.texto, 100, "%.2f", datos.precio);
        return ERR_PRECIO_INVALIDO;
    }
    if (datos.stock < 0) {
        info.cantidad = datos.stock;
        return ERR_STOCK_INVALIDO;
    }
    if (datos.stockMinimo < 0) {
        info.cantidad = datos.stockMinimo;
        return ERR_STOCK_MINIMO_INVALIDO;
    }

    Repuesto otro;
    Resultado r = buscarRepuestoActivoPorCodigo(datos.codigo, otro);
    if (r == OK) {
        copiarTexto(info.texto, 100, datos.codigo);
        info.idRelacionado = otro.id;
        return ERR_CODIGO_DUPLICADO;
    }
    if (r != ERR_REPUESTO_NO_EXISTE) return r;

    return guardarRepuesto(datos);
}

Resultado obtenerRepuestoPorID(int id, Repuesto &rep, InfoError &info) {
    limpiarInfo(info);
    return cargarRepuestoActivo(id, rep, info);
}

Resultado obtenerRepuestoPorCodigo(const char* codigo, Repuesto &rep, InfoError &info) {
    limpiarInfo(info);
    char buscado[20];
    copiarTexto(buscado, 20, codigo);
    aMayusculas(buscado);
    Resultado r = buscarRepuestoActivoPorCodigo(buscado, rep);
    if (r == ERR_REPUESTO_NO_EXISTE) copiarTexto(info.texto, 100, buscado);
    return r;
}

Resultado buscarRepuestosPorNombre(const char* subcadena, Repuesto resultados[],
                                   int &cantidad, bool &hayMas, InfoError &info) {
    limpiarInfo(info);
    cantidad = 0;
    hayMas = false;
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        Repuesto rep;
        Resultado r = leerRepuestoPorID(i, rep);
        if (r != OK) return r;
        if (rep.eliminado) continue;
        if (!contieneIgnorandoMayusculas(rep.nombre, subcadena)) continue;
        if (cantidad < MAX_RESULTADOS) {
            resultados[cantidad] = rep;
            cantidad++;
        } else {
            hayMas = true;
            break;
        }
    }
    return OK;
}

Resultado listarRepuestos(Repuesto resultados[], int &cantidad, bool &hayMas, InfoError &info) {
    limpiarInfo(info);
    cantidad = 0;
    hayMas = false;
    ArchivoHeader h;
    if (!leerHeader(RUTA_REPUESTOS, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        Repuesto rep;
        Resultado r = leerRepuestoPorID(i, rep);
        if (r != OK) return r;
        if (rep.eliminado) continue;
        if (cantidad < MAX_RESULTADOS) {
            resultados[cantidad] = rep;
            cantidad++;
        } else {
            hayMas = true;
            break;
        }
    }
    return OK;
}

Resultado registrarEntradaMercancia(int id, int cantidad, Repuesto &rep, InfoError &info) {
    limpiarInfo(info);
    Resultado r = cargarRepuestoActivo(id, rep, info);
    if (r != OK) return r;
    if (cantidad <= 0) {
        info.solicitado = cantidad;
        return ERR_CANTIDAD_INVALIDA;
    }
    rep.stock += cantidad;
    return sobrescribirRepuesto(rep);
}

Resultado cambiarPrecioRepuesto(int id, float nuevoPrecio, Repuesto &rep, InfoError &info) {
    limpiarInfo(info);
    Resultado r = cargarRepuestoActivo(id, rep, info);
    if (r != OK) return r;
    if (nuevoPrecio <= 0) {
        snprintf(info.texto, 100, "%.2f", nuevoPrecio);
        return ERR_PRECIO_INVALIDO;
    }
    rep.precio = nuevoPrecio;
    return sobrescribirRepuesto(rep);
}

Resultado eliminarRepuesto(int id, InfoError &info) {
    limpiarInfo(info);
    Repuesto rep;
    Resultado r = cargarRepuestoActivo(id, rep, info);
    if (r != OK) return r;

    int idOrden = 0;
    r = buscarOrdenConRepuesto(id, idOrden);
    if (r != OK) return r;
    if (idOrden != 0) {
        copiarTexto(info.texto, 100, rep.nombre);
        info.idRelacionado = idOrden;
        return ERR_REPUESTO_EN_USO;
    }
    return marcarRepuestoEliminado(id);
}

// ---------------------------------------------------------------------
// ORDENES DE SERVICIO
// ---------------------------------------------------------------------

float calcularTotalRepuestos(const OrdenServicio &o) {
    float total = 0;
    for (int i = 0; i < o.numLineas && i < MAX_LINEAS; i++) {
        total += o.lineas[i].cantidad * o.lineas[i].precioUnitario;
    }
    return total;
}


float calcularTotalOrden(const OrdenServicio &o) {
    return o.manoObra + calcularTotalRepuestos(o);
}

int buscarLinea(const OrdenServicio &o, int idRepuesto) {
    for (int i = 0; i < o.numLineas && i < MAX_LINEAS; i++) {
        if (o.lineas[i].idRepuesto == idRepuesto) return i;
    }
    return -1;
}

Resultado estadoIncorrecto(const OrdenServicio &o, InfoError &info) {
    info.idRelacionado = o.id;
    copiarTexto(info.texto, 100, o.estado);
    return ERR_ESTADO_INCORRECTO;
}


Resultado abrirOrden(int idVehiculo, int idMecanico, const char* descripcion, int km,
                     OrdenServicio &creada, InfoError &info) {
    limpiarInfo(info);


    Vehiculo v;
    Resultado r = cargarVehiculoActivo(idVehiculo, v, info);
    if (r != OK) return r;
    Mecanico m;
    r = cargarMecanicoActivo(idMecanico, m, info);
    if (r != OK) return r;

    if (textoVacio(descripcion)) return campoVacio("descripcion de la falla", info);
    if (km < v.kilometraje) {
        copiarTexto(info.texto, 100, v.placa);
        info.cantidad = v.kilometraje;
        info.solicitado = km;
        return ERR_KM_MENOR;
    }

    int idAbierta = 0;
    r = buscarOrdenAbiertaDeVehiculo(idVehiculo, idAbierta);
    if (r != OK) return r;
    if (idAbierta != 0) {
        copiarTexto(info.texto, 100, v.placa);
        info.idRelacionado = idAbierta;
        return ERR_VEHICULO_YA_TIENE_ABIERTA;
    }

    int abiertas = 0;
    r = contarOrdenesAbiertasDeMecanico(idMecanico, abiertas);
    if (r != OK) return r;
    if (abiertas >= MAX_ORDENES_ABIERTAS) {
        copiarTexto(info.texto, 100, m.nombre);
        info.cantidad = abiertas;
        return ERR_MECANICO_LLENO;
    }

    
    memset(&creada, 0, sizeof(creada));   // deja en cero todo (tambien el relleno)
    creada.idVehiculo = idVehiculo;
    creada.idMecanico = idMecanico;
    fechaActual(creada.fechaApertura);
    copiarTexto(creada.estado, 12, ESTADO_ABIERTA);
    copiarTexto(creada.descripcionFalla, 200, descripcion);
    creada.kilometrajeIngreso = km;
    creada.manoObra = 0;
    creada.numLineas = 0;

    r = guardarOrden(creada);          // 1o: la orden
    if (r != OK) return r;
    v.kilometraje = km;                // 2o: el kilometraje del vehiculo
    return sobrescribirVehiculo(v);
}


Resultado agregarRepuestoAOrden(int idOrden, int idRepuesto, int cantidad, InfoError &info) {
    limpiarInfo(info);


    OrdenServicio o;
    Resultado r = cargarOrden(idOrden, o, info);
    if (r != OK) return r;
    if (strcmp(o.estado, ESTADO_ABIERTA) != 0) return estadoIncorrecto(o, info);
    if (cantidad <= 0) {
        info.solicitado = cantidad;
        return ERR_CANTIDAD_INVALIDA;
    }
    Repuesto rep;
    r = cargarRepuestoActivo(idRepuesto, rep, info);
    if (r != OK) return r;
    if (rep.stock < cantidad) {
        copiarTexto(info.texto, 100, rep.nombre);
        info.cantidad = rep.stock;
        info.solicitado = cantidad;
        return ERR_STOCK_INSUFICIENTE;
    }
    int pos = buscarLinea(o, idRepuesto);
    if (pos == -1 && o.numLineas >= MAX_LINEAS) {
        info.idRelacionado = idOrden;
        info.cantidad = MAX_LINEAS;
        return ERR_LIMITE_LINEAS;
    }

    
    if (pos >= 0) {
        
        o.lineas[pos].cantidad += cantidad;
    } else {
        o.lineas[o.numLineas].idRepuesto = idRepuesto;
        o.lineas[o.numLineas].cantidad = cantidad;
        o.lineas[o.numLineas].precioUnitario = rep.precio;   // precio vigente = precio historico
        o.numLineas++;
    }
    r = sobrescribirOrden(o);          
    if (r != OK) return r;
    rep.stock -= cantidad;             
    return sobrescribirRepuesto(rep);
}

Resultado quitarRepuestoDeOrden(int idOrden, int idRepuesto, InfoError &info) {
    limpiarInfo(info);


    OrdenServicio o;
    Resultado r = cargarOrden(idOrden, o, info);
    if (r != OK) return r;
    if (strcmp(o.estado, ESTADO_ABIERTA) != 0) return estadoIncorrecto(o, info);

    Repuesto rep;
    r = leerRepuestoPorID(idRepuesto, rep);   // lectura cruda: el repuesto debe existir
    if (r == ERR_NO_EXISTE) {
        info.idRelacionado = idRepuesto;
        return ERR_REPUESTO_NO_EXISTE;
    }
    if (r != OK) return r;

    int pos = buscarLinea(o, idRepuesto);
    if (pos == -1) {
        copiarTexto(info.texto, 100, rep.nombre);
        info.idRelacionado = idOrden;
        return ERR_LINEA_NO_EXISTE;
    }

    
    rep.stock += o.lineas[pos].cantidad;
    r = sobrescribirRepuesto(rep);     
    if (r != OK) return r;


    for (int i = pos; i < o.numLineas - 1; i++) {
        o.lineas[i] = o.lineas[i + 1];
    }
    memset(&o.lineas[o.numLineas - 1], 0, sizeof(LineaDetalle));
    o.numLineas--;
    return sobrescribirOrden(o);      


Resultado cerrarOrden(int idOrden, float manoObra, InfoError &info) {
    limpiarInfo(info);
    OrdenServicio o;
    Resultado r = cargarOrden(idOrden, o, info);
    if (r != OK) return r;
    if (strcmp(o.estado, ESTADO_ABIERTA) != 0) return estadoIncorrecto(o, info);
    if (manoObra < 0) {
        snprintf(info.texto, 100, "%.2f", manoObra);
        return ERR_MANO_OBRA_INVALIDA;
    }
    if (o.numLineas == 0 && manoObra <= 0) {
        info.idRelacionado = idOrden;
        return ERR_ORDEN_VACIA;
    }

    o.manoObra = manoObra;
    copiarTexto(o.estado, 12, ESTADO_CERRADA);
    fechaActual(o.fechaCierre);
    return sobrescribirOrden(o);
}


Resultado anularOrden(int idOrden, InfoError &info) {
    limpiarInfo(info);


    OrdenServicio o;
    Resultado r = cargarOrden(idOrden, o, info);
    if (r != OK) return r;
    if (strcmp(o.estado, ESTADO_ABIERTA) != 0 && strcmp(o.estado, ESTADO_CERRADA) != 0) {
        return estadoIncorrecto(o, info);
    }
   
    for (int i = 0; i < o.numLineas && i < MAX_LINEAS; i++) {
        Repuesto rep;
        r = leerRepuestoPorID(o.lineas[i].idRepuesto, rep);
        if (r == ERR_NO_EXISTE) {
            info.idRelacionado = o.lineas[i].idRepuesto;
            return ERR_REPUESTO_NO_EXISTE;
        }
        if (r != OK) return r;
    }

   
    copiarTexto(o.estado, 12, ESTADO_ANULADA);
    r = sobrescribirOrden(o);          
    if (r != OK) return r;
    for (int i = 0; i < o.numLineas && i < MAX_LINEAS; i++) {   
        Repuesto rep;
        r = leerRepuestoPorID(o.lineas[i].idRepuesto, rep);
        if (r != OK) return r;
        rep.stock += o.lineas[i].cantidad;
        r = sobrescribirRepuesto(rep);
        if (r != OK) return r;
    }
    return OK;
}

void construirVistaOrden(const OrdenServicio &o, OrdenVista &vista) {
    memset(&vista, 0, sizeof(vista));
    vista.orden = o;

    Vehiculo v;
    if (leerVehiculoPorID(o.idVehiculo, v) == OK) {
        copiarTexto(vista.placa, 10, v.placa);
        copiarTexto(vista.marca, 50, v.marca);
        copiarTexto(vista.modelo, 50, v.modelo);
        vista.anio = v.anio;
        copiarTexto(vista.nombrePropietario, 100, v.nombrePropietario);
    } else {
        copiarTexto(vista.placa, 10, "???");
        copiarTexto(vista.nombrePropietario, 100, "(vehiculo no encontrado)");
    }

    Mecanico m;
    if (leerMecanicoPorID(o.idMecanico, m) == OK) {
        copiarTexto(vista.nombreMecanico, 100, m.nombre);
        copiarTexto(vista.especialidad, 20, m.especialidad);
        vista.mecanicoDeBaja = m.eliminado;
    } else {
        copiarTexto(vista.nombreMecanico, 100, "(mecanico no encontrado)");
    }

    vista.totalRepuestos = calcularTotalRepuestos(o);
    vista.total = vista.totalRepuestos + o.manoObra;
}


Resultado obtenerDetalleOrden(int idOrden, OrdenVista &vista, LineaVista lineas[], InfoError &info) {
    limpiarInfo(info);
    OrdenServicio o;
    Resultado r = cargarOrden(idOrden, o, info);
    if (r != OK) return r;

    construirVistaOrden(o, vista);
    for (int i = 0; i < o.numLineas && i < MAX_LINEAS; i++) {
        memset(&lineas[i], 0, sizeof(LineaVista));
        Repuesto rep;
        if (leerRepuestoPorID(o.lineas[i].idRepuesto, rep) == OK) {
            copiarTexto(lineas[i].codigo, 20, rep.codigo);
            copiarTexto(lineas[i].nombre, 100, rep.nombre);
        } else {
            copiarTexto(lineas[i].codigo, 20, "???");
            copiarTexto(lineas[i].nombre, 100, "(repuesto no encontrado)");
        }
        lineas[i].cantidad = o.lineas[i].cantidad;
        lineas[i].precioUnitario = o.lineas[i].precioUnitario;
        lineas[i].subtotal = o.lineas[i].cantidad * o.lineas[i].precioUnitario;
    }
    return OK;
}

Resultado listarOrdenes(const char* estado, int idVehiculo, OrdenVista resultados[],
                        int &cantidad, bool &hayMas, InfoError &info) {
    limpiarInfo(info);
    cantidad = 0;
    hayMas = false;
    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        OrdenServicio o;
        Resultado r = leerOrdenPorID(i, o);
        if (r != OK) return r;
        if (estado[0] != '\0' && strcmp(o.estado, estado) != 0) continue;
        if (idVehiculo != 0 && o.idVehiculo != idVehiculo) continue;
        if (cantidad < MAX_RESULTADOS) {
            construirVistaOrden(o, resultados[cantidad]);
            cantidad++;
        } else {
            hayMas = true;
            break;
        }
    }
    return OK;
}

// ---------------------------------------------------------------------
// REPORTES (leen desde los archivos sin cargarlos completos en memoria)
// ---------------------------------------------------------------------

// REPORTE 1: historial de un vehiculo (por placa)
Resultado reporteHistorialVehiculo(const char* placa, Vehiculo &vehiculo, OrdenVista ordenes[],
                                   int &cantidad, bool &hayMas, float &totalFacturado, InfoError &info) {
    cantidad = 0;
    hayMas = false;
    totalFacturado = 0;
    Resultado r = obtenerVehiculoPorPlaca(placa, vehiculo, info);
    if (r != OK) return r;

    ArchivoHeader h;
    if (!leerHeader(RUTA_ORDENES, h)) return ERR_ARCHIVO;
    for (int i = 1; i <= h.cantidadRegistros; i++) {
        OrdenServicio o;
        r = leerOrdenPorID(i, o);
        if (r != OK) return r;
        if (o.idVehiculo != vehiculo.id) continue;
        if (strcmp(o.estado, ESTADO_CERRADA) == 0) totalFacturado += calcularTotalOrden(o);
        if (cantidad < MAX_RESULTADOS) {
            construirVistaOrden(o, ordenes[cantidad]);
            cantidad++;
        } else {
            hayMas = true;
        }
    }
    return OK;
}

// REPORTE 2: los TOP_REPUESTOS repuestos con mas unidades consumidas en ordenes CERRADAS
Resultado reporteRepuestosMasUsados(RepuestoUso top[], int &cantidad, InfoError &info) {
    limpiarInfo(info);
    cantidad = 0;
    ArchivoHeader hr, ho;
    if (!leerHeader(RUTA_REPUESTOS, hr)) return ERR_ARCHIVO;
    if (!leerHeader(RUTA_ORDENES, ho)) return ERR_ARCHIVO;

    // unidades[id] = unidades consumidas del repuesto con ese ID

    int* unidades = new int[hr.cantidadRegistros + 1];
    for (int i = 0; i <= hr.cantidadRegistros; i++) unidades[i] = 0;

    Resultado resultado = OK;
    for (int i = 1; i <= ho.cantidadRegistros && resultado == OK; i++) {
        OrdenServicio o;
        resultado = leerOrdenPorID(i, o);
        if (resultado != OK) break;
        if (strcmp(o.estado, ESTADO_CERRADA) != 0) continue;
        for (int j = 0; j < o.numLineas && j < MAX_LINEAS; j++) {
            int idRep = o.lineas[j].idRepuesto;
            if (idRep >= 1 && idRep <= hr.cantidadRegistros) unidades[idRep] += o.lineas[j].cantidad;
        }
    }


    for (int k = 0; k < TOP_REPUESTOS && resultado == OK; k++) {
        int mejor = 0;
        for (int id = 1; id <= hr.cantidadRegistros; id++) {
            if (unidades[id] > 0 && (mejor == 0 || unidades[id] > unidades[mejor])) mejor = id;
        }
        if (mejor == 0) break;
        Repuesto rep;
        resultado = leerRepuestoPorID(mejor, rep);
        if (resultado != OK) break;
        copiarTexto(top[cantidad].codigo, 20, rep.codigo);
        copiarTexto(top[cantidad].nombre, 100, rep.nombre);
        top[cantidad].unidades = unidades[mejor];
        cantidad++;
        unidades[mejor] = 0;   
    }
    delete[] unidades;
    return resultado;
}

// REPORTE 3: facturacion por mecanico (activos y dados de baja), solo ordenes CERRADAS
Resultado reporteFacturacionMecanicos(FacturacionMecanico filas[], int &cantidad, bool &hayMas, InfoError &info) {
    limpiarInfo(info);
    cantidad = 0;
    hayMas = false;
    ArchivoHeader hm, ho;
    if (!leerHeader(RUTA_MECANICOS, hm)) return ERR_ARCHIVO;
    if (!leerHeader(RUTA_ORDENES, ho)) return ERR_ARCHIVO;

    for (int i = 1; i <= hm.cantidadRegistros; i++) {
        if (cantidad >= MAX_RESULTADOS) {
            hayMas = true;
            break;
        }
        Mecanico m;
        Resultado r = leerMecanicoPorID(i, m);
        if (r != OK) return r;

        FacturacionMecanico f;
        memset(&f, 0, sizeof(f));
        copiarTexto(f.nombre, 100, m.nombre);
        f.dadoDeBaja = m.eliminado;
        for (int j = 1; j <= ho.cantidadRegistros; j++) {
            OrdenServicio o;
            r = leerOrdenPorID(j, o);
            if (r != OK) return r;
            if (o.idMecanico != i || strcmp(o.estado, ESTADO_CERRADA) != 0) continue;
            f.ordenesCerradas++;
            f.manoObra += o.manoObra;
            f.repuestos += calcularTotalRepuestos(o);
        }
        filas[cantidad] = f;
        cantidad++;
    }
    return OK;
}

// ---------------------------------------------------------------------
// MANTENIMIENTO
// ---------------------------------------------------------------------

void agregarProblema(ReporteIntegridad &rep, const char* texto) {
    rep.problemas++;
    if (rep.guardados < MAX_PROBLEMAS) {
        copiarTexto(rep.mensajes[rep.guardados], 160, texto);
        rep.guardados++;
    }
}

void verificarHeader(ReporteIntegridad &rep, const char* nombreArchivo, const ArchivoHeader &h,
                     int real, long sobrante, int activosContados) {
    char msg[200];
    if (h.cantidadRegistros != real) {
        snprintf(msg, sizeof(msg), "ROTO: Header de %s indica %d registros; el archivo contiene %d",
                 nombreArchivo, h.cantidadRegistros, real);
        agregarProblema(rep, msg);
    }
    if (h.registrosActivos != activosContados) {
        snprintf(msg, sizeof(msg), "ROTO: Header de %s indica %d registros activos; se contaron %d",
                 nombreArchivo, h.registrosActivos, activosContados);
        agregarProblema(rep, msg);
    }
    if (h.proximoID != h.cantidadRegistros + 1) {
        snprintf(msg, sizeof(msg), "ROTO: Header de %s tiene proximoID %d; se esperaba %d",
                 nombreArchivo, h.proximoID, h.cantidadRegistros + 1);
        agregarProblema(rep, msg);
    }
    if (sobrante != 0) {
        snprintf(msg, sizeof(msg), "ROTO: %s tiene %ld bytes sobrantes que no forman un registro completo",
                 nombreArchivo, sobrante);
        agregarProblema(rep, msg);
    }
}


Resultado verificarIntegridad(ReporteIntegridad &rep) {
    memset(&rep, 0, sizeof(rep));
    char msg[200];

    ArchivoHeader hv, hm, hr, ho;
    if (!leerHeader(RUTA_VEHICULOS, hv)) return ERR_ARCHIVO;
    if (!leerHeader(RUTA_MECANICOS, hm)) return ERR_ARCHIVO;
    if (!leerHeader(RUTA_REPUESTOS, hr)) return ERR_ARCHIVO;
    if (!leerHeader(RUTA_ORDENES, ho)) return ERR_ARCHIVO;


    int realV = 0, realM = 0, realR = 0, realO = 0;
    long sobraV = 0, sobraM = 0, sobraR = 0, sobraO = 0;
    if (!contarRegistrosEnArchivo(RUTA_VEHICULOS, sizeof(Vehiculo), realV, sobraV)) return ERR_ARCHIVO;
    if (!contarRegistrosEnArchivo(RUTA_MECANICOS, sizeof(Mecanico), realM, sobraM)) return ERR_ARCHIVO;
    if (!contarRegistrosEnArchivo(RUTA_REPUESTOS, sizeof(Repuesto), realR, sobraR)) return ERR_ARCHIVO;
    if (!contarRegistrosEnArchivo(RUTA_ORDENES, sizeof(OrdenServicio), realO, sobraO)) return ERR_ARCHIVO;
    rep.vehiculos = realV;
    rep.mecanicos = realM;
    rep.repuestos = realR;
    rep.ordenes = realO;

    int activosV = 0;
    for (int i = 1; i <= realV; i++) {
        Vehiculo v;
        if (leerVehiculoPorID(i, v) != OK) continue;
        if (!v.eliminado) activosV++;
    }

    // --- Mecanicos ---
    int activosM = 0;
    for (int i = 1; i <= realM; i++) {
        Mecanico m;
        if (leerMecanicoPorID(i, m) != OK) continue;
        if (!m.eliminado) activosM++;
    }

    // --- Repuestos: d) stock negativo ---
    int activosR = 0;
    for (int i = 1; i <= realR; i++) {
        Repuesto r;
        if (leerRepuestoPorID(i, r) != OK) continue;
        if (!r.eliminado) activosR++;
        if (r.stock < 0) {
            snprintf(msg, sizeof(msg), "ROTO: El repuesto '%s' (ID %d) tiene stock negativo (%d)",
                     r.nombre, r.id, r.stock);
            agregarProblema(rep, msg);
        }
    }

    // --- Ordenes: a), b), c) ---
    int activasO = 0;
    for (int i = 1; i <= realO; i++) {
        OrdenServicio o;
        if (leerOrdenPorID(i, o) != OK) continue;
        if (!o.eliminado) activasO++;

        // a) y b) el vehiculo
        Vehiculo v;
        Resultado rv = leerVehiculoPorID(o.idVehiculo, v);
        if (rv == ERR_NO_EXISTE) {
            snprintf(msg, sizeof(msg), "ROTO: Orden #%d apunta al vehiculo ID %d (no existe en vehiculos.bin)",
                     o.id, o.idVehiculo);
            agregarProblema(rep, msg);
        } else if (rv == OK && v.eliminado) {
            snprintf(msg, sizeof(msg), "ROTO: Orden #%d apunta al vehiculo '%s' (ID %d) que esta eliminado",
                     o.id, v.placa, v.id);
            agregarProblema(rep, msg);
        }

        // a) y b) el mecanico (dado de baja solo es problema en ordenes ABIERTAS)
        Mecanico m;
        Resultado rm = leerMecanicoPorID(o.idMecanico, m);
        if (rm == ERR_NO_EXISTE) {
            snprintf(msg, sizeof(msg), "ROTO: Orden #%d apunta al mecanico ID %d (no existe en mecanicos.bin)",
                     o.id, o.idMecanico);
            agregarProblema(rep, msg);
        } else if (rm == OK && m.eliminado && strcmp(o.estado, ESTADO_ABIERTA) == 0) {
            snprintf(msg, sizeof(msg), "ROTO: Orden #%d esta ABIERTA pero su mecanico '%s' (ID %d) esta dado de baja",
                     o.id, m.nombre, m.id);
            agregarProblema(rep, msg);
        }

        // c) las lineas de detalle
        if (o.numLineas < 0 || o.numLineas > MAX_LINEAS) {
            snprintf(msg, sizeof(msg), "ROTO: Orden #%d tiene un numero de lineas invalido (%d)",
                     o.id, o.numLineas);
            agregarProblema(rep, msg);
            continue;
        }
        for (int j = 0; j < o.numLineas; j++) {
            Repuesto r;
            if (leerRepuestoPorID(o.lineas[j].idRepuesto, r) == ERR_NO_EXISTE) {
                snprintf(msg, sizeof(msg), "ROTO: Orden #%d, linea %d: el repuesto ID %d no existe en repuestos.bin",
                         o.id, j + 1, o.lineas[j].idRepuesto);
                agregarProblema(rep, msg);
            }
        }
    }

    // e) los headers contra el contenido real
    verificarHeader(rep, "vehiculos.bin", hv, realV, sobraV, activosV);
    verificarHeader(rep, "mecanicos.bin", hm, realM, sobraM, activosM);
    verificarHeader(rep, "repuestos.bin", hr, realR, sobraR, activosR);
    verificarHeader(rep, "ordenes.bin", ho, realO, sobraO, activasO);
    return OK;
}


Resultado crearBackup(char rutaBackup[], int tam) {
    time_t ahora = time(nullptr);
    tm* f = localtime(&ahora);
    snprintf(rutaBackup, tam, "backup_%04d-%02d-%02d_%02d-%02d-%02d",
             (f->tm_year + 1900) % 10000, f->tm_mon + 1, f->tm_mday, f->tm_hour, f->tm_min, f->tm_sec);

    if (!crearCarpeta(rutaBackup)) return ERR_ARCHIVO;
    for (int i = 0; i < CANTIDAD_ARCHIVOS; i++) {
        const char* origen = TODOS_LOS_ARCHIVOS[i];
        const char* nombre = strrchr(origen, '/');          // solo el nombre, sin "datos/"
        nombre = (nombre == nullptr) ? origen : nombre + 1;
        char destino[300];
        snprintf(destino, sizeof(destino), "%s/%s", rutaBackup, nombre);
        if (!copiarArchivo(origen, destino)) return ERR_ARCHIVO;
    }
    return OK;
}

// =====================================================================
// PARTE 3: CAPA DE PRESENTACION
// Menus y pantallas. Usa cin/cout, llama a la logica y muestra resultados.
// NO abre archivos y NO modifica datos directamente: todo pasa por la logica.
// =====================================================================

const int ANCHO_MENU    = 44;
const int ANCHO_DETALLE = 70;
const int SIN_LIMITE    = 2000000000;

bool entradaTerminada = false;   
bool modoEco = false;            

// ---------------------------------------------------------------------
// Lectura de datos (con validacion y opcion CANCELAR)
// ---------------------------------------------------------------------


bool leerLinea(string &linea) {
    if (!getline(cin, linea)) {
        entradaTerminada = true;
        return false;
    }
    if (!linea.empty() && linea[linea.size() - 1] == '\r') linea.erase(linea.size() - 1);
    if (modoEco) cout << linea << endl;
    return true;
}


void recortar(string &s) {
    size_t ini = 0;
    while (ini < s.size() && isspace((unsigned char)s[ini])) ini++;
    size_t fin = s.size();
    while (fin > ini && isspace((unsigned char)s[fin - 1])) fin--;
    s = s.substr(ini, fin - ini);
}

bool esCancelar(const string &s) {
    if (s.size() != 8) return false;
    for (size_t i = 0; i < s.size(); i++) {
        if (toupper((unsigned char)s[i]) != "CANCELAR"[i]) return false;
    }
    return true;
}


bool pedirTexto(const char* mensaje, char destino[], int tam) {
    string linea;
    while (true) {
        cout << mensaje << flush;
        if (!leerLinea(linea)) return false;
        recortar(linea);
        if (esCancelar(linea)) return false;
        if (linea.empty()) {
            cout << "ERROR: Este campo no puede estar vacío.\n";
            continue;
        }
        if ((int)linea.size() >= tam) {
            cout << "ERROR: Texto demasiado largo (máximo " << tam - 1 << " caracteres).\n";
            continue;
        }
        copiarTexto(destino, tam, linea.c_str());
        return true;
    }
}


bool pedirTextoValido(const char* mensaje, char destino[], int tam,
                      bool (*valida)(const char*), const char* textoError) {
    while (true) {
        if (!pedirTexto(mensaje, destino, tam)) return false;
        if (valida(destino)) return true;
        cout << "ERROR: " << textoError << "\n";
    }
}


bool pedirEnteroEnRango(const char* mensaje, int &valor, int minimo, int maximo) {
    string linea;
    while (true) {
        cout << mensaje << flush;
        if (!leerLinea(linea)) return false;
        recortar(linea);
        if (esCancelar(linea)) return false;

        char* fin = nullptr;
        long n = strtol(linea.c_str(), &fin, 10);
        if (linea.empty() || *fin != '\0' || n > 2000000000L || n < -2000000000L) {
            cout << "ERROR: Debe ingresar un número entero.\n";
            continue;
        }
        if (n < minimo || n > maximo) {
            if (maximo >= SIN_LIMITE) cout << "ERROR: El valor debe ser mayor o igual a " << minimo << ".\n";
            else cout << "ERROR: El valor debe estar entre " << minimo << " y " << maximo << ".\n";
            continue;
        }
        valor = (int)n;
        return true;
    }
}


bool pedirDecimal(const char* mensaje, float &valor, float minimo, bool estricto) {
    string linea;
    while (true) {
        cout << mensaje << flush;
        if (!leerLinea(linea)) return false;
        recortar(linea);
        if (esCancelar(linea)) return false;
        for (size_t i = 0; i < linea.size(); i++) {
            if (linea[i] == ',') linea[i] = '.';
        }
        char* fin = nullptr;
        double n = strtod(linea.c_str(), &fin);
        if (linea.empty() || *fin != '\0') {
            cout << "ERROR: Debe ingresar un número (ejemplo: 25.50).\n";
            continue;
        }
        if (estricto ? (n <= minimo) : (n < minimo)) {
            cout << "ERROR: El valor debe ser " << (estricto ? "mayor que " : "mayor o igual a ") << minimo << ".\n";
            continue;
        }
        valor = (float)n;
        return true;
    }
}

bool pedirSN(const char* mensaje) {
    string linea;
    while (true) {
        cout << mensaje << flush;
        if (!leerLinea(linea)) return false;
        recortar(linea);
        if (linea == "S" || linea == "s") return true;
        if (linea == "N" || linea == "n") return false;
        cout << "ERROR: Responda S o N.\n";
    }
}

int pedirOpcion() {
    int op = 0;
    if (!pedirEnteroEnRango("Seleccione una opción: ", op, 0, SIN_LIMITE - 1)) return 0;
    return op;
}

// ---------------------------------------------------------------------
// Dibujo de cajas y tablas
// ---------------------------------------------------------------------

void repetir(const char* simbolo, int veces) {
    for (int i = 0; i < veces; i++) cout << simbolo;
}

int anchoVisible(const char* s) {
    int n = 0;
    for (; *s != '\0'; s++) {
        if (((unsigned char)*s & 0xC0) != 0x80) n++;
    }
    return n;
}

void imprimirCelda(const char* texto, int ancho, bool alinearDerecha) {
    int visible = anchoVisible(texto);
    if (visible > ancho) {
        int contados = 0;
        for (const char* p = texto; *p != '\0'; p++) {
            if (((unsigned char)*p & 0xC0) != 0x80) {
                if (contados == ancho) break;
                contados++;
            }
            cout << *p;
        }
        return;
    }
    int relleno = ancho - visible;
    if (alinearDerecha) {
        for (int i = 0; i < relleno; i++) cout << ' ';
        cout << texto;
    } else {
        cout << texto;
        for (int i = 0; i < relleno; i++) cout << ' ';
    }
}

void cajaBorde(int ancho, const char* izq, const char* der) {
    cout << izq;
    repetir("═", ancho);
    cout << der << "\n";
}

void cajaLinea(int ancho, const char* texto) {
    cout << "║ ";
    imprimirCelda(texto, ancho - 2, false);
    cout << " ║\n";
}

void cajaCentrada(int ancho, const char* texto) {
    int izq = (ancho - 2 - anchoVisible(texto)) / 2;
    if (izq < 0) izq = 0;
    string s(izq, ' ');
    s += texto;
    cajaLinea(ancho, s.c_str());
}

void mostrarMenu(const char* titulo, const char* subtitulo, const char* const opciones[], int cantidad) {
    cout << "\n";
    cajaBorde(ANCHO_MENU, "╔", "╗");
    cajaCentrada(ANCHO_MENU, titulo);
    if (subtitulo[0] != '\0') cajaCentrada(ANCHO_MENU, subtitulo);
    cajaBorde(ANCHO_MENU, "╠", "╣");
    for (int i = 0; i < cantidad; i++) cajaLinea(ANCHO_MENU, opciones[i]);
    cajaBorde(ANCHO_MENU, "╚", "╝");
}

void tablaBorde(const int anchos[], int columnas, const char* izq, const char* medio, const char* der) {
    cout << izq;
    for (int i = 0; i < columnas; i++) {
        repetir("═", anchos[i] + 2);
        cout << (i < columnas - 1 ? medio : der);
    }
    cout << "\n";
}

void tablaFila(const int anchos[], const char* textos[], const bool derecha[], int columnas) {
    cout << "║";
    for (int i = 0; i < columnas; i++) {
        cout << " ";
        imprimirCelda(textos[i], anchos[i], derecha[i]);
        cout << " ║";
    }
    cout << "\n";
}

void tablaEncabezado(const int anchos[], const char* titulos[], int columnas) {
    bool izquierda[10] = {false, false, false, false, false, false, false, false, false, false};
    tablaBorde(anchos, columnas, "╔", "╦", "╗");
    tablaFila(anchos, titulos, izquierda, columnas);
    tablaBorde(anchos, columnas, "╠", "╬", "╣");
}

// ---------------------------------------------------------------------
// Mensajes de error (traducen el Resultado a un texto)
// ---------------------------------------------------------------------

void mostrarError(Resultado r, const InfoError &info) {
    switch (r) {
        case OK:
            return;
        case ERR_ARCHIVO:
            cout << "ERROR: No se pudo abrir o leer el archivo '" << depuracionArchivoConError()
                 << "'. La operación no continúa.\n";
            break;
        case ERR_ARCHIVO_CORRUPTO:
            cout << "ERROR: El header de un archivo no coincide con su contenido. "
                 << "Ejecute la verificación de integridad (menú Mantenimiento).\n";
            break;
        case ERR_NO_EXISTE:
            cout << "ERROR: El ID indicado no existe.\n";
            break;
        case ERR_ELIMINADO:
            cout << "ERROR: El registro ya estaba eliminado.\n";
            break;
        case ERR_CAMPO_VACIO:
            cout << "ERROR: El campo '" << info.texto << "' no puede estar vacío.\n";
            break;
        case ERR_PLACA_INVALIDA:
            cout << "ERROR: La placa '" << info.texto << "' no es válida (6 o 7 caracteres alfanuméricos).\n";
            break;
        case ERR_PLACA_DUPLICADA:
            cout << "ERROR: La placa '" << info.texto << "' ya está registrada.\n";
            break;
        case ERR_ANIO_INVALIDO:
            cout << "ERROR: El año " << info.cantidad << " no es válido (debe estar entre "
                 << ANIO_MINIMO << " y " << ANIO_MAXIMO << ").\n";
            break;
        case ERR_KM_INVALIDO:
            cout << "ERROR: El kilometraje " << info.cantidad << " no es válido (debe ser mayor o igual a 0).\n";
            break;
        case ERR_CEDULA_INVALIDA:
            cout << "ERROR: La cédula '" << info.texto
                 << "' no es válida (solo dígitos; se admite prefijo V-, E- o J-).\n";
            break;
        case ERR_CEDULA_DUPLICADA:
            cout << "ERROR: La cédula '" << info.texto << "' ya está registrada para otro mecánico.\n";
            break;
        case ERR_ESPECIALIDAD_INVALIDA:
            cout << "ERROR: La especialidad '" << info.texto << "' no es válida.\n";
            break;
        case ERR_FECHA_INVALIDA:
            cout << "ERROR: La fecha '" << info.texto << "' no es válida (formato AAAA-MM-DD).\n";
            break;
        case ERR_CODIGO_DUPLICADO:
            cout << "ERROR: El código '" << info.texto << "' ya está registrado.\n";
            break;
        case ERR_PRECIO_INVALIDO:
            cout << "ERROR: El precio " << info.texto << " no es válido (debe ser mayor que 0).\n";
            break;
        case ERR_STOCK_INVALIDO:
            cout << "ERROR: El stock " << info.cantidad << " no es válido (debe ser mayor o igual a 0).\n";
            break;
        case ERR_STOCK_MINIMO_INVALIDO:
            cout << "ERROR: El stock mínimo " << info.cantidad << " no es válido (debe ser mayor o igual a 0).\n";
            break;
        case ERR_CANTIDAD_INVALIDA:
            cout << "ERROR: La cantidad " << info.solicitado << " no es válida (debe ser mayor que 0).\n";
            break;
        case ERR_MANO_OBRA_INVALIDA:
            cout << "ERROR: La mano de obra " << info.texto << " no es válida (debe ser mayor o igual a 0).\n";
            break;
        case ERR_VEHICULO_NO_EXISTE:
            cout << "ERROR: No existe un vehículo con ID " << info.idRelacionado;
            if (info.texto[0] != '\0') cout << " (placa '" << info.texto << "')";
            cout << ".\n";
            break;
        case ERR_VEHICULO_ELIMINADO:
            cout << "ERROR: El vehículo '" << info.texto << "' (ID " << info.idRelacionado
                 << ") fue eliminado.\n";
            break;
        case ERR_MECANICO_NO_EXISTE:
            cout << "ERROR: No existe un mecánico con ID " << info.idRelacionado << ".\n";
            break;
        case ERR_MECANICO_ELIMINADO:
            cout << "ERROR: El mecánico '" << info.texto << "' (ID " << info.idRelacionado
                 << ") está dado de baja.\n";
            break;
        case ERR_REPUESTO_NO_EXISTE:
            if (info.idRelacionado != 0) cout << "ERROR: No existe un repuesto con ID " << info.idRelacionado << ".\n";
            else cout << "ERROR: No existe un repuesto con el código '" << info.texto << "'.\n";
            break;
        case ERR_REPUESTO_ELIMINADO:
            cout << "ERROR: El repuesto '" << info.texto << "' (ID " << info.idRelacionado
                 << ") fue eliminado.\n";
            break;
        case ERR_ORDEN_NO_EXISTE:
            cout << "ERROR: No existe la orden #" << info.idRelacionado << ".\n";
            break;
        case ERR_VEHICULO_CON_ORDENES:
            cout << "ERROR: El vehículo '" << info.texto << "' tiene " << info.cantidad
                 << " órdenes registradas y no puede eliminarse.\n";
            break;
        case ERR_MECANICO_CON_ABIERTAS:
            cout << "ERROR: El mecánico '" << info.texto << "' tiene " << info.cantidad
                 << " órdenes ABIERTAS y no puede darse de baja.\n";
            break;
        case ERR_REPUESTO_EN_USO:
            cout << "ERROR: El repuesto '" << info.texto << "' aparece en el detalle de la orden #"
                 << info.idRelacionado << " y no puede eliminarse.\n";
            break;
        case ERR_VEHICULO_YA_TIENE_ABIERTA:
            cout << "ERROR: El vehículo '" << info.texto << "' ya tiene la orden #"
                 << info.idRelacionado << " ABIERTA.\n";
            break;
        case ERR_MECANICO_LLENO:
            cout << "ERROR: El mecánico '" << info.texto << "' ya tiene " << info.cantidad
                 << " órdenes abiertas.\n";
            break;
        case ERR_KM_MENOR:
            cout << "ERROR: El kilometraje " << info.solicitado << " es menor al último registrado del vehículo '"
                 << info.texto << "' (" << info.cantidad << " km).\n";
            break;
        case ERR_ESTADO_INCORRECTO:
            cout << "ERROR: La orden #" << info.idRelacionado << " está " << info.texto
                 << " y no admite esta operación.\n";
            break;
        case ERR_STOCK_INSUFICIENTE:
            cout << "ERROR: Stock insuficiente de '" << info.texto << "'. Disponible: " << info.cantidad
                 << ", solicitado: " << info.solicitado << ".\n";
            break;
        case ERR_LIMITE_LINEAS:
            cout << "ERROR: La orden #" << info.idRelacionado << " ya tiene el máximo de " << info.cantidad
                 << " líneas de detalle. El repuesto no se agregó.\n";
            break;
        case ERR_LINEA_NO_EXISTE:
            cout << "ERROR: El repuesto '" << info.texto << "' no está en la orden #" << info.idRelacionado << ".\n";
            break;
        case ERR_ORDEN_VACIA:
            cout << "ERROR: La orden #" << info.idRelacionado
                 << " no puede cerrarse sin repuestos y sin mano de obra.\n";
            break;
    }
}

// ---------------------------------------------------------------------
// Pantallas para mostrar datos
// ---------------------------------------------------------------------

void mostrarVehiculo(const Vehiculo &v) {
    cout << "  Vehículo    : " << v.placa << " - " << v.marca << " " << v.modelo << " " << v.anio << "\n";
    cout << "  Kilometraje : " << v.kilometraje << " km\n";
    cout << "  Propietario : " << v.nombrePropietario << " (" << v.cedulaPropietario << ")\n";
    cout << "  Teléfono    : " << v.telefonoPropietario << "\n";
}

void mostrarListaVehiculos(const Vehiculo vehiculos[], int cantidad, bool hayMas) {
    int anchos[6] = {4, 9, 24, 5, 8, 24};
    const char* titulos[6] = {"ID", "Placa", "Vehículo", "Año", "Km", "Propietario"};
    bool der[6] = {true, false, false, true, true, false};
    cout << "\n";
    tablaEncabezado(anchos, titulos, 6);
    for (int i = 0; i < cantidad; i++) {
        char c0[20], c1[20], c2[120], c3[20], c4[20], c5[120];
        snprintf(c0, sizeof(c0), "%d", vehiculos[i].id);
        snprintf(c1, sizeof(c1), "%s", vehiculos[i].placa);
        snprintf(c2, sizeof(c2), "%s %s", vehiculos[i].marca, vehiculos[i].modelo);
        snprintf(c3, sizeof(c3), "%d", vehiculos[i].anio);
        snprintf(c4, sizeof(c4), "%d", vehiculos[i].kilometraje);
        snprintf(c5, sizeof(c5), "%s", vehiculos[i].nombrePropietario);
        const char* fila[6] = {c0, c1, c2, c3, c4, c5};
        tablaFila(anchos, fila, der, 6);
    }
    tablaBorde(anchos, 6, "╚", "╩", "╝");
    cout << "Total de vehículos: " << cantidad << "\n";
    if (hayMas) cout << "AVISO: Hay más resultados. Solo se muestran los primeros " << MAX_RESULTADOS << ".\n";
}

void mostrarMecanico(const Mecanico &m) {
    cout << "  Mecánico      : " << m.nombre << (m.eliminado ? "  [DADO DE BAJA]" : "") << "\n";
    cout << "  Cédula        : " << m.cedula << "\n";
    cout << "  Especialidad  : " << m.especialidad << "\n";
    cout << "  Fecha ingreso : " << m.fechaIngreso << "\n";
}

void mostrarListaMecanicos(const MecanicoVista mecanicos[], int cantidad, bool hayMas) {
    int anchos[7] = {4, 24, 12, 13, 10, 8, 8};
    const char* titulos[7] = {"ID", "Nombre", "Cédula", "Especialidad", "Ingreso", "Abiertas", "Estado"};
    bool der[7] = {true, false, false, false, false, true, false};
    cout << "\n";
    tablaEncabezado(anchos, titulos, 7);
    for (int i = 0; i < cantidad; i++) {
        const Mecanico &m = mecanicos[i].mecanico;
        char c0[20], c5[20];
        snprintf(c0, sizeof(c0), "%d", m.id);
        snprintf(c5, sizeof(c5), "%d", mecanicos[i].ordenesAbiertas);
        const char* fila[7] = {c0, m.nombre, m.cedula, m.especialidad, m.fechaIngreso, c5,
                               m.eliminado ? "BAJA" : "Activo"};
        tablaFila(anchos, fila, der, 7);
    }
    tablaBorde(anchos, 7, "╚", "╩", "╝");
    cout << "Total de mecánicos: " << cantidad << "\n";
    if (hayMas) cout << "AVISO: Hay más resultados. Solo se muestran los primeros " << MAX_RESULTADOS << ".\n";
}

void mostrarRepuesto(const Repuesto &r) {
    cout << "  Repuesto    : " << r.codigo << " - " << r.nombre << " (" << r.marca << ")\n";
    cout << "  Precio      : " << r.precio << "\n";
    cout << "  Stock       : " << r.stock << " (mínimo " << r.stockMinimo << ")"
         << (r.stock <= r.stockMinimo ? "  *** BAJO STOCK ***" : "") << "\n";
}

void mostrarListaRepuestos(const Repuesto repuestos[], int cantidad, bool hayMas) {
    int anchos[8] = {4, 10, 26, 14, 9, 6, 5, 10};
    const char* titulos[8] = {"ID", "Código", "Nombre", "Marca", "Precio", "Stock", "Mín", "Alerta"};
    bool der[8] = {true, false, false, false, true, true, true, false};
    cout << "\n";
    tablaEncabezado(anchos, titulos, 8);
    for (int i = 0; i < cantidad; i++) {
        const Repuesto &r = repuestos[i];
        char c0[20], c4[20], c5[20], c6[20];
        snprintf(c0, sizeof(c0), "%d", r.id);
        snprintf(c4, sizeof(c4), "%.2f", r.precio);
        snprintf(c5, sizeof(c5), "%d", r.stock);
        snprintf(c6, sizeof(c6), "%d", r.stockMinimo);
        const char* fila[8] = {c0, r.codigo, r.nombre, r.marca, c4, c5, c6,
                               r.stock <= r.stockMinimo ? "BAJO STOCK" : ""};
        tablaFila(anchos, fila, der, 8);
    }
    tablaBorde(anchos, 8, "╚", "╩", "╝");
    cout << "Total de repuestos: " << cantidad << "\n";
    if (hayMas) cout << "AVISO: Hay más resultados. Solo se muestran los primeros " << MAX_RESULTADOS << ".\n";
}

void mostrarListaOrdenes(const OrdenVista ordenes[], int cantidad, bool hayMas) {
    int anchos[6] = {6, 8, 10, 26, 24, 10};
    const char* titulos[6] = {"Orden", "Estado", "Apertura", "Vehículo", "Mecánico", "Total"};
    bool der[6] = {true, false, false, false, false, true};
    cout << "\n";
    tablaEncabezado(anchos, titulos, 6);
    for (int i = 0; i < cantidad; i++) {
        const OrdenVista &v = ordenes[i];
        char c0[20], c3[160], c4[160], c5[30];
        snprintf(c0, sizeof(c0), "#%d", v.orden.id);
        snprintf(c3, sizeof(c3), "%s - %s %s", v.placa, v.marca, v.modelo);
        snprintf(c4, sizeof(c4), "%s%s", v.nombreMecanico, v.mecanicoDeBaja ? " (BAJA)" : "");
        snprintf(c5, sizeof(c5), "%.2f", v.total);
        const char* fila[6] = {c0, v.orden.estado, v.orden.fechaApertura, c3, c4, c5};
        tablaFila(anchos, fila, der, 6);
    }
    tablaBorde(anchos, 6, "╚", "╩", "╝");
    cout << "Total de órdenes: " << cantidad << "\n";
    if (hayMas) cout << "AVISO: Hay más resultados. Solo se muestran los primeros " << MAX_RESULTADOS << ".\n";
}

void mostrarDetalleOrden(const OrdenVista &v, const LineaVista lineas[]) {
    const OrdenServicio &o = v.orden;
    char buf[400];

    cout << "\n";
    cajaBorde(ANCHO_DETALLE, "╔", "╗");
    snprintf(buf, sizeof(buf), "ORDEN DE SERVICIO #%d", o.id);
    cajaCentrada(ANCHO_DETALLE, buf);
    cajaBorde(ANCHO_DETALLE, "╠", "╣");
    snprintf(buf, sizeof(buf), "Estado    : %s", o.estado);
    cajaLinea(ANCHO_DETALLE, buf);
    snprintf(buf, sizeof(buf), "Apertura  : %s", o.fechaApertura);
    cajaLinea(ANCHO_DETALLE, buf);
    if (o.fechaCierre[0] != '\0') {
        snprintf(buf, sizeof(buf), "Cierre    : %s", o.fechaCierre);
        cajaLinea(ANCHO_DETALLE, buf);
    }
    snprintf(buf, sizeof(buf), "Vehículo  : %s - %s %s %d (%d km)", v.placa, v.marca, v.modelo, v.anio,
             o.kilometrajeIngreso);
    cajaLinea(ANCHO_DETALLE, buf);
    snprintf(buf, sizeof(buf), "Dueño     : %s", v.nombrePropietario);
    cajaLinea(ANCHO_DETALLE, buf);
    snprintf(buf, sizeof(buf), "Mecánico  : %s (%s)%s", v.nombreMecanico, v.especialidad,
             v.mecanicoDeBaja ? " - DADO DE BAJA" : "");
    cajaLinea(ANCHO_DETALLE, buf);
    snprintf(buf, sizeof(buf), "Falla     : %s", o.descripcionFalla);
    cajaLinea(ANCHO_DETALLE, buf);
    cajaBorde(ANCHO_DETALLE, "╚", "╝");

    int anchos[5] = {9, 28, 4, 9, 10};
    const char* titulos[5] = {"Código", "Repuesto", "Cant", "P.Unit", "Total"};
    bool der[5] = {false, false, true, true, true};
    tablaEncabezado(anchos, titulos, 5);
    if (o.numLineas == 0) {
        const char* vacia[5] = {"", "(sin repuestos)", "", "", ""};
        tablaFila(anchos, vacia, der, 5);
    }
    for (int i = 0; i < o.numLineas && i < MAX_LINEAS; i++) {
        char c2[20], c3[20], c4[20];
        snprintf(c2, sizeof(c2), "%d", lineas[i].cantidad);
        snprintf(c3, sizeof(c3), "%.2f", lineas[i].precioUnitario);
        snprintf(c4, sizeof(c4), "%.2f", lineas[i].subtotal);
        const char* fila[5] = {lineas[i].codigo, lineas[i].nombre, c2, c3, c4};
        tablaFila(anchos, fila, der, 5);
    }
    tablaBorde(anchos, 5, "╚", "╩", "╝");

    cajaBorde(ANCHO_DETALLE, "╔", "╗");
    snprintf(buf, sizeof(buf), "%48s %12.2f", "Repuestos:", v.totalRepuestos);
    cajaLinea(ANCHO_DETALLE, buf);
    snprintf(buf, sizeof(buf), "%48s %12.2f", "Mano de obra:", o.manoObra);
    cajaLinea(ANCHO_DETALLE, buf);
    snprintf(buf, sizeof(buf), "%48s %12.2f", "TOTAL:", v.total);
    cajaLinea(ANCHO_DETALLE, buf);
    cajaBorde(ANCHO_DETALLE, "╚", "╝");
}

bool pedirYMostrarOrden(const char* mensaje, int &idOrden) {
    if (!pedirEnteroEnRango(mensaje, idOrden, 1, SIN_LIMITE)) return false;
    OrdenVista vista;
    LineaVista lineas[MAX_LINEAS];
    InfoError info;
    Resultado r = obtenerDetalleOrden(idOrden, vista, lineas, info);
    if (r != OK) {
        mostrarError(r, info);
        return false;
    }
    mostrarDetalleOrden(vista, lineas);
    return true;
}

// ---------------------------------------------------------------------
// MENU DE VEHICULOS
// ---------------------------------------------------------------------

void menuRegistrarVehiculo() {
    Vehiculo v;
    memset(&v, 0, sizeof(v));
    cout << "\n--- REGISTRAR VEHÍCULO (escriba CANCELAR para cancelar) ---\n";
    bool completo =
        pedirTextoValido("Placa (6 o 7 caracteres alfanuméricos): ", v.placa, 10, esPlacaValida,
                         "La placa debe ser alfanumérica, de 6 a 7 caracteres.") &&
        pedirTexto("Marca: ", v.marca, 50) &&
        pedirTexto("Modelo: ", v.modelo, 50) &&
        pedirEnteroEnRango("Año (1950-2027): ", v.anio, ANIO_MINIMO, ANIO_MAXIMO) &&
        pedirEnteroEnRango("Kilometraje: ", v.kilometraje, 0, SIN_LIMITE) &&
        pedirTexto("Nombre del propietario: ", v.nombrePropietario, 100) &&
        pedirTextoValido("Cédula del propietario (ej: V-12345678): ", v.cedulaPropietario, 20, esCedulaValida,
                         "La cédula debe tener solo dígitos (prefijo V-, E- o J- opcional).") &&
        pedirTexto("Teléfono del propietario: ", v.telefonoPropietario, 20);
    if (!completo) {
        cout << "Registro cancelado. No se creó ningún vehículo.\n";
        return;
    }
    if (!pedirSN("¿Guardar este vehículo? (S/N): ")) {
        cout << "Registro cancelado. No se creó ningún vehículo.\n";
        return;
    }
    InfoError info;
    Resultado r = agregarVehiculo(v, info);
    if (r == OK) cout << "Vehículo registrado: " << v.placa << " - " << v.marca << " " << v.modelo
                      << " (ID " << v.id << ").\n";
    else mostrarError(r, info);
}

void menuBuscarVehiculo() {
    cout << "\nBuscar por: 1. ID   2. Placa exacta   3. Propietario (parcial)\n";
    int tipo = 0;
    if (!pedirEnteroEnRango("Opción: ", tipo, 1, 3)) return;

    InfoError info;
    Vehiculo v;
    Resultado r;
    if (tipo == 1) {
        int id = 0;
        if (!pedirEnteroEnRango("ID del vehículo: ", id, 1, SIN_LIMITE)) return;
        r = obtenerVehiculoPorID(id, v, info);
        if (r == OK) {
            cout << "\nVehículo encontrado:\n";
            mostrarVehiculo(v);
        } else {
            mostrarError(r, info);
        }

        if (r == OK || r == ERR_VEHICULO_ELIMINADO) {
            cout << "[DEBUG] Registro ID " << id << " leído directamente en el byte " << depuracionUltimaPosicion()
                 << " (= " << sizeof(ArchivoHeader) << " + " << (id - 1) << " * " << sizeof(Vehiculo)
                 << "), sin recorrer el archivo.\n";
        }
    } else if (tipo == 2) {
        char placa[10];
        if (!pedirTexto("Placa: ", placa, 10)) return;
        r = obtenerVehiculoPorPlaca(placa, v, info);
        if (r == OK) {
            cout << "\nVehículo encontrado:\n";
            mostrarVehiculo(v);
        } else {
            mostrarError(r, info);
        }
    } else {
        char texto[100];
        if (!pedirTexto("Nombre del propietario (parcial): ", texto, 100)) return;
        Vehiculo encontrados[MAX_RESULTADOS];
        int cantidad = 0;
        bool hayMas = false;
        r = buscarVehiculosPorPropietario(texto, encontrados, cantidad, hayMas, info);
        if (r != OK) {
            mostrarError(r, info);
        } else if (cantidad == 0) {
            cout << "No se encontraron vehículos con propietario que contenga '" << texto << "'.\n";
        } else {
            mostrarListaVehiculos(encontrados, cantidad, hayMas);
        }
    }
}

void menuActualizarVehiculo() {
    int id = 0;
    cout << "\n--- ACTUALIZAR VEHÍCULO (escriba CANCELAR para cancelar) ---\n";
    if (!pedirEnteroEnRango("ID del vehículo: ", id, 1, SIN_LIMITE)) return;

    InfoError info;
    Vehiculo actual;
    Resultado r = obtenerVehiculoPorID(id, actual, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    cout << "Datos actuales:\n";
    mostrarVehiculo(actual);
    cout << "(La placa y el kilometraje no se pueden cambiar.)\n";

    Vehiculo nuevo;
    memset(&nuevo, 0, sizeof(nuevo));
    bool completo =
        pedirTexto("Nueva marca: ", nuevo.marca, 50) &&
        pedirTexto("Nuevo modelo: ", nuevo.modelo, 50) &&
        pedirEnteroEnRango("Nuevo año (1950-2027): ", nuevo.anio, ANIO_MINIMO, ANIO_MAXIMO) &&
        pedirTexto("Nuevo nombre del propietario: ", nuevo.nombrePropietario, 100) &&
        pedirTextoValido("Nueva cédula del propietario: ", nuevo.cedulaPropietario, 20, esCedulaValida,
                         "La cédula debe tener solo dígitos (prefijo V-, E- o J- opcional).") &&
        pedirTexto("Nuevo teléfono del propietario: ", nuevo.telefonoPropietario, 20);
    if (!completo || !pedirSN("¿Guardar los cambios? (S/N): ")) {
        cout << "Actualización cancelada. No se modificó nada.\n";
        return;
    }
    r = actualizarVehiculo(id, nuevo, info);
    if (r == OK) cout << "Vehículo " << actual.placa << " actualizado.\n";
    else mostrarError(r, info);
}

void menuEliminarVehiculo() {
    int id = 0;
    cout << "\n--- ELIMINAR VEHÍCULO ---\n";
    if (!pedirEnteroEnRango("ID del vehículo: ", id, 1, SIN_LIMITE)) return;

    InfoError info;
    Vehiculo v;
    Resultado r = obtenerVehiculoPorID(id, v, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    mostrarVehiculo(v);
    if (!pedirSN("¿Eliminar este vehículo? (S/N): ")) {
        cout << "Eliminación cancelada.\n";
        return;
    }
    r = eliminarVehiculo(id, info);
    if (r == OK) cout << "Vehículo " << v.placa << " eliminado (borrado lógico).\n";
    else mostrarError(r, info);
}

void menuListarVehiculos() {
    Vehiculo lista[MAX_RESULTADOS];
    int cantidad = 0;
    bool hayMas = false;
    InfoError info;
    Resultado r = listarVehiculos(lista, cantidad, hayMas, info);
    if (r != OK) mostrarError(r, info);
    else if (cantidad == 0) cout << "No hay vehículos registrados.\n";
    else mostrarListaVehiculos(lista, cantidad, hayMas);
}

void menuVehiculos() {
    const char* opciones[6] = {"  1. Registrar vehículo", "  2. Buscar vehículo", "  3. Actualizar vehículo",
                               "  4. Eliminar vehículo", "  5. Listar vehículos", "  0. Volver al menú principal"};
    int op = -1;
    while (op != 0) {
        mostrarMenu("GESTIÓN DE VEHÍCULOS", "", opciones, 6);
        op = pedirOpcion();
        switch (op) {
            case 1: menuRegistrarVehiculo(); break;
            case 2: menuBuscarVehiculo(); break;
            case 3: menuActualizarVehiculo(); break;
            case 4: menuEliminarVehiculo(); break;
            case 5: menuListarVehiculos(); break;
            case 0: break;
            default: cout << "ERROR: Opción inválida.\n";
        }
    }
}

// ---------------------------------------------------------------------
// MENU DE MECANICOS
// ---------------------------------------------------------------------

void menuRegistrarMecanico() {
    Mecanico m;
    memset(&m, 0, sizeof(m));
    cout << "\n--- REGISTRAR MECÁNICO (escriba CANCELAR para cancelar) ---\n";
    if (!pedirTexto("Nombre completo: ", m.nombre, 100) ||
        !pedirTextoValido("Cédula (ej: V-12345678): ", m.cedula, 20, esCedulaValida,
                          "La cédula debe tener solo dígitos (prefijo V-, E- o J- opcional).")) {
        cout << "Registro cancelado. No se creó ningún mecánico.\n";
        return;
    }
    cout << "Especialidades: ";
    for (int i = 0; i < CANTIDAD_ESPECIALIDADES; i++) cout << (i + 1) << "=" << ESPECIALIDADES[i] << "  ";
    cout << "\n";
    int esp = 0;
    if (!pedirEnteroEnRango("Especialidad (1-5): ", esp, 1, CANTIDAD_ESPECIALIDADES) ||
        !pedirTextoValido("Fecha de ingreso (AAAA-MM-DD): ", m.fechaIngreso, 11, esFechaValida,
                          "La fecha debe tener formato AAAA-MM-DD y ser una fecha real.")) {
        cout << "Registro cancelado. No se creó ningún mecánico.\n";
        return;
    }
    copiarTexto(m.especialidad, 20, ESPECIALIDADES[esp - 1]);

    if (!pedirSN("¿Guardar este mecánico? (S/N): ")) {
        cout << "Registro cancelado. No se creó ningún mecánico.\n";
        return;
    }
    InfoError info;
    Resultado r = registrarMecanico(m, info);
    if (r == OK) cout << "Mecánico registrado: " << m.nombre << " (" << m.especialidad << "), ID " << m.id << ".\n";
    else mostrarError(r, info);
}

void menuBuscarMecanico() {
    int id = 0;
    cout << "\n--- BUSCAR MECÁNICO ---\n";
    if (!pedirEnteroEnRango("ID del mecánico: ", id, 1, SIN_LIMITE)) return;
    InfoError info;
    Mecanico m;
    Resultado r = obtenerMecanicoPorID(id, m, info);
    if (r == OK) {
        cout << "\nMecánico encontrado:\n";
        mostrarMecanico(m);
    } else {
        mostrarError(r, info);
    }
}

void menuListarMecanicos() {
    MecanicoVista lista[MAX_RESULTADOS];
    int cantidad = 0;
    bool hayMas = false;
    InfoError info;
    Resultado r = listarMecanicos(lista, cantidad, hayMas, info);
    if (r != OK) mostrarError(r, info);
    else if (cantidad == 0) cout << "No hay mecánicos registrados.\n";
    else mostrarListaMecanicos(lista, cantidad, hayMas);
}

void menuDarDeBajaMecanico() {
    int id = 0;
    cout << "\n--- DAR DE BAJA MECÁNICO ---\n";
    if (!pedirEnteroEnRango("ID del mecánico: ", id, 1, SIN_LIMITE)) return;
    InfoError info;
    Mecanico m;
    Resultado r = obtenerMecanicoPorID(id, m, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    mostrarMecanico(m);
    if (!pedirSN("¿Dar de baja a este mecánico? (S/N): ")) {
        cout << "Baja cancelada.\n";
        return;
    }
    r = darDeBajaMecanico(id, info);
    if (r == OK) cout << "Mecánico " << m.nombre << " dado de baja (sus órdenes anteriores conservan su nombre).\n";
    else mostrarError(r, info);
}

void menuMecanicos() {
    const char* opciones[5] = {"  1. Registrar mecánico", "  2. Buscar mecánico por ID", "  3. Listar mecánicos",
                               "  4. Dar de baja mecánico", "  0. Volver al menú principal"};
    int op = -1;
    while (op != 0) {
        mostrarMenu("GESTIÓN DE MECÁNICOS", "", opciones, 5);
        op = pedirOpcion();
        switch (op) {
            case 1: menuRegistrarMecanico(); break;
            case 2: menuBuscarMecanico(); break;
            case 3: menuListarMecanicos(); break;
            case 4: menuDarDeBajaMecanico(); break;
            case 0: break;
            default: cout << "ERROR: Opción inválida.\n";
        }
    }
}

// ---------------------------------------------------------------------
// MENU DE REPUESTOS
// ---------------------------------------------------------------------

void menuRegistrarRepuesto() {
    Repuesto rep;
    memset(&rep, 0, sizeof(rep));
    cout << "\n--- REGISTRAR REPUESTO (escriba CANCELAR para cancelar) ---\n";
    bool completo =
        pedirTexto("Código: ", rep.codigo, 20) &&
        pedirTexto("Nombre: ", rep.nombre, 100) &&
        pedirTexto("Marca: ", rep.marca, 50) &&
        pedirDecimal("Precio unitario: ", rep.precio, 0, true) &&
        pedirEnteroEnRango("Stock inicial: ", rep.stock, 0, SIN_LIMITE) &&
        pedirEnteroEnRango("Stock mínimo: ", rep.stockMinimo, 0, SIN_LIMITE);
    if (!completo || !pedirSN("¿Guardar este repuesto? (S/N): ")) {
        cout << "Registro cancelado. No se creó ningún repuesto.\n";
        return;
    }
    InfoError info;
    Resultado r = registrarRepuesto(rep, info);
    if (r == OK) cout << "Repuesto registrado: " << rep.codigo << " - " << rep.nombre << " (ID " << rep.id << ").\n";
    else mostrarError(r, info);
}

void menuBuscarRepuesto() {
    cout << "\nBuscar por: 1. ID   2. Código exacto   3. Nombre (parcial)\n";
    int tipo = 0;
    if (!pedirEnteroEnRango("Opción: ", tipo, 1, 3)) return;

    InfoError info;
    Repuesto rep;
    Resultado r;
    if (tipo == 1) {
        int id = 0;
        if (!pedirEnteroEnRango("ID del repuesto: ", id, 1, SIN_LIMITE)) return;
        r = obtenerRepuestoPorID(id, rep, info);
        if (r == OK) mostrarRepuesto(rep);
        else mostrarError(r, info);
    } else if (tipo == 2) {
        char codigo[20];
        if (!pedirTexto("Código: ", codigo, 20)) return;
        r = obtenerRepuestoPorCodigo(codigo, rep, info);
        if (r == OK) mostrarRepuesto(rep);
        else mostrarError(r, info);
    } else {
        char texto[100];
        if (!pedirTexto("Nombre (parcial): ", texto, 100)) return;
        Repuesto encontrados[MAX_RESULTADOS];
        int cantidad = 0;
        bool hayMas = false;
        r = buscarRepuestosPorNombre(texto, encontrados, cantidad, hayMas, info);
        if (r != OK) mostrarError(r, info);
        else if (cantidad == 0) cout << "No se encontraron repuestos cuyo nombre contenga '" << texto << "'.\n";
        else mostrarListaRepuestos(encontrados, cantidad, hayMas);
    }
}

void menuEntradaMercancia() {
    int id = 0, cantidad = 0;
    cout << "\n--- REGISTRAR ENTRADA DE MERCANCÍA ---\n";
    if (!pedirEnteroEnRango("ID del repuesto: ", id, 1, SIN_LIMITE)) return;
    InfoError info;
    Repuesto rep;
    Resultado r = obtenerRepuestoPorID(id, rep, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    cout << "Repuesto: " << rep.codigo << " - " << rep.nombre << " (stock actual: " << rep.stock << ")\n";
    if (!pedirEnteroEnRango("Cantidad que ingresa: ", cantidad, 1, SIN_LIMITE)) return;
    r = registrarEntradaMercancia(id, cantidad, rep, info);
    if (r == OK) cout << "Entrada registrada: " << rep.nombre << " ahora tiene " << rep.stock << " unidades.\n";
    else mostrarError(r, info);
}

void menuListarRepuestos() {
    Repuesto lista[MAX_RESULTADOS];
    int cantidad = 0;
    bool hayMas = false;
    InfoError info;
    Resultado r = listarRepuestos(lista, cantidad, hayMas, info);
    if (r != OK) mostrarError(r, info);
    else if (cantidad == 0) cout << "No hay repuestos registrados.\n";
    else mostrarListaRepuestos(lista, cantidad, hayMas);
}

void menuCambiarPrecio() {
    int id = 0;
    float precio = 0;
    cout << "\n--- CAMBIAR PRECIO ---\n";
    if (!pedirEnteroEnRango("ID del repuesto: ", id, 1, SIN_LIMITE)) return;
    InfoError info;
    Repuesto rep;
    Resultado r = obtenerRepuestoPorID(id, rep, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    cout << "Repuesto: " << rep.codigo << " - " << rep.nombre << " (precio actual: " << rep.precio << ")\n";
    if (!pedirDecimal("Nuevo precio: ", precio, 0, true)) return;
    r = cambiarPrecioRepuesto(id, precio, rep, info);
    if (r == OK) cout << "Precio de '" << rep.nombre << "' actualizado a " << rep.precio
                      << ". Las órdenes ya registradas conservan su precio histórico.\n";
    else mostrarError(r, info);
}

void menuEliminarRepuesto() {
    int id = 0;
    cout << "\n--- ELIMINAR REPUESTO ---\n";
    if (!pedirEnteroEnRango("ID del repuesto: ", id, 1, SIN_LIMITE)) return;
    InfoError info;
    Repuesto rep;
    Resultado r = obtenerRepuestoPorID(id, rep, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    mostrarRepuesto(rep);
    if (!pedirSN("¿Eliminar este repuesto? (S/N): ")) {
        cout << "Eliminación cancelada.\n";
        return;
    }
    r = eliminarRepuesto(id, info);
    if (r == OK) cout << "Repuesto " << rep.nombre << " eliminado (borrado lógico).\n";
    else mostrarError(r, info);
}

void menuRepuestos() {
    const char* opciones[7] = {"  1. Registrar repuesto", "  2. Buscar repuesto", "  3. Registrar entrada de mercancía",
                               "  4. Listar repuestos", "  5. Cambiar precio", "  6. Eliminar repuesto",
                               "  0. Volver al menú principal"};
    int op = -1;
    while (op != 0) {
        mostrarMenu("INVENTARIO DE REPUESTOS", "", opciones, 7);
        op = pedirOpcion();
        switch (op) {
            case 1: menuRegistrarRepuesto(); break;
            case 2: menuBuscarRepuesto(); break;
            case 3: menuEntradaMercancia(); break;
            case 4: menuListarRepuestos(); break;
            case 5: menuCambiarPrecio(); break;
            case 6: menuEliminarRepuesto(); break;
            case 0: break;
            default: cout << "ERROR: Opción inválida.\n";
        }
    }
}

// ---------------------------------------------------------------------
// MENU DE ORDENES DE SERVICIO
// ---------------------------------------------------------------------

void menuAbrirOrden() {
    int idVeh = 0, idMec = 0, km = 0;
    char falla[200];
    InfoError info;
    cout << "\n--- ABRIR ORDEN (escriba CANCELAR para cancelar) ---\n";
    if (!pedirEnteroEnRango("ID del vehículo: ", idVeh, 1, SIN_LIMITE)) {
        cout << "Apertura cancelada.\n";
        return;
    }
    Vehiculo v;
    Resultado r = obtenerVehiculoPorID(idVeh, v, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    cout << "Vehículo: " << v.placa << " - " << v.marca << " " << v.modelo << " (" << v.kilometraje
         << " km), propietario " << v.nombrePropietario << "\n";

    if (!pedirEnteroEnRango("ID del mecánico: ", idMec, 1, SIN_LIMITE)) {
        cout << "Apertura cancelada.\n";
        return;
    }
    Mecanico m;
    r = obtenerMecanicoPorID(idMec, m, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    cout << "Mecánico: " << m.nombre << " (" << m.especialidad << ")" << (m.eliminado ? " - DADO DE BAJA" : "") << "\n";

    if (!pedirEnteroEnRango("Kilometraje de ingreso: ", km, 0, SIN_LIMITE) ||
        !pedirTexto("Descripción de la falla: ", falla, 200)) {
        cout << "Apertura cancelada.\n";
        return;
    }
    if (!pedirSN("¿Abrir la orden? (S/N): ")) {
        cout << "Apertura cancelada. No se creó ninguna orden.\n";
        return;
    }
    OrdenServicio nueva;
    r = abrirOrden(idVeh, idMec, falla, km, nueva, info);
    if (r == OK) cout << "Orden #" << nueva.id << " abierta para el vehículo " << v.placa
                      << " con el mecánico " << m.nombre << ".\n";
    else mostrarError(r, info);
}

void menuAgregarRepuestoAOrden() {
    int idOrden = 0, idRep = 0, cantidad = 0;
    InfoError info;
    cout << "\n--- AGREGAR REPUESTO A UNA ORDEN ---\n";
    if (!pedirEnteroEnRango("ID de la orden: ", idOrden, 1, SIN_LIMITE)) return;
    if (!pedirEnteroEnRango("ID del repuesto: ", idRep, 1, SIN_LIMITE)) return;
    Repuesto rep;
    Resultado r = obtenerRepuestoPorID(idRep, rep, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    cout << "Repuesto: " << rep.codigo << " - " << rep.nombre << " (stock " << rep.stock << ", precio " << rep.precio << ")\n";
    if (!pedirEnteroEnRango("Cantidad: ", cantidad, 1, SIN_LIMITE)) return;
    r = agregarRepuestoAOrden(idOrden, idRep, cantidad, info);
    if (r == OK) cout << "Se agregaron " << cantidad << " unidad(es) de '" << rep.nombre << "' a la orden #" << idOrden << ".\n";
    else mostrarError(r, info);
}

void menuQuitarRepuestoDeOrden() {
    int idOrden = 0, idRep = 0;
    InfoError info;
    cout << "\n--- QUITAR REPUESTO DE UNA ORDEN ---\n";
    if (!pedirYMostrarOrden("ID de la orden: ", idOrden)) return;
    if (!pedirEnteroEnRango("ID del repuesto a quitar: ", idRep, 1, SIN_LIMITE)) return;
    Repuesto rep;
    Resultado r = obtenerRepuestoPorID(idRep, rep, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    r = quitarRepuestoDeOrden(idOrden, idRep, info);
    if (r == OK) cout << "Se quitó '" << rep.nombre << "' de la orden #" << idOrden
                      << "; sus unidades volvieron al inventario.\n";
    else mostrarError(r, info);
}

void menuCerrarOrden() {
    int idOrden = 0;
    float manoObra = 0;
    InfoError info;
    cout << "\n--- CERRAR ORDEN (escriba CANCELAR para cancelar) ---\n";
    if (!pedirYMostrarOrden("ID de la orden: ", idOrden)) return;
    if (!pedirDecimal("Monto de mano de obra: ", manoObra, 0, false)) {
        cout << "Cierre cancelado.\n";
        return;
    }
    if (!pedirSN("¿Cerrar la orden? (S/N): ")) {
        cout << "Cierre cancelado. La orden sigue como estaba.\n";
        return;
    }
    Resultado r = cerrarOrden(idOrden, manoObra, info);
    if (r == OK) cout << "Orden #" << idOrden << " cerrada.\n";
    else mostrarError(r, info);
}

void menuAnularOrden() {
    int idOrden = 0;
    InfoError info;
    cout << "\n--- ANULAR ORDEN ---\n";
    if (!pedirYMostrarOrden("ID de la orden: ", idOrden)) return;
    if (!pedirSN("¿Anular la orden y devolver sus repuestos al inventario? (S/N): ")) {
        cout << "Anulación cancelada.\n";
        return;
    }
    Resultado r = anularOrden(idOrden, info);
    if (r == OK) cout << "Orden #" << idOrden << " anulada. Los repuestos volvieron al inventario.\n";
    else mostrarError(r, info);
}

void menuVerDetalleOrden() {
    int idOrden = 0;
    cout << "\n--- DETALLE DE ORDEN ---\n";
    pedirYMostrarOrden("ID de la orden: ", idOrden);
}

void menuListarOrdenes() {
    cout << "\nListar: 1. Todas   2. Por estado   3. Historial de un vehículo\n";
    int tipo = 0;
    if (!pedirEnteroEnRango("Opción: ", tipo, 1, 3)) return;

    char estado[12] = "";
    int idVehiculo = 0;
    if (tipo == 2) {
        int e = 0;
        cout << "Estados: 1=ABIERTA  2=CERRADA  3=ANULADA\n";
        if (!pedirEnteroEnRango("Estado (1-3): ", e, 1, 3)) return;
        const char* nombres[3] = {ESTADO_ABIERTA, ESTADO_CERRADA, ESTADO_ANULADA};
        copiarTexto(estado, 12, nombres[e - 1]);
    } else if (tipo == 3) {
        if (!pedirEnteroEnRango("ID del vehículo: ", idVehiculo, 1, SIN_LIMITE)) return;
        InfoError info;
        Vehiculo v;
        Resultado r = obtenerVehiculoPorID(idVehiculo, v, info);
        if (r != OK && r != ERR_VEHICULO_ELIMINADO) {
            mostrarError(r, info);
            return;
        }
        cout << "Historial del vehículo " << v.placa << " - " << v.marca << " " << v.modelo << "\n";
    }

    OrdenVista lista[MAX_RESULTADOS];
    int cantidad = 0;
    bool hayMas = false;
    InfoError info;
    Resultado r = listarOrdenes(estado, idVehiculo, lista, cantidad, hayMas, info);
    if (r != OK) mostrarError(r, info);
    else if (cantidad == 0) cout << "No hay órdenes que coincidan.\n";
    else mostrarListaOrdenes(lista, cantidad, hayMas);
}

void menuOrdenes() {
    const char* opciones[8] = {"  1. Abrir orden", "  2. Agregar repuesto a orden", "  3. Quitar repuesto de orden",
                               "  4. Cerrar orden", "  5. Anular orden", "  6. Ver detalle de orden",
                               "  7. Listar órdenes", "  0. Volver al menú principal"};
    int op = -1;
    while (op != 0) {
        mostrarMenu("ÓRDENES DE SERVICIO", "", opciones, 8);
        op = pedirOpcion();
        switch (op) {
            case 1: menuAbrirOrden(); break;
            case 2: menuAgregarRepuestoAOrden(); break;
            case 3: menuQuitarRepuestoDeOrden(); break;
            case 4: menuCerrarOrden(); break;
            case 5: menuAnularOrden(); break;
            case 6: menuVerDetalleOrden(); break;
            case 7: menuListarOrdenes(); break;
            case 0: break;
            default: cout << "ERROR: Opción inválida.\n";
        }
    }
}

// ---------------------------------------------------------------------
// MENU DE REPORTES
// ---------------------------------------------------------------------

void menuReporteHistorialVehiculo() {
    char placa[10];
    cout << "\n--- REPORTE 1: HISTORIAL DE VEHÍCULO ---\n";
    if (!pedirTexto("Placa: ", placa, 10)) return;

    Vehiculo v;
    OrdenVista ordenes[MAX_RESULTADOS];
    int cantidad = 0;
    bool hayMas = false;
    float totalFacturado = 0;
    InfoError info;
    Resultado r = reporteHistorialVehiculo(placa, v, ordenes, cantidad, hayMas, totalFacturado, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    cout << "\nHISTORIAL DEL VEHÍCULO\n";
    mostrarVehiculo(v);
    if (cantidad == 0) {
        cout << "El vehículo no tiene órdenes.\n";
    } else {
        int anchos[5] = {6, 10, 24, 8, 10};
        const char* titulos[5] = {"Orden", "Fecha", "Mecánico", "Estado", "Total"};
        bool der[5] = {true, false, false, false, true};
        cout << "\n";
        tablaEncabezado(anchos, titulos, 5);
        for (int i = 0; i < cantidad; i++) {
            char c0[20], c2[160], c4[30];
            snprintf(c0, sizeof(c0), "#%d", ordenes[i].orden.id);
            snprintf(c2, sizeof(c2), "%s%s", ordenes[i].nombreMecanico, ordenes[i].mecanicoDeBaja ? " (BAJA)" : "");
            snprintf(c4, sizeof(c4), "%.2f", ordenes[i].total);
            const char* fila[5] = {c0, ordenes[i].orden.fechaApertura, c2, ordenes[i].orden.estado, c4};
            tablaFila(anchos, fila, der, 5);
        }
        tablaBorde(anchos, 5, "╚", "╩", "╝");
        if (hayMas) cout << "AVISO: Hay más órdenes. Solo se muestran las primeras " << MAX_RESULTADOS << ".\n";
    }
    cout << "Total facturado en órdenes CERRADAS: " << totalFacturado << "\n";
}

void menuReporteRepuestosMasUsados() {
    cout << "\n--- REPORTE 2: REPUESTOS MÁS UTILIZADOS (órdenes CERRADAS) ---\n";
    RepuestoUso top[TOP_REPUESTOS];
    int cantidad = 0;
    InfoError info;
    Resultado r = reporteRepuestosMasUsados(top, cantidad, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    if (cantidad == 0) {
        cout << "Todavía no hay repuestos consumidos en órdenes cerradas.\n";
        return;
    }
    int anchos[4] = {3, 10, 30, 9};
    const char* titulos[4] = {"#", "Código", "Repuesto", "Unidades"};
    bool der[4] = {true, false, false, true};
    cout << "\n";
    tablaEncabezado(anchos, titulos, 4);
    for (int i = 0; i < cantidad; i++) {
        char c0[20], c3[20];
        snprintf(c0, sizeof(c0), "%d", i + 1);
        snprintf(c3, sizeof(c3), "%d", top[i].unidades);
        const char* fila[4] = {c0, top[i].codigo, top[i].nombre, c3};
        tablaFila(anchos, fila, der, 4);
    }
    tablaBorde(anchos, 4, "╚", "╩", "╝");
}

void menuReporteFacturacionMecanicos() {
    cout << "\n--- REPORTE 3: FACTURACIÓN POR MECÁNICO (órdenes CERRADAS) ---\n";
    FacturacionMecanico filas[MAX_RESULTADOS];
    int cantidad = 0;
    bool hayMas = false;
    InfoError info;
    Resultado r = reporteFacturacionMecanicos(filas, cantidad, hayMas, info);
    if (r != OK) {
        mostrarError(r, info);
        return;
    }
    int anchos[4] = {28, 7, 10, 10};
    const char* titulos[4] = {"Mecánico", "Órdenes", "M. obra", "Repuestos"};
    bool der[4] = {false, true, true, true};
    cout << "\n";
    tablaEncabezado(anchos, titulos, 4);
    for (int i = 0; i < cantidad; i++) {
        char c0[160], c1[20], c2[30], c3[30];
        snprintf(c0, sizeof(c0), "%s%s", filas[i].nombre, filas[i].dadoDeBaja ? " (BAJA)" : "");
        snprintf(c1, sizeof(c1), "%d", filas[i].ordenesCerradas);
        snprintf(c2, sizeof(c2), "%.2f", filas[i].manoObra);
        snprintf(c3, sizeof(c3), "%.2f", filas[i].repuestos);
        const char* fila[4] = {c0, c1, c2, c3};
        tablaFila(anchos, fila, der, 4);
    }
    tablaBorde(anchos, 4, "╚", "╩", "╝");
    if (hayMas) cout << "AVISO: Hay más mecánicos. Solo se muestran los primeros " << MAX_RESULTADOS << ".\n";
}

void menuReportes() {
    const char* opciones[4] = {"  1. Historial de vehículo", "  2. Repuestos más utilizados",
                               "  3. Facturación por mecánico", "  0. Volver al menú principal"};
    int op = -1;
    while (op != 0) {
        mostrarMenu("REPORTES", "", opciones, 4);
        op = pedirOpcion();
        switch (op) {
            case 1: menuReporteHistorialVehiculo(); break;
            case 2: menuReporteRepuestosMasUsados(); break;
            case 3: menuReporteFacturacionMecanicos(); break;
            case 0: break;
            default: cout << "ERROR: Opción inválida.\n";
        }
    }
}

// ---------------------------------------------------------------------
// MENU DE MANTENIMIENTO
// ---------------------------------------------------------------------

void menuVerificarIntegridad() {
    cout << "\n--- VERIFICACIÓN DE INTEGRIDAD ---\n";
    ReporteIntegridad rep;
    Resultado r = verificarIntegridad(rep);
    if (r != OK) {
        InfoError info;
        limpiarInfo(info);
        mostrarError(r, info);
        return;
    }
    for (int i = 0; i < rep.guardados; i++) cout << rep.mensajes[i] << "\n";
    if (rep.problemas > rep.guardados) {
        cout << "... y " << (rep.problemas - rep.guardados) << " problemas más (solo se muestran los primeros "
             << MAX_PROBLEMAS << ").\n";
    }

    char buf[100];
    cout << "\n";
    cajaBorde(ANCHO_MENU, "╔", "╗");
    cajaCentrada(ANCHO_MENU, "REPORTE DE INTEGRIDAD DEL SISTEMA");
    cajaBorde(ANCHO_MENU, "╠", "╣");
    snprintf(buf, sizeof(buf), "Vehículos verificados : %d", rep.vehiculos);
    cajaLinea(ANCHO_MENU, buf);
    snprintf(buf, sizeof(buf), "Mecánicos verificados : %d", rep.mecanicos);
    cajaLinea(ANCHO_MENU, buf);
    snprintf(buf, sizeof(buf), "Repuestos verificados : %d", rep.repuestos);
    cajaLinea(ANCHO_MENU, buf);
    snprintf(buf, sizeof(buf), "Órdenes verificadas   : %d", rep.ordenes);
    cajaLinea(ANCHO_MENU, buf);
    cajaLinea(ANCHO_MENU, "");
    snprintf(buf, sizeof(buf), "Problemas encontrados : %d", rep.problemas);
    cajaLinea(ANCHO_MENU, buf);
    cajaLinea(ANCHO_MENU, rep.problemas == 0 ? "Estado: SISTEMA ÍNTEGRO" : "Estado: SE ENCONTRARON PROBLEMAS");
    cajaBorde(ANCHO_MENU, "╚", "╝");
}

void menuCrearBackup() {
    cout << "\n--- RESPALDO (BACKUP) ---\n";
    char ruta[100];
    Resultado r = crearBackup(ruta, sizeof(ruta));
    if (r == OK) {
        cout << "Respaldo creado correctamente en la carpeta: " << ruta << "\n";
    } else {
        InfoError info;
        limpiarInfo(info);
        mostrarError(r, info);
    }
}

void menuMantenimiento() {
    const char* opciones[3] = {"  1. Verificar integridad", "  2. Crear respaldo (backup)", "  0. Volver al menú principal"};
    int op = -1;
    while (op != 0) {
        mostrarMenu("MANTENIMIENTO", "", opciones, 3);
        op = pedirOpcion();
        switch (op) {
            case 1: menuVerificarIntegridad(); break;
            case 2: menuCrearBackup(); break;
            case 0: break;
            default: cout << "ERROR: Opción inválida.\n";
        }
    }
}

// ---------------------------------------------------------------------
// PROGRAMA PRINCIPAL
// ---------------------------------------------------------------------


bool registrarTallerPrimeraVez() {
    Taller t;
    memset(&t, 0, sizeof(t));
    cout << "=== PRIMER ARRANQUE: registre los datos del taller ===\n";
    bool completo =
        pedirTexto("Nombre del taller: ", t.nombre, 100) &&
        pedirTexto("RIF: ", t.rif, 20) &&
        pedirTexto("Dirección: ", t.direccion, 200) &&
        pedirTexto("Teléfono: ", t.telefono, 20);
    if (!completo) {
        cout << "No se registró el taller. El programa no puede continuar.\n";
        return false;
    }
    InfoError info;
    Resultado r = registrarTaller(t, info);
    if (r != OK) {
        mostrarError(r, info);
        return false;
    }
    cout << "Taller registrado: " << t.nombre << "\n";
    return true;
}

int main(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--eco") == 0) modoEco = true;
    }


    Resultado r = iniciarSistema();
    if (r != OK) {
        InfoError info;
        limpiarInfo(info);
        mostrarError(r, info);
        return 1;
    }


    if (!tallerRegistrado()) {
        if (!registrarTallerPrimeraVez()) return 1;
    }

    
    const char* opciones[7] = {"  1. Gestión de Vehículos", "  2. Gestión de Mecánicos", "  3. Inventario de Repuestos",
                               "  4. Órdenes de Servicio", "  5. Reportes", "  6. Mantenimiento", "  0. Salir"};
    while (true) {
        Taller taller;
        memset(&taller, 0, sizeof(taller));
        obtenerTaller(taller);
        char subtitulo[150];
        snprintf(subtitulo, sizeof(subtitulo), "Taller: %s", taller.nombre);

        mostrarMenu("SISTEMA DE GESTIÓN DE TALLER", subtitulo, opciones, 7);
        int op = pedirOpcion();
        switch (op) {
            case 1: menuVehiculos(); break;
            case 2: menuMecanicos(); break;
            case 3: menuRepuestos(); break;
            case 4: menuOrdenes(); break;
            case 5: menuReportes(); break;
            case 6: menuMantenimiento(); break;
            case 0:
  
                if (entradaTerminada || pedirSN("¿Seguro que desea salir? (S/N): ")) {
                    cout << "Hasta pronto. Los datos quedaron guardados en la carpeta datos/.\n";
                    return 0;
                }
                break;
            default:
                cout << "ERROR: Opción inválida.\n";
        }
    }
}
