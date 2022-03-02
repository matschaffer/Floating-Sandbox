/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2018-03-25
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Colors.h"
#include "EnumFlags.h"
#include "SysSpecifics.h"
#include "Vectors.h"

#include <picojson.h>

#include <cassert>
#include <cmath>
#include <cstdint>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

////////////////////////////////////////////////////////////////////////////////////////////////
// Basics
////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * These types define the cardinality of elements in the ElementContainer.
 *
 * Indices are equivalent to pointers in OO terms. Given that we don't believe
 * we'll ever have more than 4 billion elements, a 32-bit integer suffices.
 *
 * This also implies that where we used to store one pointer, we can now store two indices,
 * resulting in even better data locality.
 */
using ElementCount = std::uint32_t;
using ElementIndex = std::uint32_t;
static constexpr ElementIndex NoneElementIndex = std::numeric_limits<ElementIndex>::max();

/*
 * Ship identifiers.
 *
 * Comparable and ordered. Start from 0.
 */
using ShipId = std::uint32_t;
static constexpr ShipId NoneShip = std::numeric_limits<ShipId>::max();

/*
 * Connected component identifiers.
 *
 * Comparable and ordered. Start from 0.
 */
using ConnectedComponentId = std::uint32_t;
static constexpr ConnectedComponentId NoneConnectedComponentId = std::numeric_limits<ConnectedComponentId>::max();

/*
 * Plane (depth) identifiers.
 *
 * Comparable and ordered. Start from 0.
 */
using PlaneId = std::uint32_t;
static constexpr PlaneId NonePlaneId = std::numeric_limits<PlaneId>::max();

/*
 * IDs (sequential) of electrical elements that have an identity.
 *
 * Comparable and ordered. Start from 0.
 */
using ElectricalElementInstanceIndex = std::uint16_t;
static constexpr ElectricalElementInstanceIndex NoneElectricalElementInstanceIndex = std::numeric_limits<ElectricalElementInstanceIndex>::max();

/*
 * Frontier identifiers.
 *
 * Comparable and ordered. Start from 0.
 */
using FrontierId = std::uint32_t;
static constexpr FrontierId NoneFrontierId = std::numeric_limits<FrontierId>::max();

/*
 * Various other identifiers.
 */
using LocalGadgetId = std::uint32_t;

/*
 * Object ID's, identifying objects of ships across ships.
 *
 * An ObjectId is unique only in the context in which it's used; for example,
 * a gadget might have the same object ID as a switch. That's where the type tag
 * comes from.
 *
 * Not comparable, not ordered.
 */
template<typename TLocalObjectId, typename TTypeTag>
struct ObjectId
{
    using LocalObjectId = TLocalObjectId;

    ObjectId(
        ShipId shipId,
        LocalObjectId localObjectId)
        : mShipId(shipId)
        , mLocalObjectId(localObjectId)
    {}

    inline ShipId GetShipId() const noexcept
    {
        return mShipId;
    };

    inline LocalObjectId GetLocalObjectId() const noexcept
    {
        return mLocalObjectId;
    }

    ObjectId & operator=(ObjectId const & other) = default;

    inline bool operator==(ObjectId const & other) const
    {
        return this->mShipId == other.mShipId
            && this->mLocalObjectId == other.mLocalObjectId;
    }

    inline bool operator<(ObjectId const & other) const
    {
        return this->mShipId < other.mShipId
            || (this->mShipId == other.mShipId && this->mLocalObjectId < other.mLocalObjectId);
    }

    std::string ToString() const
    {
        std::stringstream ss;

        ss << static_cast<int>(mShipId) << ":" << static_cast<int>(mLocalObjectId);

        return ss.str();
    }

private:

    ShipId mShipId;
    LocalObjectId mLocalObjectId;
};

template<typename TLocalObjectId, typename TTypeTag>
inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, ObjectId<TLocalObjectId, TTypeTag> const & oid)
{
    os << oid.ToString();
    return os;
}

