(() => {
  const scrollToTop = () => {
    if (!window.location.hash) {
      window.scrollTo({ top: 0, left: 0, behavior: "auto" });
    }

    const nav = document.querySelector(".wy-nav-side");
    if (nav) {
      nav.scrollTop = 0;
    }
    const navScroll = document.querySelector(".wy-side-scroll");
    if (navScroll) {
      navScroll.scrollTop = 0;
    }
  };

  window.addEventListener("load", () => {
    scrollToTop();
    requestAnimationFrame(scrollToTop);
    setTimeout(scrollToTop, 100);
  });

  window.addEventListener("pageshow", scrollToTop);
  window.addEventListener("hashchange", scrollToTop);

  document.addEventListener("click", (event) => {
    const link = event.target.closest(".wy-nav-side a");
    if (link) {
      requestAnimationFrame(scrollToTop);
    }
  });
})();
