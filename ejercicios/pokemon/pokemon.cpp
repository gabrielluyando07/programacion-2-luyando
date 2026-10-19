#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <iomanip>

using namespace std;

// Estructura para almacenar la información de un Pokémon
struct Pokemon {
    int numPokedex;
    string nombre;
    string tipo1;
    string tipo2;
    int total;
    int hp;
    int ataque;
    int defensa;
    int ataqueEsp;
    int defensaEsp;
    int velocidad;
    int generacion;
    bool legendario;
};

// Función auxiliar para convertir cadenas a minúsculas (búsquedas insensibles a mayúsculas)
string aMinusculas(string str) {
    transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return tolower(c);
    });
    return str;
}

// Muestra la ficha detallada de un Pokémon
void mostrarPokemon(const Pokemon& p) {
    cout << "--------------------------------------------------\n";
    cout << "Nº Pokédex  : " << p.numPokedex << "\n";
    cout << "Nombre      : " << p.nombre << "\n";
    cout << "Tipo(s)     : " << p.tipo1;
    if (!p.tipo2.empty()) {
        cout << " / " << p.tipo2;
    }
    cout << "\n";
    cout << "Total Stats : " << p.total << "\n";
    cout << "HP          : " << p.hp << "\n";
    cout << "Ataque      : " << p.ataque << "\n";
    cout << "Defensa     : " << p.defensa << "\n";
    cout << "Ataque Esp. : " << p.ataqueEsp << "\n";
    cout << "Defensa Esp.: " << p.defensaEsp << "\n";
    cout << "Velocidad   : " << p.velocidad << "\n";
    cout << "Generación  : " << p.generacion << "\n";
    cout << "Legendario  : " << (p.legendario ? "Sí" : "No") << "\n";
    cout << "--------------------------------------------------\n";
}

// Lee y parsea el archivo CSV
vector<Pokemon> cargarCSV(const string& rutaArchivo) {
    vector<Pokemon> lista;
    ifstream archivo(rutaArchivo);

    if (!archivo.is_open()) {
        cerr << "ERROR: No se pudo abrir el archivo '" << rutaArchivo << "'.\n";
        cerr << "Asegúrate de que el archivo CSV esté en la misma carpeta.\n";
        return lista;
    }

    string linea;
    // Leer la cabecera e ignorarla
    getline(archivo, linea);

    while (getline(archivo, linea)) {
        if (linea.empty()) continue;

        stringstream ss(linea);
        string item;
        Pokemon p;

        // Nº Pokedex
        getline(ss, item, ','); p.numPokedex = stoi(item);
        // Nombre
        getline(ss, p.nombre, ',');
        // Tipo 1
        getline(ss, p.tipo1, ',');
        // Tipo 2
        getline(ss, p.tipo2, ',');
        // Total
        getline(ss, item, ','); p.total = stoi(item);
        // HP
        getline(ss, item, ','); p.hp = stoi(item);
        // Ataque
        getline(ss, item, ','); p.ataque = stoi(item);
        // Defensa
        getline(ss, item, ','); p.defensa = stoi(item);
        // Ataque Esp.
        getline(ss, item, ','); p.ataqueEsp = stoi(item);
        // Defensa Esp.
        getline(ss, item, ','); p.defensaEsp = stoi(item);
        // Velocidad
        getline(ss, item, ','); p.velocidad = stoi(item);
        // Generación
        getline(ss, item, ','); p.generacion = stoi(item);
        // Legendario
        getline(ss, item, ','); 
        p.legendario = (aMinusculas(item) == "true" || item == "1");

        lista.push_back(p);
    }

    archivo.close();
    return lista;
}

