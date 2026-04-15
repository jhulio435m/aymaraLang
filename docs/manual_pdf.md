# Manual de Usuario y Documentacion de AymaraLang

Este documento unifica la documentacion esencial del proyecto en un solo manual
pensado para lectura continua o distribucion en PDF.

## 1. Que es AymaraLang

AymaraLang es un lenguaje de programacion compilado inspirado en la lengua
aymara. El compilador principal es `aymc` y el gestor de proyectos es `aym`.

El flujo basico es:

1. escribir un archivo `.aym`
2. validarlo o compilarlo con `aymc`
3. ejecutarlo como binario nativo

El lenguaje ya tiene una sintaxis canonica estable para:

- variables y tipos
- control de flujo
- funciones
- modulos
- clases
- enumeraciones y seleccion
- manejo de errores

## 2. Instalacion

### 2.1 Windows

Los instaladores publicados para Windows (`AymaraLang-Setup.msi` y
`AymaraLang-Setup.exe`) ya incluyen la toolchain necesaria. No hace falta
instalar `nasm` ni `gcc` aparte para usar el compilador desde un release.

Instalacion recomendada:

```powershell
msiexec /i .\AymaraLang-Setup.msi /passive
```

Instalacion interactiva con NSIS:

```powershell
.\AymaraLang-Setup.exe
```

Verificacion inicial:

```powershell
aymc --help
aym --help
aymc .\samples\fundamentos\basicos.aym
```

### 2.2 Linux

En Linux se requieren dependencias del sistema para compilar y ejecutar:

- `nasm`
- `gcc`
- `g++`
- soporte X11 para programas GUI

Instalacion automatica de dependencias:

```bash
bash scripts/install/install_deps_linux.sh
```

Validacion sin instalar:

```bash
bash scripts/install/install_deps_linux.sh --check-only
```

Uso desde bundle:

```bash
tar -xzf AymaraLang-0.1.0-Linux.tar.gz
cd AymaraLang-0.1.0-Linux/bin
./aymc --help
./aym --help
```

## 3. Primer programa

Archivo `hola.aym`:

```aym
qallta
qillqa("Kamisaraki!")
tukuya
```

Compilacion:

```bash
aymc hola.aym
```

Esto genera un ejecutable nativo. En Windows normalmente sera `hola.exe`.

## 4. Tipos y variables

Tipos principales:

- `jakhuwi`: numerico
- `aru`: texto
- `chiqa`: booleano
- `t'aqa`: lista
- `mapa`: mapa

Ejemplo:

```aym
yatiya jakhuwi edad = 21;
yatiya aru suti = "Ana";
yatiya chiqa activo = chiqa;
```

Booleanos canonicos:

- `chiqa`
- `k'ari`

## 5. Control de flujo

### 5.1 Condicionales

```aym
ukaxa (edad >= 18) {
  qillqa("jach'a");
} maysatxa {
  qillqa("jisk'a");
}
```

### 5.2 Bucle while

```aym
ukhakamaxa (edad > 0) {
  edad = edad - 1;
}
```

### 5.3 Bucle for

```aym
kuti (yatiya jakhuwi i = 0; i < 3; i = i + 1) {
  qillqa(i);
}
```

## 6. Funciones

Definicion de funcion:

```aym
lurawi suma(jakhuwi a, jakhuwi b): jakhuwi {
  kuttaya a + b;
}
```

Uso:

```aym
qillqa(suma(2, 3));
```

## 7. Modulos

Importacion:

```aym
apnaq("modules/aritmetica");
qillqa(suma(3, 4));
```

La resolucion de modulos busca primero junto al archivo fuente, luego en
`modules/` y luego en `AYM_PATH`.

## 8. Enumeraciones y seleccion

```aym
siqicha Estado { Qalltata, Tukuyata }

khiti(1) {
  kuna 0:
    qillqa("ch'usa");
  kuna 1, 2:
    qillqa("manta");
  yaqha:
    qillqa("jani uñt'ata");
}
```

## 9. Clases

```aym
kasta Animal {
  lurawi aru(): aru {
    kuttaya "base";
  }
}

kasta Perro jila Animal {
  lurawi aru(): aru {
    kuttaya jilaaka.aru() + "+perro";
  }
}
```

Puntos importantes:

- `kasta` define una clase
- `machaqa` crea una instancia
- `aka` referencia el objeto actual
- `jila` declara herencia
- `jilaaka` llama al padre
- la sobrescritura de metodos heredados es implicita

## 10. Manejo de errores

```aym
yant'aña {
  pantja("Fallo");
} katjaña (e) {
  qillqa(e);
} tukuyawi {
  qillqa("listo");
}
```

## 11. Uso de la CLI `aymc`

Forma general:

```bash
aymc [opciones] archivo.aym
```

Opciones mas utiles:

- `--check`: valida sin generar binario
- `-o <ruta>`: define la salida
- `--backend native|ir`: selecciona backend
- `--emit-asm`: conserva ASM intermedio
- `--compile-only`: genera ASM u objeto sin enlazar
- `--link-only`: enlaza un objeto existente
- `--diagnostics-json`: exporta diagnosticos
- `--emit-ast-json`: exporta AST
- `--time-pipeline-json`: exporta tiempos de pipeline
- `--windows` o `--linux`: fija la plataforma objetivo

Ejemplos:

```bash
aymc programa.aym
aymc --check programa.aym
aymc --emit-asm -o build/app programa.aym
aymc --diagnostics-json programa.aym
```

## 12. Uso del gestor `aym`

Crear proyecto:

```bash
aym new demo
```

Compilar:

```bash
aym build
```

Ejecutar:

```bash
aym run
```

Probar:

```bash
aym test
```

Dependencias:

```bash
aym add math ^1.2.0
aym lock check
aym lock sync
```

## 13. Referencia rapida

Palabras clave mas usadas:

- `qallta`, `tukuya`
- `yatiya`
- `qillqa`
- `ukaxa`, `maysatxa`
- `ukhakamaxa`, `kuti`
- `lurawi`, `kuttaya`
- `apnaq`
- `siqicha`, `khiti`, `kuna`, `yaqha`
- `kasta`, `machaqa`, `aka`, `jila`, `jilaaka`
- `yant'aña`, `katjaña`, `tukuyawi`, `pantja`

Operadores:

- aritmeticos: `+ - * / % ^`
- comparacion: `== != < <= > >=`
- logicos: `&& || !`

Comentarios:

```aym
// linea
/* bloque */
```

## 14. Problemas frecuentes

- `nasm` no encontrado en Linux: instala dependencias y abre una terminal nueva
- `gcc` no encontrado en Linux: valida `gcc --version`
- GUI no abre en Linux: verifica X11, `DISPLAY` o usa `xvfb-run`
- instalador Windows sin toolchain: reinstala desde el release oficial
- desalineacion de `aym.toml` y `aym.lock`: ejecuta `aym lock sync`

## 15. Ruta recomendada de aprendizaje

1. instalar el lenguaje
2. compilar un hola mundo
3. practicar variables, condicionales y bucles
4. crear funciones y modulos
5. pasar a proyectos con `aym`
6. usar la referencia rapida y la gramatica cuando necesites detalle

## 16. Documentos del repositorio

Si quieres ampliar el estudio, las fuentes principales siguen siendo:

- `docs/install.md`
- `docs/language.md`
- `docs/manual_usuario.md`
- `docs/compiler.md`
- `docs/aymaraLang.md`
- `docs/grammar.md`

Fin del manual.
