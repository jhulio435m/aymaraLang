# Manejo de errores (Yant'aña / Katjaña / Tukuyawi)

Esta sección describe el manejo de errores en AymaraLang usando las palabras
clave `Yant'aña` (try), `Katjaña` (catch), `Tukuyawi` (finally) y `Pantja`
(throw). Mantiene los símbolos `() { } ; = && || ! > < >= <= == != ? : ++ --`
intactos.

> Nota: En los ejemplos se usan `Qillqa` (print) y `Yatiya` (declarar) porque el
> compilador lo necesita para probar.

## 1) Formas válidas (gramática práctica)

### 1.1 Lanzar error
```aymara
Pantja("mensaje");
Pantja("TIPU", "mensaje");
```

### 1.2 Try/Catch
```aymara
Yant'aña {
  ...
} Katjaña(e) {
  ...
}
```

### 1.3 Try/Finally
```aymara
Yant'aña {
  ...
} Tukuyawi {
  ...
}
```

### 1.4 Try/Catch/Finally
```aymara
Yant'aña {
  ...
} Katjaña(e) {
  ...
} Tukuyawi {
  ...
}
```

### 1.5 Catch tipado (recomendado)
```aymara
Yant'aña {
  ...
} Katjaña("TIPU", e) {
  ...
}
```

### 1.6 Múltiples catches + default
```aymara
Yant'aña {
  ...
}
Katjaña("A", e) { ... }
Katjaña("B", e) { ... }
Katjaña(e) { ... }       // default
Tukuyawi { ... }         // opcional
```

## 2) Semántica obligatoria (para tu compilador)

- `Pantja(...)` lanza una excepción y busca el `Katjaña` más cercano.
- `Tukuyawi` se ejecuta siempre (haya error o no).
- `Katjaña("TIPU", e)` solo atrapa si `e.suti == "TIPU"`.
- `Katjaña(e)` atrapa cualquier error (catch default).
- `Pantja(...)` dentro de `Katjaña` es rethrow.
- Si `Tukuyawi` lanza error, ese error domina sobre cualquier otro anterior.
- Los `P'akhiña` / `Sarantaña` dentro de `Yant'aña` deben ejecutar `Tukuyawi`
  antes de salir/continuar el bucle.

## 3) TODAS las combinaciones con ejemplos + salidas

### 3.1 Throw sin manejo (programa se detiene)
```aymara
Qillqa("antes");
Pantja("fallo");
Qillqa("despues");
```

Salida:
```
antes
```

### 3.2 Yant'aña + Katjaña (error atrapado)
```aymara
Yant'aña {
  Qillqa("A");
  Pantja("fallo");
  Qillqa("B");
} Katjaña(e) {
  Qillqa("C");
}
Qillqa("D");
```

Salida:
```
A
C
D
```

### 3.3 Yant'aña + Tukuyawi (sin catch)

`Tukuyawi` corre, pero el error sigue y el programa termina.

```aymara
Yant'aña {
  Qillqa("A");
  Pantja("fallo");
} Tukuyawi {
  Qillqa("F");
}
Qillqa("D");
```

Salida:
```
A
F
```

### 3.4 Yant'aña + Katjaña + Tukuyawi
```aymara
Yant'aña {
  Qillqa("A");
  Pantja("fallo");
} Katjaña(e) {
  Qillqa("C");
} Tukuyawi {
  Qillqa("F");
}
Qillqa("D");
```

Salida:
```
A
C
F
D
```

### 3.5 No hay error (catch no corre, finally sí)
```aymara
Yant'aña {
  Qillqa("A");
  Qillqa("B");
} Katjaña(e) {
  Qillqa("C");
} Tukuyawi {
  Qillqa("F");
}
Qillqa("D");
```

Salida:
```
A
B
F
D
```

### 3.6 Error dentro del catch (rethrow)
```aymara
Yant'aña {
  Pantja("X");
} Katjaña(e) {
  Qillqa("atrapado");
  Pantja("Y");
}
Qillqa("fin");
```

Salida:
```
atrapado
```

