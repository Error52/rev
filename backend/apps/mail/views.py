from django.contrib.auth.mixins import LoginRequiredMixin
from django.views.generic import TemplateView
from django.db.models import Q
from .models import Message

class DashboardView(LoginRequiredMixin, TemplateView):
    template_name = 'mail/dashboard.html'

    def get_context_data(self, **kwargs):
        ctx = super().get_context_data(**kwargs)
        q = self.request.GET.get('q','')
        folder = self.request.GET.get('folder','INBOX')
        qs = Message.objects.filter(owner=self.request.user, folder=folder)
        if q:
            qs = qs.filter(Q(subject__icontains=q)|Q(sender__icontains=q)|Q(body_text__icontains=q))
        ctx['messages'] = qs.order_by('-created_at')[:100]
        ctx['folder'] = folder
        return ctx
