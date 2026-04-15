from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path
from textwrap import wrap


PAGE_WIDTH = 595.0
PAGE_HEIGHT = 842.0
MARGIN_LEFT = 54.0
MARGIN_RIGHT = 54.0
MARGIN_TOP = 64.0
MARGIN_BOTTOM = 56.0
CONTENT_WIDTH = PAGE_WIDTH - MARGIN_LEFT - MARGIN_RIGHT


@dataclass
class Style:
    font: str
    size: float
    leading: float
    indent: float = 0.0
    bullet: str | None = None


STYLES = {
    "title": Style("F2", 22.0, 30.0),
    "h1": Style("F2", 18.0, 24.0),
    "h2": Style("F2", 14.0, 20.0),
    "h3": Style("F2", 12.0, 17.0),
    "body": Style("F1", 11.0, 15.0),
    "bullet": Style("F1", 11.0, 15.0, indent=16.0, bullet="- "),
    "number": Style("F1", 11.0, 15.0, indent=18.0),
    "code": Style("F3", 9.5, 12.5, indent=12.0),
    "footer": Style("F4", 9.0, 12.0),
}


def clean_inline(text: str) -> str:
    text = re.sub(r"\[([^\]]+)\]\([^)]+\)", r"\1", text)
    text = text.replace("`", "")
    text = text.replace("**", "")
    text = text.replace("*", "")
    return text.strip()


def normalize_pdf_text(text: str) -> str:
    text = text.replace("–", "-").replace("—", "-")
    text = text.replace("“", '"').replace("”", '"')
    text = text.replace("’", "'").replace("‘", "'")
    text = text.replace("\t", "    ")
    return text


def avg_char_width(style: Style) -> float:
    if style.font == "F3":
        return style.size * 0.58
    if style.font == "F2":
        return style.size * 0.56
    return style.size * 0.52


def wrap_text(text: str, style: Style, width: float) -> list[str]:
    text = normalize_pdf_text(clean_inline(text))
    if not text:
        return [""]
    max_chars = max(12, int(width / avg_char_width(style)))
    return wrap(text, width=max_chars, break_long_words=False, break_on_hyphens=False) or [text]


def parse_markdown(markdown: str) -> list[tuple[str, str]]:
    elements: list[tuple[str, str]] = []
    lines = markdown.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        stripped = line.strip()
        if not stripped:
            elements.append(("blank", ""))
            i += 1
            continue
        if stripped.startswith("```"):
            fence = stripped
            i += 1
            code_lines: list[str] = []
            while i < len(lines) and not lines[i].strip().startswith("```"):
                code_lines.append(lines[i].rstrip())
                i += 1
            elements.append(("code", "\n".join(code_lines)))
            if i < len(lines) and lines[i].strip().startswith("```"):
                i += 1
            continue
        if stripped.startswith("### "):
            elements.append(("h3", stripped[4:].strip()))
            i += 1
            continue
        if stripped.startswith("## "):
            elements.append(("h2", stripped[3:].strip()))
            i += 1
            continue
        if stripped.startswith("# "):
            elements.append(("h1", stripped[2:].strip()))
            i += 1
            continue
        if re.match(r"^\d+\.\s+", stripped):
            elements.append(("number", stripped))
            i += 1
            continue
        if stripped.startswith("- "):
            elements.append(("bullet", stripped[2:].strip()))
            i += 1
            continue

        para_lines = [stripped]
        i += 1
        while i < len(lines):
            nxt = lines[i].strip()
            if not nxt or nxt.startswith("#") or nxt.startswith("```") or nxt.startswith("- ") or re.match(r"^\d+\.\s+", nxt):
                break
            para_lines.append(nxt)
            i += 1
        elements.append(("body", " ".join(para_lines)))
    return elements


def escape_pdf_bytes(text: str) -> bytes:
    raw = normalize_pdf_text(text).encode("cp1252", errors="replace")
    raw = raw.replace(b"\\", b"\\\\").replace(b"(", b"\\(").replace(b")", b"\\)")
    return raw


