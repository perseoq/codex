# Codex - Extractor de Códigos Legales

Una herramienta CLI para extraer artículos de códigos en formato PDF y convertirlos a JSON estructurado.

## Características

- ✅ Conversión automática de PDF a texto
- ✅ Extracción inteligente de artículos con múltiples patrones
- ✅ Procesamiento asíncrono para mejor rendimiento
- ✅ Limpieza de headers y footers
- ✅ Salida en formato JSON estructurado
- ✅ Diagnóstico integrado y depuración
- ✅ Soporte para múltiples formatos de artículos

## Instalación

### Dependencias

```bash
# Ubuntu/Debian
sudo apt update
sudo apt install poppler-utils nlohmann-json3-dev g++

# Fedora
sudo dnf install poppler-utils nlohmann-json-devel gcc-c++

# O para versiones más antiguas de Fedora/RHEL/CentOS:
# sudo yum install poppler-utils nlohmann-json-devel gcc-c++

# Compilar el programa
g++ -std=c++17 -o codex codex.cpp -lstdc++fs -pthread
```

### Instalación rápida

```bash
# Clonar o descargar el código
git clone <repositorio>
cd codex

# Compilar
make  # o compilar manualmente
```

## Uso

### Sintaxis básica

```bash
./codex [OPCIONES] ARCHIVO_PDF
```

### Ejemplos

```bash
# Extraer artículos a archivo por defecto (codigo_penal.json)
./codex documento.pdf

# Especificar archivo de salida
./codex documento.pdf -o resultados.json
./codex documento.pdf --output resultados.json

# Mostrar ayuda
./codex --help
./codex -h

# Mostrar versión
./codex --version
./codex -v
```

### Opciones

| Opción | Descripción |
|--------|-------------|
| `-h, --help` | Muestra mensaje de ayuda |
| `-o, --output` | Especifica archivo de salida JSON |
| `-v, --version` | Muestra versión del programa |

## Formato de salida

El archivo JSON generado contiene un array de objetos con la siguiente estructura:

```json
[
  {
    "articulo": "Artículo 1.-",
    "contenido": "Texto completo del artículo..."
  },
  {
    "articulo": "Artículo 2.-", 
    "contenido": "Texto completo del artículo..."
  }
]
```

## Formatos soportados

La herramienta reconoce múltiples formatos de artículos:

- `Artículo 1.-`
- `ARTÍCULO 1.-` 
- `Artículo 1o.-`
- `Artículo 1 Bis.-`
- `Artículo 1 Ter.-`
- `Art. 1.-`
- `Artículo 1.1.-`
- Y otras variaciones similares

## Diagnóstico y depuración

Si la herramienta no encuentra artículos, genera archivos de diagnóstico:

- `debug_texto_completo.txt` - Texto completo extraído del PDF
- Muestra vista previa de las primeras líneas en consola
- Proporciona estadísticas del proceso

## Requisitos del sistema

- **Sistema operativo**: Linux (probado en Ubuntu/Debian)
- **Dependencias**: 
  - `poppler-utils` (para pdftotext)
  - `g++` (compilador C++17)
  - `nlohmann-json3-dev` (para manejo de JSON)
- **Memoria**: Suficiente para procesar documentos grandes

## Solución de problemas

### Error de compilación

```bash
# Si falta nlohmann/json.hpp
sudo apt install nlohmann-json3-dev

# Si falta poppler-utils
sudo apt install poppler-utils
```

### Pocos artículos encontrados

1. Verifica el archivo `debug_texto_completo.txt`
2. Revisa el formato de los artículos en el PDF original
3. El PDF podría tener protección o formato complejo

### Violación de segmento

- Generalmente causado por PDFs corruptos o muy grandes
- Verifica que el PDF sea legible con otros lectores

## Limitaciones

- Funciona mejor con PDFs de texto (no escaneados)
- El reconocimiento de OCR no está incluido
- PDFs con formatos muy complejos pueden requerir preprocesamiento

## Contribuir

1. Haz fork del proyecto
2. Crea una rama para tu feature (`git checkout -b feature/AmazingFeature`)
3. Commit tus cambios (`git commit -m 'Add AmazingFeature'`)
4. Push a la rama (`git push origin feature/AmazingFeature`)
5. Abre un Pull Request


## Soporte

Si encuentras problemas o tienes preguntas:

1. Revisa los archivos de diagnóstico generados
2. Verifica que todas las dependencias estén instaladas
3. Asegúrate de que el PDF no esté protegido o corrupto

---

**Nota**: Esta herramienta está diseñada para procesar documentos legales públicos y facilitar su análisis. Siempre verifica los resultados con el documento original.
