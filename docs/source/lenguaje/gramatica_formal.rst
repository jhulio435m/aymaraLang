Gramática de AymaraLang
=======================

Este documento describe la gramática actual del lenguaje AymaraLang
utilizando notación EBNF y relaciona cada producción con la
implementación en el parser LL(1) del proyecto. También se enumeran los
tokens reconocidos por el analizador léxico junto con las referencias al
código fuente donde se definen.

Tokens léxicos
--------------

Las siguientes tablas resumen cada ``TokenType`` expuesto por el
compilador y el lexema que lo activa. Todas las reglas se encuentran
implementadas en
`compiler/lexer/lexer.cpp <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp>`_.

Palabras clave
~~~~~~~~~~~~~~

+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| Token               | Lexema(s) aceptado(s)     | Uso sintáctico     | Referencia                                                                                                      |
+=====================+===========================+====================+=================================================================================================================+
| ``KeywordPrint``    | ``willt’aña``             | Sentencia de       | `lexer.cpp#L49-L50 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L49-L50>`__     |
|                     |                           | impresión          |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordIf``       | ``si``                    | Condicional ``if`` | `lexer.cpp#L51-L52 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L51-L52>`__     |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordElse``     | ``sino``                  | Rama ``else`` /    | `lexer.cpp#L53-L54 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L53-L54>`__     |
|                     |                           | ``else if``        |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordWhile``    | ``mientras``              | Bucle ``while``    | `lexer.cpp#L55-L56 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L55-L56>`__     |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordDo``       | ``haceña``                | Bucle              | `lexer.cpp#L57-L58 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L57-L58>`__     |
|                     |                           | ``do … while``     |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordFor``      | ``para``                  | Bucle ``for``      | `lexer.cpp#L59-L60 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L59-L60>`__     |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordIn``       | ``en``                    | Azúcar de          | `lexer.cpp#L61-L62 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L61-L62>`__     |
|                     |                           | ``for … in range`` |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordBreak``    | ``jalña``                 | Sentencia          | `lexer.cpp#L63-L64 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L63-L64>`__     |
|                     |                           | ``break``          |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordContinue`` | ``sarantaña``             | Sentencia          | `lexer.cpp#L65-L66 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L65-L66>`__     |
|                     |                           | ``continue``       |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordFunc``     | ``luräwi``                | Definición de      | `lexer.cpp#L67-L68 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L67-L68>`__     |
|                     |                           | funciones          |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordReturn``   | ``kutiyana``              | Sentencia          | `lexer.cpp#L69-L70 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L69-L70>`__     |
|                     |                           | ``return``         |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordSwitch``   | ``tantachaña``            | Sentencia          | `lexer.cpp#L71-L72 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L71-L72>`__     |
|                     |                           | ``switch``         |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordCase``     | ``jamusa``                | Cláusulas ``case`` | `lexer.cpp#L73-L74 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L73-L74>`__     |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordDefault``  | ``akhamawa``              | Cláusula           | `lexer.cpp#L75-L76 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L75-L76>`__     |
|                     |                           | ``default``        |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordAnd``      | ``uka``                   | Operador lógico    | `lexer.cpp#L77-L78 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L77-L78>`__     |
|                     |                           | AND                |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordOr``       | ``jan uka``               | Operador lógico OR | `lexer.cpp#L79-L100 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L79-L100>`__   |
|                     |                           | compuesto          |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordNot``      | ``janiwa``                | Operador lógico    | `lexer.cpp#L103-L104 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L103-L104>`__ |
|                     |                           | NOT                |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordInt``      | ``jach’a``                | Declaración de     | `lexer.cpp#L105-L106 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L105-L106>`__ |
|                     |                           | enteros            |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordFloat``    | ``lliphiphi``             | Declaración de     | `lexer.cpp#L107-L108 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L107-L108>`__ |
|                     |                           | flotantes          |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordBool``     | ``chuymani``              | Declaración de     | `lexer.cpp#L109-L110 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L109-L110>`__ |
|                     |                           | booleanos          |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordString``   | ``qillqa``                | Declaración de     | `lexer.cpp#L111-L112 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L111-L112>`__ |
|                     |                           | cadenas            |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``KeywordImport``   | ``apu``                   | Importación de     | `lexer.cpp#L113-L114 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L113-L114>`__ |
|                     |                           | módulos            |                                                                                                                 |
+---------------------+---------------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+

Operadores y delimitadores
~~~~~~~~~~~~~~~~~~~~~~~~~~

