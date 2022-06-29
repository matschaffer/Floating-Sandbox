/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-09-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Tool.h"

#include "Controller.h"

#include <optional>

namespace ShipBuilder {

void Tool::SetCursor(wxImage const & cursorImage)
{
    mController.GetUserInterface().SetToolCursor(cursorImage);
}

ShipSpaceCoordinates Tool::ScreenToShipSpace(DisplayLogicalCoordinates const & displayCoordinates) const
{
    return mController.GetView().ScreenToShipSpace(displayCoordinates);
}

std::optional<DisplayLogicalCoordinates> Tool::GetMouseCoordinatesIfInWorkCanvas() const
{
    return mController.GetUserInterface().GetMouseCoordinatesIfInWorkCanvas();
}

std::optional<ImageCoordinates> Tool::GetMouseCoordinatesInTextureSpaceIfInTexture() const
{
    DisplayLogicalCoordinates const mouseCoords = GetCurrentMouseCoordinates();
    ImageCoordinates const mouseTextureCoords = mController.GetView().ScreenToTextureSpace(mouseCoords);

    if (mouseTextureCoords.IsInRect(ImageRect(mController.GetModelController().GetTextureSize())))
    {
        return mouseTextureCoords;
    }
    else
    {
        return std::nullopt;
    }
}

DisplayLogicalCoordinates Tool::GetCurrentMouseCoordinates() const
{
    return mController.GetUserInterface().GetMouseCoordinates();
}

ShipSpaceCoordinates Tool::GetCurrentMouseCoordinatesInShipSpace() const
{
    return mController.GetView().ScreenToShipSpace(GetCurrentMouseCoordinates());
}

}