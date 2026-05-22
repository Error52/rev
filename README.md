# Industrial-Hell Mail — Frontend Preview Only

Теперь это полностью статический frontend-сайт для просмотра дизайна.

## Быстрый запуск

### Вариант 1: открыть напрямую
- Откройте `index.html` в браузере.

### Вариант 2: локальный HTTP сервер (рекомендуется)
```bash
python3 -m http.server 8080
```
После этого откройте: `http://localhost:8080`

## Страницы
- `index.html` — login screen
- `dashboard.html` — webmail dashboard preview

## Структура
- `static/css/style.css` — все стили
- `templates/auth/login.html` и `templates/mail/dashboard.html` — дубли шаблонов для удобства