+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| Token            | Lexema             | Referencia                                                                                                      |
+==================+====================+=================================================================================================================+
| ``Plus``         | ``+``              | `lexer.cpp#L165 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L165>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Minus``        | ``-``              | `lexer.cpp#L166 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L166>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Star``         | ``*``              | `lexer.cpp#L167 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L167>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Slash``        | ``/``              | `lexer.cpp#L168 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L168>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Percent``      | ``%``              | `lexer.cpp#L169 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L169>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Caret``        | ``^``              | `lexer.cpp#L170 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L170>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``AmpAmp``       | ``&&``             | `lexer.cpp#L171-L175 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L171-L175>`__ |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``PipePipe``     | ``||``             | `lexer.cpp#L179-L183 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L179-L183>`__ |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Bang``         | ``!``              | `lexer.cpp#L187-L194 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L187-L194>`__ |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Equal``        | ``=``              | `lexer.cpp#L196-L203 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L196-L203>`__ |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``EqualEqual``   | ``==``             | `lexer.cpp#L196-L203 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L196-L203>`__ |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``BangEqual``    | ``!=``             | `lexer.cpp#L187-L194 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L187-L194>`__ |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Less``         | ``<``              | `lexer.cpp#L205-L212 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L205-L212>`__ |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``LessEqual``    | ``<=``             | `lexer.cpp#L205-L212 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L205-L212>`__ |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Greater``      | ``>``              | `lexer.cpp#L214-L221 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L214-L221>`__ |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``GreaterEqual`` | ``>=``             | `lexer.cpp#L214-L221 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L214-L221>`__ |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``LParen``       | ``(``              | `lexer.cpp#L223 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L223>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``RParen``       | ``)``              | `lexer.cpp#L224 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L224>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``LBrace``       | ``{``              | `lexer.cpp#L225 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L225>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``RBrace``       | ``}``              | `lexer.cpp#L226 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L226>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``LBracket``     | ``[``              | `lexer.cpp#L227 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L227>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``RBracket``     | ``]``              | `lexer.cpp#L228 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L228>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Colon``        | ``:``              | `lexer.cpp#L229 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L229>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Comma``        | ``,``              | `lexer.cpp#L230 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L230>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Semicolon``    | ``;``              | `lexer.cpp#L231 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L231>`__           |
+------------------+--------------------+-----------------------------------------------------------------------------------------------------------------+

Identificadores, literales y control
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

