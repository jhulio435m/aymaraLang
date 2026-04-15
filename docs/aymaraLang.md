# Referencia rápida del lenguaje

Hoja de consulta breve. Para el detalle completo revisa [Gramática](grammar.md)
o el [Manual de usuario](manual_usuario.md).

## Tipos y literales

- `jakhüwi`: numérico
- `aru`: texto
- `chiqa`: booleano
- `t'aqa`: listas
- `mapa`: mapas
- Booleanos: `chiqa` / `k'ari`
- Números: decimal, `0x`, `0b`
- Cadenas: comillas simples o dobles con escapes

## Estructuras del lenguaje

- Variables: `yatiya`
- Inicio/fin opcional del programa: `qallta` / `tukuya`
- Condicionales: `ukaxa` / `maysatxa`
- Bucles: `ukhakamaxa`, `kuti`
- Funciones: `lurawi`, `kuttaya`
- Enumeraciones: `siqicha`
- Selección: `khiti`, `kuna`, `yaqha`
- Excepciones: `yant'aña`, `katjaña`, `tukuyawi`, `pantja`
- Módulos: `apnaq`

## POO

- Clases: `kasta`
- Instanciación: `machaqa`
- Referencia actual: `aka`
- Herencia: `jila`
- Miembro privado: `sapa`
- Miembro estático: `sapakasta`
- Llamada a base: `jilaaka`
- La sobrescritura de métodos heredados es implícita

## Builtins más comunes

- Salida: `qillqa`
- Entrada: `katu`, `input`
- Tiempo/azar: `sleep`, `random`, `tiempo_ms`
- Longitud y tamaño: `largo`, `suyut`, `suyum`
- Texto: `ch'usa`, `jaljta`, `mayachta`, `sikta`
- Listas: `push`, `ch'ullu`, `apsu`
- Matemática: `sin`, `cos`, `tan`, `sqrt`, `pow`, `log`

## Operadores

- Aritméticos: `+ - * / % ^`
- Comparación: `== != < <= > >=`
- Lógicos: `&& || !`
- Ternario: `cond ? a : b`

## Ejemplo mínimo

```aym
qallta
yatiya jakhüwi n = 3;
ukaxa (n > 0) {
  qillqa("waliki");
}
tukuya
```
