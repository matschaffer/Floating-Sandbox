/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2021-08-29
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ShipBuilderTypes.h"

#include <GameCore/GameTypes.h>
#include <Game/Materials.h>
#include <Game/MaterialDatabase.h>
#include <Game/ResourceLocator.h>
#include <Game/ShipTexturizer.h>

#include <wx/wx.h>
#include <wx/popupwin.h>
#include <wx/tglbtn.h>

#include <optional>
#include <vector>

namespace ShipBuilder {

/*
 * Event fired when a structural|electrical material has been selected.
 */
template<typename TMaterial>
class _fsMaterialSelectedEvent : public wxEvent
{
public:

    _fsMaterialSelectedEvent(
        wxEventType eventType,
        int winid,
        TMaterial const * material,
        MaterialPlaneType materialPlane)
        : wxEvent(winid, eventType)
        , mMaterial(material)
        , mMaterialPlane(materialPlane)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    _fsMaterialSelectedEvent(_fsMaterialSelectedEvent const & other)
        : wxEvent(other)
        , mMaterial(other.mMaterial)
        , mMaterialPlane(other.mMaterialPlane)
    {
        m_propagationLevel = wxEVENT_PROPAGATE_MAX;
    }

    virtual wxEvent * Clone() const override
    {
        return new _fsMaterialSelectedEvent(*this);
    }

    TMaterial const * GetMaterial() const
    {
        return mMaterial;
    }

    MaterialPlaneType GetMaterialPlane() const
    {
        return mMaterialPlane;
    }

private:

    TMaterial const * const mMaterial;
    MaterialPlaneType const mMaterialPlane;
};

using fsStructuralMaterialSelectedEvent = _fsMaterialSelectedEvent<StructuralMaterial>;
using fsElectricalMaterialSelectedEvent = _fsMaterialSelectedEvent<ElectricalMaterial>;

wxDECLARE_EVENT(fsEVT_STRUCTURAL_MATERIAL_SELECTED, fsStructuralMaterialSelectedEvent);
wxDECLARE_EVENT(fsEVT_ELECTRICAL_MATERIAL_SELECTED, fsElectricalMaterialSelectedEvent);

template<typename TMaterial>
class MaterialPalette : public wxPopupTransientWindow
{
public:

    MaterialPalette(
        wxWindow * parent,
        MaterialDatabase::Palette<TMaterial> const & materialPalette,
        ShipTexturizer const & shipTexturizer,
        ResourceLocator const & resourceLocator);

    void Open(
        wxPoint const & position,
        wxRect const & referenceArea,
        MaterialPlaneType planeType,
        TMaterial const * initialMaterial);

private:

    // All categories + 1 ("empty")
    std::vector<wxToggleButton *> mCategoryButtons;

    std::optional<MaterialPlaneType> mCurrentPlaneType;

    // TODOTEST
    wxPanel * mCategoryPanelTODOTEST;
};

}