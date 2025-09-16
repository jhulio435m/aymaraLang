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

## Módulos (`apu`)

Desde ahora es posible dividir el código en varios archivos y reutilizarlo con la
declaración `apu`.

```aymara
apu "modules/aritmetica";

jach’a total = suma(3, 4);
willt’aña(total);
```

Coloca el archivo `modules/aritmetica.aym` junto al programa o dentro de una
carpeta `modules/`. El resolvedor busca módulos en:

- El directorio del archivo principal.
- Una carpeta `modules/` dentro de ese directorio.
- Rutas adicionales indicadas en la variable de entorno `AYM_PATH`.

Cada módulo se procesa una sola vez y puede importar a su vez otros módulos con
`apu`.
