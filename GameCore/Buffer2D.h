/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-11
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "GameTypes.h"
#include "Vectors.h"

#include <algorithm>
#include <memory>

template <typename TElement, typename TSize>
struct Buffer2D
{
public:

    using element_type = TElement;

    TSize const Size;
    std::unique_ptr<TElement[]> Data;

    Buffer2D(
        int width,
        int height,
        TElement defaultValue)
        : Size(width, height)
        , mLinearSize(width * height)
    {
        Data = std::make_unique<TElement[]>(mLinearSize);
        std::fill(
            Data.get(),
            Data.get() + mLinearSize,
            defaultValue);
    }

    Buffer2D(
        int width,
        int height,
        std::unique_ptr<TElement[]> data)
        : Size(width, height)
        , Data(std::move(data))
        , mLinearSize(width * height)
    {
    }

    Buffer2D(
        TSize size,
        std::unique_ptr<TElement[]> data)
        : Size(size)
        , Data(std::move(data))
        , mLinearSize(size.Width * size.Height)
    {
    }

    Buffer2D(Buffer2D && other) noexcept
        : Size(other.Size)
        , Data(std::move(other.Data))
        , mLinearSize(other.mLinearSize)
    {
    }

    Buffer2D & operator=(Buffer2D && other) noexcept
    {
        Size = other.Size;
        Data = std::move(other.Data);
        mLinearSize = other.mLinearSize;
    }

    size_t GetByteSize() const
    {
        return mLinearSize * sizeof(TElement);
    }

    std::unique_ptr<Buffer2D> MakeCopy() const
    {
        auto newData = std::make_unique<TElement[]>(mLinearSize);
        std::memcpy(newData.get(), Data.get(), mLinearSize * sizeof(TElement));

        return std::make_unique<Buffer2D>(
            Size,
            std::move(newData));
    }

    TElement & operator[](vec2i const & index)
    {
        return const_cast<TElement &>((static_cast<Buffer2D const &>(*this))[index]);
    }

    TElement const & operator[](vec2i const & index) const
    {
        assert(index.IsInRect(Size));

        size_t const linearIndex = index.y * Size.Width + index.x;
        assert(linearIndex < mLinearSize);

        return Data[linearIndex];
    }

private:

    size_t const mLinearSize;
};
