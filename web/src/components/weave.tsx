import { THREAD, tokenLines } from "@/lib/aym";

/**
 * Franja tejida de borde a borde: cada línea de `source` es una pasada, cada
 * token un tramo de hilo tan ancho como sus letras, y la pasada se refleja
 * sobre el eje central. Es decoración derivada del código: va oculta a lectores.
 */
export function Franja({ source, pass = 22, flip = false }: { source: string; pass?: number; flip?: boolean }) {
  let rows = tokenLines(source)
    .map((line) => line.filter((t) => t.kind !== "w"))
    .filter((line) => line.length > 0);
  if (flip) rows = [...rows].reverse();

  return (
    <div className="franja" aria-hidden="true">
      {rows.map((row, r) => (
        <div key={r} className="franja-pasada" style={{ height: pass }}>
          {[...row, ...[...row].reverse()].map((t, i) => (
            <span key={i} style={{ flexGrow: t.text.length, background: THREAD[t.kind as keyof typeof THREAD] }} />
          ))}
        </div>
      ))}
    </div>
  );
}

/** Código resaltado con los mismos colores de hilo. */
export function Code({ source, className }: { source: string; className?: string }) {
  return (
    <pre className={className}>
      <code>
        {tokenLines(source).map((line, i) => (
          <span key={i} className="linea">
            {line.map((t, j) => (t.kind === "w" ? t.text : <span key={j} data-h={t.kind}>{t.text}</span>))}
            {"\n"}
          </span>
        ))}
      </code>
    </pre>
  );
}
