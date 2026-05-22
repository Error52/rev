from django.contrib.auth.views import LoginView as DjangoLoginView
from django_ratelimit.decorators import ratelimit
from django.utils.decorators import method_decorator

@method_decorator(ratelimit(key='ip', rate='10/m', method='POST', block=True), name='dispatch')
class LoginView(DjangoLoginView):
    template_name = 'auth/login.html'
