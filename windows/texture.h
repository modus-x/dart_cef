#pragma once

#include <flutter/texture_registrar.h>

#include <mutex>

class Texture
{
public:
    typedef std::function<void()> FrameAvailableCallback;

    Texture();

    ~Texture();

    const FlutterDesktopPixelBuffer *CopyPixelBuffer(size_t width, size_t height);

    void setOnFrameAvailable(FrameAvailableCallback callback)
    {
        frame_available_ = std::move(callback);
    }

    void sendBuffer(const void *buffer, int32_t width, int32_t height);

    std::mutex buffer_mutex_;


private:
    FrameAvailableCallback frame_available_;
    std::unique_ptr<uint8_t> backing_pixel_buffer_;
    std::unique_ptr<FlutterDesktopPixelBuffer> pixel_buffer_;
};