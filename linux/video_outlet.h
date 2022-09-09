
#ifndef VIDEO_OUTLET_H_
#define VIDEO_OUTLET_H_

#include <flutter_linux/flutter_linux.h>
#include <gtk/gtk.h>
#include <mutex>
#include <memory>

G_DECLARE_DERIVABLE_TYPE(VideoOutlet, video_outlet, DART_VLC, VIDEO_OUTLET,
                         FlPixelBufferTexture)

struct _VideoOutletClass
{
  FlPixelBufferTextureClass parent_class;
};

struct VideoOutletPrivate
{
  int64_t texture_id = 0;
  std::unique_ptr<uint8_t> buffer;
  int32_t video_width = 0;
  int32_t video_height = 0;

  // save buffer here for render in flutter
  std::unique_ptr<uint8_t> previous_buffer;

  std::mutex mutex;
};

VideoOutlet *video_outlet_new();

VideoOutletPrivate *get_video_outlet_private(VideoOutlet *video_outlet);

#endif