# Guía de características

Esta guía agrupa las funcionalidades más usadas de AymaraLang con ejemplos.

## Variables string
```aymara
yatiya aru saludo = "Kamisaraki";
qillqa(saludo);
```

## Operaciones aritméticas
Soporta `%` (módulo) y `^` (potencia de enteros).
```aymara
qillqa(5 % 2);
qillqa(2 ^ 3);
```

## Condicionales encadenados
```aymara
yatiya jakhüwi valor = 2;
jisa (valor == 1) {
    qillqa("uno");
} maysatxa {
    jisa (valor == 2) {
        qillqa("dos");
    } maysatxa {
        qillqa("otro");
    }
}
```

## Operadores lógicos
Se admiten `&&`, `||` y `!`.
```aymara
jisa (1 && !0) {
    qillqa("ok");
}
```

## Comentarios
Usa `//` para comentarios de línea y `/* ... */` para bloques.

## Lectura con `katu()`
```aymara
yatiya aru nombre = katu("Suti?: ", tuku="");
qillqa("Suti =", nombre);
yatiya jakhüwi numero = jakhüwi(katu("Numero?: ", tuku=""));
qillqa(numero);
```

## Comparaciones
```aymara
jisa (5 >= 3) {
    qillqa("mayor");
}
```

## Módulos (`apnaq`)

Desde ahora es posible dividir el código en varios archivos y reutilizarlo con la
 declaración `apnaq`.

```aymara
apnaq("modules/aritmetica");

yatiya jakhüwi total = suma(3, 4);
qillqa(total);
```

Coloca el archivo `modules/aritmetica.aym` junto al programa o dentro de una
carpeta `modules/`. El resolvedor busca módulos en:

- El directorio del archivo principal.
- Una carpeta `modules/` dentro de ese directorio.
- Rutas adicionales indicadas en la variable de entorno `AYM_PATH`.

Cada módulo se procesa una sola vez y puede importar a su vez otros módulos con
`apnaq`.

## Listas (`t'aqa`)

```aymara
yatiya t'aqa numeros = [1, 2, 3];
push(numeros, 4);
qillqa(numeros);
qillqa(largo(numeros));
```

## Biblioteca estándar (texto y listas)

Las funciones estándar son **puras y previsibles**. Cuando algo falla lanzan
errores con `Pantja("TIPU", "mensaje")`. Tipos recomendados: `CONVERSION`,
`INDICE`, `VACIO`, `ARG`, `CLAVE`.

### Texto (Aru)

```aymara
Qillqa(Suyu(""));
Qillqa(Suyu("abc"));
Qillqa(Ch'usa("  hola  "));
Qillqa(Jaljta("1,2,3", ","));
Qillqa(Jaljta("a--b--c", "--"));
Qillqa(Jaljta("", ","));
Qillqa(Jaljta("a", ","));
Qillqa(Mayachta(["a","b","c"], "-"));
Qillqa(Sikta("xd xdd xd", "xd", "ok"));
Qillqa(Utji("kamisaraki", "sara") ? "Chiqa" : "K'ari");
```

Salida esperada:

```
0
3
hola
["1", "2", "3"]
["a", "b", "c"]
[""]
["a"]
a-b-c
ok xdd ok
Chiqa
```

### Listas (T'aqa)

```aymara
Qillqa(SuyuT([]));
Qillqa(SuyuT([1,2,3]));

Yatiya T'aqa xs = [1,2];
Ch'ullu(xs, 3);
Qillqa(xs);

Yatiya T'aqa ys = ["a","b","c"];
Qillqa(Apsu(ys));
Qillqa(ys);

Yatiya T'aqa zs = [10,20,30];
Qillqa(ApsuUka(zs, 1));
Qillqa(zs);

Qillqa(UtjiT([1,2,3], 2) ? "Chiqa" : "K'ari");
Qillqa(UtjiT([1,2,3], 9) ? "Chiqa" : "K'ari");
```

### Combinaciones y errores

```aymara
Yatiya Aru linea = Katu("jakhunaka (1,2,3): ", tuku="");
Yatiya T'aqa ps = Jaljta(Ch'usa(linea), ",");
Yatiya Jakhüwi s = 0;

Taki(Yatiya Jakhüwi i = 0; i < SuyuT(ps); i++) {
  Yant'aña {
    s = s + Jakhüwi(Ch'usa(ps[i]));
  } Katjaña("CONVERSION", e) {
    Pantja("CONVERSION", "no valido: " + ps[i]);
  }
}

Qillqa(s);
```

