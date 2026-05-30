# Engineering Reference

This section is for developers and maintainers. It groups technical reference
material that supports implementation, debugging, and validation.

## Runtime and architecture

- [architecture/architecture.md](architecture/architecture.md) - firmware module map
- [architecture/boot_sequence.md](architecture/boot_sequence.md) - startup flow and phase ordering
- [architecture/configuration_system.md](architecture/configuration_system.md) - compile-time vs runtime configuration
- [architecture/rtc_persistence.md](architecture/rtc_persistence.md) - retained config and deep-sleep restore model
- [architecture/power_modes.md](architecture/power_modes.md) - power and sleep subsystem behavior
- [architecture/matrix_manager.md](architecture/matrix_manager.md) - matrix display layering and queue orchestration

## Integrations and specialized subsystems

- [integrations/bluetooth_overview.md](integrations/bluetooth_overview.md) - scanner-only BLE model
- [integrations/csi.md](integrations/csi.md) - Wi-Fi CSI runtime path
- [integrations/telegram_integration.md](integrations/telegram_integration.md) - Telegram runtime model, delivery path, and TLS policy
- [integrations/telegram_commands.md](integrations/telegram_commands.md) - Telegram command registry, polling rules, and extension notes

## Operations

- [operations/security_hardening.md](operations/security_hardening.md) - security model and hardening notes
- [operations/testing.md](operations/testing.md) - backend and frontend validation
- [operations/dram_optimizations.md](operations/dram_optimizations.md) - DRAM usage guidance
- [operations/hygienic_sleep.md](operations/hygienic_sleep.md) - hygiene sleep and maintenance restart flow

## Planning

- [planning/ROADMAP.md](planning/ROADMAP.md) - active engineering backlog
