Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Operations](../README.md#operations)

# Security Hardening Documentation

This document outlines the security measures implemented to protect the ESP32 platform when exposed to the internet. The focus is on session security, DoS protection, and authentication integrity.

## 1. Authentication & Session Security

### Access Model
The firmware exposes only three effective access levels:

| Level | Backend meaning | Typical examples |
| --- | --- | --- |
| `public` | Endpoint is not wrapped by auth middleware | Static frontend, `POST /rest/signIn`, `GET /rest/features` |
| `authenticated` | Any logged-in user, regardless of `admin` flag | Read-only status/config like `GET /api/logs`, `GET /api/alarms/rules`, `GET /api/ble/status`, `GET /api/shelly/devices`, `GET /api/matrix/settings`, JWT-protected `/ws/system` and `/ws/csi` |
| `admin` | Logged-in user with `admin=true` | Mutating settings, operational actions, scans, deletes, uploads, and control endpoints |

Important semantic note:
- There is no separate backend `guest` role.
- A non-admin account is still an `authenticated` user.
- If the dashboard intentionally hides some authenticated read-only areas from non-admin users, that should be treated as a UI/product restriction, not as a distinct firmware role.

Legacy note:
- `GET /rest/generateToken` and the empty-URI `DELETE` exception in `filterRequest()` are compatibility-era paths. Preserve them only where required; do not copy them into new endpoints.

### 🛡️ Countermeasure against Timing Attacks
Modified `ArduinoJsonJWT.cpp` to use a **constant-time memory comparison** for signature verification. This prevents attackers from guessing valid signatures one byte at a time by measuring server response times.

### 🔑 JWT Secret Lifecycle
Normal runtime now persists a randomly generated JWT signing secret instead of
staying on the factory placeholder. `SecuritySettings::update()` resolves a new
secret from `esp_fill_random()` when config does not provide one yet, then
stores it as part of the saved security settings.

`JwtTokenService` still has an in-memory fallback when no persisted secret is
available yet, but that is bootstrap behavior, not the normal user-facing
"volatile mode" contract.

### ⌛ Session Lifetime
JWTs currently carry `iat` for revocation checks, but they no longer include an
`exp` claim. The frontend also does not maintain a separate local expiry model.

In practice, sessions remain valid until one of these happens:

- the user is explicitly signed out
- the backend rejects the token and the UI invalidates the session
- the JWT secret changes
- `validAfter` revokes older tokens for that user

### 🚫 Instant Token Revocation
Implemented a robust revocation mechanism using `iat` (Issued At) and `validAfter` timestamps:
- **User State Tracking**: Each user has a `validAfter` timestamp.
- **Payload Validation**: JWTs include an `iat` claim. Tokens are rejected if `iat < validAfter`.
- **Trigger**: Changing a password or administrative status immediately updates `validAfter`, instantly invalidating all existing tokens for that user.

## 2. Denial of Service (DoS) Protection

### 🚦 Advanced Rate Limiting (Token Bucket)
Replaced fixed-window limiting with the **Token Bucket** algorithm for smoother traffic shaping.
- **Global Rate Limiter**: Limits general API traffic (1200 req/60s).
- **Login Rate Limiter**: Protects against brute-force (3 attempts/60s).
- **LRU-like Eviction**: When the client capacity (1000 IPs) is reached, the system purges the oldest inactive entries to make room for new legitimate traffic, preventing IP-spoofing lockout.

### 📦 Payload Size Mitigation
- **Global Limit**: `PsychicHttpServer` is configured with a 16KB `maxRequestBodySize`.
- **Middleware Guard**: API wrappers check `Content-Length` before processing, performing "fail-fast" rejection of oversized payloads to protect DRAM/PSRAM.

### 🕵️ Slowloris Protection
Configured aggressive socket timeouts (`recv_wait_timeout: 5s`). This prevents attackers from holding connection slots open indefinitely by sending data extremely slowly.

## 3. Resilience & Recovery

### 🆘 Dev-Phase Admin Recovery
Current code keeps a deterministic recovery admin as a development-phase
exception. When a security-settings update would leave the configured user list
without an administrator, `SecuritySettings` appends the default
`admin/admin` recovery user instead of failing closed.

That behavior is intentionally documented as a dev-only compromise in the code
comments and should not be described as a finished production recovery design.

### 🔒 Fail-Closed Policy
Critical sections (like the Rate Limiter mutex) use a short timeout (100ms). If a lock cannot be acquired during a heavy attack, the system **fails closed**, rejecting the request rather than hanging the server threads ("thread starvation").

## 4. Standardized Logging
All security events are logged using the project's internal logging system (`LOGW`, `LOGI`, `LOGE`) with the `Security` tag for easy monitoring and auditing via the serial console.

Navigation: [Project README](../../../README.md) · [Engineering Reference](../README.md) · [Operations](../README.md#operations)
