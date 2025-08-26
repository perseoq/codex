// sudo apt install poppler-utils nlohmann-json3-dev

#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <cstdlib>
#include <future>
#include <thread>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

struct Articulo {
    std::string articulo;
    std::string contenido;
};

// Leer archivo completo a string
std::string loadFile(const std::string &filename) {
    std::ifstream in(filename, std::ios::binary);
    if (!in) throw std::runtime_error("No se pudo abrir " + filename);
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// Función asíncrona para convertir PDF a texto
std::future<std::string> convertirPDFAsync(const std::string& pdfPath) {
    return std::async(std::launch::async, [pdfPath]() {
        std::string tempTxt = fs::temp_directory_path() / ("salida_temp_" + std::to_string(std::time(nullptr)) + ".txt");
        
        std::string comando = "pdftotext -layout \"" + pdfPath + "\" \"" + tempTxt + "\"";
        int resultado = system(comando.c_str());
        
        if (resultado != 0) {
            throw std::runtime_error("Error al convertir PDF a texto");
        }
        
        if (!fs::exists(tempTxt) || fs::file_size(tempTxt) == 0) {
            throw std::runtime_error("La conversión produjo un archivo vacío");
        }
        
        std::string texto = loadFile(tempTxt);
        fs::remove(tempTxt);
        
        return texto;
    });
}

// Divide en páginas usando \f (form feed)
std::vector<std::string> splitPages(const std::string &texto) {
    std::vector<std::string> paginas;
    std::stringstream ss(texto);
    std::string pagina;
    
    while (std::getline(ss, pagina, '\f')) {
        if (!pagina.empty()) {
            paginas.push_back(pagina);
        }
    }
    return paginas;
}

// Elimina headers/footers (versión simplificada)
std::string quitarHeadersFooters(const std::vector<std::string> &paginas) {
    if (paginas.empty()) return "";
    
    std::ostringstream limpio;
    for (const auto &pagina : paginas) {
        limpio << pagina << "\n";
    }
    return limpio.str();
}

// Función mejorada para extraer artículos con múltiples patrones
std::vector<Articulo> extraerArticulosMejorado(const std::string &texto) {
    std::vector<Articulo> articulos;
    
    if (texto.empty()) {
        return articulos;
    }
    
    // Múltiples patrones para capturar diferentes formatos de artículos
    std::vector<std::regex> patrones = {
        std::regex(R"(Artículo\s+(\d+)(?:o|°)?(?:\s+([A-Za-záéíóúÁÉÍÓÚñÑ]+))?\s*\.-)", std::regex::icase),
        std::regex(R"(ARTÍCULO\s+(\d+)(?:o|°)?(?:\s+([A-Za-záéíóúÁÉÍÓÚñÑ]+))?\s*\.-)", std::regex::icase),
        std::regex(R"(Art\.\s*(\d+)(?:o|°)?(?:\s+([A-Za-záéíóúÁÉÍÓÚñÑ]+))?\s*\.-)", std::regex::icase),
        std::regex(R"(artículo\s+(\d+)(?:o|°)?(?:\s+([A-Za-záéíóúÁÉÍÓÚñÑ]+))?\s*\.-)", std::regex::icase)
    };
    
    // Encontrar todas las posiciones de inicio de artículos
    std::vector<std::pair<size_t, std::string>> inicios;
    
    for (const auto& patron : patrones) {
        std::sregex_iterator it(texto.begin(), texto.end(), patron);
        std::sregex_iterator end;
        
        for (; it != end; ++it) {
            size_t pos = it->position();
            std::string match = it->str();
            inicios.emplace_back(pos, match);
        }
    }
    
    // Ordenar por posición
    std::sort(inicios.begin(), inicios.end(), 
        [](const auto& a, const auto& b) { return a.first < b.first; });
    
    // Eliminar duplicados (misma posición)
    inicios.erase(std::unique(inicios.begin(), inicios.end(), 
        [](const auto& a, const auto& b) { return a.first == b.first; }), inicios.end());
    
    // Extraer contenido entre artículos
    for (size_t i = 0; i < inicios.size(); ++i) {
        size_t inicio_pos = inicios[i].first;
        const std::string& encabezado = inicios[i].second;
        
        size_t contenido_inicio = inicio_pos + encabezado.size();
        size_t contenido_fin = (i + 1 < inicios.size()) ? inicios[i + 1].first : texto.size();
        
        if (contenido_inicio < contenido_fin) {
            std::string contenido = texto.substr(contenido_inicio, contenido_fin - contenido_inicio);
            
            // Limpiar contenido
            contenido = std::regex_replace(contenido, std::regex("\\s+"), " ");
            contenido = std::regex_replace(contenido, std::regex("^\\s+|\\s+$"), "");
            
            // Eliminar números de página u otros artefactos comunes
            contenido = std::regex_replace(contenido, std::regex("\\b\\d+\\s*\\/\\s*\\d+\\b"), "");
            contenido = std::regex_replace(contenido, std::regex("^\\d+\\s*$"), "");
            
            if (!contenido.empty() && contenido.size() > 10) { // Mínimo 10 caracteres
                articulos.push_back({encabezado, contenido});
            }
        }
    }
    
    return articulos;
}

// Función alternativa: búsqueda por líneas para documentos complejos
std::vector<Articulo> extraerPorLineas(const std::string& texto) {
    std::vector<Articulo> articulos;
    std::istringstream stream(texto);
    std::string linea;
    std::string articulo_actual;
    std::string contenido_actual;
    bool en_articulo = false;
    
    // Patrones para identificar inicio de artículos
    std::regex patron_articulo(R"(Artículo\s+\d+(?:o|°)?(?:\s+[A-Za-záéíóúÁÉÍÓÚñÑ]+)?\s*\.-)", std::regex::icase);
    std::regex patron_articulo_alt(R"(ARTÍCULO\s+\d+(?:o|°)?(?:\s+[A-Za-záéíóúÁÉÍÓÚñÑ]+)?\s*\.-)", std::regex::icase);
    std::regex patron_art(R"(Art\.\s*\d+(?:o|°)?(?:\s+[A-Za-záéíóúÁÉÍÓÚñÑ]+)?\s*\.-)", std::regex::icase);
    
    while (std::getline(stream, linea)) {
        // Limpiar línea
        linea = std::regex_replace(linea, std::regex("^\\s+|\\s+$"), "");
        
        if (linea.empty()) continue;
        
        // Verificar si es inicio de artículo
        if (std::regex_search(linea, patron_articulo) || 
            std::regex_search(linea, patron_articulo_alt) || 
            std::regex_search(linea, patron_art)) {
            
            // Guardar artículo anterior si existe
            if (!articulo_actual.empty() && !contenido_actual.empty()) {
                contenido_actual = std::regex_replace(contenido_actual, std::regex("\\s+"), " ");
                contenido_actual = std::regex_replace(contenido_actual, std::regex("^\\s+|\\s+$"), "");
                if (!contenido_actual.empty()) {
                    articulos.push_back({articulo_actual, contenido_actual});
                }
            }
            
            // Nuevo artículo
            articulo_actual = linea;
            contenido_actual = "";
            en_articulo = true;
        } 
        else if (en_articulo) {
            // Continuar acumulando contenido
            if (!contenido_actual.empty()) {
                contenido_actual += " ";
            }
            contenido_actual += linea;
        }
    }
    
    // Guardar último artículo
    if (!articulo_actual.empty() && !contenido_actual.empty()) {
        contenido_actual = std::regex_replace(contenido_actual, std::regex("\\s+"), " ");
        contenido_actual = std::regex_replace(contenido_actual, std::regex("^\\s+|\\s+$"), "");
        if (!contenido_actual.empty()) {
            articulos.push_back({articulo_actual, contenido_actual});
        }
    }
    
    return articulos;
}

// Función asíncrona para procesamiento pesado
std::future<std::vector<Articulo>> procesarArticulosAsync(const std::string& texto) {
    return std::async(std::launch::async, [texto]() {
        // Primero intentar con el método mejorado
        auto articulos = extraerArticulosMejorado(texto);
        
        // Si no encuentra suficientes artículos, probar con el método por líneas
        if (articulos.size() < 10) {
            std::cout << "⚠️  Pocos artículos encontrados (" << articulos.size() 
                      << "), intentando método alternativo..." << std::endl;
            auto articulos_alt = extraerPorLineas(texto);
            
            if (articulos_alt.size() > articulos.size()) {
                return articulos_alt;
            }
        }
        
        return articulos;
    });
}

// Función para mostrar ayuda
void mostrarAyuda(const std::string& nombrePrograma) {
    std::cout << "Uso: " << nombrePrograma << " [OPCIONES] ARCHIVO_PDF\n\n"
              << "Extrae artículos de un código penal en PDF y los guarda en formato JSON.\n\n"
              << "Opciones:\n"
              << "  -h, --help                Muestra este mensaje de ayuda\n"
              << "  -o, --output ARCHIVO      Especifica el archivo de salida JSON\n"
              << "  -v, --version             Muestra la versión del programa\n\n"
              << "Ejemplos:\n"
              << "  " << nombrePrograma << " codigo_penal.pdf\n"
              << "  " << nombrePrograma << " codigo_penal.pdf -o articulos.json\n"
              << "  " << nombrePrograma << " codigo_penal.pdf --output articulos.json\n";
}

// Función para mostrar versión
void mostrarVersion() {
    std::cout << "codex v1.2 (mejorado)\n"
              << "Herramienta para extraer artículos de códigos penales desde PDF\n";
}

// Función principal asíncrona
void procesarPDFAsync(const std::string& pdfPath, const std::string& jsonPath) {
    std::cout << "📖 Procesando: " << pdfPath << std::endl;
    
    if (!fs::exists(pdfPath)) {
        throw std::runtime_error("El archivo PDF no existe");
    }
    
    // Convertir PDF a texto de forma asíncrona
    std::cout << "🔄 Convirtiendo PDF a texto (async)..." << std::endl;
    auto futuroTexto = convertirPDFAsync(pdfPath);
    
    // Mostrar progreso mientras esperamos
    while (futuroTexto.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << std::endl;
    
    std::string texto = futuroTexto.get();
    std::cout << "✅ PDF convertido: " << texto.size() << " caracteres" << std::endl;
    
    if (texto.size() < 1000) {
        std::cout << "⚠️  El texto parece muy corto, puede haber errores" << std::endl;
    }
    
    // Guardar texto completo para depuración
    std::ofstream debug_raw("debug_texto_completo.txt");
    debug_raw << texto;
    debug_raw.close();
    std::cout << "📁 Texto completo guardado en: debug_texto_completo.txt" << std::endl;
    
    // Mostrar primeras líneas para diagnóstico
    std::istringstream preview_stream(texto);
    std::string linea;
    int line_count = 0;
    std::cout << "\n🔍 Vista previa de las primeras líneas:" << std::endl;
    std::cout << "========================================" << std::endl;
    while (std::getline(preview_stream, linea) && line_count < 10) {
        if (!linea.empty() && linea.size() > 3) {
            std::cout << "Línea " << (line_count + 1) << ": " << linea << std::endl;
            line_count++;
        }
    }
    std::cout << "========================================" << std::endl;
    
    // Procesar páginas
    std::cout << "📄 Procesando páginas..." << std::endl;
    auto paginas = splitPages(texto);
    std::string textoLimpio = quitarHeadersFooters(paginas);
    
    // Procesar artículos de forma asíncrona
    std::cout << "🔍 Extrayendo artículos (async)..." << std::endl;
    auto futuroArticulos = procesarArticulosAsync(textoLimpio);
    
    // Mostrar progreso
    while (futuroArticulos.wait_for(std::chrono::milliseconds(100)) != std::future_status::ready) {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << std::endl;
    
    auto articulos = futuroArticulos.get();
    std::cout << "✅ " << articulos.size() << " artículos encontrados" << std::endl;
    
    if (articulos.empty()) {
        std::cout << "❌ No se encontraron artículos. Revisa el archivo debug_texto_completo.txt" << std::endl;
        return;
    }
    
    // Mostrar primeros artículos para verificación
    std::cout << "\n📋 Primeros artículos encontrados:" << std::endl;
    std::cout << "========================================" << std::endl;
    for (size_t i = 0; i < std::min(articulos.size(), size_t(3)); ++i) {
        std::cout << "Artículo " << (i + 1) << ": " << articulos[i].articulo << std::endl;
        std::cout << "Contenido (inicio): " 
                  << articulos[i].contenido.substr(0, std::min(100, (int)articulos[i].contenido.size()))
                  << "..." << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    }
    
    // Guardar resultados
    std::cout << "💾 Guardando JSON..." << std::endl;
    json j;
    for (const auto& articulo : articulos) {
        j.push_back({{"articulo", articulo.articulo}, {"contenido", articulo.contenido}});
    }
    
    std::ofstream out(jsonPath);
    out << j.dump(2);
    out.close();
    
    std::cout << "✅ Archivo generado: " << jsonPath << std::endl;
    std::cout << "🎉 Proceso completado exitosamente!" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string pdfPath;
    std::string jsonPath = "codigo_penal.json";
    
    // Procesar argumentos
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            mostrarAyuda(argv[0]);
            return 0;
        } else if (arg == "-v" || arg == "--version") {
            mostrarVersion();
            return 0;
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                jsonPath = argv[++i];
            } else {
                std::cerr << "Error: La opción " << arg << " requiere un argumento.\n";
                return 1;
            }
        } else if (arg[0] == '-') {
            std::cerr << "Error: Opción desconocida: " << arg << "\n";
            return 1;
        } else {
            pdfPath = arg;
        }
    }
    
    if (pdfPath.empty()) {
        std::cerr << "Error: Debe especificar un archivo PDF.\n\n";
        mostrarAyuda(argv[0]);
        return 1;
    }
    
    if (fs::path(pdfPath).extension() != ".pdf") {
        std::cerr << "Error: El archivo debe tener extensión .pdf\n";
        return 1;
    }
    
    try {
        procesarPDFAsync(pdfPath, jsonPath);
    } catch (const std::exception &e) {
        std::cerr << "❌ Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