+----------------+----------------------------+-----------------------------------------------------------------------------------------------------------------+
| Token          | Descripción                | Referencia                                                                                                      |
+================+============================+=================================================================================================================+
| ``Identifier`` | Secuencia alfanumérica     | `lexer.cpp#L39-L47 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L39-L47>`__     |
|                | (permite UTF-8 y ``_``)    |                                                                                                                 |
|                | que no coincide con        |                                                                                                                 |
|                | palabra reservada          |                                                                                                                 |
+----------------+----------------------------+-----------------------------------------------------------------------------------------------------------------+
| ``Number``     | Literales numéricos        | `lexer.cpp#L79-L129 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L79-L129>`__   |
|                | decimales, además de los   |                                                                                                                 |
|                | azúcares ``cheka`` (=1) y  |                                                                                                                 |
|                | ``jan cheka`` (=0)         |                                                                                                                 |
+----------------+----------------------------+-----------------------------------------------------------------------------------------------------------------+
| ``String``     | Literales de cadena entre  | `lexer.cpp#L132-L161 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L132-L161>`__ |
|                | comillas dobles con        |                                                                                                                 |
|                | escapes (``\n``, ``\t``,   |                                                                                                                 |
|                | ``\"``, etc.)              |                                                                                                                 |
+----------------+----------------------------+-----------------------------------------------------------------------------------------------------------------+
| ``EndOfFile``  | Marcador al final de la    | `lexer.cpp#L236-L237 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L236-L237>`__ |
|                | tokenización               |                                                                                                                 |
+----------------+----------------------------+-----------------------------------------------------------------------------------------------------------------+

..

   **Nota:** Los tokens ``LBracket`` y ``RBracket`` aún no se emplean en
   producciones del parser, pero quedan reservados para futuras
   extensiones (p.ej. arreglos).

Notación EBNF utilizada
-----------------------

- ``a*`` indica repetición de cero o más veces.
- ``a+`` indica una o más repeticiones.
- ``[a]`` marca un elemento opcional.
- ``a | b`` representa alternativas.
- Los literales de palabra clave se escriben con comillas
  (``"lexema"``).

Gramática sintáctica
--------------------

Programa y bloques
~~~~~~~~~~~~~~~~~~

.. code:: ebnf

   program        ::= statement* EOF ;
   block          ::= "{" statement* "}" ;

Estas producciones corresponden a
`Parser::parse <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L8-L14>`_
y
`Parser::parseStatements <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L41-L46>`_,
que reutilizan la misma rutina para cuerpos delimitados por llaves
(``block``). Cuando ``parseStatements`` se invoca con
``stopAtBrace = true`` consume el ``RBrace`` final.

Sentencias
~~~~~~~~~~

Cada alternativa de ``statement`` proviene de
`Parser::parseSingleStatement <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L48-L318>`_.
A continuación se detalla la sintaxis concreta de cada forma:

.. code:: ebnf

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

Importaciones
^^^^^^^^^^^^^

.. code:: ebnf

   importStmt     ::= "apu" (Identifier | String) ";" ;

Implementado en
`parseSingleStatement <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L49-L65>`_.

Declaraciones de variables
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: ebnf

   varDecl        ::= typeKeyword Identifier ["=" expression] ";" ;
   typeKeyword    ::= "jach’a" | "lliphiphi" | "chuymani" | "qillqa" ;

Corresponde a
`parseSingleStatement <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L67-L81>`_.

Sentencias de retorno y control de bucle
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: ebnf

   returnStmt     ::= "kutiyana" [expression] ";" ;
   breakStmt      ::= "jalña" ";" ;
   continueStmt   ::= "sarantaña" ";" ;

Ver
`parser.cpp#L83-L107 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L83-L107>`_.

Impresión
^^^^^^^^^

.. code:: ebnf

   printStmt      ::= "willt’aña" "(" expression ")" ";" ;

Implementado en
`parser.cpp#L109-L118 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L109-L118>`_.

Condicionales
^^^^^^^^^^^^^

.. code:: ebnf

   ifStmt         ::= "si" "(" expression ")" block [elseClause] ;
   elseClause     ::= "sino" (block | ifStmt) ;

Refleja
`parser.cpp#L120-L151 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L120-L151>`_,
que anida ``if`` sucesivos para soportar ``sino si``.

Bucles ``while`` y ``do … while``
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: ebnf

   whileStmt      ::= "mientras" "(" expression ")" block ;
   doWhileStmt    ::= "haceña" block "mientras" "(" expression ")" ";" ;

Implementación en
`parser.cpp#L153-L184 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L153-L184>`_.

Bucles ``for``
^^^^^^^^^^^^^^

.. code:: ebnf

   forRangeStmt   ::= "para" Identifier "en" "range" "(" expression "," expression ")" block ;
   forClassicStmt ::= "para" "(" statement expression ";" statement ")" block ;

Ambas variantes viven en
`parser.cpp#L186-L233 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L186-L233>`_.
La forma clásica reutiliza ``statement`` para las secciones de
inicialización y post-procesamiento, lo que implica que dichas porciones
incluyen su propio ``";"`` antes de continuar con el resto del
encabezado.

Sentencia ``switch``
^^^^^^^^^^^^^^^^^^^^

.. code:: ebnf

   switchStmt     ::= "tantachaña" "(" expression ")" "{" caseClause* [defaultClause] "}" ;
   caseClause     ::= "jamusa" expression ":" statement* ;
   defaultClause  ::= "akhamawa" ":" statement* ;

Implementación en
`parser.cpp#L235-L273 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L235-L273>`_.

Definiciones de funciones
^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: ebnf

   functionStmt   ::= "luräwi" [Identifier] "(" parameterList? ")" block ;
   parameterList  ::= Identifier ("," Identifier)* ;

Definido en
`parser.cpp#L276-L295 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L276-L295>`_.
El nombre es opcional y los parámetros se reconocen como identificadores
desnudos.

Asignaciones y expresiones
^^^^^^^^^^^^^^^^^^^^^^^^^^

.. code:: ebnf

   assignmentStmt ::= Identifier "=" expression ";" ;
   exprStmt       ::= expression ";" ;

Generadas por
`parser.cpp#L297-L318 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L297-L318>`_.
Cuando un identificador no está seguido de ``=``, la sentencia se
degrada a una expresión pura.

Expresiones
~~~~~~~~~~~

La jerarquía de expresiones está factorada en métodos dedicados dentro
de ``Parser``. Todas las producciones terminan referenciando
`parseFactor <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L465-L520>`_
y
`parseArguments <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L522-L529>`_.

.. code:: ebnf

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

- ``logicalExpr`` y sus operadores se derivan de
  `parseLogic <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L325-L345>`_.
- ``equalityExpr`` proviene de
  `parseEquality <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L347-L367>`_.
- ``comparisonExpr`` está en
  `parseComparison <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L369-L401>`_.
- ``additiveExpr``, ``multiplicativeExpr`` y ``powerExpr`` se
  corresponden con
  `parseAdd <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L403-L423>`_,
  `parseTerm <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L425-L451>`_
  y
  `parsePower <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L453-L463>`_
  respectivamente.
- ``unaryExpr`` y ``primaryExpr`` se implementan en
  `parseFactor <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L465-L520>`_.
- ``argumentList`` reusa
  `parseArguments <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/parser/parser.cpp#L522-L529>`_.

Los literales ``Number`` incluyen tanto cifras decimales como los
azúcares ``cheka`` y ``jan cheka`` definidos en el lexer. Las llamadas a
funciones requieren que el calificador sea un ``Identifier``; las
palabras reservadas no pueden emplearse como nombres válidos.

Tokens finales
~~~~~~~~~~~~~~

El flujo sintáctico concluye cuando se consume el token ``EndOfFile``,
añadido explícitamente por el lexer en
`lexer.cpp#L236-L237 <https://github.com/aymaraLang/aymaraLang/blob/main/compiler/lexer/lexer.cpp#L236-L237>`_
y verificado por el parser durante la iteración principal.

--------------

Este documento debe mantenerse sincronizado con el código fuente para
reflejar cualquier cambio en la gramática del lenguaje.

--------------

**Anterior:** :doc:`Referencia del lenguaje <lenguaje/referencia_del_lenguaje>` \|
**Siguiente:** :doc:`Arquitectura del compilador <arquitectura/vision_general_del_sistema>`
