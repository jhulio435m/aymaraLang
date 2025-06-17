# Nuevas características de AymaraLang

## Variables string
```aymara
qillqa saludo = "Kamisaraki";
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
```

## Operadores lógicos
Se admiten `uka`, `jan uka` y `janiwa`.
```aymara
si (1 uka janiwa 0) {
    willt’aña("ok");
}
```

## Comentarios
Usa `//` para comentarios de línea y `/* ... */` para bloques.

## Lectura con `input()`
```aymara
jach’a numero = input();
willt’aña(numero);
```

## Comparaciones
```aymara
si (5 >= 3) {
    willt’aña("mayor");
}
```