### 3.7 Error tipado (Pantja con tipo)
```aymara
Yant'aña {
  Pantja("CONVERSION", "no es numero");
} Katjaña(e) {
  Qillqa(e.suti);
  Qillqa(e.aru);
}
```

Salida:
```
CONVERSION
no es numero
```

### 3.8 Catch tipado (solo atrapa el tipo correcto)
```aymara
Yant'aña {
  Pantja("INDICE", "fuera de rango");
} Katjaña("CONVERSION", e) {
  Qillqa("conv");
} Katjaña("INDICE", e) {
  Qillqa("idx");
}
```

Salida:
```
idx
```

### 3.9 Catch default al final
```aymara
Yant'aña {
  Pantja("OTRO", "x");
}
Katjaña("CONVERSION", e) { Qillqa("conv"); }
Katjaña(e) { Qillqa("default"); }
```

Salida:
```
default
```

### 3.10 Múltiples catches + finally
```aymara
Yant'aña {
  Pantja("INDICE", "x");
}
Katjaña("CONVERSION", e) { Qillqa("conv"); }
Katjaña("INDICE", e) { Qillqa("idx"); }
Katjaña(e) { Qillqa("default"); }
Tukuyawi { Qillqa("F"); }

Qillqa("D");
```

Salida:
```
idx
F
D
```

### 3.11 Finally lanza error (domina sobre todo)
```aymara
Yant'aña {
  Pantja("A", "x");
} Katjaña(e) {
  Qillqa("catch");
} Tukuyawi {
  Pantja("F", "y");
}
Qillqa("D");
```

Salida:
```
catch
```

### 3.12 Error dentro de try, catch “lo arregla” y sigue normal
```aymara
Yant'aña {
  Pantja("X", "x");
} Katjaña(e) {
  Qillqa("arreglado");
}
Qillqa("sigue");
```

Salida:
```
arreglado
sigue
```

### 3.13 Try anidado (try dentro de try)
```aymara
Yant'aña {
  Yant'aña {
    Pantja("X");
  } Katjaña(e) {
    Qillqa("inner");
  }
  Qillqa("outer");
} Katjaña(e) {
  Qillqa("catch outer");
}
```

Salida:
```
inner
outer
```

### 3.14 Catch anidado (catch que genera otro error)
```aymara
Yant'aña {
  Pantja("X");
} Katjaña(e) {
  Qillqa("C1");
  Pantja("Y");
} Tukuyawi {
  Qillqa("F");
}
```

Salida:
```
C1
F
```

(Luego se detiene por Y)

### 3.15 Interacción con bucles: P'akhiña dentro de try ejecuta Tukuyawi
```aymara
Taki(Yatiya Jakhüwi i = 0; i < 5; i++) {
  Yant'aña {
    Jisa(i == 2) { P'akhiña; }
    Qillqa(i);
  } Tukuyawi {
    Qillqa("F");
  }
}
```

Salida:
```
0
F
1
F
F
```

### 3.16 Interacción con bucles: Sarantaña ejecuta Tukuyawi y sigue
```aymara
Taki(Yatiya Jakhüwi i = 0; i < 4; i++) {
  Yant'aña {
    Jisa(i == 1) { Sarantaña; }
    Qillqa(i);
  } Tukuyawi {
    Qillqa("F");
  }
}
```

Salida:
```
0
F
F
2
F
3
F
```

### 3.17 Try con Kuttaya (return) ejecuta Tukuyawi
```aymara
Lurawi f(): Jakhüwi {
  Yant'aña {
    Kuttaya 7;
  } Tukuyawi {
    Qillqa("F");
  }
}

Qillqa(f());
```

Salida:
```
F
7
```

### 3.18 “Propagar” error hacia arriba (no atrapado en función)
```aymara
Lurawi g(): Jakhüwi {
  Pantja("X", "x");
}

Yant'aña {
  Qillqa(g());
} Katjaña("X", e) {
  Qillqa("atrapado arriba");
}
```

Salida:
```
atrapado arriba
```

---

**Anterior:** [Referencia del lenguaje](aymaraLang.md) | **Siguiente:** [Gramática formal](grammar.md)
