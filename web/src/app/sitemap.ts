import type { MetadataRoute } from "next";
import { RELEASE, SITE_URL } from "@/content/site";

export const dynamic = "force-static";

// La página cambia cuando se publica una versión: esa es su fecha de modificación.
export default function sitemap(): MetadataRoute.Sitemap {
  return [{ url: `${SITE_URL}/`, lastModified: RELEASE.publishedAt, changeFrequency: "monthly", priority: 1 }];
}
