"use client";

import { useMemo, useRef, useState } from "react";
import { tokenLines } from "@/lib/aym";
import { Salta } from "./salta";

export interface Example {
  id: string;
  label: string;
  note: string;
  path: string;
  source: string;
  lines: number;
  href: string;
}

const PASS = 7;

/** Programas completos del repositorio, cada uno junto a la figura que teje. */
export function Examples({ items }: { items: Example[] }) {
  const [current, setCurrent] = useState(0);
  const [line, setLine] = useState<number | null>(null);
  const tabs = useRef<(HTMLButtonElement | null)[]>([]);
  const pane = useRef<HTMLPreElement>(null);
  const item = items[current];
  const lines = useMemo(() => tokenLines(item.source), [item.source]);

  const select = (index: number) => {
    const next = (index + items.length) % items.length;
    setCurrent(next);
    setLine(null);
    if (pane.current) pane.current.scrollTop = 0;
    return next;
  };

  const onKey = (e: React.KeyboardEvent) => {
    const step = e.key === "ArrowRight" ? 1 : e.key === "ArrowLeft" ? -1 : 0;
    if (!step) return;
    e.preventDefault();
    tabs.current[select(current + step)]?.focus();
  };

  // Al recorrer el tejido, la línea correspondiente se acerca dentro del panel.
  const fromWeave = (row: number | null) => {
    setLine(row);
    const box = pane.current;
    const el = row == null ? null : box?.querySelector<HTMLElement>(`[data-i="${row}"]`);
    if (!box || !el) return;
    const top = el.offsetTop - box.offsetTop;
    if (top < box.scrollTop || top > box.scrollTop + box.clientHeight - 40) box.scrollTop = top - box.clientHeight / 2;
  };

  // Con el foco en el código, las flechas recorren líneas y su pasada.
  const onCodeKey = (e: React.KeyboardEvent) => {
    const step = e.key === "ArrowDown" ? 1 : e.key === "ArrowUp" ? -1 : 0;
    if (!step) return;
    e.preventDefault();
    fromWeave(Math.max(0, Math.min(lines.length - 1, (line ?? -1) + step)));
  };

  return (
    <div className="ejemplos">
      <div className="pestanas" role="tablist" aria-label="Programas de ejemplo" onKeyDown={onKey}>
        {items.map((it, i) => (
          <button
            key={it.id}
            ref={(el) => {
              tabs.current[i] = el;
            }}
            role="tab"
            id={`tab-${it.id}`}
            aria-selected={i === current}
            aria-controls="panel-ejemplo"
            tabIndex={i === current ? 0 : -1}
            onClick={() => select(i)}
          >
            {it.label}
          </button>
        ))}
      </div>

      <div className="telar" role="tabpanel" id="panel-ejemplo" aria-labelledby={`tab-${item.id}`}>
        <div className="telar-codigo">
          <p className="telar-pie">
            <a href={item.href}>{item.path}</a>
            <span>
              {item.note} {item.lines} líneas.
            </span>
          </p>
          <pre ref={pane} className="codigo codigo-panel" tabIndex={0} aria-label={`Código de ${item.path}. Usa las flechas para recorrer las líneas.`} onKeyDown={onCodeKey} onPointerLeave={() => setLine(null)}>
            <code>
              {lines.map((toks, i) => (
                <span key={i} data-i={i} className="linea" data-on={line === i || undefined} onPointerEnter={() => setLine(i)}>
                  {toks.map((t, j) => (t.kind === "w" ? t.text : <span key={j} data-h={t.kind}>{t.text}</span>))}
                  {"\n"}
                </span>
              ))}
            </code>
          </pre>
        </div>

        <div className="telar-tejido">
          <Salta
            key={item.id}
            source={item.source}
            pass={PASS}
            columns={56}
            active={line}
            onPick={fromWeave}
            label={`Tejido de ${item.path}: ${item.lines} pasadas, una por línea de código.`}
          />
        </div>
      </div>
    </div>
  );
}