namespace std {

template <typename TLocalObjectId, typename TTypeTag>
struct hash<ObjectId<TLocalObjectId, TTypeTag>>
{
    std::size_t operator()(ObjectId<TLocalObjectId, TTypeTag> const & objectId) const
    {
        return std::hash<ShipId>()(static_cast<uint16_t>(objectId.GetShipId()))
            ^ std::hash<typename ObjectId<TLocalObjectId, TTypeTag>::LocalObjectId>()(objectId.GetLocalObjectId());
    }
};

}

// Generic ID for generic elements (points, springs, etc.)
using ElementId = ObjectId<ElementIndex, struct ElementTypeTag>;

// ID for a gadget
using GadgetId = ObjectId<LocalGadgetId, struct GadgetTypeTag>;

// ID for electrical elements (switches, probes, etc.)
using ElectricalElementId = ObjectId<ElementIndex, struct ElectricalElementTypeTag>;

/*
 * A sequence number which is never zero.
 *
 * Assuming an increment at each frame, this sequence will wrap
 * every ~700 days.
 */
struct SequenceNumber
{
public:

    static constexpr SequenceNumber None()
    {
        return SequenceNumber();
    }

    inline constexpr SequenceNumber()
        : mValue(0)
    {}

    inline SequenceNumber & operator=(SequenceNumber const & other)
    {
        mValue = other.mValue;

        return *this;
    }

    SequenceNumber & operator++()
    {
        ++mValue;
        if (0 == mValue)
            mValue = 1;

        return *this;
    }

    SequenceNumber Previous() const
    {
        SequenceNumber res = *this;
        --res.mValue;
        if (res.mValue == 0)
            res.mValue = std::numeric_limits<std::uint32_t>::max();

        return res;
    }

    inline bool operator==(SequenceNumber const & other) const
    {
        return mValue == other.mValue;
    }

    inline bool operator!=(SequenceNumber const & other) const
    {
        return !(*this == other);
    }

    inline explicit operator bool() const
    {
        return (*this) != None();
    }

    inline bool IsStepOf(std::uint32_t step, std::uint32_t period)
    {
        return step == (mValue % period);
    }

private:

    friend std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, SequenceNumber const & s);

    std::uint32_t mValue;
};

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, SequenceNumber const & s)
{
    os << s.mValue;

    return os;
}

// Password hash
using PasswordHash = std::uint64_t;

// Variable-length 16-bit unsigned integer
struct var_uint16_t
{
public:

    static std::uint16_t constexpr MaxValue = 0x03fff;

    std::uint16_t value() const
    {
        return mValue;
    }

    var_uint16_t() = default;

    constexpr explicit var_uint16_t(std::uint16_t value)
        : mValue(value)
    {
        assert(value <= MaxValue);
    }

private:

    std::uint16_t mValue;
};

namespace std {
    template<> class numeric_limits<var_uint16_t>
    {
    public:
        static constexpr var_uint16_t min() { return var_uint16_t(0); };
        static constexpr var_uint16_t max() { return var_uint16_t(var_uint16_t::MaxValue); };
        static constexpr var_uint16_t lowest() { return min(); };
    };
}

////////////////////////////////////////////////////////////////////////////////////////////////
// Geometry
////////////////////////////////////////////////////////////////////////////////////////////////

/* 
 * Integral system
 */

#pragma pack(push, 1)

template<typename TIntegralTag>
struct _IntegralSize
{
    using integral_type = int;

    integral_type width;
    integral_type height;

    constexpr _IntegralSize(
        integral_type _width,
        integral_type _height)
        : width(_width)
        , height(_height)
    {}

    static _IntegralSize<TIntegralTag> FromFloatRound(vec2f const & vec)
    {
        return _IntegralSize<TIntegralTag>(
            static_cast<integral_type>(FastTruncateToArchInt(vec.x + 0.5f)),
            static_cast<integral_type>(FastTruncateToArchInt(vec.y + 0.5f)));
    }

    inline bool operator==(_IntegralSize<TIntegralTag> const & other) const
    {
        return this->width == other.width
            && this->height == other.height;
    }

