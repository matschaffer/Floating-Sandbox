/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-10-05
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SelectionTool.h"

#include "Controller.h"

#include <UILib/WxHelpers.h>

namespace ShipBuilder {

StructuralSelectionTool::StructuralSelectionTool(
    Controller & controller,
    SelectionManager & selectionManager,
    ResourceLocator const & resourceLocator)
    : SelectionTool(
        ToolType::StructuralSelection,
        controller,
        selectionManager,
        resourceLocator)
{}

ElectricalSelectionTool::ElectricalSelectionTool(
    Controller & controller,
    SelectionManager & selectionManager,
    ResourceLocator const & resourceLocator)
    : SelectionTool(
        ToolType::ElectricalSelection,
        controller,
        selectionManager,
        resourceLocator)
{}

RopeSelectionTool::RopeSelectionTool(
    Controller & controller,
    SelectionManager & selectionManager,
    ResourceLocator const & resourceLocator)
    : SelectionTool(
        ToolType::RopeSelection,
        controller,
        selectionManager,
        resourceLocator)
{}

TextureSelectionTool::TextureSelectionTool(
    Controller & controller,
    SelectionManager & selectionManager,
    ResourceLocator const & resourceLocator)
    : SelectionTool(
        ToolType::TextureSelection,
        controller,
        selectionManager,
        resourceLocator)
{}

template<LayerType TLayer>
SelectionTool<TLayer>::SelectionTool(
    ToolType toolType,
    Controller & controller,
    SelectionManager & selectionManager,
    ResourceLocator const & resourceLocator)
    : Tool(
        toolType,
        controller)
    , mSelectionManager(selectionManager)
    , mCurrentSelection()
    , mEngagementData()
    , mIsShiftDown(false)
{
    SetCursor(WxHelpers::LoadCursorImage("selection_cursor", 10, 10, resourceLocator));
}

template<LayerType TLayer>
SelectionTool<TLayer>::~SelectionTool()
{
    if (mCurrentSelection || mEngagementData)
    {
        // Remove overlay
        mController.GetView().RemoveSelectionOverlay();
        mController.GetUserInterface().RefreshView();

        // Remove measurement
        mController.GetUserInterface().OnMeasuredSelectionSizeChanged(std::nullopt);
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnMouseMove(DisplayLogicalCoordinates const & mouseCoordinates)
{
    if (mEngagementData)
    {
        auto const cornerCoordinates = GetCornerCoordinatesEngaged(mouseCoordinates);

        UpdateEphemeralSelection(cornerCoordinates);
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnLeftMouseDown()
{
    assert(!mEngagementData);

    auto const cornerCoordinates = GetCornerCoordinatesFree();
    if (cornerCoordinates)
    {
        // Create new corner - init with current coords, eventually
        // we end up with an empty rect
        ShipSpaceCoordinates selectionStartCorner = *cornerCoordinates;

        // Check if hitting a corner
        if (mCurrentSelection)
        {
            if (*cornerCoordinates == mCurrentSelection->CornerA())
            {
                selectionStartCorner = mCurrentSelection->CornerC();
            }
            else if (*cornerCoordinates == mCurrentSelection->CornerB())
            {
                selectionStartCorner = mCurrentSelection->CornerD();
            }
            else if (*cornerCoordinates == mCurrentSelection->CornerC())
            {
                selectionStartCorner = mCurrentSelection->CornerA();
            }
            else if (*cornerCoordinates == mCurrentSelection->CornerD())
            {
                selectionStartCorner = mCurrentSelection->CornerB();
            }
        }

        // Engage at selection start corner
        mEngagementData.emplace(selectionStartCorner);

        UpdateEphemeralSelection(*cornerCoordinates);
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnLeftMouseUp()
{
    if (mEngagementData)
    {
        // Calculate corner
        ShipSpaceCoordinates const cornerCoordinates = GetCornerCoordinatesEngaged();

        // Calculate selection
        std::optional<ShipSpaceRect> selection;
        if (cornerCoordinates.x != mEngagementData->SelectionStartCorner.x
            && cornerCoordinates.y != mEngagementData->SelectionStartCorner.y)
        {
            // Non-empty selection
            selection = ShipSpaceRect(
                mEngagementData->SelectionStartCorner,
                cornerCoordinates);

            // Update overlay
            mController.GetView().UploadSelectionOverlay(
                mEngagementData->SelectionStartCorner,
                cornerCoordinates);

            // Update measurement
            mController.GetUserInterface().OnMeasuredSelectionSizeChanged(selection->size);
        }
        else
        {
            // Empty selection

            // Update overlay
            mController.GetView().RemoveSelectionOverlay();

            // Update measurement
            mController.GetUserInterface().OnMeasuredSelectionSizeChanged(std::nullopt);
        }

        // Commit selection
        mCurrentSelection = selection;
        mSelectionManager.SetSelection(mCurrentSelection);

        // Disengage
        mEngagementData.reset();

        mController.GetUserInterface().RefreshView();
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnShiftKeyDown()
{
    mIsShiftDown = true;

    if (mEngagementData)
    {
        auto const cornerCoordinates = GetCornerCoordinatesEngaged();

        UpdateEphemeralSelection(cornerCoordinates);
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::OnShiftKeyUp()
{
    mIsShiftDown = false;

    if (mEngagementData)
    {
        auto const cornerCoordinates = GetCornerCoordinatesEngaged();

        UpdateEphemeralSelection(cornerCoordinates);
    }
}

//////////////////////////////////////////////////////////////////////////////

template<LayerType TLayer>
ShipSpaceCoordinates SelectionTool<TLayer>::GetCornerCoordinatesEngaged() const
{
    return GetCornerCoordinatesEngaged(GetCurrentMouseCoordinates());
}

template<LayerType TLayer>
ShipSpaceCoordinates SelectionTool<TLayer>::GetCornerCoordinatesEngaged(DisplayLogicalCoordinates const & input) const
{
    // Convert to ship coords closest to grid point
    ShipSpaceCoordinates const nearestGridPointCoordinates = ScreenToShipSpaceNearest(input);

    // Clamp - allowing for point at (w,h)
    ShipSpaceCoordinates const cornerCoordinates = nearestGridPointCoordinates.Clamp(mController.GetModelController().GetShipSize());

    // Eventually constrain to square
    if (mIsShiftDown)
    {
        auto const width = cornerCoordinates.x - mEngagementData->SelectionStartCorner.x;
        auto const height = cornerCoordinates.y - mEngagementData->SelectionStartCorner.y;
        if (std::abs(width) < std::abs(height))
        {
            // Use width
            return ShipSpaceCoordinates(
                cornerCoordinates.x,
                mEngagementData->SelectionStartCorner.y + std::abs(width) * Sign(height));
        }
        else
        {
            // Use height
            return ShipSpaceCoordinates(
                mEngagementData->SelectionStartCorner.x + std::abs(height) * Sign(width),
                cornerCoordinates.y);
        }
    }
    else
    {
        return cornerCoordinates;
    }
}

template<LayerType TLayer>
std::optional<ShipSpaceCoordinates> SelectionTool<TLayer>::GetCornerCoordinatesFree() const
{
    ShipSpaceCoordinates const mouseShipCoordinates = ScreenToShipSpaceNearest(GetCurrentMouseCoordinates());

    auto const shipSize = mController.GetModelController().GetShipSize();
    if (mouseShipCoordinates.IsInRect(
        ShipSpaceRect(
            ShipSpaceCoordinates(0, 0),
            ShipSpaceSize(shipSize.width + 1, shipSize.height + 1))))
    {
        return mouseShipCoordinates;
    }
    else
    {
        return std::nullopt;
    }
}

template<LayerType TLayer>
void SelectionTool<TLayer>::UpdateEphemeralSelection(ShipSpaceCoordinates const & cornerCoordinates)
{
    // Update overlay
    mController.GetView().UploadSelectionOverlay(
        mEngagementData->SelectionStartCorner,
        cornerCoordinates);
    mController.GetUserInterface().RefreshView();

    // Update measurement
    mController.GetUserInterface().OnMeasuredSelectionSizeChanged(
        ShipSpaceSize(
            std::abs(cornerCoordinates.x - mEngagementData->SelectionStartCorner.x),
            std::abs(cornerCoordinates.y - mEngagementData->SelectionStartCorner.y)));
}

}