Guía de características
=======================

Esta guía agrupa las funcionalidades más usadas de AymaraLang con
ejemplos.

Variables string
----------------

.. code:: aymara

   qillqa saludo = "Kamisaraki";
   willt’aña(saludo);

Operaciones aritméticas
-----------------------

Soporta ``%`` (módulo) y ``^`` (potencia de enteros).

.. code:: aymara

   willt’aña(5 % 2);
   willt’aña(2 ^ 3);

Switch-Case
-----------

.. code:: aymara

   valor = 2;
   tantachaña(valor) {
       jamusa 1 {
           willt’aña("uno");
       }
       jamusa 2 {
           willt’aña("dos");
       }
       akhamawa {
           willt’aña("otro");
       }
   }

Operadores lógicos
------------------

Se admiten ``uka``, ``jan uka`` y ``janiwa``.

.. code:: aymara

   si (1 uka janiwa 0) {
       willt’aña("ok");
   }

Comentarios
-----------

Usa ``//`` para comentarios de línea y ``/* ... */`` para bloques.

Lectura con ``input()``
-----------------------

.. code:: aymara

   jach’a numero = input();
   willt’aña(numero);

Comparaciones
-------------

.. code:: aymara

   si (5 >= 3) {
       willt’aña("mayor");
   }

Módulos (``apu``)
-----------------

Desde ahora es posible dividir el código en varios archivos y
reutilizarlo con la declaración ``apu``.

.. code:: aymara

   apu "modules/aritmetica";

   jach’a total = suma(3, 4);
   willt’aña(total);

Coloca el archivo ``modules/aritmetica.aym`` junto al programa o dentro
de una carpeta ``modules/``. El resolvedor busca módulos en:

- El directorio del archivo principal.
- Una carpeta ``modules/`` dentro de ese directorio.
- Rutas adicionales indicadas en la variable de entorno ``AYM_PATH``.

Cada módulo se procesa una sola vez y puede importar a su vez otros
módulos con ``apu``.

Arreglos dinámicos
------------------

.. code:: aymara

   jach’a n = 5;
   jach’a arr = array(n);
   array_set(arr, 0, 10);
   willt’aña(array_get(arr, 0));
   willt’aña(array_length(arr));
   array_free(arr);

Funciones matemáticas
---------------------

.. code:: aymara

   lliphiphi ang = 1.57;
   willt’aña(sin(ang));
   willt’aña(sqrt(9));

--------------

**Anterior:** :doc:`Primeros pasos <primeros_pasos/introduccion_al_lenguaje>` \| **Siguiente:**
:doc:`Referencia del lenguaje <lenguaje/referencia_del_lenguaje>`
