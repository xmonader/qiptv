# Security Policy

## Supported Versions

| Version | Supported |
| ------- | --------- |
| 0.1.x   | ✅        |

Only the latest 0.1.x line receives security fixes. Upgrade to the newest
release before reporting an issue that may already be fixed.

## Reporting a Vulnerability

If you discover a security vulnerability in qiptv, **please do not open a
public issue**. Instead, email the maintainer at **xmonader@gmail.com** with:

- A description of the vulnerability and its impact
- Steps to reproduce, or a proof of concept
- The affected versions, if known

You will receive an acknowledgment within a reasonable time. Once the issue is
confirmed and a fix is available, a coordinated disclosure will be published
and the reporter credited (unless they prefer to remain anonymous).

## Scope

qiptv is a desktop client that:

- Stores provider credentials locally in `~/.config/qiptvrc` via `QSettings`.
  Passwords are **obfuscated, not cryptographically secured** - treat any
  saved profile as sensitive.
- Sends network requests to user-supplied Xtream Codes servers over HTTP(S).

**Never commit real provider credentials.** The repository ignores `*.local`
and `secrets.*` files; keep it that way. Do not paste live server URLs,
usernames, or passwords into issues, PRs, or logs.