    inline bool operator!=(_IntegralSize<TIntegralTag> const & other) const
    {
        return !(*this == other);
    }

    inline _IntegralSize<TIntegralTag> operator*(integral_type factor) const
    {
        return _IntegralSize<TIntegralTag>(
            this->width * factor,
            this->height * factor);
    }

    inline size_t GetLinearSize() const
    {
        return this->width * this->height;
    }

    inline _IntegralSize<TIntegralTag> Union(_IntegralSize<TIntegralTag> const & other) const
    {
        return _IntegralSize<TIntegralTag>(
            std::max(this->width, other.width),
            std::max(this->height, other.height));
    }

    inline _IntegralSize<TIntegralTag> Intersection(_IntegralSize<TIntegralTag> const & other) const
    {
        return _IntegralSize<TIntegralTag>(
            std::min(this->width, other.width),
            std::min(this->height, other.height));
    }

    vec2f ToFloat() const
    {
        return vec2f(
            static_cast<float>(width),
            static_cast<float>(height));
    }

    template<typename TCoordsRatio>
    vec2f ToFractionalCoords(TCoordsRatio const & coordsRatio) const
    {
        assert(coordsRatio.inputUnits != 0.0f);

        return vec2f(
            static_cast<float>(width) / coordsRatio.inputUnits * coordsRatio.outputUnits,
            static_cast<float>(height) / coordsRatio.inputUnits * coordsRatio.outputUnits);
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "(" << width << " x " << height << ")";
        return ss.str();
    }
};

#pragma pack(pop)

template<typename TTag>
inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, _IntegralSize<TTag> const & is)
{
    os << is.ToString();
    return os;
}

using IntegralRectSize = _IntegralSize<struct IntegralTag>;
using ImageSize = _IntegralSize<struct ImageTag>;
using ShipSpaceSize = _IntegralSize<struct ShipSpaceTag>;
using DisplayLogicalSize = _IntegralSize<struct DisplayLogicalTag>;
using DisplayPhysicalSize = _IntegralSize<struct DisplayPhysicalTag>;

#pragma pack(push, 1)

template<typename TIntegralTag>
struct _IntegralCoordinates
{
    using integral_type = int;

    integral_type x;
    integral_type y;

    constexpr _IntegralCoordinates(
        integral_type _x,
        integral_type _y)
        : x(_x)
        , y(_y)
    {}

    static _IntegralCoordinates<TIntegralTag> FromFloatRound(vec2f const & vec)
    {
        return _IntegralCoordinates<TIntegralTag>(
            static_cast<integral_type>(FastTruncateToArchInt(vec.x + 0.5f)),
            static_cast<integral_type>(FastTruncateToArchInt(vec.y + 0.5f)));
    }

    inline bool operator==(_IntegralCoordinates<TIntegralTag> const & other) const
    {
        return this->x == other.x
            && this->y == other.y;
    }

    inline bool operator!=(_IntegralCoordinates<TIntegralTag> const & other) const
    {
        return !(*this == other);
    }

    inline _IntegralCoordinates<TIntegralTag> operator+(_IntegralSize<TIntegralTag> const & sz) const
    {
        return _IntegralCoordinates<TIntegralTag>(
            this->x + sz.width,
            this->y + sz.height);
    }

    inline void operator+=(_IntegralSize<TIntegralTag> const & sz)
    {
        this->x += sz.width;
        this->y += sz.height;
    }

    inline _IntegralCoordinates<TIntegralTag> operator-() const
    {
        return _IntegralCoordinates<TIntegralTag>(
            -this->x,
            -this->y);
    }

    inline _IntegralSize<TIntegralTag> operator-(_IntegralCoordinates<TIntegralTag> const & other) const
    {
        return _IntegralSize<TIntegralTag>(
            this->x - other.x,
            this->y - other.y);
    }

    inline _IntegralCoordinates<TIntegralTag> operator-(_IntegralSize<TIntegralTag> const & offset) const
    {
        return _IntegralCoordinates<TIntegralTag>(
            this->x - offset.width,
            this->y - offset.height);
    }

