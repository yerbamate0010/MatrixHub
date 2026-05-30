# File Manager Service

Standardised helpers for the dashboard file manager.

- `api.ts` – orchestrated actions for listing, uploads, downloads, and system state probing.
- `utils.ts` – pure utilities (state derivation, label helpers).
- `types.ts` – public types consumed by stores and components.

Consumers should import from `$lib/services/fileManager` to access the barrel exports.
