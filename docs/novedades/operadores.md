# Operadores y estructuras de control nuevas

## Asignación compuesta

Los operadores `+=`, `-=`, `*=`, `/=` y `%=` funcionan sobre:

- Variables: `x += 5;`
- Elementos de lista o mapa: `xs[i] += 1;`, `mapa["clave"] *= 2;`
- Atributos de instancia o estáticos: `obj.campo += 1;`, `Clase.total += 1;`
- Cláusula de actualización en `kuti`: `kuti (yatiya jakhüwi i = 0; i < 10; i += 2) { ... }`

```aymara
yatiya jakhüwi x = 10;
x += 5;   // 15
x -= 3;   // 12
x *= 4;   // 48
x /= 6;   // 8
x %= 5;   // 3

yatiya t'aqa<jakhüwi> xs = [1, 2, 3];
xs[1] += 10;  // [1, 12, 3]

yatiya jakhüwi suma = 0;
kuti (yatiya jakhüwi i = 0; i < 6; i += 2) {
  suma += i;
}
qillqa(x, xs, suma);
```

Nota: la base y el índice del lado izquierdo en asignaciones compuestas deben ser
expresiones simples (variable, literal, acceso a campo o índice simple); de lo
contrario el compilador reporta `AYM2001` (`"asignacion compuesta solo admite variables, indices o campos simples"`).

## Operadores bit a bit

Se agregan los operadores bit a bit sobre enteros (`jakhüwi`):

| Operador | Descripción | Precedencia relativa |
|----------|-------------|----------------------|
| `~` | Complemento bit a bit (unario) | Unario |
| `<<`, `>>` | Desplazamiento izquierda / derecha | Entre comparación (`<`, `<=`, `>`, `>=`) y suma (`+`, `-`) |
| `&` | AND bit a bit | Debajo de igualdad (`==`, `!=`) |
| `^^` | XOR bit a bit | Debajo de `&` (`^` sigue siendo potencia) |
| `\|` | OR bit a bit | Debajo de `^^` y encima de `&&`/`\|\|` |

```aymara
yatiya jakhüwi a = 0b1100;
yatiya jakhüwi b = 0b1010;
qillqa(a & b);    // 8
qillqa(a | b);    // 14
qillqa(a ^^ b);   // 6
qillqa(3 << 2);   // 12
qillqa(24 >> 2);  // 6
qillqa(~0);       // -1
```

Notas:
- Todos los operandos deben ser `jakhüwi`; usar otro tipo reporta `AYM3003` (`"operacion bit a bit requiere jakhüwi"`).
- El prefijo unario `&ident` se mantiene para referencias a funciones (por ejemplo en `map(xs, &f)`).

## Bucle `luraña ... ukhakamaxa` (`do-while`)

Ejecuta el bloque al menos una vez y evalúa la condición al final de cada iteración:

```aymara
yatiya jakhüwi n = 0;
luraña {
  qillqa(n);
  n += 1;
} ukhakamaxa (n < 3);
```

Soporta `p'akhiña` (`break`) y `sarantaña` (`continue`).

## Constantes inmutables (`wiñaya`)

Declara constantes de bloque, función o nivel global que requieren inicialización y no pueden reasignarse ni modificarse con `=`, `op=`, `++` o `--`:

```aymara
wiñaya jakhüwi MAX = 100;
yatiya wiñaya aru SALUDO = "kamisaraki";
```

Reglas:
- Omitir el inicializador reporta `AYM3001` (`"constante requiere inicializacion"`).
- Intentar modificar la constante reporta `AYM3003` (`"no se puede modificar la constante '...'"`).
- Se respeta el sombreado de ámbitos: una variable local mutable con el mismo nombre dentro de un bloque o función interna sí puede modificarse.
