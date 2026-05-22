from django.contrib import admin
from .models import Message, Attachment, Label, AuditLog
admin.site.register([Message, Attachment, Label, AuditLog])
