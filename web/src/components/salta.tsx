"use client";

import { useEffect, useMemo, useRef } from "react";
import { tokenLines, type Kind } from "@/lib/aym";

const VARS: Record<Exclude<Kind, "w">, string> = {
  k: "--hilo-clave",
  t: "--hilo-tipo",
  s: "--hilo-texto",
  n: "--hilo-numero",
  i: "--hilo-nombre",
  c: "--hilo-nota",
  p: "--hilo-signo",
};

interface Props {
  source: string;
  /** Alto de cada pasada en píxeles CSS. */
  pass: number;
  /** Columnas de texto que caben en media figura. */
  columns?: number;
  /** Línea resaltada (las demás se atenúan). */
  active?: number | null;
  onPick?: (line: number | null) => void;
  /** Sin entrada animada (figuras pequeñas en serie). */
  still?: boolean;
  label: string;
}

/**
 * Figura tejida de un programa: cada línea es una pasada que crece desde el eje
 * central hacia ambos orillos, con cada token en su columna y su color de hilo.
 * La sangría queda como pampa en el centro, así que los bloques anidados abren
 * rombos. Se dibuja en canvas porque un programa largo son miles de tramos.
 */
export function Salta({ source, pass, columns = 64, active = null, onPick, still = false, label }: Props) {
  const ref = useRef<HTMLCanvasElement>(null);
  const lines = useMemo(() => tokenLines(source), [source]);
  const shown = useRef(0);
  const activeRef = useRef(active);

  useEffect(() => {
    const canvas = ref.current;
    if (!canvas) return;
    const ctx = canvas.getContext("2d");
    if (!ctx) return;
    const css = getComputedStyle(canvas);
    const color = Object.fromEntries(Object.entries(VARS).map(([k, v]) => [k, css.getPropertyValue(v).trim()]));
    const gap = pass >= 4 ? 1 : 0;
    let frame = 0;

    const draw = () => {
      const width = canvas.clientWidth;
      const dpr = Math.min(window.devicePixelRatio || 1, 2);
      const height = lines.length * pass;
      if (canvas.width !== Math.round(width * dpr) || canvas.height !== Math.round(height * dpr)) {
        canvas.width = Math.round(width * dpr);
        canvas.height = Math.round(height * dpr);
      }
      ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      ctx.clearRect(0, 0, width, height);
      const unit = width / 2 / columns;
      const mid = width / 2;
      const upTo = Math.min(lines.length, Math.ceil(shown.current));
      for (let r = 0; r < upTo; r++) {
        ctx.globalAlpha = activeRef.current == null || activeRef.current === r ? 1 : 0.26;
        let col = 0;
        for (const t of lines[r]) {
          const len = t.text.length;
          if (t.kind !== "w" && col < columns) {
            const w = Math.min(len, columns - col) * unit;
            ctx.fillStyle = color[t.kind];
            ctx.fillRect(mid + col * unit, r * pass, w, pass - gap);
            ctx.fillRect(mid - col * unit - w, r * pass, w, pass - gap);
          }
          col += len;
        }
      }
      ctx.globalAlpha = 1;
    };

    // La trama entra pasada a pasada; sin movimiento si el visitante lo pide.
    const quiet = still || window.matchMedia("(prefers-reduced-motion: reduce)").matches;
    const start = performance.now();
    const duration = Math.min(900, 240 + lines.length * 4);
    shown.current = quiet ? lines.length : 0;
    const tick = (now: number) => {
      const p = Math.min(1, (now - start) / duration);
      shown.current = lines.length * (1 - Math.pow(1 - p, 3));
      draw();
      if (p < 1) frame = requestAnimationFrame(tick);
    };
    if (quiet) draw();
    else frame = requestAnimationFrame(tick);

    const observer = new ResizeObserver(() => {
      if (shown.current >= lines.length) draw();
    });
    observer.observe(canvas);
    (canvas as HTMLCanvasElement & { redraw?: () => void }).redraw = draw;
    return () => {
      cancelAnimationFrame(frame);
      observer.disconnect();
    };
  }, [lines, pass, columns, still]);

  useEffect(() => {
    activeRef.current = active;
    (ref.current as (HTMLCanvasElement & { redraw?: () => void }) | null)?.redraw?.();
  }, [active]);

  const pick = (e: React.PointerEvent<HTMLCanvasElement>) => {
    if (!onPick) return;
    const top = e.currentTarget.getBoundingClientRect().top;
    const row = Math.floor((e.clientY - top) / pass);
    onPick(row >= 0 && row < lines.length ? row : null);
  };

  return (
    <canvas
      ref={ref}
      className="salta"
      role="img"
      aria-label={label}
      style={{ height: lines.length * pass }}
      onPointerMove={onPick ? pick : undefined}
      onPointerLeave={onPick ? () => onPick(null) : undefined}
    />
  );
}