    inline _IntegralCoordinates<TIntegralTag> scale(_IntegralCoordinates<TIntegralTag> const & multiplier) const
    {
        return _IntegralCoordinates<TIntegralTag>(
            this->x * multiplier.x,
            this->y * multiplier.y);
    }

    template<typename TSize>
    bool IsInSize(TSize const & size) const
    {
        return x >= 0 && x < size.width && y >= 0 && y < size.height;
    }

    template<typename TRect>
    bool IsInRect(TRect const & rect) const
    {
        return x >= rect.origin.x && x < rect.origin.x + rect.size.width 
            && y >= rect.origin.y && y < rect.origin.y + rect.size.height;
    }

    _IntegralCoordinates<TIntegralTag> FlipX(integral_type width) const
    {
        assert(width > x);
        return _IntegralCoordinates<TIntegralTag>(width - 1 - x, y);
    }

    _IntegralCoordinates<TIntegralTag> FlipY(integral_type height) const
    {
        assert(height > y);
        return _IntegralCoordinates<TIntegralTag>(x, height - 1 - y);
    }

    vec2f ToFloat() const
    {
        return vec2f(
            static_cast<float>(x),
            static_cast<float>(y));
    }

    template<typename TCoordsRatio>
    vec2f ToFractionalCoords(TCoordsRatio const & coordsRatio) const
    {
        assert(coordsRatio.inputUnits != 0.0f);

        return vec2f(
            static_cast<float>(x) / coordsRatio.inputUnits * coordsRatio.outputUnits,
            static_cast<float>(y) / coordsRatio.inputUnits * coordsRatio.outputUnits);
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "(" << x << ", " << y << ")";
        return ss.str();
    }
};

#pragma pack(pop)

template<typename TTag>
inline std::basic_ostream<char> & operator<<(std::basic_ostream<char> & os, _IntegralCoordinates<TTag> const & p)
{
    os << p.ToString();
    return os;
}

using IntegralCoordinates = _IntegralCoordinates<struct IntegralTag>; // Generic integer
using ImageCoordinates = _IntegralCoordinates<struct ImageTag>; // Image
using ShipSpaceCoordinates = _IntegralCoordinates<struct ShipSpaceTag>; // Y=0 at bottom
using DisplayLogicalCoordinates = _IntegralCoordinates<struct DisplayLogicalTag>; // Y=0 at top
using DisplayPhysicalCoordinates = _IntegralCoordinates<struct DisplayPhysicalTag>; // Y=0 at top

#pragma pack(push)

template<typename TIntegralTag>
struct _IntegralRect
{
    _IntegralCoordinates<TIntegralTag> origin;
    _IntegralSize<TIntegralTag> size;

    constexpr _IntegralRect()
        : origin(0,0 )
        , size(0, 0)
    {}

    constexpr _IntegralRect(
        _IntegralCoordinates<TIntegralTag> const & _origin,
        _IntegralSize<TIntegralTag> const & _size)
        : origin(_origin)
        , size(_size)
    {}

    constexpr _IntegralRect(_IntegralCoordinates<TIntegralTag> const & _origin)
        : origin(_origin)
        , size(1, 1)
    {}

    constexpr _IntegralRect(_IntegralSize<TIntegralTag> const & _size)
        : origin(0, 0)
        , size(_size)
    {}

    inline bool operator==(_IntegralRect<TIntegralTag> const & other) const
    {
        return origin == other.origin
            && size == other.size;
    }

    inline bool operator!=(_IntegralRect<TIntegralTag> const & other) const
    {
        return !(*this == other);
    }

    bool IsContainedInRect(_IntegralRect<TIntegralTag> const & container) const
    {
        return origin.x >= container.origin.x
            && origin.y >= container.origin.y
            && origin.x + size.width <= container.origin.x + container.size.width
            && origin.y + size.height <= container.origin.y + container.size.height;
    }

