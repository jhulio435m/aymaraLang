# Nuevas características de AymaraLang

## Variables string
```aymara
string saludo = "Kamisaraki";
willt’aña(saludo);
```

## Operaciones aritméticas
Soporta `%` (módulo) y `^` (potencia de enteros).
```aymara
willt’aña(5 % 2);
willt’aña(2 ^ 3);
```

## Switch-Case
```aymara
valor = 2;
switch(valor) {
    case 1 {
        willt’aña("uno");
    }
    case 2 {
        willt’aña("dos");
    }
    default {
        willt’aña("otro");
    }
}
```

## Operadores lógicos
Se admiten `and`, `or` y `not`.
```aymara
si (1 and not 0) {
    willt’aña("ok");
}
```

## Comentarios
Usa `//` para comentarios de línea y `/* ... */` para bloques.

## Lectura con `input()`
```aymara
numero = input();
willt’aña(numero);
```

## Comparaciones
```aymara
si (5 >= 3) {
    willt’aña("mayor");
}
```
