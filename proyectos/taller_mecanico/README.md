# Sistema de Gestión de Taller Mecánico

## 📋 Descripción del Sistema e Integrantes

Sistema modular desarrollado en C++ diseñado para la administración integral de un taller mecánico. Permite gestionar de manera eficiente las operaciones de negocio mediante estructuras de datos dinámicas en memoria, abarcando:

Vehículos: Registro por placa, marca, modelo, año, kilometraje y datos del propietario.

Mecánicos: Control de personal, especialidades (MOTOR, FRENOS, ELECTRICIDAD, SUSPENSION, GENERAL) y limitación operativa de hasta 3 órdenes abiertas simultáneas.

Repuestos e Inventario: Control de stock, precios unitarios, alertas automáticas de bajo stock y registro de entradas de mercancía.

Órdenes de Servicio: Apertura de órdenes vinculadas a vehículos y mecánicos, adición/eliminación dinámica de repuestos (afectando el inventario en tiempo real), cálculo de mano de obra y estados (ABIERTA, CERRADA, ANULADA).

## Integrantes de la pareja:

### Gabriel Luyando

=======

### José Brach


# ⚙️ Instrucciones de Compilación y Ejecución

El proyecto está estructurado de manera modular separando los encabezados (include/) de las implementaciones (src/).

## 1. Compilación mediante g++ (Requerido en terminal)

=======
Abre la terminal en la carpeta raíz del proyecto (taller_mecanico) y ejecuta el siguiente comando:

g++ -Wall -Wextra -std=c++11 -Iinclude src/logica.cpp src/presentacion.cpp src/main.cpp -o taller.exe


## 2. Ejecución del programa

En la terminal de VS Code:

./taller


# 🗂️ Diagrama de Estructuras y Relaciones

El sistema se basa en un contenedor principal SistemaTaller que administra arreglos dinámicos redimensionables para cada entidad:

       ┌────────────────────────┐
       │     SistemaTaller      │
       └───┬───┬──────┬─────┬───┘
           │   │      │     │
           │   │      │     └──────► [OrdenServicio] (Dinámico)
           │   │      │                    │ (1)
           │   │      │                    ▼ (N)
           │   │      │             [DetalleOrden] (Dinámico: idRepuesto, cantidad, precioUnitario)
           │   │      └────────────► [Repuesto] (Dinámico)
           │   └───────────────────► [Mecanico] (Dinámico)
           └───────────────────────► [Vehiculo] (Dinámico)


Relación Orden ➔ Detalles: Cada OrdenServicio posee un puntero dinámico a DetalleOrden* detalles y un contador numDetalles, permitiendo asociar múltiples repuestos con sus respectivas cantidades y precios congelados al momento de la transacción.


# 🧩 Separación de Capas
=======

El proyecto divide responsabilidades en tres capas bien definidas para garantizar mantenibilidad:

### Capa de Estructuras (include/estructuras.h): Define los tipos de datos abstractos (struct), los códigos de error mediante enumeraciones (ResultadoOperacion) y las capacidades de los arreglos. No contiene lógica de negocio ni I/O.

### Capa de Lógica de Negocio (include/logica.h y src/logica.cpp): Contiene todas las reglas del sistema (validación de stock, límites de órdenes por mecánico, control de unicidad de placas y cédulas, y redimensionamiento dinámico de memoria con new y delete[]). Es completamente independiente de la pantalla.

### Capa de Presentación (include/presentacion.h y src/presentacion.cpp): Maneja exclusivamente la interfaz de usuario por consola (cin, cout, menús interactivos, formato de tablas con iomanip y lectura segura de datos). Se comunica con la lógica llamando a sus funciones expuestas.

# ✒️ Firmas Diseñadas y Justificación

##  Módulo de Mecánicos

### Mecanico* agregarMecanico(SistemaTaller* s, Mecanico datos);

Retorna un puntero al objeto recién creado dentro del arreglo dinámico de la estructura principal para confirmar su inserción exitosa en memoria, o nullptr si la cédula está duplicada o la especialidad no es válida.

### Mecanico** listarMecanicos(SistemaTaller* s, int* cantidad);

