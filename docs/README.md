# MatrixHub Documentation

This directory keeps the current, maintained documentation for MatrixHub. It is
intentionally small: dated plans, checkpoint patches, diagnostic captures, and
screenshot-heavy walkthroughs are kept out of docs so the remaining files can
act as source-of-truth references.

## Start Here

- [Engineering reference](engineering/README.md) - firmware architecture,
  operations, integrations, and validation gates.
- [Agent workflows](agent-workflows/README.md) - task-focused workflows linked
  from the root `AGENTS.md`.
- [User guide](user-guide/README.md) - current UI route map and short operator
  notes without static screenshot walkthroughs.
- [Build speed report](main_docs/BUILD_SPEED_OPTIMIZATION.md) - measured
  PlatformIO build optimization notes for this Raspberry Pi 5 host.

## Maintenance Rules

- Keep documentation tied to current code paths and commands.
- Do not commit generated reports, Playwright traces, one-off patches, or dated
  diagnostics as documentation.
- Prefer one concise maintained page over many stale pages.
- If a doc mentions an endpoint, keep `docs/engineering/api-contract.json` and
  `scripts/contract/verify_api_contract.py` in sync.
