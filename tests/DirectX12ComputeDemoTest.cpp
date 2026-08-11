#include <gtest/gtest.h>

#include <vector>

#include "graphics/renderer/directx12/DirectX12ComputeDemo.h"

TEST(DirectX12ComputeDemoTest, TransformDataDoublesEveryElement)
{
    DirectX12ComputeDemo demo;
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
