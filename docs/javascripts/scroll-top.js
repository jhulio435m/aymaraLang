(() => {
  const scrollToTop = () => {
    if (!window.location.hash) {
      window.scrollTo({ top: 0, left: 0, behavior: "auto" });
    }

    const nav = document.querySelector(".wy-nav-side");
    if (nav) {
      nav.scrollTop = 0;
    }
  };

  window.addEventListener("load", () => {
    scrollToTop();
    requestAnimationFrame(scrollToTop);
  });

  window.addEventListener("pageshow", scrollToTop);
  window.addEventListener("hashchange", scrollToTop);
})();