int main() {
    string ruta = "pokemon_150_registros.csv";
    vector<Pokemon> pokemons = cargarCSV(ruta);

    if (pokemons.empty()) {
        return 1;
    }

    int opcion = -1;

    while (opcion != 0) {
        cout << "\n=========================================\n";
        cout << "      SISTEMA DE CONSULTA POKÉMON (C++)  \n";
        cout << "=========================================\n";
        cout << "1. Ver info de Pokémon por número de Pokédex\n";
        cout << "2. Buscar Pokémon por nombre (coincidencia parcial)\n";
        cout << "3. Ver Pokémon por tipo\n";
        cout << "4. Ver info de Pokémon por nombre (búsqueda exacta)\n";
        cout << "5. Ver Top 3 Pokémon más fuertes y Top 3 más débiles\n";
        cout << "0. Salir\n";
        cout << "-----------------------------------------\n";
        cout << "Seleccione una opción: ";

        if (!(cin >> opcion)) {
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "\nEntrada inválida. Intente de nuevo.\n";
            continue;
        }

        cin.ignore(10000, '\n'); // Limpiar el búfer de entrada

        switch (opcion) {
            case 1: {
                int num;
                cout << "Ingrese el número de Pokédex: ";
                cin >> num;
                bool encontrado = false;
                for (const auto& p : pokemons) {
                    if (p.numPokedex == num) {
                        mostrarPokemon(p);
                        encontrado = true;
                        break;
                    }
                }
                if (!encontrado) {
                    cout << "\nNo se encontró ningún Pokémon con el Nº " << num << ".\n";
                }
                break;
            }

            case 2: {
                string subcadena;
                cout << "Ingrese el nombre o parte del nombre a buscar: ";
                getline(cin, subcadena);
                string busqueda = aMinusculas(subcadena);

                int coincidencias = 0;
                cout << "\nCoincidencias encontradas:\n";
                for (const auto& p : pokemons) {
                    if (aMinusculas(p.nombre).find(busqueda) != string::npos) {
                        cout << " - Nº " << setw(3) << p.numPokedex << ": " 
                             << setw(15) << left << p.nombre 
                             << " | Tipo 1: " << setw(10) << p.tipo1 
                             << " | Total Stats: " << p.total << "\n";
                        coincidencias++;
                    }
                }
                if (coincidencias == 0) {
                    cout << "No se encontraron Pokémon con ese criterio.\n";
                } else {
                    cout << "Total de coincidencias: " << coincidencias << "\n";
                }
                break;
            }

            case 3: {
                string tipoBuscado;
                cout << "Ingrese el tipo a consultar (ej. Fire, Water, Grass, Poison): ";
                getline(cin, tipoBuscado);
                string tipoMin = aMinusculas(tipoBuscado);

                int contador = 0;
                cout << "\nPokémon del tipo '" << tipoBuscado << "':\n";
                for (const auto& p : pokemons) {
                    if (aMinusculas(p.tipo1) == tipoMin || aMinusculas(p.tipo2) == tipoMin) {
                        cout << " - Nº " << setw(3) << p.numPokedex << ": " 
                             << setw(15) << left << p.nombre 
                             << " | Total: " << p.total << "\n";
                        contador++;
                    }
                }
                if (contador == 0) {
                    cout << "No se encontraron Pokémon pertenecientes a ese tipo.\n";
                } else {
                    cout << "Total encontrados: " << contador << "\n";
                }
                break;
            }

            case 4: {
                string nombreExacto;
                cout << "Ingrese el nombre exacto del Pokémon: ";
                getline(cin, nombreExacto);
                string nombreMin = aMinusculas(nombreExacto);

                bool encontrado = false;
                for (const auto& p : pokemons) {
                    if (aMinusculas(p.nombre) == nombreMin) {
                        mostrarPokemon(p);
                        encontrado = true;
                        break;
                    }
                }
                if (!encontrado) {
                    cout << "\nNo se encontró ningún Pokémon con el nombre exacto '" << nombreExacto << "'.\n";
                }
                break;
            }

            case 5: {
                // Crear una copia para ordenar por la estadística 'total'
                vector<Pokemon> copia = pokemons;

                sort(copia.begin(), copia.end(), [](const Pokemon& a, const Pokemon& b) {
                    return a.total > b.total; // Orden descendente
                });

                cout << "\n=========================================\n";
                cout << "       TOP 3 POKÉMON MÁS FUERTES         \n";
                cout << "=========================================\n";
                for (size_t i = 0; i < 3 && i < copia.size(); ++i) {
                    cout << " " << (i + 1) << ". " << setw(15) << left << copia[i].nombre 
                         << " (Nº " << setw(3) << copia[i].numPokedex << ") | Total Stats: " << copia[i].total << "\n";
                }

                cout << "\n=========================================\n";
                cout << "       TOP 3 POKÉMON MÁS DÉBILES         \n";
                cout << "=========================================\n";
                int n = copia.size();
                for (int i = 0; i < 3 && i < n; ++i) {
                    const auto& p = copia[n - 1 - i];
                    cout << " " << (i + 1) << ". " << setw(15) << left << p.nombre 
                         << " (Nº " << setw(3) << p.numPokedex << ") | Total Stats: " << p.total << "\n";
                }
                break;
            }

            case 0:
                cout << "\n¡Saliendo del programa!\n";
                break;

            default:
                cout << "\nOpción no válida. Intente nuevamente.\n";
                break;
        }
    }

    return 0;
}