    void UnionWith(_IntegralCoordinates<TIntegralTag> const & other)
    {
        auto const newOrigin = _IntegralCoordinates<TIntegralTag>(
            std::min(origin.x, other.x),
            std::min(origin.y, other.y));

        auto const newSize = _IntegralSize<TIntegralTag>(
            std::max(origin.x + size.width, other.x + 1) - newOrigin.x,
            std::max(origin.y + size.height, other.y + 1) - newOrigin.y);

        assert(newSize.width >= 0 && newSize.height >= 0);

        origin = newOrigin;
        size = newSize;
    }

    void UnionWith(_IntegralRect<TIntegralTag> const & other)
    {
        auto const newOrigin = _IntegralCoordinates<TIntegralTag>(
            std::min(origin.x, other.origin.x),
            std::min(origin.y, other.origin.y));

        auto const newSize = _IntegralSize<TIntegralTag>(
            std::max(origin.x + size.width, other.origin.x + other.size.width) - newOrigin.x,
            std::max(origin.y + size.height, other.origin.y + other.size.height) - newOrigin.y);

        assert(newSize.width >= 0 && newSize.height >= 0);

        origin = newOrigin;
        size = newSize;
    }

    std::optional<_IntegralRect<TIntegralTag>> MakeIntersectionWith(_IntegralRect<TIntegralTag> const & other) const
    {
        auto const newOrigin = _IntegralCoordinates<TIntegralTag>(
            std::max(origin.x, other.origin.x),
            std::max(origin.y, other.origin.y));

        auto const newSize = _IntegralSize<TIntegralTag>(
            std::min(size.width - (newOrigin.x - origin.x), other.size.width - (newOrigin.x - other.origin.x)),
            std::min(size.height - (newOrigin.y - origin.y), other.size.height - (newOrigin.y - other.origin.y)));

        if (newSize.width <= 0 || newSize.height <= 0)
        {
            return std::nullopt;
        }
        else
        {
            return _IntegralRect<TIntegralTag>(
                newOrigin,
                newSize);
        }
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "(" << origin.x << ", " << origin.y << " -> " << size.width << " x " << size.height << ")";
        return ss.str();
    }
};

#pragma pack(pop)

using IntegralRect = _IntegralRect<struct IntegralTag>;
using ImageRect = _IntegralRect<struct ImageTag>;
using ShipSpaceRect = _IntegralRect<struct ShipSpaceTag>;
using DisplayPhysicalRect = _IntegralRect<struct DisplayPhysicalTag>; // Y=0 at top

template<typename TIntegralTag>
struct _IntegralCoordsRatio
{
    float inputUnits; // i.e. how many integral units
    float outputUnits; // i.e. how many float units

    constexpr _IntegralCoordsRatio(
        float _inputUnits,
        float _outputUnits)
        : inputUnits(_inputUnits)
        , outputUnits(_outputUnits)
    {}

    inline bool operator==(_IntegralCoordsRatio<TIntegralTag> const & other) const
    {
        return inputUnits == other.inputUnits
            && outputUnits == other.outputUnits;
    }

    inline bool operator!=(_IntegralCoordsRatio<TIntegralTag> const & other) const
    {
        return !(*this == other);
    }
};

using ShipSpaceToWorldSpaceCoordsRatio = _IntegralCoordsRatio<struct ShipSpaceTag>;

/*
 * Float rectangle.
 */

#pragma pack(push)

struct FloatRect
{
    vec2f origin;
    vec2f size;

    constexpr FloatRect()
        : origin(vec2f::zero())
        , size(vec2f::zero())
    {}

    constexpr FloatRect(
        vec2f const & _origin,
        vec2f const & _size)
        : origin(_origin)
        , size(_size)
    {}

    inline bool operator==(FloatRect const & other) const
    {
        return origin == other.origin
            && size == other.size;
    }

    bool IsContainedInRect(FloatRect const & container) const
    {
        return origin.x >= container.origin.x
            && origin.y >= container.origin.y
            && origin.x + size.x <= container.origin.x + container.size.x
            && origin.y + size.y <= container.origin.y + container.size.y;
    }

