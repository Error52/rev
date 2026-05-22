# Industrial-Hell Mail

Self-hosted cyberpunk mail platform on **Django + Postfix + Dovecot + PostgreSQL** with Docker deployment.

## Features
- Admin-only user provisioning (no public signup).
- SMTP send through Postfix.
- IMAP/POP3 storage with Dovecot (Maildir).
- Webmail folders: Inbox/Sent/Drafts/Spam/Trash/Starred.
- Search by subject/sender/content.
- Attachments support and REST API.
- Realtime notifications via Django Channels (WebSocket).
- Celery + Redis for async jobs.
- Hardened defaults: CSRF, secure cookies, CSP, HSTS, rate limiting.

## Run (Linux)
```bash
cp .env.example .env
docker compose up -d --build
docker compose exec web python manage.py migrate
docker compose exec web python manage.py createsuperuser
```
Open `http://localhost`.

## Architecture
- `web` Django ASGI app
- `nginx` reverse proxy
- `db` PostgreSQL
- `redis` broker/channel layer
- `celery` workers
- `postfix` SMTP
- `dovecot` IMAP/POP3 + LMTP

## Admin policy
Users must be created/managed only in Django Admin (`/admin`). Frontend registration is intentionally absent.

## Production notes
- Replace domains, TLS certificates, and secrets.
- Add fail2ban on host (`nginx`, `postfix`, `dovecot` jails).
- Configure SPF/DKIM/DMARC externally.
- Mount persistent volumes for mail and DB.
