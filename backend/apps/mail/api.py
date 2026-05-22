from rest_framework import viewsets
from rest_framework.permissions import IsAuthenticated
from .models import Message
from .serializers import MessageSerializer

class MessageViewSet(viewsets.ModelViewSet):
    serializer_class = MessageSerializer
    permission_classes = [IsAuthenticated]
    search_fields = ['subject','sender','body_text']
    filterset_fields = ['folder','unread','starred']
    def get_queryset(self):
        return Message.objects.filter(owner=self.request.user).order_by('-created_at')
