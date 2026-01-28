#include <iostream>     
#include <string>       
#include <list>         
#include <map>          
#include <functional>       // para std::function para el polimorfismo de callables
#include <algorithm>        // para std::for_each, std::find


// utilizamos algunos alias

// los argumentos de los comandos siempre seran una lista de strings
using Argumentos = std::list<std::string>;


// definimos Command para guardar, una lambda, una función global o una clase con operator()
// dualquier cosa que tenga la firma void(const Argumentos&)
using Command = std::function<void(const std::list<std::string>&)>;


class Entity {
private:
    std::string nombre;
    int posX, posY;
    float nivelEnergia;

public:
    // constructor
    Entity(std::string n) : nombre(n), posX(0), posY(0), nivelEnergia(100.0f) {}

    // metodo para cambiar las coordenadas
    void desplazar(int dx, int dy) {
        posX += dx;
        posY += dy;
    }

    // para aumentar la energia con un tope de 100
    void recargarEnergia(float cantidad) {
        nivelEnergia += cantidad;
        if (nivelEnergia > 100.0f) nivelEnergia = 100.0f;
    }

    // reduce la energia, no se admite negativo
    void recibirDanio(float cantidad) {
        nivelEnergia -= cantidad;
        if (nivelEnergia < 0.0f) nivelEnergia = 0.0f;
    }

    // vuelve al estado inicial
    void resetearTotal() {
        posX = 0; posY = 0; nivelEnergia = 100.0f;
    }

    //impresion
    std::string obtenerEstado() const {
        return "[Entity: " + nombre + 
               " | Pos: (" + std::to_string(posX) + "," + std::to_string(posY) + ")" +
               " | Energia: " + std::to_string(nivelEnergia) + "]";
    }
};

//funcion global para movernos de coordenadas
void funcionMover(Entity& entidad, const Argumentos& args) {

    if (args.size() < 2) {
        std::cout << "Error: Se requieren 2 argumentos (x, y)" << std::endl;
        return;
    }

    try {
// try-catch para evitar que el programa se cierre si meten letras en vez de numeros
    
        // obtengo el iterador al inicio de la lista
        auto it = args.begin();

        // desreferenciamos y convertimos a int
        int dx = std::stoi(*it); 

        // avanzamos al siguiente argumento
        std::advance(it, 1);     

        int dy = std::stoi(*it);
        
        // movemos
        entidad.desplazar(dx, dy);
        
    } catch (...) {
        std::cout << "Error: Argumentos no numericos en comando mover" << std::endl;
    }
}

// functor para contar cuantas veces curo
class FunctorCurar {
private:
    Entity& refEntidad;     // referencia a la entidad real
    int contadorUso;        // estado interno del comando     

public:

    // constructor sobre la entidad que operara
    FunctorCurar(Entity& e) : refEntidad(e), contadorUso(0) {}

    void operator()(const Argumentos& args) {
        
        //validar argumentos no vacios
        if (args.empty()) return;

        try {

            // front() accede al primer elemento de la lista
            float cantidad = std::stof(args.front()); 

            refEntidad.recargarEnergia(cantidad);
            contadorUso++;  // actualizo el estado interno


            std::cout << "Curacion aplicada. Total de veces usada: " << contadorUso << std::endl;
        } catch (...) {
            std::cout << "Error: Argumento invalido para curar" << std::endl;
        }
    }
};


//clase para manejar los comandos

class CommandCenter {
private:
    
    // mapa para guardar comandos por su nombre
    std::map<std::string, Command> mapaComandos;

    // lista para el historial 
    std::list<std::string> historial;

    //usamos un alias para la estructura de una macro, el primer elemento del par guarda el nombre del comando
    //y el segundo la lista de Argumentos
    using PasosMacro = std::list<std::pair<std::string, Argumentos>>;
    
    // mapa para almacenar los macros registrados
    std::map<std::string, PasosMacro> mapaMacros;

    // referencia a la entidad que este motor controla
    Entity& entidadControlada;

public:
    CommandCenter(Entity& e) : entidadControlada(e) {}

    // registra un comando en el mapa, gracias a std::function en el mapa, puedo pasarle lambdas, punteros, o functores.
    void registerCommand(const std::string& clave, Command cmd) {
        mapaComandos[clave] = cmd; 
    }

    // para registrar un Macro
    void registerMacro(const std::string& name, const std::list<std::pair<std::string, std::list<std::string>>>& steps) {
        mapaMacros[name] = steps;
    }

    // ejecuta un comando buscando en el mapa
    void execute(const std::string& clave, const Argumentos& args) {


        //uso de iterador explicito para buscar (mapa.find)
        std::map<std::string, Command>::iterator it = mapaComandos.find(clave);


        // verificamos si it == end(), el comando no existe
        if (it != mapaComandos.end()) {

            // guardo como estaba antes
            std::string estadoAntes = entidadControlada.obtenerEstado();

            //ejectutamos el comando, it-> second es la funcion/lambda guardada
            it->second(args); 

            std::string log = "CMD: " + clave + " | " + estadoAntes + " -> " + entidadControlada.obtenerEstado();
            historial.push_back(log);
            
        } else {
            std::cout << "Error: El comando '" << clave << "' no es reconocido" << std::endl;
        }
    }



    // para la ejecucion de un Macro

