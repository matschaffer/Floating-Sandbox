
#
# ShipBuilder library
#

set (SHIPBUILDER_LIB_SOURCES
	ClipboardManager.h
	Controller.cpp
	Controller.h
	IModelObservable.h
	InstancedElectricalElementSet.h
	IUserInterface.h
	MainFrame.cpp
	MainFrame.h
	Model.cpp
	Model.h
	ModelController.cpp
	ModelController.h
	ModelValidationResults.h
	ModelValidator.cpp
	ModelValidator.h
	OpenGLManager.h
	SelectionManager.h
	ShaderTypes.cpp
	ShaderTypes.h
	ShipBuilderTypes.h
	ShipNameNormalizer.cpp
	ShipNameNormalizer.h
	TextureTypes.h
	UndoStack.cpp
	UndoStack.h
	View.cpp
	View.h
	ViewModel.h
	WaterlineAnalyzer.cpp
	WaterlineAnalyzer.h
	WorkbenchState.cpp
	WorkbenchState.h
)

set (UI_SOURCES
	UI/AskPasswordDialog.cpp
	UI/AskPasswordDialog.h
	UI/CompositeMaterialPalette.cpp
	UI/CompositeMaterialPalette.h
	UI/ElectricalPanelEditDialog.cpp
	UI/ElectricalPanelEditDialog.h
	UI/ElectricalPanelLayoutControl.cpp
	UI/ElectricalPanelLayoutControl.h
	UI/MaterialPalette.cpp
	UI/MaterialPalette.h
	UI/ModelValidationDialog.cpp
	UI/ModelValidationDialog.h
	UI/NewPasswordDialog.cpp
	UI/NewPasswordDialog.h
	UI/NewShipNameDialog.cpp
	UI/NewShipNameDialog.h
	UI/PreferencesDialog.cpp
	UI/PreferencesDialog.h
	UI/ResizeDialog.cpp
	UI/ResizeDialog.h
	UI/RibbonToolbarButton.h
	UI/ShipNameSuggestionDialog.cpp
	UI/ShipNameSuggestionDialog.h
	UI/ShipOffsetVisualizationControl.cpp
	UI/ShipOffsetVisualizationControl.h
	UI/ShipPropertiesEditDialog.cpp
	UI/ShipPropertiesEditDialog.h
	UI/ShipResizeVisualizationControl.cpp
	UI/ShipResizeVisualizationControl.h
	UI/StatusBar.cpp
	UI/StatusBar.h
	UI/WaterlineAnalysisOutcomeVisualizationControl.cpp
	UI/WaterlineAnalysisOutcomeVisualizationControl.h
	UI/WaterlineAnalyzerDialog.cpp
	UI/WaterlineAnalyzerDialog.h
)

set (TOOLS_SOURCES
	Tools/FloodTool.cpp
	Tools/FloodTool.h
	Tools/LineTool.cpp
	Tools/LineTool.h
	Tools/MeasuringTapeTool.cpp
	Tools/MeasuringTapeTool.h
	Tools/PencilTool.cpp
	Tools/PencilTool.h
	Tools/RopeEraserTool.cpp
	Tools/RopeEraserTool.h
	Tools/RopePencilTool.cpp
	Tools/RopePencilTool.h
	Tools/SamplerTool.cpp
	Tools/SamplerTool.h
	Tools/SelectionTool.cpp
	Tools/SelectionTool.h
	Tools/TextureEraserTool.cpp
	Tools/TextureEraserTool.h
	Tools/TextureMagicWandTool.cpp
	Tools/TextureMagicWandTool.h
	Tools/Tool.cpp
	Tools/Tool.h
)

source_group(" " FILES ${SHIPBUILDER_LIB_SOURCES})
source_group("Tools" FILES ${TOOLS_SOURCES})
source_group("UI" FILES ${UI_SOURCES})

add_library (ShipBuilderLib ${SHIPBUILDER_LIB_SOURCES} ${TOOLS_SOURCES} ${UI_SOURCES})

target_include_directories(ShipBuilderLib PUBLIC .)
target_include_directories(ShipBuilderLib PRIVATE ${wxWidgets_INCLUDE_DIRS})
target_compile_definitions(ShipBuilderLib PRIVATE "${wxWidgets_DEFINITIONS}")
target_link_libraries (ShipBuilderLib
	GameLib
	GameCoreLib
	${wxWidgets_LIBRARIES}
	${ADDITIONAL_LIBRARIES})
