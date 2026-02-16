#include "diagnostic_catalog.h"

#include <unordered_map>

namespace aym {

std::string diagnosticSuggestionForCode(const std::string &code) {
    static const std::unordered_map<std::string, std::string> suggestions = {
        {"AYM2001", "Revisa la sintaxis cerca de esa posicion; suele faltar un delimitador como ';', ')' o '}'."},
        {"AYM2002", "Simplifica la sentencia actual y verifica que el parser pueda avanzar consumiendo tokens."},
        {"AYM3001", "Revisa tipos y alcance de variables en la expresion o sentencia reportada."},
        {"AYM3002", "Declara la variable antes de usarla o corrige el nombre si hay un typo."},
        {"AYM3003", "Asegura que ambos lados usen tipos compatibles o aplica conversion explicita."},
        {"AYM3004", "Verifica que la funcion exista en el alcance actual o importa el modulo correcto."},
        {"AYM3005", "Ajusta la cantidad de argumentos para coincidir con la firma de la funcion/metodo."},
        {"AYM3006", "Usa 'break' solo dentro de 'taki', 'ukhakamaxa' o 'match'."},
        {"AYM3007", "Usa 'continue' solo dentro de bucles."},
        {"AYM3008", "Usa 'kuttaya' solo dentro de funciones."},
        {"AYM3009", "Revisa que la clase base y la firma del metodo sean compatibles al sobrescribir."},
        {"AYM3010", "Confirma que la clase exista y que el nombre coincida exactamente con su declaracion."},
        {"AYM4001", "Verifica la ruta del modulo y los directorios de busqueda (directorio actual, modules/ y AYM_PATH)."},
        {"AYM4002", "Rompe el ciclo de importacion extrayendo codigo comun a un modulo base sin dependencias circulares."},
        {"AYM4003", "Corrige el error lexico dentro del modulo importado y vuelve a compilar."},
        {"AYM4004", "Corrige la sintaxis del modulo importado en la linea indicada."},
        {"AYM4005", "Verifica que los simbolos importados existan y que sus nombres coincidan exactamente."},
        {"AYM4006", "Sincroniza y valida dependencias del proyecto (aym.toml/aym.lock) antes de importar paquetes."},
        {"AYM5001", "Asegura que el archivo aym.toml exista y que la ruta indicada sea correcta."},
        {"AYM5002", "Corrige la sintaxis TOML en la linea indicada usando formato clave = \"valor\" y secciones validas."},
        {"AYM5003", "Completa los campos requeridos en [package]: name y version."},
        {"AYM5004", "Verifica permisos de escritura y la ruta de salida del lockfile."},
        {"AYM5005", "Asegura que el archivo aym.lock exista y que la ruta indicada sea correcta."},
        {"AYM5006", "Corrige la sintaxis de aym.lock para cumplir el formato lock_version/cabecera (incluye manifest_checksum)/[[dependency]]."},
        {"AYM5007", "Regenera el lockfile para que coincida con aym.toml y valide checksums."},
        {"AYM5008", "Usa versionado semantico MAJOR.MINOR.PATCH para package.version y project_version."},
        {"AYM5009", "Corrige el requirement de dependencia (ej: 1.2.3, ^1.2.3, ~1.2.3, >=1.0.0, <2.0.0 o *)."},
        {"AYM5010", "Ajusta la version resuelta en aym.lock para que sea semver valida y satisfaga el requirement."}
    };

    auto it = suggestions.find(code);
    if (it == suggestions.end()) {
        return "";
    }
    return it->second;
}

} // namespace aym
