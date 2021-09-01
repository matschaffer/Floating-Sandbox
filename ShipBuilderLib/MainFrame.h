/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2021-08-27
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Controller.h"
#include "IUserInterface.h"
#include "MaterialPalette.h"
#include "View.h"
#include "WorkbenchState.h"

#include <UILib/LocalizationManager.h>
#include <UILib/LoggingDialog.h>

#include <Game/MaterialDatabase.h>
#include <Game/ResourceLocator.h>
#include <Game/ShipTexturizer.h>

#include <GameOpenGL/GameOpenGL.h>

#include <wx/app.h>
#include <wx/frame.h>
#include <wx/glcanvas.h> // Need to include this *after* our glad.h has been included, so that wxGLCanvas ends
                         // up *not* including the system's OpenGL header but glad's instead
#include <wx/menu.h>
#include <wx/panel.h>
#include <wx/scrolbar.h>
#include <wx/statbmp.h>
#include <wx/statusbr.h>

#include <filesystem>
#include <functional>
#include <memory>

namespace ShipBuilder {

/*
 * The main window of the ship builder GUI.
 *
 * - Owns Controller and View
 * - Very thin, calls into Controller for each high-level interaction (e.g. new tool selected, tool setting changed) and for each mouse event
 * - Implements IUserInterface with interface needed by Controller, e.g. to make UI state changes, to capture the mouse, to update visualization of undo stack
 */
class MainFrame final : public wxFrame, public IUserInterface
{
public:

    MainFrame(
        wxApp * mainApp,
        ResourceLocator const & resourceLocator,
        LocalizationManager const & localizationManager,
        MaterialDatabase const & materialDatabase,
        ShipTexturizer const & shipTexturizer,
        std::function<void(std::optional<std::filesystem::path>)> returnToGameFunctor);

    void OpenForNewShip();

    void OpenForShip(std::filesystem::path const & shipFilePath);

public:

    //
    // IUserInterface
    //

    void DisplayToolCoordinates(std::optional<WorkSpaceCoordinates> coordinates) override;

    void OnWorkSpaceSizeChanged() override;

    void OnWorkbenchStateChanged() override;

private:

    wxPanel * CreateFilePanel(wxWindow * parent);
    wxPanel * CreateToolSettingsPanel(wxWindow * parent);
    wxPanel * CreateGamePanel(wxWindow * parent);
    wxPanel * CreateViewPanel(wxWindow * parent);
    wxPanel * CreateToolbarPanel(wxWindow * parent);
    wxPanel * CreateWorkPanel(wxWindow * parent);

    void OnForegroundMaterialSwath(wxMouseEvent & event);
    void OnBackgroundMaterialSwath(wxMouseEvent & event);

    void OnWorkCanvasPaint(wxPaintEvent & event);
    void OnWorkCanvasResize(wxSizeEvent & event);
    void OnWorkCanvasLeftDown(wxMouseEvent & event);
    void OnWorkCanvasLeftUp(wxMouseEvent & event);
    void OnWorkCanvasRightDown(wxMouseEvent & event);
    void OnWorkCanvasRightUp(wxMouseEvent & event);
    void OnWorkCanvasMouseMove(wxMouseEvent & event);
    void OnWorkCanvasMouseWheel(wxMouseEvent & event);
    void OnWorkCanvasCaptureMouseLost(wxMouseCaptureLostEvent & event);
    void OnWorkCanvasMouseLeftWindow(wxMouseEvent & event);

    void OnSaveAndGoBack(wxCommandEvent & event);
    void OnQuitAndGoBack(wxCommandEvent & event);
    void OnQuit(wxCommandEvent & event);
    void OnOpenLogWindowMenuItemSelected(wxCommandEvent & event);

private:

    bool IsStandAlone() const
    {
        return !mReturnToGameFunctor;
    }

    void Open();

    void SaveAndSwitchBackToGame();

    void QuitAndSwitchBackToGame();

    void SwitchBackToGame(std::optional<std::filesystem::path> shipFilePath);

    void RecalculatePanning();

    void SyncWorkbenchStateToUI();

private:

    wxApp * const mMainApp;

    std::function<void(std::optional<std::filesystem::path>)> const mReturnToGameFunctor;

    //
    // Owned members
    //

    std::unique_ptr<Controller> mController;
    std::unique_ptr<View> mView;

    //
    // Helpers
    //

    ResourceLocator const & mResourceLocator;
    LocalizationManager const & mLocalizationManager;
    MaterialDatabase const & mMaterialDatabase;
    ShipTexturizer const & mShipTexturizer;

    //
    // UI
    //

    wxPanel * mMainPanel;

    // Toolbar panel
    wxStaticBitmap * mForegroundMaterialStaticBitmap;
    wxStaticBitmap * mBackgroundMaterialStaticBitmap;

    // Work panel
    std::unique_ptr<wxGLCanvas> mWorkCanvas;
    std::unique_ptr<wxGLContext> mGLContext;
    wxScrollBar * mWorkCanvasHScrollBar;
    wxScrollBar * mWorkCanvasVScrollBar;

    // Misc UI elements
    std::unique_ptr<MaterialPalette<MaterialLayerType::Structural>> mStructuralMaterialPalette;
    std::unique_ptr<MaterialPalette<MaterialLayerType::Electrical>> mElectricalMaterialPalette;
    wxStatusBar * mStatusBar;

    //
    // Dialogs
    //

    std::unique_ptr<LoggingDialog> mLoggingDialog;

    //
    // UI state
    //

    bool mIsMouseCapturedByWorkCanvas;

    //
    // Abstract state
    //

    WorkbenchState mWorkbenchState;
    std::filesystem::path mOriginalGameShipFilePath;
};

}