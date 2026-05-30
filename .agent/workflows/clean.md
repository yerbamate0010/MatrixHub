---
description: Deep clean project artifacts and dependencies (PlatformIO + Node.modules)
---

# Deep Clean Project

Use this workflow to reset the environment if you encounter weird build errors or dependency issues.

// turbo-all

1. Clean PlatformIO build artifacts:
   ```bash
   pio run -t clean
   ```

2. Remove PlatformIO internal folder (optional, but recommended for deep clean):
   ```bash
   rm -rf .pio
   ```

3. Clean Interface (Frontend) artifacts and dependencies:
   ```bash
   cd interface && rm -rf node_modules build .svelte-kit
   ```

4. Re-install Interface dependencies:
   ```bash
   cd interface && npm install
   ```
