# Gramática de AymaraLang (AruQillqa)

Este documento describe la gramática actual del lenguaje AymaraLang utilizando
notación EBNF y resume los tokens reconocidos por el compilador. Las reglas de
tokenización están implementadas en `compiler/lexer/lexer.cpp`.

## Tokens léxicos

### Palabras clave

| Token | Lexema(s) aceptado(s) | Uso sintáctico |
|-------|------------------------|----------------|
| `KeywordStart` | `qallta` | Inicio del programa |
| `KeywordEnd` | `tukuya` | Fin del programa |
| `KeywordDeclare` | `yatiya` | Declaración de variables |
| `KeywordPrint` | `qillqa` | Sentencia de impresión |
| `KeywordIf` | `suti` | Condicional |
| `KeywordElse` | `jani` | Rama alternativa |
| `KeywordWhile` | `kunawsati` | Bucle `while` |
| `KeywordFor` | `sapüru` | Bucle `for` |
| `KeywordFunc` | `lurawi` | Definición de funciones |
| `KeywordReturn` | `kuttaya` | Retorno |
| `KeywordImport` | `apnaq` | Importación |
| `KeywordTypeNumber` | `jakhüwi` | Tipo numérico |
| `KeywordTypeString` | `aru` | Tipo cadena |
| `KeywordTypeBool` | `chiqa` | Tipo booleano |
| `KeywordTypeList` | `listaña` | Tipo lista |
| `KeywordTypeMap` | `mapa` | Tipo mapa (Español) |
| `KeywordTrue` | `utji` | Literal verdadero |
| `KeywordFalse` | `janiutji` | Literal falso |

### Operadores y delimitadores

Los símbolos se mantienen sin cambios:

- Agrupación: `()`, `{}`, `[]`
- Fin de sentencia: `;`
- Separador: `,`
- Asignación: `=`
- Dos puntos: `:`
- Operadores: `+ - * / % ^`
- Comparación: `== != < <= > >=`
- Lógicos: `&& || !`
- Comentarios: `//` y `/* */`

### Literales

- `Number`: números enteros decimales con soporte `0x` y `0b`.
- `String`: cadenas entre comillas dobles o simples con escapes (`\n`, `\t`, `\"`, `\'`, etc.).

## Notación EBNF utilizada

- `a*` indica repetición de cero o más veces.
- `a+` indica una o más repeticiones.
- `[a]` marca un elemento opcional.
- `a | b` representa alternativas.

## Gramática sintáctica

```ebnf
programa      = "qallta" { sentencia } "tukuya" ;

sentencia     = decl | asigna | if | while | for | func_def
              | retorno | imprimir | importar | bloque | ";" ;

bloque        = "{" { sentencia } "}" ;

decl          = "yatiya" tipo id [ "=" expr ] ";" ;

tipo          = "jakhüwi" | "aru" | "chiqa" | "listaña" | "mapa" ;

asigna        = id "=" expr ";" ;

if            = "suti" "(" cond ")" bloque [ "jani" bloque ] ;

while         = "kunawsati" "(" cond ")" bloque ;

for           = "sapüru" "(" [decl|asigna] ";" [cond] ";" [asigna_np] ")" bloque ;
asigna_np     = id "=" expr ;

func_def      = "lurawi" id "(" [ params ] ")" [ ":" tipo ] bloque ;
params        = param { "," param } ;
param         = tipo id ;

retorno       = "kuttaya" [ expr ] ";" ;

imprimir      = "qillqa" "(" [ args ] ")" ";" ;
importar      = "apnaq" "(" str ")" ";" ;

args          = expr { "," expr } ;

cond          = expr ;

expr          = or ;

or            = and { "||" and } ;
and           = igualdad { "&&" igualdad } ;

igualdad      = rel { ("==" | "!=") rel } ;
rel           = suma { ("<" | "<=" | ">" | ">=") suma } ;
suma          = multi { ("+" | "-") multi } ;
multi         = unario { ("*" | "/" | "%") unario } ;

unario        = [ "!" | "-" ] primario ;

primario      = num | str | "utji" | "janiutji"
              | id | llamada | "(" expr ")" | lista ;

llamada       = id "(" [ args ] ")" ;

lista         = "[" [ args ] "]" ;

id            = letra { letra | num | "_" } ;
num           = digito { digito } ;
str           = '\"' { caracter } '\"' | \"'\" { caracter } \"'\" ;
```

---

**Anterior:** [Referencia del lenguaje](aymaraLang.md) | **Siguiente:** [Arquitectura del compilador](arquitectura.md)