```aymara
Yatiya Aru t = Mayachta(["xd","xdd"], "");
Qillqa(t);
Qillqa(Sikta(t, "xd", "ok"));
Qillqa(Utji(t, "xdd") ? "Chiqa" : "K'ari");
```

```aymara
Yatiya T'aqa vacia = [];

Yant'aña {
  Qillqa(Apsu(vacia));
} Katjaña("VACIO", e) {
  Qillqa("lista vacia");
} Tukuyawi {
  Qillqa("fin");
}
```

## Funciones matemáticas

```aymara
yatiya jakhüwi ang = 1;
qillqa(sin(ang));
qillqa(sqrt(9));
```

## Programación orientada a objetos (POO)

### Palabras clave (v1.0)

- `Kasta`: class.
- `Machaqa`: new (crear instancia).
- `Aka`: this (referencia al objeto actual).
- `Qallta`: constructor.
- `Jila`: extends (hereda de…).
- `Jikxata`: override (sobrescribir). (Opcional; puedes permitir override sin keyword).
- `Sapa`: private (opcional).
- `Taqi`: public (opcional).
- `SapaKasta`: static (opcional).
- `Uñt'aya`: getter / `Chura`: setter (opcional).

Todo lo demás son símbolos normales: `. () {} ; =` etc.

### Sintaxis base (plantillas)

**Definir clase**

```aymara
Kasta Nombre {
  // atributos
  // métodos
}
```

**Constructor**

```aymara
Qallta(Tipo a, Tipo b) {
  Aka.a = a;
  Aka.b = b;
}
```

**Crear objeto**

```aymara
Yatiya Nombre x = Machaqa Nombre(args...);
```

**Llamar método y acceder atributo**

```aymara
x.metodo();
x.atributo;
```

### Combinaciones típicas con ejemplos + salidas

#### 1) Clase simple + atributos + método

```aymara
Kasta Persona {
  Yatiya Aru suti;
  Yatiya Jakhüwi edad;

  Qallta(Aru s, Jakhüwi e) {
    Aka.suti = s;
    Aka.edad = e;
  }

  Lurawi saluda(): Aru {
    Kuttaya "hola " + Aka.suti;
  }
}

Yatiya Persona p = Machaqa Persona("Ana", 20);
Qillqa(p.saluda());
```

Salida esperada:

```
hola Ana
```

#### 2) Método “void” (sin retorno) + estado interno

```aymara
Kasta Contador {
  Yatiya Jakhüwi n;

  Qallta() { Aka.n = 0; }

  Lurawi inc() {
    Aka.n = Aka.n + 1;
    Kuttaya;
  }

  Lurawi ver(): Jakhüwi { Kuttaya Aka.n; }
}

Yatiya Contador c = Machaqa Contador();
c.inc();
c.inc();
Qillqa(c.ver());
```

Salida esperada:

```
2
```

#### 3) Encapsulación (private) + getter/setter

```aymara
Kasta Caja {
  Sapa Yatiya Jakhüwi x;

  Qallta(Jakhüwi v) { Aka.x = v; }

  Lurawi Uñt'aya(): Jakhüwi { Kuttaya Aka.x; }

  Lurawi Chura(Jakhüwi v) {
    Aka.x = v;
    Kuttaya;
  }
}

Yatiya Caja a = Machaqa Caja(10);
Qillqa(a.Uñt'aya());
a.Chura(99);
Qillqa(a.Uñt'aya());
```

Salida esperada:

```
10
99
```

#### 4) Static (método/atributo de clase)

```aymara
Kasta Mathu {
  SapaKasta Yatiya Jakhüwi version = 1;

  SapaKasta Lurawi suma(Jakhüwi a, Jakhüwi b): Jakhüwi {
    Kuttaya a + b;
  }
}

Qillqa(Mathu.version);
Qillqa(Mathu.suma(2, 3));
```

Salida esperada:

```
1
5
```

#### 5) Herencia (Jila) + override (Jikxata)

```aymara
Kasta Animal {
  Lurawi aru(): Aru { Kuttaya "???"; }
}

Kasta Anu Jila Animal {
  Jikxata Lurawi aru(): Aru { Kuttaya "guau"; }
}

Kasta Misi Jila Animal {
  Jikxata Lurawi aru(): Aru { Kuttaya "miau"; }
}

Yatiya Animal a = Machaqa Anu();
Yatiya Animal b = Machaqa Misi();
Qillqa(a.aru());
Qillqa(b.aru());
```

