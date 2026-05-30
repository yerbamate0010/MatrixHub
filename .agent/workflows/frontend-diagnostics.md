---
description: Run comprehensive frontend diagnostics (lint, type-check, tests, size, deps)
---

// turbo-all

1. Navigate to the interface directory
   - Command: `cd interface`

2. Check code style and linting
   - Command: `npm run lint`

3. Verify TypeScript types and Svelte components
   - Command: `npm run check`

4. Run unit tests
   - Command: `npm run test:run`

5. Check production build size budgets
   - Command: `npm run size`

6. Analyze dependency graph constraints
   - Command: `npm run deps:check`
