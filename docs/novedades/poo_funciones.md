# Novedades: funciones, lambdas y POO

## Valores por defecto en parámetros (funciones y métodos)

Los parámetros de funciones (`lurawi`) y métodos de clase (`kasta`) pueden
declarar valores por defecto usando `= expr`. Se admiten literales (`NumberExpr`,
`BoolExpr`, `StringExpr`), expresiones unarias/binarias constantes (`-1`, `1 + 1`,
`2 * 3`) y listas constantes (`[1, 2]`).

```aymara
lurawi suma(jakhüwi a, jakhüwi b = 10, jakhüwi c = -2): jakhüwi {
  kuttaya a + b + c;
}

kasta Contador {
  yatiya jakhüwi valor = 0;
  lurawi incrementar(jakhüwi paso = 1 + 1): jakhüwi {
    aka.valor += paso;
    kuttaya aka.valor;
  }
}

qillqa(suma(5));          // 13
yatiya Contador c = machaqa Contador();
qillqa(c.incrementar());  // 2
```

Reglas:
- Los parámetros con valor por defecto deben ir al final; un parámetro requerido después de uno con valor por defecto reporta `AYM2001`.
- Si faltan argumentos requeridos o la expresión por defecto no es evaluable estáticamente, se reporta `AYM3005`.
- Los métodos heredados y las llamadas mediante `jilaaka` también aplican los valores por defecto declarados en la clase correspondiente.

## Funciones anónimas (lambdas)

Se pueden definir funciones anónimas en expresiones con `lurawi(params)[: tipo] { ... }`, ideales para funciones de orden superior (`map`/`mayjtaya`, `filter`/`ajlli`, `reduce`/`thaqthapi`):

```aymara
yatiya t'aqa<jakhüwi> xs = [1, 2, 3, 4];
yatiya t'aqa<jakhüwi> dobles = map(xs, lurawi(jakhüwi x): jakhüwi {
  kuttaya x * 2;
});
yatiya t'aqa<jakhüwi> mayores = filter(dobles, lurawi(jakhüwi x): chiqa {
  kuttaya x > 4;
});
```

## Métodos y clases abstractas

Un método de instancia sin cuerpo terminado en `;` se declara como abstracto. Toda clase que contenga o herede al menos un método abstracto sin sobrescribir es abstracta:

```aymara
kasta Figura {
  lurawi area(): jakhüwi;
}

kasta Cuadrado jila Figura {
  yatiya jakhüwi lado;
  qallta(jakhüwi l) { aka.lado = l; }
  lurawi area(): jakhüwi { kuttaya aka.lado * aka.lado; }
}
```

Reglas:
- Un método `sapakasta` (estático) no puede ser abstracto (`AYM2001`).
- Instanciar una clase abstracta (`machaqa Figura()`) se rechaza con `AYM3009` (`"no se puede instanciar la clase abstracta '...'"`).
- Al sobrescribir un método abstracto heredado se valida la compatibilidad de firma y tipo de retorno (`AYM3009`).

## Colecciones tipadas (`t'aqa<T>`, `mapa<T>`) e indexación de texto

Las anotaciones de listas y mapas aceptan tipo de elemento/valor entre `< >`:

```aymara
yatiya t'aqa<aru> nombres = ["Ana", "Luis"];
yatiya t'aqa<aru> vacia = [];
push(vacia, "Maya");

yatiya mapa<jakhüwi> edades = {"Ana": 20};
yatiya aru texto = "aymara";
qillqa(texto[0]);  // "a"
```

Reglas:
- `t'aqa` sin parámetro es compatible con cualquier `t'aqa<T>` y viceversa, pero `t'aqa<aru>` y `t'aqa<jakhüwi>` son incompatibles entre sí (`AYM3003`). Lo mismo aplica para `mapa` y `mapa<T>`.
- Indexar un texto (`s[i]`) requiere índice `jakhüwi` (`AYM3003` si no lo es) y produce tipo `aru`.
