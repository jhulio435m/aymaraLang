# Gramática de AymaraLang

Este documento describe la gramática actual del lenguaje AymaraLang utilizando notación EBNF y relaciona cada producción con la implementación en el parser LL(1) del proyecto. También se enumeran los tokens reconocidos por el analizador léxico junto con las referencias al código fuente donde se definen.

## Tokens léxicos

Las siguientes tablas resumen cada `TokenType` expuesto por el compilador y el lexema que lo activa. Todas las reglas se encuentran implementadas en [`compiler/lexer/lexer.cpp`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp).

### Palabras clave

| Token | Lexema(s) aceptado(s) | Uso sintáctico | Referencia |
|-------|-----------------------|----------------|------------|
| `KeywordPrint` | `willt’aña` | Sentencia de impresión | [lexer.cpp#L49-L50](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L49-L50) |
| `KeywordIf` | `si` | Condicional `if` | [lexer.cpp#L51-L52](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L51-L52) |
| `KeywordElse` | `sino` | Rama `else` / `else if` | [lexer.cpp#L53-L54](https://github.comjhulio435mg/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L53-L54) |
| `KeywordWhile` | `mientras` | Bucle `while` | [lexer.cpp#L55-L56](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L55-L56) |
| `KeywordDo` | `haceña` | Bucle `do … while` | [lexer.cpp#L57-L58](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L57-L58) |
| `KeywordFor` | `para` | Bucle `for` | [lexer.cpp#L59-L60](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L59-L60) |
| `KeywordIn` | `en` | Azúcar de `for … in range` | [lexer.cpp#L61-L62](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L61-L62) |
| `KeywordBreak` | `jalña` | Sentencia `break` | [lexer.cpp#L63-L64](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L63-L64) |
| `KeywordContinue` | `sarantaña` | Sentencia `continue` | [lexer.cpp#L65-L66](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L65-L66) |
| `KeywordFunc` | `luräwi` | Definición de funciones | [lexer.cpp#L67-L68](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L67-L68) |
| `KeywordReturn` | `kutiyana` | Sentencia `return` | [lexer.cpp#L69-L70](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L69-L70) |
| `KeywordSwitch` | `tantachaña` | Sentencia `switch` | [lexer.cpp#L71-L72](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L71-L72) |
| `KeywordCase` | `jamusa` | Cláusulas `case` | [lexer.cpp#L73-L74](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L73-L74) |
| `KeywordDefault` | `akhamawa` | Cláusula `default` | [lexer.cpp#L75-L76](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L75-L76) |
| `KeywordAnd` | `uka` | Operador lógico AND | [lexer.cpp#L77-L78](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L77-L78) |
| `KeywordOr` | `jan uka` | Operador lógico OR compuesto | [lexer.cpp#L79-L100](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L79-L100) |
| `KeywordNot` | `janiwa` | Operador lógico NOT | [lexer.cpp#L103-L104](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L103-L104) |
| `KeywordInt` | `jach’a` | Declaración de enteros | [lexer.cpp#L105-L106](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L105-L106) |
| `KeywordFloat` | `lliphiphi` | Declaración de flotantes | [lexer.cpp#L107-L108](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L107-L108) |
| `KeywordBool` | `chuymani` | Declaración de booleanos | [lexer.cpp#L109-L110](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L109-L110) |
| `KeywordString` | `qillqa` | Declaración de cadenas | [lexer.cpp#L111-L112](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L111-L112) |
| `KeywordImport` | `apu` | Importación de módulos | [lexer.cpp#L113-L114](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L113-L114) |

### Operadores y delimitadores

| Token | Lexema | Referencia |
|-------|--------|------------|
| `Plus` | `+` | [lexer.cpp#L165](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L165) |
| `Minus` | `-` | [lexer.cpp#L166](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L166) |
| `Star` | `*` | [lexer.cpp#L167](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L167) |
| `Slash` | `/` | [lexer.cpp#L168](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L168) |
| `Percent` | `%` | [lexer.cpp#L169](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L169) |
| `Caret` | `^` | [lexer.cpp#L170](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L170) |
| `AmpAmp` | `&&` | [lexer.cpp#L171-L175](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L171-L175) |
| `PipePipe` | `||` | [lexer.cpp#L179-L183](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L179-L183) |
| `Bang` | `!` | [lexer.cpp#L187-L194](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L187-L194) |
| `Equal` | `=` | [lexer.cpp#L196-L203](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L196-L203) |
| `EqualEqual` | `==` | [lexer.cpp#L196-L203](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L196-L203) |
| `BangEqual` | `!=` | [lexer.cpp#L187-L194](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L187-L194) |
| `Less` | `<` | [lexer.cpp#L205-L212](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L205-L212) |
| `LessEqual` | `<=` | [lexer.cpp#L205-L212](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L205-L212) |
| `Greater` | `>` | [lexer.cpp#L214-L221](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L214-L221) |
| `GreaterEqual` | `>=` | [lexer.cpp#L214-L221](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L214-L221) |
| `LParen` | `(` | [lexer.cpp#L223](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L223) |
| `RParen` | `)` | [lexer.cpp#L224](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L224) |
| `LBrace` | `{` | [lexer.cpp#L225](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L225) |
| `RBrace` | `}` | [lexer.cpp#L226](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L226) |
| `LBracket` | `[` | [lexer.cpp#L227](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L227) |
| `RBracket` | `]` | [lexer.cpp#L228](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L228) |
| `Colon` | `:` | [lexer.cpp#L229](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L229) |
| `Comma` | `,` | [lexer.cpp#L230](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L230) |
| `Semicolon` | `;` | [lexer.cpp#L231](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L231) |

### Identificadores, literales y control

| Token | Descripción | Referencia |
|-------|-------------|------------|
| `Identifier` | Secuencia alfanumérica (permite UTF-8 y `_`) que no coincide con palabra reservada | [lexer.cpp#L39-L47](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L39-L47) |
| `Number` | Literales numéricos decimales y prefijos `0x` (hexadecimal), `0b` (binario), además de los azúcares `cheka` (=1) y `jan cheka` (=0) | [lexer.cpp#L79-L129](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L79-L129) |
| `String` | Literales de cadena entre comillas dobles con escapes (`\n`, `\t`, `\"`, etc.) | [lexer.cpp#L132-L161](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L132-L161) |
| `EndOfFile` | Marcador al final de la tokenización | [lexer.cpp#L236-L237](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L236-L237) |

> **Nota:** Los tokens `LBracket` y `RBracket` aún no se emplean en producciones del parser, pero quedan reservados para futuras extensiones (p.ej. arreglos).

## Notación EBNF utilizada

- `a*` indica repetición de cero o más veces.
- `a+` indica una o más repeticiones.
- `[a]` marca un elemento opcional.
- `a | b` representa alternativas.
- Los literales de palabra clave se escriben con comillas (`"lexema"`).

## Gramática sintáctica

### Programa y bloques

```ebnf
program        ::= statement* EOF ;
block          ::= "{" statement* "}" ;
```

Estas producciones corresponden a [`Parser::parse`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L8-L14) y [`Parser::parseStatements`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L41-L46), que reutilizan la misma rutina para cuerpos delimitados por llaves (`block`). Cuando `parseStatements` se invoca con `stopAtBrace = true` consume el `RBrace` final.

### Sentencias

Cada alternativa de `statement` proviene de [`Parser::parseSingleStatement`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L48-L318). A continuación se detalla la sintaxis concreta de cada forma:

```ebnf
statement      ::= importStmt
                 | varDecl
                 | returnStmt
                 | breakStmt
                 | continueStmt
                 | printStmt
                 | ifStmt
                 | whileStmt
                 | doWhileStmt
                 | forRangeStmt
                 | forClassicStmt
                 | switchStmt
                 | functionStmt
                 | assignmentStmt
                 | exprStmt ;
```

#### Importaciones
```ebnf
importStmt     ::= "apu" (Identifier | String) ";" ;
```
Implementado en [`parseSingleStatement`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L49-L65).

#### Declaraciones de variables
```ebnf
varDecl        ::= typeKeyword Identifier ["=" expression] ";" ;
typeKeyword    ::= "jach’a" | "lliphiphi" | "chuymani" | "qillqa" ;
```
Corresponde a [`parseSingleStatement`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L67-L81).

#### Sentencias de retorno y control de bucle
```ebnf
returnStmt     ::= "kutiyana" [expression] ";" ;
breakStmt      ::= "jalña" ";" ;
continueStmt   ::= "sarantaña" ";" ;
```
Ver [`parser.cpp#L83-L107`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L83-L107).

#### Impresión
```ebnf
printStmt      ::= "willt’aña" "(" expression ")" ";" ;
```
Implementado en [`parser.cpp#L109-L118`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L109-L118).

#### Condicionales
```ebnf
ifStmt         ::= "si" "(" expression ")" block [elseClause] ;
elseClause     ::= "sino" (block | ifStmt) ;
```
Refleja [`parser.cpp#L120-L151`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L120-L151), que anida `if` sucesivos para soportar `sino si`.

#### Bucles `while` y `do … while`
```ebnf
whileStmt      ::= "mientras" "(" expression ")" block ;
doWhileStmt    ::= "haceña" block "mientras" "(" expression ")" ";" ;
```
Implementación en [`parser.cpp#L153-L184`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L153-L184).

#### Bucles `for`
```ebnf
forRangeStmt   ::= "para" Identifier "en" "range" "(" expression "," expression ")" block ;
forClassicStmt ::= "para" "(" statement expression ";" statement ")" block ;
```
Ambas variantes viven en [`parser.cpp#L186-L233`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L186-L233). La forma clásica reutiliza `statement` para las secciones de inicialización y post-procesamiento, lo que implica que dichas porciones incluyen su propio `";"` antes de continuar con el resto del encabezado.

#### Sentencia `switch`
```ebnf
switchStmt     ::= "tantachaña" "(" expression ")" "{" caseClause* [defaultClause] "}" ;
caseClause     ::= "jamusa" expression ":" statement* ;
defaultClause  ::= "akhamawa" ":" statement* ;
```
Implementación en [`parser.cpp#L235-L273`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L235-L273).

#### Definiciones de funciones
```ebnf
functionStmt   ::= "luräwi" [Identifier] "(" parameterList? ")" block ;
parameterList  ::= Identifier ("," Identifier)* ;
```
Definido en [`parser.cpp#L276-L295`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L276-L295). El nombre es opcional y los parámetros se reconocen como identificadores desnudos.

#### Asignaciones y expresiones
```ebnf
assignmentStmt ::= Identifier "=" expression ";" ;
exprStmt       ::= expression ";" ;
```
Generadas por [`parser.cpp#L297-L318`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L297-L318). Cuando un identificador no está seguido de `=`, la sentencia se degrada a una expresión pura.

### Expresiones

La jerarquía de expresiones está factorada en métodos dedicados dentro de `Parser`. Todas las producciones terminan referenciando [`parseFactor`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L465-L520) y [`parseArguments`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L522-L529).

```ebnf
expression     ::= logicalExpr ;
logicalExpr    ::= equalityExpr { logicalOp equalityExpr } ;
logicalOp      ::= "uka" | "&&" | "jan uka" | "||" ;
equalityExpr   ::= comparisonExpr { ("==" | "!=") comparisonExpr } ;
comparisonExpr ::= additiveExpr { ("<" | "<=" | ">" | ">=") additiveExpr } ;
additiveExpr   ::= multiplicativeExpr { ("+" | "-") multiplicativeExpr } ;
multiplicativeExpr ::= powerExpr { ("*" | "/" | "%") powerExpr } ;
powerExpr      ::= unaryExpr { "^" unaryExpr } ;
unaryExpr      ::= ("-" | "+" | "janiwa" | "!") unaryExpr
                 | primaryExpr ;
primaryExpr    ::= Number
                 | String
                 | Identifier ["(" argumentList? ")"]
                 | "(" expression ")" ;
argumentList   ::= expression { "," expression } ;
```

- `logicalExpr` y sus operadores se derivan de [`parseLogic`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L325-L345).
- `equalityExpr` proviene de [`parseEquality`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L347-L367).
- `comparisonExpr` está en [`parseComparison`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L369-L401).
- `additiveExpr`, `multiplicativeExpr` y `powerExpr` se corresponden con [`parseAdd`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L403-L423), [`parseTerm`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L425-L451) y [`parsePower`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L453-L463) respectivamente.
- `unaryExpr` y `primaryExpr` se implementan en [`parseFactor`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L465-L520).
- `argumentList` reusa [`parseArguments`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/parser/parser.cpp#L522-L529).

Los literales `Number` incluyen tanto cifras decimales como los azúcares `cheka` y `jan cheka` definidos en el lexer. Las llamadas a funciones requieren que el calificador sea un `Identifier`; las palabras reservadas no pueden emplearse como nombres válidos.

### Tokens finales

El flujo sintáctico concluye cuando se consume el token `EndOfFile`, añadido explícitamente por el lexer en [`lexer.cpp#L236-L237`](https://github.com/jhulio435m/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L236-L237) y verificado por el parser durante la iteración principal.

---

Este documento debe mantenerse sincronizado con el código fuente para reflejar cualquier cambio en la gramática del lenguaje.

---

**Anterior:** [Referencia del lenguaje](aymaraLang.md) | **Siguiente:** [Arquitectura del compilador](arquitectura.md)
