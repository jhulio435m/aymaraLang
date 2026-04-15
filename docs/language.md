# Tutorial rápido

Esta página te lleva por el flujo mínimo: escribir, compilar y ejecutar un
programa sin entrar todavía en detalles del compilador.

## 1. Hola mundo

```aym
qallta
qillqa("Kamisaraki!")
tukuya
```

Guárdalo como `hola.aym` y compílalo así:

```bash
aymc hola.aym
```

## 2. Variables y tipos

```aym
yatiya jakhüwi contador = 3;
yatiya aru saludo = "kamisaraki";
yatiya chiqa activo = chiqa;
```

- `jakhüwi`: número
- `aru`: texto
- `chiqa`: booleano

## 3. Control de flujo

```aym
ukaxa (contador > 0) {
  qillqa(saludo);
} maysatxa {
  qillqa("janiwa");
}

kuti (yatiya jakhüwi i = 0; i < 3; i = i + 1) {
  qillqa(i);
}
```

La variante actual usa una sola sintaxis canónica: `ukaxa`, `maysatxa`,
`ukhakamaxa`, `kuti`, `chiqa` y `k'ari`.

## 4. Funciones

```aym
lurawi inc(jakhüwi n): jakhüwi {
  kuttaya n + 1;
}

qillqa(inc(5));
```

## 5. Módulos

```aym
apnaq("modules/aritmetica");
qillqa("suma: " + suma(3, 4));
```

## 6. Comentarios

```aym
// comentario de línea
/* comentario de bloque */
```

## 7. Siguiente paso

- Si quieres trabajar de forma práctica: [Manual de usuario](manual_usuario.md)
- Si quieres ver todas las palabras clave: [Referencia rápida](aymaraLang.md)
- Si quieres detalle sintáctico: [Gramática formal](grammar.md)
