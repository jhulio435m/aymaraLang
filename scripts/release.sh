#!/usr/bin/env bash
# Publica una version de AymaraLang.
#
#   scripts/release.sh 1.1.0
#
# Actualiza VERSION.txt, crea el commit "release: v1.1.0" y la etiqueta v1.1.0
# en main, y los sube. Al llegar la etiqueta a GitHub, .github/workflows/release.yml
# compila y prueba todo, publica la release con los instaladores (.exe, .msi,
# .deb) y despliega la web con los nuevos enlaces de descarga.
set -euo pipefail

usage() {
  echo "uso: scripts/release.sh X.Y.Z" >&2
  exit 2
}

[[ $# -eq 1 ]] || usage
version="${1#v}"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "version invalida: $1 (se espera X.Y.Z)" >&2; exit 2; }
tag="v${version}"

root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$root"

branch="$(git rev-parse --abbrev-ref HEAD)"
[[ "$branch" == "main" ]] || { echo "las versiones se publican desde main (rama actual: $branch)" >&2; exit 1; }

if [[ -n "$(git status --porcelain --untracked-files=no)" ]]; then
  echo "hay cambios sin commit; confirma o descarta antes de publicar" >&2
  exit 1
fi

git fetch --quiet --tags origin
if [[ "$(git rev-parse HEAD)" != "$(git rev-parse origin/main)" ]]; then
  echo "main local no coincide con origin/main; sincroniza antes (git pull / git push)" >&2
  exit 1
fi

if git rev-parse -q --verify "refs/tags/${tag}" >/dev/null; then
  echo "la etiqueta ${tag} ya existe" >&2
  exit 1
fi

latest="$(git tag --list 'v*.*.*' --sort=-v:refname | head -n1)"
if [[ -n "$latest" ]] && [[ "$(printf '%s\n%s\n' "${latest#v}" "$version" | sort -V | tail -n1)" != "$version" ]]; then
  echo "${tag} no es mayor que la ultima version publicada (${latest})" >&2
  exit 1
fi

current="$(tr -d '\r\n' < VERSION.txt)"
echo "Version: ${current} -> ${version}  (ultima etiqueta: ${latest:-ninguna})"
read -r -p "Publicar ${tag}? [s/N] " answer
[[ "$answer" =~ ^[sS]$ ]] || { echo "cancelado"; exit 1; }

printf '%s\n' "$version" > VERSION.txt
git add VERSION.txt
git commit --quiet -m "release: ${tag}"
git tag -a "$tag" -m "AymaraLang ${tag}"
git push --quiet origin main
git push --quiet origin "$tag"

echo "Publicado ${tag}. Sigue el progreso en:"
echo "  https://github.com/jhulio435m/aymaraLang/actions/workflows/release.yml"
