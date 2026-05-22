import os
from pathlib import Path
BASE_DIR = Path(__file__).resolve().parent.parent
SECRET_KEY = os.getenv('DJANGO_SECRET_KEY', 'dev-secret')
DEBUG = os.getenv('DJANGO_DEBUG', '0') == '1'
ALLOWED_HOSTS = os.getenv('DJANGO_ALLOWED_HOSTS', '*').split(',')

INSTALLED_APPS = [
    'django.contrib.admin','django.contrib.auth','django.contrib.contenttypes','django.contrib.sessions','django.contrib.messages','django.contrib.staticfiles',
    'rest_framework','django_filters','channels','apps.core','apps.mail'
]
MIDDLEWARE = [
    'django.middleware.security.SecurityMiddleware','django.contrib.sessions.middleware.SessionMiddleware','django.middleware.common.CommonMiddleware',
    'django.middleware.csrf.CsrfViewMiddleware','django.contrib.auth.middleware.AuthenticationMiddleware','django.contrib.messages.middleware.MessageMiddleware',
    'django.middleware.clickjacking.XFrameOptionsMiddleware',
]
ROOT_URLCONF = 'config.urls'
TEMPLATES = [{
    'BACKEND':'django.template.backends.django.DjangoTemplates',
    'DIRS':[BASE_DIR / 'templates'],
    'APP_DIRS':True,
    'OPTIONS':{'context_processors':['django.template.context_processors.request','django.contrib.auth.context_processors.auth','django.contrib.messages.context_processors.messages']}
}]
WSGI_APPLICATION='config.wsgi.application'
ASGI_APPLICATION='config.asgi.application'
DATABASES={'default':{'ENGINE':'django.db.backends.postgresql','NAME':os.getenv('POSTGRES_DB','industrial_hell_mail'),'USER':os.getenv('POSTGRES_USER','ihm'),'PASSWORD':os.getenv('POSTGRES_PASSWORD','ihm'),'HOST':os.getenv('POSTGRES_HOST','db'),'PORT':os.getenv('POSTGRES_PORT','5432')}}
AUTH_PASSWORD_VALIDATORS=[{'NAME':'django.contrib.auth.password_validation.UserAttributeSimilarityValidator'},{'NAME':'django.contrib.auth.password_validation.MinimumLengthValidator'}]
LANGUAGE_CODE='en-us'; TIME_ZONE='UTC'; USE_I18N=True; USE_TZ=True
STATIC_URL='/static/'; STATIC_ROOT=BASE_DIR/'staticfiles'; STATICFILES_DIRS=[BASE_DIR/'static']
DEFAULT_AUTO_FIELD='django.db.models.BigAutoField'
LOGIN_URL='login'; LOGIN_REDIRECT_URL='mail:dashboard'; LOGOUT_REDIRECT_URL='login'
SESSION_COOKIE_SECURE=True; CSRF_COOKIE_SECURE=True; SESSION_COOKIE_HTTPONLY=True
SECURE_BROWSER_XSS_FILTER=True; SECURE_CONTENT_TYPE_NOSNIFF=True; X_FRAME_OPTIONS='DENY'
SECURE_HSTS_SECONDS=31536000; SECURE_HSTS_INCLUDE_SUBDOMAINS=True; SECURE_HSTS_PRELOAD=True
CSP_DEFAULT_SRC=("'self'",); CSP_STYLE_SRC=("'self'", "'unsafe-inline'"); CSP_SCRIPT_SRC=("'self'", "'unsafe-inline'")
EMAIL_BACKEND='django.core.mail.backends.smtp.EmailBackend'; EMAIL_HOST=os.getenv('SMTP_HOST','postfix'); EMAIL_PORT=int(os.getenv('SMTP_PORT','587')); EMAIL_USE_TLS=True; EMAIL_HOST_USER=os.getenv('SMTP_USER',''); EMAIL_HOST_PASSWORD=os.getenv('SMTP_PASSWORD','')
CELERY_BROKER_URL=os.getenv('CELERY_BROKER_URL','redis://redis:6379/0'); CELERY_RESULT_BACKEND=os.getenv('CELERY_RESULT_BACKEND','redis://redis:6379/1')
CHANNEL_LAYERS={'default':{'BACKEND':'channels_redis.core.RedisChannelLayer','CONFIG':{'hosts':[os.getenv('REDIS_URL','redis://redis:6379/2')]}}}
REST_FRAMEWORK={'DEFAULT_AUTHENTICATION_CLASSES':['rest_framework.authentication.SessionAuthentication'],'DEFAULT_PERMISSION_CLASSES':['rest_framework.permissions.IsAuthenticated'],'DEFAULT_FILTER_BACKENDS':['django_filters.rest_framework.DjangoFilterBackend','rest_framework.filters.SearchFilter','rest_framework.filters.OrderingFilter']}
