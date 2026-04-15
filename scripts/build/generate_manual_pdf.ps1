param(
    [string]$InputFile = "docs/manual_pdf.md",
    [string]$OutputFile = "artifacts/docs/AymaraLang-Manual-y-Documentacion.pdf"
)

python .\scripts\build\generate_manual_pdf.py --input $InputFile --output $OutputFile
