import { Examples } from "@/components/examples";
import { Salta } from "@/components/salta";
import { Code, Franja } from "@/components/weave";
import {
  COMMANDS,
  DESCRIPTION,
  DOCS_URL,
  EXAMPLES,
  HELLO,
  PIPELINE,
  RELEASE,
  REPO_URL,
  SITE_URL,
  SOURCE_URL,
  VOCABULARY,
  WORKS,
  mb,
} from "@/content/site";

// Fragmentos de docs/language.md que tejen las franjas entre secciones.
const FN = `lurawi inc(jakhüwi n): jakhüwi {
  kuttaya n + 1;
}
qillqa(inc(5));`;
const FLOW = `ukaxa (contador > 0) {
  qillqa(saludo);
} maysatxa {
  qillqa("janiwa");
}`;
const VARS = `yatiya jakhüwi contador = 3;
yatiya aru saludo = "kamisaraki";
yatiya chiqa activo = chiqa;`;
const LOOP = `kuti (yatiya jakhüwi i = 0; i < 3; i = i + 1) {
  qillqa(i);
}`;

const HILOS = [
  { h: "k", name: "palabra clave" },
  { h: "t", name: "tipo" },
  { h: "s", name: "texto" },
  { h: "n", name: "número o valor" },
  { h: "i", name: "nombre" },
];

function Down() {
  return (
    <svg className="flecha" viewBox="0 0 16 16" width="16" height="16" aria-hidden="true">
      <path d="M8 2v10M3.5 7.5 8 12l4.5-4.5M3 14h10" fill="none" stroke="currentColor" strokeWidth="1.75" strokeLinecap="square" />
    </svg>
  );
}

const jsonLd = {
  "@context": "https://schema.org",
  "@graph": [
    {
      "@type": "Organization",
      "@id": "https://soryntel.com/#organization",
      name: "Soryntel",
      url: "https://soryntel.com",
    },
    {
      "@type": "WebSite",
      "@id": `${SITE_URL}/#website`,
      name: "AymaraLang",
      url: `${SITE_URL}/`,
      description: DESCRIPTION,
      inLanguage: "es",
      publisher: { "@id": "https://soryntel.com/#organization" },
    },
    {
      "@type": "SoftwareApplication",
      "@id": `${SITE_URL}/#software`,
      name: "AymaraLang",
      alternateName: ["aym", "aymc"],
      url: `${SITE_URL}/`,
      image: `${SITE_URL}/og.png`,
      description: DESCRIPTION,
      applicationCategory: "DeveloperApplication",
      applicationSubCategory: "Lenguaje de programación",
      operatingSystem: "Windows, Linux",
      softwareVersion: RELEASE.version,
      datePublished: RELEASE.publishedAt,
      downloadUrl: RELEASE.page,
      releaseNotes: RELEASE.page,
      softwareHelp: DOCS_URL,
      codeRepository: REPO_URL,
      license: "https://opensource.org/license/mit",
      isAccessibleForFree: true,
      inLanguage: "es",
      offers: { "@type": "Offer", price: "0", priceCurrency: "USD" },
      publisher: { "@id": "https://soryntel.com/#organization" },
    },
  ],
};

