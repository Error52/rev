from django.contrib import admin
from django.urls import path, include
from apps.core.views import LoginView
urlpatterns = [
    path('admin/', admin.site.urls),
    path('', LoginView.as_view(), name='login'),
    path('accounts/', include('django.contrib.auth.urls')),
    path('mail/', include('apps.mail.urls')),
    path('api/', include('apps.mail.api_urls')),
]