    void UnionWith(FloatRect const & other)
    {
        auto const newOrigin = vec2f(
            std::min(origin.x, other.origin.x),
            std::min(origin.y, other.origin.y));

        auto const newSize = vec2f(
            std::max(origin.x + size.x, other.origin.x + other.size.x) - newOrigin.x,
            std::max(origin.y + size.y, other.origin.y + other.size.y) - newOrigin.y);

        assert(newSize.x >= 0 && newSize.y >= 0);

        origin = newOrigin;
        size = newSize;
    }

    std::optional<FloatRect> MakeIntersectionWith(FloatRect const & other) const
    {
        auto const newOrigin = vec2f(
            std::max(origin.x, other.origin.x),
            std::max(origin.y, other.origin.y));

        auto const newSize = vec2f(
            std::min(size.x - (newOrigin.x - origin.x), other.size.x - (newOrigin.x - other.origin.x)),
            std::min(size.y - (newOrigin.y - origin.y), other.size.y - (newOrigin.y - other.origin.y)));

        if (newSize.x <= 0 || newSize.y <= 0)
        {
            return std::nullopt;
        }
        else
        {
            return FloatRect(
                newOrigin,
                newSize);
        }
    }

    std::string ToString() const
    {
        std::stringstream ss;
        ss << "(" << origin.x << ", " << origin.y << " -> " << size.x << " x " << size.y << ")";
        return ss.str();
    }
};

#pragma pack(pop)

/*
 * Octants, i.e. the direction of a spring connecting two neighbors.
 *
 * Octant 0 is E, octant 1 is SE, ..., Octant 7 is NE.
 */
using Octant = std::int32_t;

/*
 * Generic directions.
 */
enum class DirectionType
{
    Horizontal = 1,
    Vertical = 2
};

template <> struct is_flag<DirectionType> : std::true_type {};

////////////////////////////////////////////////////////////////////////////////////////////////
// Game
////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * The color key of materials.
 */
using MaterialColorKey = rgbColor;

static MaterialColorKey constexpr EmptyMaterialColorKey = MaterialColorKey(255, 255, 255);

/*
 * The different layers.
 */
enum class LayerType : std::uint32_t
{
    Structural = 0,
    Electrical = 1,
    Ropes = 2,
    Texture = 3
};

/*
 * The different material layers.
 */
enum class MaterialLayerType
{
    Structural,
    Electrical
};

/*
 * Types of frontiers (duh).
 */
enum class FrontierType
{
    External,
    Internal
};

/*
 * Types of gadgets.
 */
enum class GadgetType
{
    AntiMatterBomb,
    ImpactBomb,
    PhysicsProbe,
    RCBomb,
    TimerBomb
};

/*
 * Types of explosions.
 */
enum class ExplosionType
{
    Combustion,
    Deflagration
};

/*
 * Types of electrical switches.
 */
enum class SwitchType
{
    InteractiveToggleSwitch,
    InteractivePushSwitch,
    AutomaticSwitch,
    ShipSoundSwitch
};

/*
 * Types of power probes.
 */
enum class PowerProbeType
{
    PowerMonitor,
    Generator
};

/*
 * Electrical states.
 */
enum class ElectricalState : bool
{
    Off = false,
    On = true
};

inline std::basic_ostream<char> & operator<<(std::basic_ostream<char>& os, ElectricalState const & s)
{
    if (s == ElectricalState::On)
    {
        os << "ON";
    }
    else
    {
        assert(s == ElectricalState::Off);
        os << "OFF";
    }

    return os;
}

/*
 * Unit systems.
 */
enum class UnitsSystem
{
    SI_Kelvin,
    SI_Celsius,
    USCS
};

/*
 * Generic duration enum - short and long.
 */
enum class DurationShortLongType
{
    Short,
    Long
};

DurationShortLongType StrToDurationShortLongType(std::string const & str);

/*
 * Information (layout, etc.) for an element in the electrical panel.
 */
struct ElectricalPanelElementMetadata
{
    std::optional<IntegralCoordinates> PanelCoordinates;
    std::optional<std::string> Label;
    bool IsHidden;

