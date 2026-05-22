from rest_framework.routers import DefaultRouter
from .api import MessageViewSet
router = DefaultRouter(); router.register('messages', MessageViewSet, basename='message')
urlpatterns = router.urls
