# Primeros pasos con AymaraLang

Esta sección introduce la sintaxis básica del lenguaje con ejemplos cortos.
Si buscas detalles exhaustivos de operadores y funciones integradas, consulta la
[Referencia del lenguaje](aymaraLang.md).

## Hola mundo

```aymara
willt’aña("Kamisaraki!");
```

## Tipos y variables

```aymara
jach’a contador = 3;
lliphiphi promedio = 3.14;
qillqa saludo = "kamisaraki";
chuymani activo = cheka;
```

## Condicionales

```aymara
si (contador > 0) {
    willt’aña(saludo);
} sino {
    willt’aña("janiwa");
}
```

## Bucles

```aymara
para i en range(0, 3) {
    willt’aña(i);
}

mientras (contador > 0) {
    contador = contador - 1;
}
```

## Funciones

```aymara
luräwi inc(n) {
    kutiyana n + 1;
}

willt’aña(inc(5));
```

## Módulos

```aymara
apu "modules/aritmetica";

willt’aña("suma: " + suma(3, 4));
```

Los módulos se resuelven desde el directorio del archivo principal, desde una
carpeta `modules/` y desde rutas adicionales definidas en la variable de entorno
`AYM_PATH`.

## Comentarios

```aymara
# comentario estilo aym
// comentario estilo C
/* bloque de comentario */
```

---

**Anterior:** [Visión general](overview.md) | **Siguiente:** [Guía de características](guide.md)
