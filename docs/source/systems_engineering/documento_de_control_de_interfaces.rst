Documento de Control de Interfaces (ICD)
========================================

1. Propósito
------------

Definir las interfaces principales del sistema AymaraLang y sus
contratos de uso.

2. Interfaces externas
----------------------

2.1 CLI del compilador
~~~~~~~~~~~~~~~~~~~~~~

- **Comando:** ``aymc [opciones] archivo.aym ...``
- **Opciones principales:** ``--debug``, ``--dump-ast``, ``--seed``,
  ``--llvm``, ``--windows``, ``--linux``, ``-o``.

2.2 Sistema de archivos
~~~~~~~~~~~~~~~~~~~~~~~

- **Entradas:** archivos ``.aym``.
- **Salidas:** ``.asm``, ``.ll`` y binario nativo.

2.3 Runtime
~~~~~~~~~~~

- **Interfaz:** llamadas a funciones integradas (``willt’aña``,
  ``input``, matemáticas, arreglos).

3. Interfaces internas
----------------------

================== ===============================
Interfaz           Descripción
================== ===============================
Lexer → Parser     Tokens con tipo/lexema/posición
Parser → Semantic  AST de nodos ``Expr``/``Stmt``
Semantic → Codegen AST validado + símbolos
Codegen → Linker   ``.asm`` o ``.ll``
================== ===============================

4. Reglas de interoperabilidad
------------------------------

- El parser debe consumir tokens bien formados del lexer.
- El codegen solo opera sobre AST validado.
- El runtime debe mantener compatibilidad con el ABI usado por codegen.
