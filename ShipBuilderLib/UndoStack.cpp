/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-09-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "UndoStack.h"

#include "Controller.h"

#include <GameCore/Log.h>

namespace ShipBuilder {

template<typename TLayerBuffer>
void LayerBufferRegionUndoAction<TLayerBuffer>::ApplyAction(Controller & controller) const
{
    controller.RestoreLayerBufferRegion(mLayerBufferRegion, mOrigin);
}

//
// Explicit specializations
//

template class LayerBufferRegionUndoAction<StructuralLayerBuffer>;
template class LayerBufferRegionUndoAction<ElectricalLayerBuffer>;

void UndoStack::Push(std::unique_ptr<UndoAction> && undoAction)
{
    // Update total cost
    mTotalCost += undoAction->GetCost();

    // Push undo action
    mStack.push_back(std::move(undoAction));

    // Trim stack if too big
    while (mStack.size() > MaxEntries || mTotalCost > MaxCost)
    {
        assert(mTotalCost >= mStack.back()->GetCost());
        mTotalCost -= mStack.back()->GetCost();
        mStack.pop_back();
    }
}

std::unique_ptr<UndoAction> UndoStack::Pop()
{
    assert(!mStack.empty());

    // Extract undo action
    auto undoAction = std::move(mStack.back());
    mStack.pop_back();

    // Update total cost
    assert(mTotalCost >= undoAction->GetCost());
    mTotalCost -= undoAction->GetCost();

    return undoAction;
}

}