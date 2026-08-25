import socket, time

s = socket.socket()
s.connect(('127.0.0.1', 6667))
s.send(b"NICK test\r\nUSER test 0 * :test\r\n")
time.sleep(1)

# Cierre limpio → TCP FIN → POLLHUP en el servidor
s.close()

"""
Actúa como un ingeniero de documentación técnica senior. Necesito que generes
el comentario @mainpage de Doxygen para este proyecto, usando EXACTAMENTE
la siguiente plantilla como estructura base (mismas secciones, mismo orden):
```
/**
 * @mainpage Nombre del Proyecto
 *
 * @section intro_sec Introducción
 * Breve descripción del propósito del proyecto, qué problema resuelve
 * y para quién está pensado (usuarios finales, desarrolladores, etc.).
 *
 * @section features_sec Características principales
 * - Característica 1
 * - Característica 2
 * - Característica 3
 *
 * @section arch_sec Arquitectura
 * Descripción general de los módulos/componentes principales y cómo
 * se relacionan entre sí. Si aplica, referenciar los namespaces o
 * clases más importantes con @ref.
 *
 * @section install_sec Instalación
 * @subsection req_subsec Requisitos previos
 * - Requisito 1 (versión mínima)
 * - Requisito 2
 *
 * @subsection install_steps_subsec Pasos de instalación
 * @code{.sh}
 * git clone <url>
 * cd proyecto
 * # comandos de instalación
 * @endcode
 *
 * @section build_sec Compilación
 * @code{.sh}
 * # comandos de compilación (cmake, make, etc.)
 * @endcode
 *
 * @section usage_sec Modo de uso
 * Ejemplo básico de uso:
 * @code{.cpp}
 * // ejemplo de código
 * @endcode
 *
 * @section deps_sec Dependencias
 * - Dependencia 1 (versión)
 * - Dependencia 2 (versión)
 *
 * @section config_sec Configuración
 * Variables de entorno, archivos de configuración u opciones relevantes.
 *
 * @section testing_sec Pruebas
 * Cómo ejecutar la suite de tests.
 *
 * @section contrib_sec Contribución
 * Enlace o resumen de las guías de contribución (CONTRIBUTING.md).
 *
 * @section license_sec Licencia
 * Tipo de licencia y enlace al archivo LICENSE.
 *
 * @author Nombre del autor/equipo
 * @version 1.0.0
 * @date 2026
 */
```
Instrucciones:
1. Analiza el código fuente, README, CMakeLists/package.json/pyproject.toml
   (o el gestor de dependencias que corresponda) y cualquier archivo de
   configuración del repositorio antes de escribir nada.
2. Rellena cada sección con información REAL y verificable del proyecto.
   No inventes características, dependencias, comandos ni versiones que
   no puedas confirmar en el código o en los archivos del repo.
3. Si no encuentras información suficiente para alguna sección
   (por ejemplo, licencia o autor), déjala como placeholder claramente
   marcado con "TODO: completar" en lugar de inventar contenido.
4. Usa ejemplos de código reales extraídos del propio proyecto (funciones
   públicas, clases principales) en la sección de uso, no pseudocódigo genérico.
5. Respeta la sintaxis correcta de Doxygen (@section, @subsection, @code,
   @endcode, @ref, @author, @version, @date) para que compile sin warnings.
6. Mantén un tono profesional, claro y conciso; evita relleno innecesario.
7. Si el proyecto tiene módulos o namespaces relevantes, enlázalos con @ref
   en la sección de arquitectura.
8. Al final, indícame qué archivos revisaste para generar el contenido,
   para que pueda verificar la precisión.

Genera el bloque de comentario completo, listo para pegar en el archivo
principal del proyecto (main.h, README.dox o el archivo que corresponda).
"""
