// sudo apt install poppler-utils nlohmann-json3-dev

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <cstdlib>
#include <future>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

struct Articulo {
    std::string articulo;
    std::string contenido;
};

// Función para limpiar caracteres no UTF-8
std::string limpiarUTF8(const std::string& texto) {
    std::string resultado;
    resultado.reserve(texto.size());
    
    for (size_t i = 0; i < texto.size(); ++i) {
        unsigned char c = texto[i];
        
        // Caracteres ASCII válidos (0-127)
        if (c <= 0x7F) {
            resultado += c;
        }
        // Caracteres UTF-8 de 2 bytes (inicio: 110xxxxx)
        else if (c >= 0xC2 && c <= 0xDF) {
            if (i + 1 < texto.size()) {
                unsigned char c2 = texto[i + 1];
                if (c2 >= 0x80 && c2 <= 0xBF) {
                    resultado += c;
                    resultado += c2;
                    i++;
                }
            }
        }
        // Caracteres UTF-8 de 3 bytes (inicio: 1110xxxx)
        else if (c >= 0xE0 && c <= 0xEF) {
            if (i + 2 < texto.size()) {
                unsigned char c2 = texto[i + 1];
                unsigned char c3 = texto[i + 2];
                if (c2 >= 0x80 && c2 <= 0xBF && c3 >= 0x80 && c3 <= 0xBF) {
                    resultado += c;
                    resultado += c2;
                    resultado += c3;
                    i += 2;
                }
            }
        }
        // Caracteres UTF-8 de 4 bytes (inicio: 11110xxx)
        else if (c >= 0xF0 && c <= 0xF4) {
            if (i + 3 < texto.size()) {
                unsigned char c2 = texto[i + 1];
                unsigned char c3 = texto[i + 2];
                unsigned char c4 = texto[i + 3];
                if (c2 >= 0x80 && c2 <= 0xBF && c3 >= 0x80 && c3 <= 0xBF && c4 >= 0x80 && c4 <= 0xBF) {
                    resultado += c;
                    resultado += c2;
                    resultado += c3;
                    resultado += c4;
                    i += 3;
                }
            }
        }
        // Reemplazar caracteres inválidos con espacio
        else {
            resultado += ' ';
        }
    }
    
    return resultado;
}

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
        
        // Usar -enc UTF-8 para forzar encoding UTF-8
        std::string comando = "pdftotext -enc UTF-8 -layout \"" + pdfPath + "\" \"" + tempTxt + "\"";
        int resultado = system(comando.c_str());
        
        if (resultado != 0) {
            throw std::runtime_error("Error al convertir PDF a texto");
        }
        
        if (!fs::exists(tempTxt) || fs::file_size(tempTxt) == 0) {
            throw std::runtime_error("La conversión produjo un archivo vacío");
        }
        
        std::string texto = loadFile(tempTxt);
        fs::remove(tempTxt);
        
        // Limpiar UTF-8
        return limpiarUTF8(texto);
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

// Función para limpiar espacios múltiples
std::string limpiarEspacios(const std::string& texto) {
    std::string resultado;
    bool espacio_previo = false;
    
    for (char c : texto) {
        if (std::isspace(c)) {
            if (!espacio_previo) {
                resultado += ' ';
                espacio_previo = true;
            }
        } else {
            resultado += c;
            espacio_previo = false;
        }
    }
    
    // Eliminar espacios al inicio y final
    size_t start = resultado.find_first_not_of(" ");
    size_t end = resultado.find_last_not_of(" ");
    
    if (start == std::string::npos || end == std::string::npos) {
        return "";
    }
    
    return resultado.substr(start, end - start + 1);
}

// Función para verificar si una línea es inicio de artículo
bool esInicioArticulo(const std::string& linea, std::string& articulo_completo) {
    // Convertir a minúsculas para búsqueda case-insensitive
    std::string linea_min = linea;
    std::transform(linea_min.begin(), linea_min.end(), linea_min.begin(), ::tolower);
    
    // Buscar patrones de artículo
    size_t pos = std::string::npos;
    std::string patron;
    
    if (linea_min.find("articulo") != std::string::npos) {
        pos = linea_min.find("articulo");
        patron = "articulo";
    } else if (linea_min.find("artículo") != std::string::npos) {
        pos = linea_min.find("artículo");
        patron = "artículo";
    } else if (linea_min.find("art.") != std::string::npos) {
        pos = linea_min.find("art.");
        patron = "art.";
    }
    
    if (pos == std::string::npos) {
        return false;
    }
    
    // Extraer el texto completo del artículo
    articulo_completo = linea.substr(pos);
    
    // Verificar que sigue con un número
    size_t num_pos = pos + patron.size();
    if (num_pos >= linea.size()) {
        return false;
    }
    
    // Buscar el número después del artículo
    std::string resto = linea.substr(num_pos);
    size_t digito_pos = resto.find_first_of("0123456789");
    
    if (digito_pos == std::string::npos) {
        return false;
    }
    
    return true;
}

