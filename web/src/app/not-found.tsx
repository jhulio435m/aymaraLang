import type { Metadata } from "next";
import Link from "next/link";
import { Franja } from "@/components/weave";
import { DOCS_URL, HELLO } from "@/content/site";

export const metadata: Metadata = { title: "Página no encontrada", robots: { index: false, follow: true },
  alternates: { canonical: null },
};

export default function NotFound() {
  return (
    <main className="portada" id="contenido">
      <Franja source={HELLO} />
      <div className="ancho portada-pampa">
        <div>
          <h1>Esta página no existe.</h1>
          <p className="entrada">La dirección no corresponde a ninguna página de AymaraLang.</p>
          <p className="portada-enlaces">
            <Link href="/">Volver al inicio</Link>
            <a href={DOCS_URL}>Ir a la documentación</a>
          </p>
        </div>
      </div>
      <Franja source={HELLO} flip />
    </main>
  );
}
