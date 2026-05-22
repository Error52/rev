from django.conf import settings
from django.db import models

class Label(models.Model):
    user = models.ForeignKey(settings.AUTH_USER_MODEL, on_delete=models.CASCADE)
    name = models.CharField(max_length=64)
    color = models.CharField(max_length=16, default='#39ff14')

class Message(models.Model):
    FOLDERS=[('INBOX','Inbox'),('SENT','Sent'),('DRAFTS','Drafts'),('SPAM','Spam'),('TRASH','Trash')]
    owner = models.ForeignKey(settings.AUTH_USER_MODEL, on_delete=models.CASCADE, related_name='messages')
    uid = models.CharField(max_length=128, blank=True)
    thread_id = models.CharField(max_length=128, blank=True)
    sender = models.EmailField()
    recipients = models.TextField()
    cc = models.TextField(blank=True)
    bcc = models.TextField(blank=True)
    subject = models.CharField(max_length=255, blank=True)
    body_text = models.TextField(blank=True)
    body_html = models.TextField(blank=True)
    folder = models.CharField(max_length=16, choices=FOLDERS, default='INBOX')
    unread = models.BooleanField(default=True)
    starred = models.BooleanField(default=False)
    labels = models.ManyToManyField(Label, blank=True)
    created_at = models.DateTimeField(auto_now_add=True)

class Attachment(models.Model):
    message = models.ForeignKey(Message, on_delete=models.CASCADE, related_name='attachments')
    file = models.FileField(upload_to='attachments/%Y/%m/%d')
    filename = models.CharField(max_length=255)
    size = models.BigIntegerField(default=0)

class AuditLog(models.Model):
    user = models.ForeignKey(settings.AUTH_USER_MODEL, on_delete=models.SET_NULL, null=True)
    action = models.CharField(max_length=255)
    ip_address = models.GenericIPAddressField(null=True, blank=True)
    created_at = models.DateTimeField(auto_now_add=True)
