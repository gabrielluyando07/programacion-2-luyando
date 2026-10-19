#include "logica.h"
#include "presentacion.h"

int main() {
    SistemaTaller sistema;
    inicializarSistema(&sistema, "Autoservicio El Lago", "J-12345678-9", "Av. Bella Vista, Maracaibo", "0261-7000000");

    menuPrincipal(&sistema);

    liberarSistema(&sistema);
    return 0;
}