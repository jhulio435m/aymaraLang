# Guía de características

Esta guía agrupa las funcionalidades más usadas de AymaraLang con ejemplos.

## Variables string
```aymara
yatiya aru saludo = "Kamisaraki";
qillqa(saludo);
```

## Operaciones aritméticas
Soporta `%` (módulo) y `^` (potencia de enteros).
```aymara
qillqa(5 % 2);
qillqa(2 ^ 3);
```

## Condicionales encadenados
```aymara
yatiya jakhüwi valor = 2;
jisa (valor == 1) {
    qillqa("uno");
} maysatxa {
    jisa (valor == 2) {
        qillqa("dos");
    } maysatxa {
        qillqa("otro");
    }
}
```

## Operadores lógicos
Se admiten `&&`, `||` y `!`.
```aymara
jisa (1 && !0) {
    qillqa("ok");
}
```

## Comentarios
Usa `//` para comentarios de línea y `/* ... */` para bloques.

## Lectura con `katu()`
```aymara
yatiya aru nombre = katu("Suti?: ", tuku="");
qillqa("Suti =", nombre);
yatiya jakhüwi numero = jakhüwi(katu("Numero?: ", tuku=""));
qillqa(numero);
```

## Comparaciones
```aymara
jisa (5 >= 3) {
    qillqa("mayor");
}
```

## Módulos (`apnaq`)

Desde ahora es posible dividir el código en varios archivos y reutilizarlo con la
 declaración `apnaq`.

```aymara
apnaq("modules/aritmetica");

yatiya jakhüwi total = suma(3, 4);
qillqa(total);
```

Coloca el archivo `modules/aritmetica.aym` junto al programa o dentro de una
carpeta `modules/`. El resolvedor busca módulos en:

- El directorio del archivo principal.
- Una carpeta `modules/` dentro de ese directorio.
- Rutas adicionales indicadas en la variable de entorno `AYM_PATH`.

Cada módulo se procesa una sola vez y puede importar a su vez otros módulos con
`apnaq`.

## Listas (`t'aqa`)

```aymara
yatiya t'aqa numeros = [1, 2, 3];
push(numeros, 4);
qillqa(numeros);
qillqa(largo(numeros));
```

## Funciones matemáticas

```aymara
yatiya jakhüwi ang = 1;
qillqa(sin(ang));
qillqa(sqrt(9));
```

## Manejo de errores

El lenguaje permite usar `Yant'aña`/`Katjaña`/`Tukuyawi` y `Pantja` para
controlar errores. Revisa la guía completa con ejemplos y salidas en
[Manejo de errores](exceptions.md).

## Nota sobre palabras clave legacy

El compilador sigue aceptando `suti`/`jani`, `kunawsati`, `sapüru` y
`utji`/`janiutji`. Los ejemplos de esta guía usan la sintaxis actual con
`jisa`/`maysatxa`, `ukhakamaxa`, `taki` y `chiqa`/`k'ari`.

---

**Anterior:** [Primeros pasos](language.md) | **Siguiente:** [Referencia del lenguaje](aymaraLang.md)
