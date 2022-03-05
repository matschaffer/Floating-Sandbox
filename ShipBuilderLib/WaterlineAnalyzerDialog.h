/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2022-03-03
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "IUserInterface.h"
#include "Model.h"
#include "View.h"
#include "WaterlineAnalyzer.h"

#include <Game/ResourceLocator.h>

#include <GameCore/GameTypes.h>

#include <wx/bmpbuttn.h>
#include <wx/dialog.h>
#include <wx/textctrl.h>
#include <wx/timer.h>

#include <memory>
#include <optional>

namespace ShipBuilder {

class WaterlineAnalyzerDialog : public wxDialog
{
public:

    WaterlineAnalyzerDialog(
        wxWindow * parent,
        wxPoint const & centerScreen,
        Model const & model,
        View & view,
        IUserInterface & userInterface,
        UnitsSystem displayUnitsSystem,
        ResourceLocator const & resourceLocator);

private:

    void OnRefreshTimer(wxTimerEvent & event);
    void OnClose(wxCloseEvent & event);

    void InitializeAnalysis();
    void ReconcileUIWithState();
    void PopulateAnalysisText(
        std::optional<WaterlineAnalyzer::StaticResults> const & staticResults,
        std::optional<float> totalBuoyantForce);

    void DoStep();

private:

    Model const & mModel;
    View & mView;
    IUserInterface & mUserInterface;

    UnitsSystem const mDisplayUnitsSystem;

    //
    // UI
    //

    wxBitmapButton * mPlayContinuouslyButton;
    wxBitmapButton * mPlayStepByStepButton;
    wxBitmapButton * mRewindButton;
    wxTextCtrl * mAnalysisTextCtrl;
    std::unique_ptr<wxTimer> mRefreshTimer;

    //
    // State
    //

    std::unique_ptr<WaterlineAnalyzer> mWaterlineAnalyzer;

    enum class StateType
    {
        Paused,
        Playing,
        Completed
    };

    StateType mCurrentState;
};

}