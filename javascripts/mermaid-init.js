function renderMermaid() {
  if (!window.mermaid) return;
  mermaid.initialize({ startOnLoad: false });
  mermaid.run({ querySelector: ".mermaid" });
}

document.addEventListener("DOMContentLoaded", renderMermaid);

// Para navegación “rápida”/cambios de página sin refresh completo
window.addEventListener("hashchange", renderMermaid);
window.addEventListener("popstate", renderMermaid);