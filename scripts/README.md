# Scripts

This folder is organized by purpose:

- `scripts/build/`: build and packaging helpers.
- `scripts/install/`: dependency installers.
- `scripts/test/`: local validation helpers.

Common commands:

```powershell
pwsh -File .\scripts\install\install_deps_windows.ps1
pwsh -File .\scripts\build\build_dist.ps1 -Config Release
pwsh -File .\scripts\build\build_msi.ps1
pwsh -File .\scripts\build\build_nsis.ps1
pwsh -File .\scripts\publish_extension.ps1 -Target package
pwsh -File .\scripts\publish_extension.ps1 -Target all
```

```bash
bash scripts/install/install_deps_linux.sh
bash scripts/build/build_deb.sh
bash scripts/test/test_deb.sh
bash scripts/test/test_deb_e2e.sh
bash scripts/test/test_samples.sh
```

Extension publishing uses:

- `VSCE_PAT` for VS Code Marketplace
- `OVSX_PAT` for Open VSX

Windows signing environment variables:

- `AYM_SIGN_PFX_PATH`
- `AYM_SIGN_PFX_PASSWORD`
- `AYM_SIGN_CERT_THUMBPRINT`
- `AYM_SIGN_TIMESTAMP_URL`
- `AYM_SIGNTOOL_PATH`
- `AYM_SIGN_CERT_MACHINE_STORE`

To require signing during packaging:

```powershell
pwsh -File .\scripts\build\build_dist.ps1 -Config Release -RequireSigning
pwsh -File .\scripts\build\build_nsis.ps1 -DistDir dist -OutputDir artifacts\release-windows -RequireSigning
pwsh -File .\scripts\build\build_msi.ps1 -DistDir dist -OutputDir artifacts\release-windows -RequireSigning
```
