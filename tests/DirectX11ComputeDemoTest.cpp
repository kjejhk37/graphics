#include <gtest/gtest.h>

#include <vector>

#include "graphics/renderer/directx11/DirectX11ComputeDemo.h"

TEST(DirectX11ComputeDemoTest, TransformDataDoublesEveryElement)
{
    DirectX11ComputeDemo demo;
    ASSERT_TRUE(demo.Initialize());

    std::vector<float> input(1024);
    for (std::size_t i = 0; i < input.size(); ++i)
    {
        input[i] = static_cast<float>(i + 1);
    }

    const std::vector<float> result = demo.TransformData(input);

    ASSERT_EQ(result.size(), input.size());
    for (std::size_t i = 0; i < input.size(); ++i)
    {
        EXPECT_FLOAT_EQ(result[i], input[i] * 2.0f);
    }
}