Salida esperada:

```
guau
miau
```

(Polimorfismo: variable tipo `Animal`, comportamiento depende de la instancia real.)

#### 6) super (llamar al padre) — `JilaAka`

```aymara
Kasta Base {
  Lurawi hi(): Aru { Kuttaya "base"; }
}

Kasta Hijo Jila Base {
  Jikxata Lurawi hi(): Aru {
    Kuttaya JilaAka.hi() + "+hijo";
  }
}

Qillqa(Machaqa Hijo().hi());
```

Salida esperada:

```
base+hijo
```

#### 7) Composición (objeto dentro de objeto)

```aymara
Kasta Motor {
  Yatiya Jakhüwi hp;
  Qallta(Jakhüwi h) { Aka.hp = h; }
}

Kasta Auto {
  Yatiya Motor m;
  Qallta(Motor mm) { Aka.m = mm; }

  Lurawi info(): Aru { Kuttaya "hp=" + Aru(Aka.m.hp); }
}

Yatiya Motor m = Machaqa Motor(150);
Yatiya Auto a = Machaqa Auto(m);
Qillqa(a.info());
```

Salida esperada:

```
hp=150
```

#### 8) Errores dentro de métodos + try/catch/finally

```aymara
Kasta Cuenta {
  Yatiya Jakhüwi saldo;

  Qallta(Jakhüwi s) { Aka.saldo = s; }

  Lurawi retirar(Jakhüwi x) {
    Jisa(x > Aka.saldo) { Pantja("SALDO", "no alcanza"); }
    Aka.saldo = Aka.saldo - x;
    Kuttaya;
  }
}

Yatiya Cuenta c = Machaqa Cuenta(50);

Yant'aña {
  c.retirar(100);
} Katjaña("SALDO", e) {
  Qillqa("error saldo");
} Tukuyawi {
  Qillqa("fin");
}
```

Salida esperada:

```
error saldo
fin
```

#### 9) Constructor por defecto + sobrecarga (si la soportas)

Regla: permitir múltiples `Qallta(...)` con firmas distintas.

```aymara
Kasta Punto {
  Yatiya Jakhüwi x;
  Yatiya Jakhüwi y;

  Qallta() { Aka.x = 0; Aka.y = 0; }

  Qallta(Jakhüwi a, Jakhüwi b) { Aka.x = a; Aka.y = b; }
}

Qillqa(Machaqa Punto().x);
Qillqa(Machaqa Punto(2, 3).y);
```

Salida esperada:

```
0
3
```

(Si no quieres sobrecarga, puedes obligar parámetros opcionales en vez de varios constructores).

#### 10) Clases “tipo interfaz” (opcional)

Si quieres interfaces, propuesta:

- `Yatiqawi`: contrato.
- `Apnaq`: implements.

```aymara
Yatiqawi X { metodo(); }

Kasta A Apnaq X { ... }
```

### Reglas para el compilador (núcleo obligatorio)

- `Kasta` crea un tipo con tabla de campos y métodos.
- `Machaqa Kasta(...)`:
  - aloca objeto.
  - inicializa campos (default).
  - ejecuta `Qallta(...)`.
- `Aka` dentro de métodos apunta a la instancia.
- `obj.metodo()` resuelve método en runtime (dinámico si hay herencia).
- `Jila` (herencia):
  - hereda campos/métodos.
  - `override` reemplaza método del padre.
- `Sapa` (privado) opcional: valida acceso solo dentro de la misma clase.
- `SapaKasta` (static) opcional: miembros colgados del tipo.

Interacciones obligatorias con tus errores:

- Si un método hace `Pantja`, se propaga a `Katjaña` más cercano.
- `Tukuyawi` se ejecuta aunque haya `Kuttaya`, `P'akhiña`, `Sarantaña`.

## Manejo de errores

El lenguaje permite usar `Yant'aña`/`Katjaña`/`Tukuyawi` y `Pantja` para
controlar errores. Revisa la guía completa con ejemplos y salidas en
[Manejo de errores](exceptions.md).

## Nota sobre palabras clave legacy

El compilador sigue aceptando `suti`/`jani`, `kunawsati`, `sapüru` y
`utji`/`janiutji`. Los ejemplos de esta guía usan la sintaxis actual con
`jisa`/`maysatxa`, `ukhakamaxa`, `taki` y `chiqa`/`k'ari`.

---

**Anterior:** [Primeros pasos](language.md) | **Siguiente:** [Referencia del lenguaje](aymaraLang.md)