class PdfBuilder:
    def __init__(self) -> None:
        self.pages: list[bytes] = []
        self.current: list[bytes] = []
        self.y = PAGE_HEIGHT - MARGIN_TOP
        self.page_number = 0
        self.new_page()

    def new_page(self) -> None:
        if self.current:
            self.finish_page()
        self.current = []
        self.y = PAGE_HEIGHT - MARGIN_TOP
        self.page_number += 1

    def finish_page(self) -> None:
        footer = f"Pagina {self.page_number}"
        self.current.append(self.text_line(footer, STYLES["footer"], MARGIN_LEFT, 28.0))
        stream = b"\n".join(self.current)
        self.pages.append(stream)

    def ensure_space(self, needed: float) -> None:
        if self.y - needed < MARGIN_BOTTOM:
            self.new_page()

    def text_line(self, text: str, style: Style, x: float, y: float) -> bytes:
        return (
            b"BT\n"
            + f"/{style.font} {style.size:.1f} Tf\n".encode("ascii")
            + f"1 0 0 1 {x:.1f} {y:.1f} Tm\n".encode("ascii")
            + b"("
            + escape_pdf_bytes(text)
            + b") Tj\nET"
        )

    def add_block(self, kind: str, text: str) -> None:
        if kind == "blank":
            self.y -= 8.0
            return

        style = STYLES[kind]
        block_width = CONTENT_WIDTH - style.indent
        x = MARGIN_LEFT + style.indent

        if kind == "number":
            prefix, body = text.split(". ", 1)
            wrapped = wrap_text(body, style, block_width - 18.0)
            needed = style.leading * len(wrapped) + 6.0
            self.ensure_space(needed)
            for idx, line in enumerate(wrapped):
                current_x = x
                current_line = line
                if idx == 0:
                    current_x = MARGIN_LEFT
                    current_line = f"{prefix}. {line}"
                self.current.append(self.text_line(current_line, style, current_x, self.y))
                self.y -= style.leading
            self.y -= 4.0
            return

        if kind == "code":
            code_lines = text.splitlines() or [""]
            wrapped_lines: list[str] = []
            for line in code_lines:
                wrapped_lines.extend(wrap_text(line or " ", style, block_width))
            needed = style.leading * len(wrapped_lines) + 8.0
            self.ensure_space(needed)
            for line in wrapped_lines:
                self.current.append(self.text_line(line, style, x, self.y))
                self.y -= style.leading
            self.y -= 4.0
            return

        wrapped = wrap_text(text, style, block_width)
        needed = style.leading * len(wrapped) + 8.0
        self.ensure_space(needed)
        for idx, line in enumerate(wrapped):
            line_text = line
            current_x = x
            if idx == 0 and style.bullet:
                line_text = f"{style.bullet}{line}"
                current_x = MARGIN_LEFT
            self.current.append(self.text_line(line_text, style, current_x, self.y))
            self.y -= style.leading
        self.y -= 4.0

    def build(self, output_path: Path) -> None:
        if self.current:
            self.finish_page()

        objects: list[bytes] = []
        objects.append(b"<< /Type /Catalog /Pages 2 0 R >>")

        page_kids = " ".join(f"{index} 0 R" for index in range(3, 3 + len(self.pages))).encode("ascii")
        objects.append(
            b"<< /Type /Pages /Count "
            + str(len(self.pages)).encode("ascii")
            + b" /Kids [ "
            + page_kids
            + b" ] >>"
        )

        content_start = 3 + len(self.pages)
        for idx, _page in enumerate(self.pages):
            content_obj = content_start + idx
            objects.append(
                b"<< /Type /Page /Parent 2 0 R /MediaBox [0 0 595 842] "
                b"/Resources << /Font << /F1 "
                + str(content_start + len(self.pages)).encode("ascii")
                + b" 0 R /F2 "
                + str(content_start + len(self.pages) + 1).encode("ascii")
                + b" 0 R /F3 "
                + str(content_start + len(self.pages) + 2).encode("ascii")
                + b" 0 R /F4 "
                + str(content_start + len(self.pages) + 3).encode("ascii")
                + b" 0 R >> >> /Contents "
                + str(content_obj).encode("ascii")
                + b" 0 R >>"
            )

        for page in self.pages:
            objects.append(b"<< /Length " + str(len(page)).encode("ascii") + b" >>\nstream\n" + page + b"\nendstream")

        objects.append(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>")
        objects.append(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Bold /Encoding /WinAnsiEncoding >>")
        objects.append(b"<< /Type /Font /Subtype /Type1 /BaseFont /BaseFont /Courier /Encoding /WinAnsiEncoding >>".replace(b"/BaseFont /BaseFont", b"/BaseFont"))
        objects.append(b"<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica-Oblique /Encoding /WinAnsiEncoding >>")

        pdf = bytearray(b"%PDF-1.4\n%\xe2\xe3\xcf\xd3\n")
        offsets = [0]
        for number, obj in enumerate(objects, start=1):
            offsets.append(len(pdf))
            pdf.extend(f"{number} 0 obj\n".encode("ascii"))
            pdf.extend(obj)
            pdf.extend(b"\nendobj\n")

        xref_start = len(pdf)
        pdf.extend(f"xref\n0 {len(offsets)}\n".encode("ascii"))
        pdf.extend(b"0000000000 65535 f \n")
        for offset in offsets[1:]:
            pdf.extend(f"{offset:010d} 00000 n \n".encode("ascii"))
        pdf.extend(
            b"trailer\n<< /Size "
            + str(len(offsets)).encode("ascii")
            + b" /Root 1 0 R >>\nstartxref\n"
            + str(xref_start).encode("ascii")
            + b"\n%%EOF"
        )

        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_bytes(pdf)


def build_manual(markdown_path: Path, output_path: Path) -> None:
    content = markdown_path.read_text(encoding="utf-8")
    elements = parse_markdown(content)

    pdf = PdfBuilder()
    first_heading = True
    for kind, text in elements:
        if kind == "h1" and first_heading:
            pdf.add_block("title", text)
            pdf.add_block("body", "Manual unificado de usuario y documentacion esencial.")
            pdf.add_block("body", "Incluye instalacion, tutorial, referencia practica y uso de herramientas.")
            pdf.add_block("blank", "")
            first_heading = False
            continue
        pdf.add_block(kind, text)

    pdf.build(output_path)


def main() -> None:
    parser = argparse.ArgumentParser(description="Genera un PDF simple a partir del manual Markdown.")
    parser.add_argument(
        "--input",
        default="docs/manual_pdf.md",
        help="Archivo Markdown fuente del manual.",
    )
    parser.add_argument(
        "--output",
        default="artifacts/docs/AymaraLang-Manual-y-Documentacion.pdf",
        help="Ruta del PDF de salida.",
    )
    args = parser.parse_args()

    build_manual(Path(args.input), Path(args.output))


if __name__ == "__main__":
    main()