// Función principal para extraer artículos sin usar regex
std::vector<Articulo> extraerArticulosManual(const std::string &texto) {
    std::vector<Articulo> articulos;
    
    if (texto.empty()) {
        return articulos;
    }
    
    std::istringstream stream(texto);
    std::string linea;
    std::vector<std::pair<size_t, std::string>> inicios_articulos;
    size_t posicion_actual = 0;
    
    // Primera pasada: encontrar todas las líneas que son artículos
    while (std::getline(stream, linea)) {
        std::string articulo_completo;
        if (esInicioArticulo(linea, articulo_completo)) {
            inicios_articulos.emplace_back(posicion_actual, articulo_completo);
        }
        posicion_actual += linea.size() + 1; // +1 por el salto de línea
    }
    
    std::cout << "🔍 Encontrados " << inicios_articulos.size() << " posibles inicios de artículos" << std::endl;
    
    // Segunda pasada: extraer contenido entre artículos
    for (size_t i = 0; i < inicios_articulos.size(); ++i) {
        size_t inicio_pos = inicios_articulos[i].first;
        const std::string& encabezado = inicios_articulos[i].second;
        
        size_t contenido_inicio = inicio_pos + encabezado.size();
        size_t contenido_fin = (i + 1 < inicios_articulos.size()) ? inicios_articulos[i + 1].first : texto.size();
        
        if (contenido_inicio < contenido_fin && (contenido_fin - contenido_inicio) > 10) {
            std::string contenido = texto.substr(contenido_inicio, contenido_fin - contenido_inicio);
            
            // Limpiar contenido
            contenido = limpiarEspacios(contenido);
            
            if (!contenido.empty() && contenido.size() > 20) {
                // Limpiar UTF-8 del contenido también
                std::string contenido_limpio = limpiarUTF8(contenido);
                std::string encabezado_limpio = limpiarUTF8(encabezado);
                
                articulos.push_back({encabezado_limpio, contenido_limpio});
            }
        }
    }
    
    return articulos;
}

// Función para analizar el texto y detectar patrones
void analizarPatronesTexto(const std::string& texto) {
    std::cout << "🔍 Analizando patrones en el texto..." << std::endl;
    
    int count_articulo = 0;
    int count_articulo_decimal = 0;
    std::vector<std::string> ejemplos;
    
    std::istringstream stream(texto);
    std::string linea;
    
    while (std::getline(stream, linea)) {
        std::string articulo_completo;
        if (esInicioArticulo(linea, articulo_completo)) {
            count_articulo++;
            
            // Verificar si es decimal
            if (articulo_completo.find('.') != std::string::npos) {
                count_articulo_decimal++;
            }
            
            // Guardar algunos ejemplos
            if (ejemplos.size() < 3) {
                ejemplos.push_back(limpiarUTF8(articulo_completo));
            }
        }
    }
    
    std::cout << "   • Artículos encontrados: " << count_articulo << std::endl;
    std::cout << "   • Artículos decimales: " << count_articulo_decimal << std::endl;
    
    if (!ejemplos.empty()) {
        std::cout << "   • Ejemplos:" << std::endl;
        for (const auto& ejemplo : ejemplos) {
            std::cout << "     → \"" << ejemplo << "\"" << std::endl;
        }
    }
}

// Función asíncrona para procesamiento pesado
std::future<std::vector<Articulo>> procesarArticulosAsync(const std::string& texto) {
    return std::async(std::launch::async, [texto]() {
        // Primero analizar los patrones presentes
        analizarPatronesTexto(texto);
        
        // Intentar con el método manual
        auto articulos = extraerArticulosManual(texto);
        
        std::cout << "✅ " << articulos.size() << " artículos extraídos" << std::endl;
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
    std::cout << "codex v2.1 (UTF-8 fix)\n"
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
    
    // Guardar texto completo para depuración
    std::ofstream debug_raw("debug_texto_completo.txt");
    debug_raw << texto;
    debug_raw.close();
    std::cout << "📁 Texto completo guardado en: debug_texto_completo.txt" << std::endl;
    
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
    
    if (articulos.empty()) {
        std::cout << "❌ No se encontraron artículos. Revisa el archivo debug_texto_completo.txt" << std::endl;
        return;
    }
    
    // Mostrar estadísticas de artículos
    std::cout << "\n📊 Estadísticas de artículos:" << std::endl;
    std::cout << "========================================" << std::endl;
    for (size_t i = 0; i < std::min(articulos.size(), size_t(5)); ++i) {
        std::cout << "Artículo " << (i + 1) << ": " << articulos[i].articulo << std::endl;
        std::cout << "Longitud: " << articulos[i].contenido.size() << " caracteres" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
    }
    
    if (articulos.size() > 5) {
        std::cout << "... y " << (articulos.size() - 5) << " artículos más" << std::endl;
    }
    
    // Guardar resultados con manejo de errores UTF-8
    std::cout << "💾 Guardando JSON..." << std::endl;
    try {
        json j;
        for (const auto& articulo : articulos) {
            j.push_back({{"articulo", articulo.articulo}, {"contenido", articulo.contenido}});
        }
        
        std::ofstream out(jsonPath);
        out << j.dump(2);
        out.close();
        
        std::cout << "✅ Archivo generado: " << jsonPath << std::endl;
        std::cout << "🎉 Proceso completado exitosamente!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "❌ Error al guardar JSON: " << e.what() << std::endl;
        
        // Intentar guardar de forma más simple
        std::cout << "🔄 Intentando guardar de forma alternativa..." << std::endl;
        
        std::ofstream out_simple(jsonPath);
        out_simple << "[\n";
        for (size_t i = 0; i < articulos.size(); ++i) {
            out_simple << "  {\n";
            out_simple << "    \"articulo\": \"" << articulos[i].articulo << "\",\n";
            out_simple << "    \"contenido\": \"" << articulos[i].contenido << "\"\n";
            out_simple << "  }";
            if (i < articulos.size() - 1) {
                out_simple << ",";
            }
            out_simple << "\n";
        }
        out_simple << "]\n";
        out_simple.close();
        
        std::cout << "✅ Archivo generado (formato simple): " << jsonPath << std::endl;
    }
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
