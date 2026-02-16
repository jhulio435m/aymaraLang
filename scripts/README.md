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
```

```bash
bash scripts/install/install_deps_linux.sh
bash scripts/build/build_deb.sh
bash scripts/test/test_deb.sh
bash scripts/test/test_deb_e2e.sh
bash scripts/test/test_samples.sh
```