export default function Page() {
  const { winExe, winMsi, linuxDeb } = RELEASE;

  return (
    <>
      <script type="application/ld+json" dangerouslySetInnerHTML={{ __html: JSON.stringify(jsonLd) }} />
      <a href="#contenido" className="saltar">
        Saltar al contenido
      </a>

      <header className="cabecera">
        <div className="ancho cabecera-fila">
          <a href="#inicio" className="marca" aria-label="AymaraLang, inicio">
            {/* eslint-disable-next-line @next/next/no-img-element */}
            <img src="/brand/aymaralang-mark.webp" alt="" width="44" height="44" />
            <span>
              Aymara<b>Lang</b>
            </span>
          </a>
          <nav aria-label="Navegación principal">
            <a href="#palabras">Las palabras</a>
            <a href="#ejemplos">Ejemplos</a>
            <a href="#hecho">Hecho con él</a>
            <a href="#compila">Cómo compila</a>
            <a href={DOCS_URL} className="nav-docs">
              Documentación
            </a>
          </nav>
          <a href="#descargar" className="boton boton-chico">
            Descargar
          </a>
        </div>
      </header>

      <main id="contenido">
        <section id="inicio" className="portada">
          <Franja source={HELLO} />
          <div className="ancho portada-pampa">
            <div>
              <h1>Programa en aymara. Compila a nativo.</h1>
              <p className="entrada">
                AymaraLang es un lenguaje de programación con palabras clave en aymara y compilador propio,{" "}
                <code>aymc</code>, que genera ejecutables para Windows y Linux.
              </p>
              <p className="portada-enlaces">
                <a href={DOCS_URL}>Leer la documentación</a>
                <a href={REPO_URL}>Código en GitHub</a>
              </p>
            </div>
            <figure className="portada-codigo">
              <figcaption>hola.aym</figcaption>
              <Code source={HELLO} className="codigo codigo-grande" />
              <p>Las tres franjas de arriba y de abajo son este programa: una por línea, un color por clase de palabra.</p>
            </figure>
          </div>
          <Franja source={HELLO} flip />
          <div className="ribete">
            <p className="ribete-rotulo">
              Descargar <span>{RELEASE.tag} · gratis · licencia MIT</span>
            </p>
            {winExe && (
              <a href={winExe.url} className="ribete-celda">
                <span>
                  Windows <small>Instalador .exe · {mb(winExe.size)}</small>
                </span>
                <Down />
              </a>
            )}
            {linuxDeb && (
              <a href={linuxDeb.url} className="ribete-celda">
                <span>
                  Linux <small>Paquete .deb · {mb(linuxDeb.size)}</small>
                </span>
                <Down />
              </a>
            )}
          </div>
        </section>

        <section id="palabras" className="pano">
          <div className="ancho">
            <div className="titular">
              <h2>El vocabulario cabe en una página.</h2>
              <p>
                Estas son las palabras reservadas de AymaraLang. Todo lo demás (nombres de variables, funciones y
                textos) lo escribes tú, en la lengua que quieras.
              </p>
            </div>
            <div className="muestrario">
              {VOCABULARY.map((g) => (
                <section key={g.group}>
                  <h3>{g.group}</h3>
                  <dl>
                    {g.words.map((w) => (
                      <div key={w.word}>
                        <dt>
                          <i data-h={w.hilo} aria-hidden="true" />
                          {w.word}
                        </dt>
                        <dd>{w.use}</dd>
                      </div>
                    ))}
                  </dl>
                </section>
              ))}
            </div>
            <p className="nota">
              La lista completa de funciones integradas está en la{" "}
              <a href={`${DOCS_URL}aymaraLang/`}>referencia rápida</a>.
            </p>
          </div>
        </section>

        <Franja source={FN} pass={14} />

        <section id="ejemplos" className="pampa">
          <div className="ancho">
            <div className="titular">
              <h2>Cada programa teje su propia figura.</h2>
              <p>
                Un archivo completo del repositorio y, a su lado, el mismo archivo como tejido: cada pasada es una
                línea de código y la sangría abre el centro. Toca o recorre una línea para encontrar su hilo.
              </p>
              <ul className="hilos" aria-label="Colores del tejido">
                {HILOS.map((x) => (
                  <li key={x.h}>
                    <i data-h={x.h} aria-hidden="true" />
                    {x.name}
                  </li>
                ))}
              </ul>
            </div>
            <Examples items={EXAMPLES.map((e) => ({ ...e, href: SOURCE_URL(e.path) }))} />
          </div>
        </section>

        <Franja source={FLOW} pass={14} />

        <section id="hecho" className="campo-azul">
          <div className="ancho">
            <div className="titular">
              <h2>Hecho con AymaraLang.</h2>
              <p>
                Diez programas que vienen en el repositorio, colgados como lo que son: juegos con ventana, juegos de
                consola y utilidades. El largo de cada tejido es el largo de su código. Ábrelos para leerlos.
              </p>
            </div>
            <ul className="tendedero">
              {WORKS.map((w) => (
                <li key={w.path}>
                  <a href={SOURCE_URL(w.path)}>
                    <span className="tendedero-rotulo">
                      <b>{w.name}</b>
                      <span>{w.kind}</span>
                      <span>{w.lines} líneas</span>
                    </span>
                    <Salta source={w.source} pass={2} columns={44} still label={`Tejido de ${w.path}`} />
                  </a>
                </li>
              ))}
            </ul>
          </div>
        </section>

        <Franja source={VARS} pass={14} />

        <section id="compila" className="pano">
          <div className="ancho">
            <div className="titular">
              <h2>De .aym a ejecutable, con un comando.</h2>
              <p>
                <code>aymc</code> está escrito desde cero en C++17. Valida tu programa, genera ensamblador y entrega un
                binario que corre solo, sin intérprete ni máquina virtual.
              </p>
            </div>
            <ol className="urdimbre">
              {PIPELINE.map((p) => (
                <li key={p.step}>
                  <b>{p.step}</b>
                  <code>{p.file}</code>
                  <span>{p.note}</span>
                </li>
              ))}
            </ol>
            <div className="consola">
              <pre className="codigo comandos">
                <code>
                  {COMMANDS.map((c, i) =>
                    c ? (
                      <span key={i} className="linea">
                        <span>{c[0]}</span>
                        {c[1] && <span data-h="c"># {c[1]}</span>}
                      </span>
                    ) : (
                      <span key={i} className="linea" aria-hidden="true" />
                    ),
                  )}
                </code>
              </pre>
              <p>
                Todas las opciones están en la <a href={`${DOCS_URL}compiler/`}>guía de aymc</a>.
              </p>
            </div>
          </div>
        </section>

        <Franja source={LOOP} pass={14} />

        <section id="descargar" className="campo-naranja">
          <div className="ancho">
            <div className="titular">
              <h2>Instálalo y escribe qallta.</h2>
              <p>
                Versión {RELEASE.version}, gratuita y de código abierto. Los archivos se sirven desde{" "}
                <a href={RELEASE.page}>GitHub Releases</a>.
              </p>
            </div>
            <div className="descargas">
              <div>
                <h3>Windows</h3>
                {winExe && (
                  <a href={winExe.url} className="boton">
                    <span>
                      Descargar instalador .exe
                      <small>
                        {winExe.name} · {mb(winExe.size)}
                      </small>
                    </span>
                    <Down />
                  </a>
                )}
                <ul>
                  <li>Incluye NASM y gcc (MinGW): no hay nada más que instalar.</li>
                  <li>
                    Agrega <code>aymc</code> y <code>aym</code> al PATH y asocia los archivos <code>.aym</code>.
                  </li>
                  {winMsi && (
                    <li>
                      Para despliegue administrado: <a href={winMsi.url}>{winMsi.name}</a> ({mb(winMsi.size)}).
                    </li>
                  )}
                </ul>
              </div>
              <div>
                <h3>Linux</h3>
                {linuxDeb && (
                  <a href={linuxDeb.url} className="boton">
                    <span>
                      Descargar paquete .deb
                      <small>
                        {linuxDeb.name} · {mb(linuxDeb.size)}
                      </small>
                    </span>
                    <Down />
                  </a>
                )}
                {linuxDeb && (
                  <pre className="codigo">
                    <code>{`sudo dpkg -i ${linuxDeb.name}\nsudo apt-get install -f -y`}</code>
                  </pre>
                )}
                <ul>
                  <li>
                    Usa <code>nasm</code>, <code>gcc</code> y <code>g++</code> del sistema, y X11 para los programas con
                    ventana.
                  </li>
                  <li>
                    ¿Otra distribución? <a href={`${DOCS_URL}build/`}>Compílalo desde el código fuente</a>.
                  </li>
                </ul>
              </div>
            </div>
            <p className="siguiente">
              Después de instalar, sigue el <a href={`${DOCS_URL}language/`}>tutorial rápido</a>: tu primer programa
              toma tres líneas.
            </p>
          </div>
        </section>
      </main>

      <Franja source={HELLO} pass={14} />
      <footer className="pie">
        <div className="ancho pie-fila">
          <p className="marca">
            <span>
              Aymara<b>Lang</b>
            </span>
          </p>
          <nav aria-label="Enlaces del pie">
            <a href={DOCS_URL}>Documentación</a>
            <a href={REPO_URL}>GitHub</a>
            <a href={`${REPO_URL}/blob/main/LICENSE`}>Licencia MIT</a>
            <a href={RELEASE.page}>Versión {RELEASE.version}</a>
          </nav>
          <a href="https://soryntel.com" className="pie-soryntel">
            Un producto de
            {/* eslint-disable-next-line @next/next/no-img-element */}
            <img src="/brand/soryntel.svg" alt="Soryntel" width="96" height="40" />
          </a>
        </div>
      </footer>
    </>
  );
}