    void executeMacro(const std::string& name) {

        // busco el macro en su mapa correspondiente
        auto itMacro = mapaMacros.find(name); 

        if (itMacro != mapaMacros.end()) {
            std::cout << "Ejecutando Macro: " << name << std::endl;
            

            // obtengo la lista de pasos
            const PasosMacro& pasos = itMacro->second;

            // Itero sobre la lista de pares comando, argumentos, uno por uno
            for (auto it = pasos.begin(); it != pasos.end(); ++it) {
                
                //it->first es el nombre, it->second son los argumentos
                execute(it->first, it->second);
            }
            std::cout << "Fin de Macro" << std::endl;
        } else {
            std::cout << "Error: Macro '" << name << "' no encontrado" << std::endl;
        }
    }

    

    //para eliminar comandos dinamicamente
    void eliminarComando(const std::string& clave) {

        // aseguramos existencia
        auto it = mapaComandos.find(clave);
        if (it != mapaComandos.end()) {

            // eliminamos con map::erase
            mapaComandos.erase(it); 
            std::cout << "Comando '" << clave << "' eliminado del sistema" << std::endl;
        }
    }


    //impresion del historial
    void verHistorial() {
        std::cout << "\nHistorial---------" << std::endl;

        // imprimimos la lista con for_each
        std::for_each(historial.begin(), historial.end(), 
            [](const std::string& entrada) {
                std::cout << entrada << std::endl;
            }
        );
        std::cout << "-----------------------------------\n" << std::endl;
    }
};

int main() {


    // instanciacion
    Entity robot("Wall-E");
    CommandCenter center(robot); // instancia 'center'

    std::cout << "Sistema prendido. " << robot.obtenerEstado() << "\n" << std::endl;



    // registro de comandos

    // caso de función libre
    center.registerCommand("mover", [&robot](const Argumentos& args) {
        funcionMover(robot, args);
    });

    // caso de functor
    FunctorCurar objetoCurador(robot);
    center.registerCommand("curar", objetoCurador);

    // caso de funcion lambda
    center.registerCommand("daniar", [&robot](const Argumentos& args) {
        if (args.empty()) return;
        try {
            float dmg = std::stof(args.front());
            robot.recibirDanio(dmg);
        } catch (...) {std::cout << "Error: Argumento invalido para daniar no numerico" << std::endl;}
    });

    // caso de lambda sin argumentos
    center.registerCommand("status", [&robot](const Argumentos&) {
        std::cout << " STATUS REPORTE: " << robot.obtenerEstado() << std::endl;
    });

   

    // registro de macros
    
    // macro 1: definimos una lista de pasos mover -> mover -> status como macro 'patrulla' del robot
    std::list<std::pair<std::string, Argumentos>> pasosPatrulla;
    pasosPatrulla.push_back({ "mover", {"10", "0"} }); 
    pasosPatrulla.push_back({ "mover", {"0", "10"} }); 
    pasosPatrulla.push_back({ "status", {} });
    
    center.registerMacro("patrulla", pasosPatrulla);

    



    //macro 2: macro de reinicio resetea el estado del robot y lanza status

    // resgistramos una lambda generica como comando auxiliar
    // para poner resetearTotal() al sistema en incluirla en la secuencia de la macro
    center.registerCommand("reset_interno", [&robot](auto){ robot.resetearTotal(); });
    
    std::list<std::pair<std::string, Argumentos>> pasosReset;
    pasosReset.push_back({ "reset_interno", {} });
    pasosReset.push_back({ "status", {} });
    
    center.registerMacro("reinicio_fabrica", pasosReset);



    //macro 3: macro "huida" conjunto de functor 'curar' y funcion libre 'mover' 

    std::list<std::pair<std::string, Argumentos>> pasosHuida;
    pasosHuida.push_back({ "curar", {"50"} }); 
    pasosHuida.push_back({ "mover", {"100", "100"} }); 
    center.registerMacro("huida", pasosHuida);




    //pruebas


    //probando funcion global mover
    std::cout << "TEST 1: Probando comando mover" << std::endl;
    center.execute("mover", {"5", "5"});    // movimiento positivo
    center.execute("mover", {"-2", "10"});  // movimiento negativo
    center.execute("mover", {"0", "-20"});  // movimiento vertical


    //probando lambda daniar 
    std::cout << "\nTEST 2: Probando comando lambda 'daniar'" << std::endl;
    center.execute("daniar", {"10.5"});     //  usando decimales
    center.execute("daniar", {"50"});       // dano fuerte
    center.execute("daniar", {"200"});      // debería bajar a 0 pero no negativo

    
    // probando functor curar 
    std::cout << "\nTEST 3: Probando comando tipo functor 'curar'" << std::endl;
    center.execute("curar", {"10"});        
    center.execute("curar", {"500"});       //curaciones
    center.execute("curar", {"1"});         

    // validaciones y errores
    std::cout << "\nTEST 4: Probando validaciones" << std::endl;
    center.execute("mover", {"1"});         // error: faltan argumentos
    center.execute("volar", {});            // error: comando inexistente
    center.execute("daniar", {"letras"});   // error: argumento invalido hay texto en lugar de numero

    // ejecucion de macros
    std::cout << "\nTEST 5: Ejecutando Macros" << std::endl;
    center.executeMacro("patrulla"); 
    center.executeMacro("reinicio_fabrica");
    center.executeMacro("huida");


    //reporte
    center.verHistorial();

    return 0;
}