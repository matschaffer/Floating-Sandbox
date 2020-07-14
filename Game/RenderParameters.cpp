/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-07-12
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderParameters.h"

namespace Render {

RenderParameters::RenderParameters(ImageSize const & initialCanvasSize)
	: View(1.0f, vec2f::zero(), initialCanvasSize.Width, initialCanvasSize.Height)
	, IsViewDirty(true)
	, IsCanvasSizeDirty(true)
	, AmbientLightIntensity(1.0f)
	, StormAmbientDarkening(1.0f)
	, EffectiveAmbientLightIntensity(1.0f)
	, IsEffectiveAmbientLightIntensityDirty(true)
	, RainDensity(0.0f)
	, IsRainDensityDirty(true)
	// TODOHERE
	, FlatSkyColor(0x87, 0xce, 0xfa) // (cornflower blue)	
	, OceanTransparency(0.8125f)
	, OceanDarkeningRate(0.356993f)
	, OceanRenderMode(OceanRenderModeType::Texture)
	, SelectedOceanTextureIndex(0) // Wavy Clear Thin
	, DepthOceanColorStart(0x4a, 0x84, 0x9f)
	, DepthOceanColorEnd(0x00, 0x00, 0x00)
	, FlatOceanColor(0x00, 0x3d, 0x99)
	, LandRenderMode(LandRenderModeType::Texture)
	, SelectedLandTextureIndex(3) // Rock Coarse 3
	, FlatLandColor(0x72, 0x46, 0x05)
	// Ship
	, ShipCount(0)
	, IsShipCountDirty(true)
	, FlatLampLightColor(0xff, 0xff, 0xbf)
	, DefaultWaterColor(0x00, 0x00, 0xcc)
	, ShowShipThroughOcean(false)
	, WaterContrast(0.71875f)
	, WaterLevelOfDetail(0.6875f)
	, DebugShipRenderMode(DebugShipRenderModeType::None)
	, VectorFieldRenderMode(VectorFieldRenderModeType::None)
	, VectorFieldLengthMultiplier(1.0f)
	, ShowStressedSprings(false)
	, DrawHeatOverlay(false)
	, HeatOverlayTransparency(0.1875f)
	, ShipFlameRenderMode(ShipFlameRenderModeType::Mode1)
	, ShipFlameSizeAdjustment(1.0f)
{
}

RenderParameters RenderParameters::Snapshot()
{
	// Make copy
	RenderParameters copy = *this;

	// Clear own 'dirty' flags
	IsViewDirty = false;
	IsCanvasSizeDirty = false;
	IsEffectiveAmbientLightIntensityDirty = false;
	IsRainDensityDirty = false;
	IsShipCountDirty = false;

	return copy;
}

}