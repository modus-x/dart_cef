#include "video_outlet.h"

G_DEFINE_TYPE_WITH_CODE(VideoOutlet, video_outlet,
                        fl_pixel_buffer_texture_get_type(),
                        G_ADD_PRIVATE(VideoOutlet))

void video_outlet_init(VideoOutlet *self) {}

static gboolean video_outlet_copy_pixels(FlPixelBufferTexture *texture,
                                         const uint8_t **out_buffer,
                                         uint32_t *width, uint32_t *height,
                                         GError **error)
{
  auto video_outlet_private =
      (VideoOutletPrivate *)video_outlet_get_instance_private(
          DART_VLC_VIDEO_OUTLET(texture));

  const std::lock_guard<std::mutex> lock(video_outlet_private->mutex);
  *width = video_outlet_private->video_width;
  *height = video_outlet_private->video_height;
  const auto size = video_outlet_private->video_width * video_outlet_private->video_height * 4;
  video_outlet_private->previous_buffer.reset(new uint8_t[size]);
  memcpy(video_outlet_private->previous_buffer.get(), video_outlet_private->buffer.get(), size);
  *out_buffer = video_outlet_private->previous_buffer.get();
  return TRUE;
}

static void video_outlet_class_init(VideoOutletClass *klass)
{
  FL_PIXEL_BUFFER_TEXTURE_CLASS(klass)->copy_pixels = video_outlet_copy_pixels;
}

VideoOutlet *video_outlet_new()
{
  return DART_VLC_VIDEO_OUTLET(g_object_new(video_outlet_get_type(), nullptr));
}

VideoOutletPrivate *get_video_outlet_private(VideoOutlet *video_outlet)
{
  return (VideoOutletPrivate *)video_outlet_get_instance_private(video_outlet);
}