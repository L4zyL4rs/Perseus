#pragma once
#include "VulkanHelper.h"

struct Bitmap {
	int width;
	int height;
	const VkFormat format;
	std::vector<unsigned char> pixels;  // Grayscale, 1 byte per pixel

	Bitmap(int w, int h, VkFormat f = VK_FORMAT_R8_SRGB) : width(w), height(h), format(f) {
		const auto& info = lookupFormat(f);
		if (!info) {
			throw std::runtime_error("Unsupported VkFormat in Bitmap constructor!");
		}
		pixels.resize(static_cast<size_t>(w) * h * info->channels, 0);
	}

	unsigned char* ptr(int x, int y) {
		return &pixels[(y * width + x) * lookupFormat(format)->channels];
	}

	void blit(const FT_Bitmap& ftBitmap, int x, int y) {
		for (unsigned int row = 0; row < ftBitmap.rows; ++row) {
			for (unsigned int col = 0; col < ftBitmap.width; ++col) {
				int px = x + col;
				int py = y + row;
				unsigned char grey = ftBitmap.buffer[row * ftBitmap.pitch + col];
				if (px < width && py < height) {
					auto p = this->ptr(px, py);
					if (format == VK_FORMAT_R8_SRGB) {
						p[0] = grey;
					}
					else if (format == VK_FORMAT_R8G8B8_SRGB) {
						p[0] = grey; p[1] = grey; p[2] = grey;
					}
					else if (format == VK_FORMAT_R8G8B8A8_SRGB) {
						p[0] = 255; p[1] = 255; p[2] = 255;
						p[3] = grey;
					}
					pixels[py * width + px] = ftBitmap.buffer[row * ftBitmap.pitch + col];
				}
			}
		}
	}

};
