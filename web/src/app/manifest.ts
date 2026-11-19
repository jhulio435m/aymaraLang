import type { MetadataRoute } from "next";
import { DESCRIPTION } from "@/content/site";

export const dynamic = "force-static";

export default function manifest(): MetadataRoute.Manifest {
  return {
    name: "AymaraLang",
    short_name: "AymaraLang",
    description: DESCRIPTION,
    lang: "es",
    start_url: "/",
    display: "browser",
    background_color: "#f6efe2",
    theme_color: "#22112a",
    icons: [
      { src: "/icon.png", sizes: "64x64", type: "image/png" },
      { src: "/apple-icon.png", sizes: "180x180", type: "image/png" },
    ],
  };
}