Retorna un arreglo dinámico de punteros (Mecanico**) para facilitar la visualización externa sin duplicar los registros originales en memoria, devolviendo por referencia la cantidad total mediante el parámetro int* cantidad.

## Módulo de Repuestos

### bool registrarEntradaMercancia(SistemaTaller* s, int id, int cantidad);

Retorna un booleano (true o false) indicando de forma binaria si la operación de sumar stock al repuesto especificado se completó con éxito o si el ID no fue encontrado.

### ResultadoOperacion eliminarRepuesto(SistemaTaller* s, int id);

Retorna un código enumerado (ResultadoOperacion) para detallar con precisión la causa exacta en caso de fallo (ej. ERROR_NO_ENCONTRADO o ERROR_TIENE_ORDENES si el repuesto está asociado a una orden histórica).

## Módulo de Órdenes de Servicio

### OrdenServicio* abrirOrden(SistemaTaller* s, int idVehiculo, int idMecanico, const char* fechaApertura, const char* descripcionFalla, int kilometrajeIngreso, ResultadoOperacion* resultado);

Retorna un puntero a la orden creada para permitir operaciones encadenadas inmediatas (como agregar repuestos), y utiliza un parámetro de salida por referencia (ResultadoOperacion* resultado) para reportar validaciones de negocio fallidas (ej. mecánico con más de 3 órdenes abiertas o vehículo duplicado).

# 🧠 Preguntas de Análisis

### 1. agregarVehiculo retorna un puntero a un elemento de s->vehiculos. Si guardan ese puntero en una variable, registran varios vehículos más y luego lo usan, ¿qué puede ocurrir y por qué?

Respuesta:
Puede producirse un fallo de segmento (segmentation fault) o corrupción de memoria (dangling pointer). Esto ocurre porque s->vehiculos se implementa como un arreglo dinámico redimensionable. Cuando se supera la capacidad máxima (numVehiculos == capacidadVehiculos), la función asigna un bloque de memoria nuevo y más grande (new Vehiculo[nuevaCap]), copia los elementos anteriores, y libera el bloque de memoria antiguo (delete[] s->vehiculos). Como consecuencia, el puntero guardado previamente en la variable externa apuntará a una dirección de memoria liberada e inválida.

### 2. ¿Por qué DetalleOrden guarda precioUnitario en lugar de consultar el precio del repuesto cada vez que se calcula el total?

Respuesta:
Se almacena por razones de consistencia histórica e integridad financiera. En un entorno real, los precios de los repuestos cambian con el tiempo debido a la inflación o actualización de costos. Si se consultara el precio actual del inventario al momento de cerrar o auditar una orden pasada, el monto total cambiaría alterando facturas ya emitidas y cerradas. Guardar el precio unitario en el detalle congela el costo exacto que tenía el repuesto en el preciso instante en que fue agregado a la orden.

### 3. Cuando se redimensiona el arreglo de órdenes, ¿qué ocurre con el arreglo de detalles de cada orden? ¿Se copia, se comparte o se pierde? Justifiquen con su implementación.

Respuesta:
Los arreglos de detalles se copian superficialmente en cuanto al puntero, pero sus datos subyacentes se preservan de manera segura.
En nuestra implementación en logica.cpp, cuando el arreglo principal s->ordenes alcanza su capacidad máxima, se crea un nuevo bloque de memoria para las órdenes (OrdenServicio* temp = new OrdenServicio[nuevaCap]) y se realiza una copia miembro a miembro (temp[i] = s->ordenes[i]). Esto significa que el puntero interno detalles (que apunta al bloque dinámico de repuestos de cada orden) se copia bit a bit a la nueva estructura. Dado que los bloques de memoria independientes donde residen los detalles no se destruyen durante el cambio de capacidad del contenedor padre (únicamente se libera el arreglo contenedor viejo con delete[] s->ordenes), las órdenes reubicadas conservan intacto y sin pérdidas el acceso exclusivo a sus respectivos detalles. Al finalizar el programa, la función liberarSistema recorre cada orden activa y libera explícitamente cada bloque de detalles (delete[] s->ordenes[i].detalles) antes de liberar el arreglo de órdenes principal, evitando fugas de memoria.
