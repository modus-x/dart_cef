#include "texture.h"
#include <iostream>

void SwapBufferFromBgraToRgba(void *_dest, const void *_src, int width, int height)
{
    int32_t *dest = (int32_t *)_dest;
    int32_t *src = (int32_t *)_src;
    int32_t rgba;
    int32_t bgra;
    int length = width * height;
    for (int i = 0; i < length; i++)
    {
        bgra = src[i];
        // BGRA in hex = 0xAARRGGBB.
        rgba = (bgra & 0x00ff0000) >> 16    // Red >> Blue.
               | (bgra & 0xff00ff00)        // Green Alpha.
               | (bgra & 0x000000ff) << 16; // Blue >> Red.
        dest[i] = rgba;
    }
}
Texture::Texture()
{
}

Texture::~Texture()
{
    std::cout << "texture destructor" << std::endl;
    const std::lock_guard<std::mutex> lock(buffer_mutex_);
}

const FlutterDesktopPixelBuffer *Texture::CopyPixelBuffer(
    size_t width, size_t height)
{
    auto buffer = pixel_buffer_.get();
    // Only lock the mutex if the buffer is not null
    // (to ensure the release callback gets called)
    if (buffer)
    {
        // Gets unlocked in the FlutterDesktopPixelBuffer's release callback.
        buffer_mutex_.lock();
    }
    return buffer;
}

void Texture::sendBuffer(const void *buffer, int32_t width, int32_t height)
{
    try
    {
        const std::lock_guard<std::mutex> lock(buffer_mutex_);
        if (!pixel_buffer_.get() || pixel_buffer_.get()->width != width || pixel_buffer_.get()->height != height)
        {
            if (!pixel_buffer_.get())
            {
                pixel_buffer_ = std::make_unique<FlutterDesktopPixelBuffer>();
                pixel_buffer_->release_context = &buffer_mutex_;
                // Gets invoked after the FlutterDesktopPixelBuffer's
                // backing buffer has been uploaded.
                pixel_buffer_->release_callback = [](void *opaque)
                {
                    try
                    {
                        auto mutex = reinterpret_cast<std::mutex *>(opaque);
                        // Gets locked just before |CopyPixelBuffer| returns.
                        mutex->unlock();
                    }
                    catch (...)
                    {
                        // Block of code to handle errors
                    }
                };
            }
            pixel_buffer_->width = width;
            pixel_buffer_->height = height;
            const auto size = width * height * 4;
            backing_pixel_buffer_.reset(new uint8_t[size]);
            pixel_buffer_->buffer = backing_pixel_buffer_.get();
        }

        SwapBufferFromBgraToRgba((void *)pixel_buffer_->buffer, buffer, width, height);
        if (frame_available_)
        {
            frame_available_();
        }
    }
    catch (...)
    {
        // Block of code to handle errors
    }
}