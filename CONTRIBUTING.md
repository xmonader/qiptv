# Contributing to qiptv

Thanks for your interest in contributing! qiptv is a small, focused Qt/C++
desktop IPTV client, and contributions of all kinds are welcome.

## Getting started

1. Fork the repository and clone your fork.
2. Install prerequisites (see [README.md](README.md)): a C++17 compiler,
   CMake >= 3.20, Qt 6.5+, and `libmpv`. For packaging work you'll also want
   `desktop-file-validate` and `appstreamcli`.
3. Build and test:

   ```bash
   make build
   make test
   ```

4. Create a feature branch off `main`:

   ```bash
   git checkout -b feature/my-change
   ```

## Development workflow

- Follow the micro-commit discipline: small, focused, working commits.
- Run `make build && make test` before pushing. CI runs the same.
- Run `make lint` (clang-tidy) when available and fix new warnings.
- Keep the diff minimal - only change what the issue/PR requires.

## Code style

- C++17 with Qt idioms (`QObject`, signals/slots, `QSettings`).
- Strict string handling is enforced (`QT_NO_CAST_FROM_ASCII`, etc.) - use
  explicit conversions rather than implicit casts.
- One responsibility per function; name things so intent is obvious.
- Add tests for new or changed behavior in `tests/` (QtTest). UI-free logic
  belongs in the `qiptv_core` library so it can be unit-tested.

## Pull requests

- Open against `main`.
- Fill out the PR template.
- Describe *what* and *why*; link the related issue.
- Keep PRs small and reviewable. Split large refactors.

## Reporting bugs

Use the **Bug report** issue template. Include reproduction steps, expected
vs. actual behavior, and your environment (OS, Qt version, mpv version).

## Code of Conduct

This project follows the [Contributor Covenant](CODE_OF_CONDUCT.md). By
participating you agree to uphold it.
