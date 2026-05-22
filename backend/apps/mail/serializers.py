from rest_framework import serializers
from .models import Message, Attachment
class AttachmentSerializer(serializers.ModelSerializer):
    class Meta:
        model = Attachment
        fields = ['id','filename','size','file']
class MessageSerializer(serializers.ModelSerializer):
    attachments = AttachmentSerializer(many=True, read_only=True)
    class Meta:
        model = Message
        fields = '__all__'
