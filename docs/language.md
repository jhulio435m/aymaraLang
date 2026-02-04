# Primeros pasos con AymaraLang

Esta sección introduce la sintaxis básica del lenguaje con ejemplos cortos.
Si buscas detalles exhaustivos de operadores y funciones integradas, consulta la
[Referencia del lenguaje](aymaraLang.md).

## Hola mundo

```aymara
qillqa("Kamisaraki!");
```

## Tipos y variables

```aymara
yatiya jakhüwi contador = 3;
yatiya jakhüwi promedio = 3;
yatiya aru saludo = "kamisaraki";
yatiya chiqa activo = chiqa;
```

También puedes usar literales enteros en hexadecimal o binario:

```aymara
yatiya jakhüwi mascara = 0b1010;
yatiya jakhüwi color = 0xFF;
```

## Condicionales

```aymara
jisa (contador > 0) {
    qillqa(saludo);
} maysatxa {
    qillqa("janiwa");
}
```

## Bucles

```aymara
taki (yatiya jakhüwi i = 0; i < 3; i++) {
    qillqa(i);
}

ukhakamaxa (contador > 0) {
    contador--;
}
```

## Funciones

```aymara
lurawi inc(jakhüwi n) : jakhüwi {
    kuttaya n + 1;
}

qillqa(inc(5));
```

## Módulos

```aymara
apnaq("modules/aritmetica");

qillqa("suma: " + suma(3, 4));
```

Los módulos se resuelven desde el directorio del archivo principal, desde una
carpeta `modules/` y desde rutas adicionales definidas en la variable de entorno
`AYM_PATH`.

## Comentarios

```aymara
// comentario estilo C
/* bloque de comentario */
```

## Nota sobre palabras clave legacy

El compilador mantiene compatibilidad con las formas `suti`/`jani`,
`kunawsati`, `sapüru` y `utji`/`janiutji`. La sintaxis recomendada en esta guía
usa `jisa`/`maysatxa`, `ukhakamaxa`, `taki` y `chiqa`/`k'ari`.

---

**Anterior:** [Visión general](overview.md) | **Siguiente:** [Guía de características](guide.md)
