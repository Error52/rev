from celery import shared_task
from django.core.mail import EmailMultiAlternatives

@shared_task
def send_mail_via_postfix(subject, body, from_email, recipients, html=None):
    msg = EmailMultiAlternatives(subject, body, from_email, recipients)
    if html:
        msg.attach_alternative(html, 'text/html')
    msg.send()
