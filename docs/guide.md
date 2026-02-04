# Guía de características

Esta guía agrupa las funcionalidades más usadas de AymaraLang con ejemplos.

## Variables string
```aymara
qallta
yatiya aru saludo = "Kamisaraki";
qillqa(saludo);
tukuya
```

## Operaciones aritméticas
Soporta `%` (módulo) y `^` (potencia de enteros).
```aymara
qallta
qillqa(5 % 2);
qillqa(2 ^ 3);
tukuya
```

## Condicionales encadenados
```aymara
qallta
yatiya jakhüwi valor = 2;
suti (valor == 1) {
    qillqa("uno");
} jani {
    suti (valor == 2) {
        qillqa("dos");
    } jani {
        qillqa("otro");
    }
}
tukuya
```

## Operadores lógicos
Se admiten `&&`, `||` y `!`.
```aymara
qallta
suti (1 && !0) {
    qillqa("ok");
}
tukuya
```

## Comentarios
Usa `//` para comentarios de línea y `/* ... */` para bloques.

## Lectura con `input()`
```aymara
qallta
yatiya jakhüwi numero = input();
qillqa(numero);
tukuya
```

## Comparaciones
```aymara
qallta
suti (5 >= 3) {
    qillqa("mayor");
}
tukuya
```

## Módulos (`apnaq`)

Desde ahora es posible dividir el código en varios archivos y reutilizarlo con la
 declaración `apnaq`.

```aymara
qallta
apnaq("modules/aritmetica");

yatiya jakhüwi total = suma(3, 4);
qillqa(total);
tukuya
```

Coloca el archivo `modules/aritmetica.aym` junto al programa o dentro de una
carpeta `modules/`. El resolvedor busca módulos en:

- El directorio del archivo principal.
- Una carpeta `modules/` dentro de ese directorio.
- Rutas adicionales indicadas en la variable de entorno `AYM_PATH`.

Cada módulo se procesa una sola vez y puede importar a su vez otros módulos con
`apnaq`.

## Arreglos dinámicos

```aymara
qallta
yatiya jakhüwi n = 5;
yatiya jakhüwi arr = array(n);
array_set(arr, 0, 10);
qillqa(array_get(arr, 0));
qillqa(array_length(arr));
array_free(arr);
tukuya
```

## Funciones matemáticas

```aymara
qallta
yatiya jakhüwi ang = 1;
qillqa(sin(ang));
qillqa(sqrt(9));
tukuya
```

---

**Anterior:** [Primeros pasos](language.md) | **Siguiente:** [Referencia del lenguaje](aymaraLang.md)
