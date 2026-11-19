import type { Metadata, Viewport } from "next";
import "@fontsource-variable/anybody/wdth.css";
import "@fontsource-variable/martian-mono/wdth.css";
import "@fontsource-variable/schibsted-grotesk";
import { DESCRIPTION, KEYWORDS, SITE_URL, TITLE } from "@/content/site";
import "./globals.css";

export const metadata: Metadata = {
  metadataBase: new URL(SITE_URL),
  title: { default: TITLE, template: "%s · AymaraLang" },
  description: DESCRIPTION,
  applicationName: "AymaraLang",
  keywords: KEYWORDS,
  category: "technology",
  authors: [{ name: "Soryntel", url: "https://soryntel.com" }],
  creator: "Soryntel",
  publisher: "Soryntel",
  alternates: { canonical: "/" },
  robots: {
    index: true,
    follow: true,
    googleBot: { index: true, follow: true, "max-image-preview": "large", "max-snippet": -1, "max-video-preview": -1 },
  },
  formatDetection: { telephone: false, email: false, address: false },
  openGraph: {
    type: "website",
    locale: "es_PE",
    url: "/",
    siteName: "AymaraLang",
    title: TITLE,
    description: DESCRIPTION,
    images: [{ url: "/og.png", width: 1200, height: 630, alt: "AymaraLang: programa en aymara, compila a nativo" }],
  },
  twitter: { card: "summary_large_image", title: TITLE, description: DESCRIPTION, images: ["/og.png"] },
};

export const viewport: Viewport = { themeColor: "#f6efe2" };

export default function RootLayout({ children }: { children: React.ReactNode }) {
  return (
    <html lang="es">
      <body>{children}</body>
    </html>
  );
}
