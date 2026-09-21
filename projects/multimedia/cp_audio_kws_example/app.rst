CP Audio KWS Example
====================

This project is a CP-first validation project for BK7259 low-power audio work.

The project keeps the AP/CP baseline close to the existing audio example so
CP audio service, LPVAD, and KWS verification can share one board/configuration
environment.

The CP KWS smoke CLI is registered by default in stub mode. The CP KWS
algorithm library is intentionally not linked because the currently imported
CPU KWS archive is an AP/M55 hard-float/MVE build, while the CP image uses the
default M52 soft-float ABI. Replace the stub only after importing a
CP-compatible KWS archive or deliberately changing the CP toolchain ABI and its
dependent libraries.
