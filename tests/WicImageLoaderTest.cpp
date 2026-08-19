#include <gtest/gtest.h>

#include "graphics/renderer/WicImageLoader.h"

TEST(WicImageLoaderTest, LoadDecodesValidPngWithExpectedDimensionsAndPixels)
{
    WicImageLoader::DecodedImage image;
    ASSERT_TRUE(WicImageLoader::Load("assets/textures/test_texture.png", image));

    EXPECT_EQ(image.width, 4u);
    EXPECT_EQ(image.height, 4u);
    ASSERT_EQ(image.pixels.size(), static_cast<size_t>(4 * 4 * 4));

    // 픽스처는 (200, 80, 40) 불투명(alpha=255) 단색으로 생성했다 - 첫 픽셀(RGBA8)이 그대로 나오는지 확인.
    EXPECT_EQ(image.pixels[0], 200);
    EXPECT_EQ(image.pixels[1], 80);
    EXPECT_EQ(image.pixels[2], 40);
    EXPECT_EQ(image.pixels[3], 255);
}

TEST(WicImageLoaderTest, LoadReturnsFalseForNonExistentFile)
{
    WicImageLoader::DecodedImage image;
    EXPECT_FALSE(WicImageLoader::Load("assets/textures/does_not_exist.png", image));
}