    ElectricalPanelElementMetadata(
        std::optional<IntegralCoordinates> panelCoordinates,
        std::optional<std::string> label,
        bool isHidden)
        : PanelCoordinates(std::move(panelCoordinates))
        , Label(std::move(label))
        , IsHidden(isHidden)
    {}
};

/*
 * HeatBlaster action.
 */
enum class HeatBlasterActionType
{
    Heat,
    Cool
};

/*
 * Location that a tool is applied to.
 */
enum class ToolApplicationLocus
{
    World = 1,
    Ship = 2,

    AboveWater = 4,
    UnderWater = 8
};

template <> struct is_flag<ToolApplicationLocus> : std::true_type {};

////////////////////////////////////////////////////////////////////////////////////////////////
// Rendering
////////////////////////////////////////////////////////////////////////////////////////////////

/*
 * The different auto-texturization modes for ships that don't have a texture layer.
 *
 * Note: enum value are serialized in ship files, do not change.
 */
enum class ShipAutoTexturizationModeType : std::uint32_t
{
    FlatStructure = 1,      // Builds texture using structural materials' RenderColor
    MaterialTextures = 2    // Builds texture using materials' "Bump Maps"
};

/*
 * The different visual ways in which we render highlights.
 */
enum class HighlightModeType : size_t
{
    Circle = 0,
    ElectricalElement,

    _Last = ElectricalElement
};

/*
 * The ways in which heat may be rendered.
 */
enum class HeatRenderModeType
{
    None,
    Incandescence,
    HeatOverlay
};

/*
 * The debug ways in which ships may be rendered.
 */
enum class DebugShipRenderModeType
{
    None,
    Wireframe,
    Points,
    Springs,
    EdgeSprings,
    Structure,
    Decay,
    InternalPressure,
    Strength
};

/*
 * The different ways in which the ocean may be rendered.
 */
enum class OceanRenderModeType
{
    Texture,
    Depth,
    Flat
};

/*
 * The different levels of detail with which the ocean may be rendered.
 */
enum class OceanRenderDetailType
{
    Basic,
    Detailed
};

/*
 * The different ways in which the ocean floor may be rendered.
 */
enum class LandRenderModeType
{
    Texture,
    Flat
};

/*
 * The different vector fields that may be rendered.
 */
enum class VectorFieldRenderModeType
{
    None,
    PointVelocity,
    PointStaticForce,
    PointDynamicForce,
    PointWaterVelocity,
    PointWaterMomentum
};

/*
 * The index of a single texture frame in a group of textures.
 */
using TextureFrameIndex = std::uint16_t;

/*
 * The global identifier of a single texture frame.
 *
 * The identifier of a frame is hierarchical:
 * - A group, identified by a value of the enum that
 *   this identifier is templated on
 * - The index of the frame in that group
 */
template <typename TextureGroups>
struct TextureFrameId
{
    TextureGroups Group;
    TextureFrameIndex FrameIndex;

    TextureFrameId(
        TextureGroups group,
        TextureFrameIndex frameIndex)
        : Group(group)
        , FrameIndex(frameIndex)
    {}

    TextureFrameId & operator=(TextureFrameId const & other) = default;

    inline bool operator==(TextureFrameId const & other) const
    {
        return this->Group == other.Group
            && this->FrameIndex == other.FrameIndex;
    }

    inline bool operator<(TextureFrameId const & other) const
    {
        return this->Group < other.Group
            || (this->Group == other.Group && this->FrameIndex < other.FrameIndex);
    }

    std::string ToString() const
    {
        std::stringstream ss;

        ss << static_cast<int>(Group) << ":" << static_cast<int>(FrameIndex);

        return ss.str();
    }
};

namespace std {

    template <typename TextureGroups>
    struct hash<TextureFrameId<TextureGroups>>
    {
        std::size_t operator()(TextureFrameId<TextureGroups> const & frameId) const
        {
            return std::hash<uint16_t>()(static_cast<uint16_t>(frameId.Group))
                ^ std::hash<TextureFrameIndex>()(frameId.FrameIndex);
        }
    };

}
