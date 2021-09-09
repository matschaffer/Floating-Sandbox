/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-28
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IUserInterface.h"
#include "Model.h"
#include "ShipBuilderTypes.h"
#include "View.h"

#include <filesystem>
#include <memory>

namespace ShipBuilder {

/*
 * This class implements operations on the model.
 */
class ModelController
{
public:

    static std::unique_ptr<ModelController> CreateNew(
        WorkSpaceSize const & workSpaceSize,
        View & view,
        IUserInterface & userInterface);

    static std::unique_ptr<ModelController> Load(
        std::filesystem::path const & shipFilePath,
        View & view,
        IUserInterface & userInterface);

    WorkSpaceSize const & GetWorkSpaceSize() const
    {
        return mModel.GetWorkSpaceSize();
    }

    LayerType GetPrimaryLayer() const
    {
        return mPrimaryLayer;
    }

    void SelectPrimaryLayer(LayerType primaryLayer);

private:

    ModelController(
        WorkSpaceSize const & workSpaceSize,
        View & view,
        IUserInterface & userInterface);

private:

    View & mView;
    IUserInterface & mUserInterface;

    Model mModel;

    //
    // State
    //

    LayerType mPrimaryLayer;
};

}