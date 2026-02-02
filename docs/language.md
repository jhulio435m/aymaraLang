# Características y ejemplos

## Características del lenguaje

AymaraLang incluye un conjunto de construcciones inspiradas en Python pero con
palabras clave en aymara. Entre ellas:

```aymara
// variables
jach’a contador = 3;
qillqa saludo = "kamisaraki";

// condicional
si (contador > 0) {
    willt’aña(saludo);
}

// bucle for
para i en range(0, 3) {
    willt’aña(i);
}

luräwi inc(n) {
    kutiyana n + 1;
}
```

Las funciones integradas `input()` y `willt’aña()` permiten entrada/salida
sencilla y `tantachaña` ofrece un control tipo `switch`.

## Ejemplos de código

### `hola.aym`
```aymara
willt’aña("Kamisaraki!");
```

### `ops.aym`
```aymara
willt’aña(3 + 4 * 2);
```

### `condloop.aym`
```aymara
si (1) {
    willt’aña("cond");
}

mientras (3) {
    willt’aña("loop");
}
```

### `vars.aym`
```aymara
x = 5;
y = x * 2 + 3;
willt’aña(y);
```

### `recursion.aym`
```aymara
luräwi fact(n) {
    si (n == 0) {
        kutiyana(1);
    }
    kutiyana(n * fact(n - 1));
}

willt’aña(fact(5));
```

### `module_demo.aym`
```aymara
apu "modules/aritmetica";

jach’a base = 10;
jach’a incremento = 5;
willt’aña("suma: " + suma(base, incremento));
willt’aña("resta: " + resta(base, incremento));
```
