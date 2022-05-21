﻿/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2022-04-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ViewManager.h"

#include <GameCore/GameMath.h>
#include <GameCore/Log.h>

#include <cassert>

float constexpr NdcFractionZoomTarget = 0.7f; // Fraction of the [0, 2] NDC space that needs to be occupied by AABB

ViewManager::ViewManager(
    Render::RenderContext & renderContext,
    NotificationLayer & notificationLayer)
    : mRenderContext(renderContext)
    , mNotificationLayer(notificationLayer)
    , mZoomParameterSmoother()
    , mCameraWorldPositionParameterSmoother()
    // Defaults
    , mDoAutoFocusOnShipLoad(true)
    , mAutoFocus()
{
    mZoomParameterSmoother = std::make_unique<ParameterSmoother<float>>(
        [this]() -> float const &
        {
            return mRenderContext.GetZoom();
        },
        [this](float const & value) -> float const &
        {
            return mRenderContext.SetZoom(value);
        },
        [this](float const & value)
        {
            return mRenderContext.ClampZoom(value);
        },
        0.05f);

    mCameraWorldPositionParameterSmoother = std::make_unique<ParameterSmoother<vec2f>>(
        [this]() -> vec2f const &
        {
            return mRenderContext.GetCameraWorldPosition();
        },
        [this](vec2f const & value) -> vec2f const &
        {
            return mRenderContext.SetCameraWorldPosition(value);
        },
        [this](vec2f const & value)
        {
            return mRenderContext.ClampCameraWorldPosition(value);
        },
        // TODOTEST
        //0.18f);
        0.1f);
}

bool ViewManager::GetDoAutoFocusOnShipLoad() const
{ 
    return mDoAutoFocusOnShipLoad;
}

void ViewManager::SetDoAutoFocusOnShipLoad(bool value)
{ 
    mDoAutoFocusOnShipLoad = value;
}

bool ViewManager::GetDoContinuousAutoFocus() const
{ 
    return mAutoFocus.has_value(); 
}

void ViewManager::SetDoContinuousAutoFocus(bool value)
{
    if (value)
    {
        // Start auto-focus
        assert(!mAutoFocus.has_value());
        mAutoFocus.emplace(
            mZoomParameterSmoother->GetValue(),
            mCameraWorldPositionParameterSmoother->GetValue());
    }
    else
    {
        // Stop auto-focus
        assert(mAutoFocus.has_value());
        mAutoFocus.reset();
    }

    mNotificationLayer.SetAutoFocusIndicator(mAutoFocus.has_value());
}

void ViewManager::OnViewModelUpdated()
{
    // Pickup eventual changes to view model constraints
    mZoomParameterSmoother->ReClamp();
    mCameraWorldPositionParameterSmoother->ReClamp();
}

void ViewManager::OnNewShip(Geometry::AABBSet const & allAABBs)
{
    if (mDoAutoFocusOnShipLoad)
    {
        FocusOnShip(allAABBs);
    }
}

void ViewManager::Pan(vec2f const & worldOffset)
{
    if (!mAutoFocus.has_value())
    {
        vec2f const newTargetCameraWorldPosition =
            mCameraWorldPositionParameterSmoother->GetValue()
            + worldOffset;

        mCameraWorldPositionParameterSmoother->SetValue(newTargetCameraWorldPosition);
    }
    else
    {
        mAutoFocus->UserCameraWorldPositionOffset += worldOffset;
    }
}

void ViewManager::PanToWorldX(float worldX)
{
    if (!mAutoFocus.has_value())
    {
        vec2f const newTargetCameraWorldPosition = vec2f(
            worldX,
            mCameraWorldPositionParameterSmoother->GetValue().y);

        mCameraWorldPositionParameterSmoother->SetValue(newTargetCameraWorldPosition);
    }
    else
    {
        mAutoFocus->UserCameraWorldPositionOffset.x = worldX;
    }
}

void ViewManager::AdjustZoom(float amount)
{
    if (!mAutoFocus.has_value())
    {
        float const newTargetZoom = mZoomParameterSmoother->GetValue() * amount;

        mZoomParameterSmoother->SetValue(newTargetZoom);
    }
    else
    {
        mAutoFocus->UserZoomOffset *= amount;
    }
}

void ViewManager::ResetView(Geometry::AABBSet const & allAABBs)
{
    // When continuous auto-focus is off, "view reset" focuses on ship;
    // When continuous auto-focus is on, "view reset" zeroes-out user offsets
    if (!mAutoFocus.has_value())
    {
        InternalFocusOnShip(allAABBs);
    }
    else
    {
        mAutoFocus->Reset();
    }
}

void ViewManager::FocusOnShip(Geometry::AABBSet const & allAABBs)
{
    if (!mAutoFocus.has_value())
    {
        InternalFocusOnShip(allAABBs);
    }
    else
    {
        mAutoFocus->Reset();
    }
}

void ViewManager::Update(Geometry::AABBSet const & allAABBs)
{
    if (mAutoFocus.has_value())
    {
        float newAutoFocusZoom;
        vec2f newAutoFocusCameraWorldPosition;

        auto const unionAABB = allAABBs.MakeUnion();
        if (unionAABB.has_value())
        {
            //
            // Auto-focus algorithm:
            // - Zoom:
            //      - Auto-focus zoom is the *minimum* (closest to zero, i.e. furthest) zoom that ensures
            //        the AABB's edges are within a specific sub-window of the physical display window
            //      - User zoom is the zoom offset exherted by the user
            //      - The final zoom is the sum of the two zooms
            // - Pan:
            //      - Auto-focus pan is the pan required to ensure that the center of the AABB is at
            //        the center of the physical display window, after auto-focus zoom is applied
            //      - User pan is the pan offset exherted by the user
            //      - The final pan is the sum of the two pans
            //

            // TODOTEST - orig
            //////
            ////// Zoom
            //////
            ////
            //////float constexpr ZoomThreshold = 0.1f;
            ////float constexpr ZoomThreshold = 0.0f;

            ////// Calculate NDC of AABB, using the current auto-focus params (net of user offsets)
            ////vec2f const ndcBottomLeft = mRenderContext.WorldToNdc(unionAABB->BottomLeft, mAutoFocus->CurrentAutoFocusZoom, mAutoFocus->CurrentAutoFocusCameraWorldPosition);
            ////vec2f const ndcTopRight = mRenderContext.WorldToNdc(unionAABB->TopRight, mAutoFocus->CurrentAutoFocusZoom, mAutoFocus->CurrentAutoFocusCameraWorldPosition);

            ////// Calculate how to change current auto-focus zoom (thus _net_ of user offset) so that AABB is within view
            ////float autoFocusZoomAdjustment = 0.0f; // Value out of domain

            ////if (ndcBottomLeft.x < 0.0f)
            ////{
            ////    autoFocusZoomAdjustment = NdcFractionZoomTarget / (-ndcBottomLeft.x);
            ////}

            ////if (ndcTopRight.x > 0.0f)
            ////{
            ////    if (autoFocusZoomAdjustment == 0.0f)
            ////        autoFocusZoomAdjustment = NdcFractionZoomTarget / (ndcTopRight.x);
            ////    else
            ////        autoFocusZoomAdjustment = std::min(NdcFractionZoomTarget / (ndcTopRight.x), autoFocusZoomAdjustment);
            ////}

            ////if (ndcBottomLeft.y < 0.0f)
            ////{
            ////    if (autoFocusZoomAdjustment == 0.0f)
            ////        autoFocusZoomAdjustment = NdcFractionZoomTarget / (-ndcBottomLeft.y);
            ////    else
            ////        autoFocusZoomAdjustment = std::min(NdcFractionZoomTarget / (-ndcBottomLeft.y), autoFocusZoomAdjustment);
            ////}

            ////if (ndcTopRight.y > 0.0f)
            ////{
            ////    if (autoFocusZoomAdjustment == 0.0f)
            ////        autoFocusZoomAdjustment = NdcFractionZoomTarget / (ndcTopRight.y);
            ////    else
            ////        autoFocusZoomAdjustment = std::min(NdcFractionZoomTarget / (ndcTopRight.y), autoFocusZoomAdjustment);
            ////}

            ////if (autoFocusZoomAdjustment == 0.0f)
            ////    autoFocusZoomAdjustment = 1.0f;

            ////// Calculate new auto-focus zoom
            ////newAutoFocusZoom = mAutoFocus->CurrentAutoFocusZoom * autoFocusZoomAdjustment;

            ////// Check change against threshold
            ////if (std::abs(newAutoFocusZoom - mAutoFocus->CurrentAutoFocusZoom) > ZoomThreshold)
            ////{
            ////    // Apply auto-focus
            ////    mAutoFocus->CurrentAutoFocusZoom = newAutoFocusZoom;
            ////}
            ////else
            ////{
            ////    // Stay
            ////    newAutoFocusZoom = mAutoFocus->CurrentAutoFocusZoom;
            ////}

            //////
            ////// Pan
            //////

            //////float constexpr CameraPositionThreshold = 30.0f; // Attempt to avoid panning with waves
            ////float constexpr CameraPositionThreshold = 0.0f;

            ////// Calculate world offset required to center view onto AABB's center
            ////vec2f const aabbCenterWorld = unionAABB->CalculateCenter();
            ////vec2f const newAutoFocusCameraWorldPositionOffset = (aabbCenterWorld - mAutoFocus->CurrentAutoFocusCameraWorldPosition) / 2.0f;

            ////// Check change against threshold
            ////if (std::abs(newAutoFocusCameraWorldPositionOffset.x) > CameraPositionThreshold
            ////    || std::abs(newAutoFocusCameraWorldPositionOffset.y) > CameraPositionThreshold)
            ////{
            ////    // Apply auto-focus
            ////    newAutoFocusCameraWorldPosition = mAutoFocus->CurrentAutoFocusCameraWorldPosition + newAutoFocusCameraWorldPositionOffset;
            ////    mAutoFocus->CurrentAutoFocusCameraWorldPosition = newAutoFocusCameraWorldPosition;
            ////}
            ////else
            ////{
            ////    // Stay
            ////    newAutoFocusCameraWorldPosition = mAutoFocus->CurrentAutoFocusCameraWorldPosition;
            ////}


            // TODOTEST - 2
            //////
            ////// Calculate required zoom
            //////
            ////
            ////// Calculate NDC of AABB, using the current auto-focus params (net of user offsets)
            ////vec2f const ndcBottomLeft = mRenderContext.WorldToNdc(unionAABB->BottomLeft, mAutoFocus->CurrentAutoFocusZoom, mAutoFocus->CurrentAutoFocusCameraWorldPosition);
            ////vec2f const ndcTopRight = mRenderContext.WorldToNdc(unionAABB->TopRight, mAutoFocus->CurrentAutoFocusZoom, mAutoFocus->CurrentAutoFocusCameraWorldPosition);

            ////// Calculate how to change current auto-focus zoom (thus _net_ of user offset) so that AABB is within view
            ////float autoFocusZoomAdjustment = 0.0f; // Value out of domain

            ////if (ndcBottomLeft.x < 0.0f)
            ////{
            ////    autoFocusZoomAdjustment = NdcFractionZoomTarget / (-ndcBottomLeft.x);
            ////}

            ////if (ndcTopRight.x > 0.0f)
            ////{
            ////    if (autoFocusZoomAdjustment == 0.0f)
            ////        autoFocusZoomAdjustment = NdcFractionZoomTarget / (ndcTopRight.x);
            ////    else
            ////        autoFocusZoomAdjustment = std::min(NdcFractionZoomTarget / (ndcTopRight.x), autoFocusZoomAdjustment);
            ////}

            ////if (ndcBottomLeft.y < 0.0f)
            ////{
            ////    if (autoFocusZoomAdjustment == 0.0f)
            ////        autoFocusZoomAdjustment = NdcFractionZoomTarget / (-ndcBottomLeft.y);
            ////    else
            ////        autoFocusZoomAdjustment = std::min(NdcFractionZoomTarget / (-ndcBottomLeft.y), autoFocusZoomAdjustment);
            ////}

            ////if (ndcTopRight.y > 0.0f)
            ////{
            ////    if (autoFocusZoomAdjustment == 0.0f)
            ////        autoFocusZoomAdjustment = NdcFractionZoomTarget / (ndcTopRight.y);
            ////    else
            ////        autoFocusZoomAdjustment = std::min(NdcFractionZoomTarget / (ndcTopRight.y), autoFocusZoomAdjustment);
            ////}

            ////if (autoFocusZoomAdjustment == 0.0f)
            ////    autoFocusZoomAdjustment = 1.0f;

            ////// Calculate new auto-focus zoom
            ////newAutoFocusZoom = mAutoFocus->CurrentAutoFocusZoom * autoFocusZoomAdjustment;

            //////
            ////// Calculate required pan
            //////

            ////// Calculate world offset required to center view onto AABB's center
            ////vec2f const aabbCenterWorld = unionAABB->CalculateCenter();
            ////vec2f const newAutoFocusCameraWorldPositionAdjustment = (aabbCenterWorld - mAutoFocus->CurrentAutoFocusCameraWorldPosition) / 2.0f;

            //////
            ////// Check thresholds and decide whether to adjust or not
            //////

            ////float constexpr ZoomThreshold = 0.1f;
            //////float constexpr ZoomThreshold = 0.0f;

            ////float constexpr CameraPositionThreshold = 30.0f; // Attempt to avoid panning with waves
            //////float constexpr CameraPositionThreshold = 0.0f;

            ////////if (std::abs(newAutoFocusZoom - mAutoFocus->CurrentAutoFocusZoom) > ZoomThreshold
            ////////    || std::abs(newAutoFocusCameraWorldPositionAdjustment.x) > CameraPositionThreshold
            ////////    || std::abs(newAutoFocusCameraWorldPositionAdjustment.y) > CameraPositionThreshold
            ////////    || mZoomParameterSmoother->IsSmoothing()
            ////////    || mCameraWorldPositionParameterSmoother->IsSmoothing())
            ////////{
            ////////    // Apply auto-focus
            ////////    mAutoFocus->CurrentAutoFocusZoom = newAutoFocusZoom;
            ////////    newAutoFocusCameraWorldPosition = mAutoFocus->CurrentAutoFocusCameraWorldPosition + newAutoFocusCameraWorldPositionAdjustment;
            ////////    mAutoFocus->CurrentAutoFocusCameraWorldPosition = newAutoFocusCameraWorldPosition;
            ////////}
            ////////else
            ////////{
            ////////    // Stay
            ////////    newAutoFocusZoom = mAutoFocus->CurrentAutoFocusZoom;
            ////////    newAutoFocusCameraWorldPosition = mAutoFocus->CurrentAutoFocusCameraWorldPosition;
            ////////}

            ////mAutoFocus->CurrentAutoFocusZoom = newAutoFocusZoom;

            ////newAutoFocusCameraWorldPosition.x = mAutoFocus->CurrentAutoFocusCameraWorldPosition.x + newAutoFocusCameraWorldPositionAdjustment.x;

            ////if (std::abs(newAutoFocusCameraWorldPositionAdjustment.y) > CameraPositionThreshold)
            ////{
            ////    // Apply auto-focus
            ////    newAutoFocusCameraWorldPosition.y = mAutoFocus->CurrentAutoFocusCameraWorldPosition.y + newAutoFocusCameraWorldPositionAdjustment.y;
            ////}
            ////else
            ////{
            ////    // Stay
            ////    newAutoFocusCameraWorldPosition.y = mAutoFocus->CurrentAutoFocusCameraWorldPosition.y;
            ////}

            ////mAutoFocus->CurrentAutoFocusCameraWorldPosition = newAutoFocusCameraWorldPosition;

            ////// TODOTEST - 3 || Smoothstep, Pan based on World Center

            //////
            ////// Pan
            //////

            ////// Calculate world offset required to center view onto AABB's center
            ////vec2f const aabbCenterWorld = unionAABB->CalculateCenter();
            ////vec2f const newAutoFocusCameraWorldPositionOffset = (aabbCenterWorld - mAutoFocus->CurrentAutoFocusCameraWorldPosition) / 2.0f;

            ////// X
            ////newAutoFocusCameraWorldPosition.x = mAutoFocus->CurrentAutoFocusCameraWorldPosition.x + newAutoFocusCameraWorldPositionOffset.x;

            ////// Y
            ////newAutoFocusCameraWorldPosition.y = mAutoFocus->CurrentAutoFocusCameraWorldPosition.y + newAutoFocusCameraWorldPositionOffset.y * SmoothStep(0.0f, 30.0f, std::abs(newAutoFocusCameraWorldPositionOffset.y));

            ////mAutoFocus->CurrentAutoFocusCameraWorldPosition = newAutoFocusCameraWorldPosition;


            //////
            ////// Zoom
            //////
            ////
            //////float constexpr ZoomThreshold = 0.1f;
            ////float constexpr ZoomThreshold = 0.0f;

            ////// Calculate NDC of AABB, using the current auto-focus params (net of user offsets)
            ////vec2f const ndcBottomLeft = mRenderContext.WorldToNdc(unionAABB->BottomLeft, mAutoFocus->CurrentAutoFocusZoom, mAutoFocus->CurrentAutoFocusCameraWorldPosition);
            ////vec2f const ndcTopRight = mRenderContext.WorldToNdc(unionAABB->TopRight, mAutoFocus->CurrentAutoFocusZoom, mAutoFocus->CurrentAutoFocusCameraWorldPosition);

            ////// Calculate how to change current auto-focus zoom (thus _net_ of user offset) so that AABB is within view
            ////float autoFocusZoomAdjustment = 0.0f; // Value out of domain

            ////if (ndcBottomLeft.x < 0.0f)
            ////{
            ////    autoFocusZoomAdjustment = NdcFractionZoomTarget / (-ndcBottomLeft.x);
            ////}

            ////if (ndcTopRight.x > 0.0f)
            ////{
            ////    if (autoFocusZoomAdjustment == 0.0f)
            ////        autoFocusZoomAdjustment = NdcFractionZoomTarget / (ndcTopRight.x);
            ////    else
            ////        autoFocusZoomAdjustment = std::min(NdcFractionZoomTarget / (ndcTopRight.x), autoFocusZoomAdjustment);
            ////}

            ////if (ndcBottomLeft.y < 0.0f)
            ////{
            ////    if (autoFocusZoomAdjustment == 0.0f)
            ////        autoFocusZoomAdjustment = NdcFractionZoomTarget / (-ndcBottomLeft.y);
            ////    else
            ////        autoFocusZoomAdjustment = std::min(NdcFractionZoomTarget / (-ndcBottomLeft.y), autoFocusZoomAdjustment);
            ////}

            ////if (ndcTopRight.y > 0.0f)
            ////{
            ////    if (autoFocusZoomAdjustment == 0.0f)
            ////        autoFocusZoomAdjustment = NdcFractionZoomTarget / (ndcTopRight.y);
            ////    else
            ////        autoFocusZoomAdjustment = std::min(NdcFractionZoomTarget / (ndcTopRight.y), autoFocusZoomAdjustment);
            ////}

            ////if (autoFocusZoomAdjustment == 0.0f)
            ////    autoFocusZoomAdjustment = 1.0f;

            ////// Calculate new auto-focus zoom
            ////newAutoFocusZoom = mAutoFocus->CurrentAutoFocusZoom * autoFocusZoomAdjustment;

            ////// Check change against threshold
            ////if (std::abs(newAutoFocusZoom - mAutoFocus->CurrentAutoFocusZoom) > ZoomThreshold)
            ////{
            ////    // Apply auto-focus
            ////    mAutoFocus->CurrentAutoFocusZoom = newAutoFocusZoom;
            ////}
            ////else
            ////{
            ////    // Stay
            ////    newAutoFocusZoom = mAutoFocus->CurrentAutoFocusZoom;
            ////}



            // TODOTEST - 4 || Smoothstep, Pan based on NDC

            //
            // Pan
            //

            // Calculate NDC offset required to center view onto AABB's center (net of user offsets)
            vec2f const aabbCenterNdc = mRenderContext.WorldToNdc(unionAABB->CalculateCenter(), mAutoFocus->CurrentAutoFocusZoom, mAutoFocus->CurrentAutoFocusCameraWorldPosition);
            vec2f const newAutoFocusCameraPositionNdcOffset = aabbCenterNdc / 2.0f;

            // Convert back into world offset
            vec2f const newAutoFocusCameraWorldPositionOffset = mRenderContext.NdcOffsetToWorldOffset(
                vec2f(
                    newAutoFocusCameraPositionNdcOffset.x,
                    newAutoFocusCameraPositionNdcOffset.y * SmoothStep(0.0f, 0.4f, std::abs(newAutoFocusCameraPositionNdcOffset.y))),
                mAutoFocus->CurrentAutoFocusZoom);

            newAutoFocusCameraWorldPosition = mAutoFocus->CurrentAutoFocusCameraWorldPosition + newAutoFocusCameraWorldPositionOffset;
            mAutoFocus->CurrentAutoFocusCameraWorldPosition = newAutoFocusCameraWorldPosition;


            //
            // Zoom
            //
            
            // Calculate NDC of AABB, using the current auto-focus params (net of user offsets)
            vec2f const ndcBottomLeft = mRenderContext.WorldToNdc(unionAABB->BottomLeft, mAutoFocus->CurrentAutoFocusZoom, mAutoFocus->CurrentAutoFocusCameraWorldPosition);
            vec2f const ndcTopRight = mRenderContext.WorldToNdc(unionAABB->TopRight, mAutoFocus->CurrentAutoFocusZoom, mAutoFocus->CurrentAutoFocusCameraWorldPosition);

            // Calculate how to change current auto-focus zoom (thus _net_ of user offset) so that AABB is within view
            float autoFocusZoomAdjustment = 0.0f; // Value out of domain

            if (ndcBottomLeft.x < 0.0f)
            {
                autoFocusZoomAdjustment = NdcFractionZoomTarget / (-ndcBottomLeft.x);
            }

            if (ndcTopRight.x > 0.0f)
            {
                float const adj = NdcFractionZoomTarget / (ndcTopRight.x);

                if (autoFocusZoomAdjustment == 0.0f)
                    autoFocusZoomAdjustment = adj;
                else
                    autoFocusZoomAdjustment = std::min(adj, autoFocusZoomAdjustment);
            }

            if (ndcBottomLeft.y < 0.0f)
            {
                float const adj = NdcFractionZoomTarget / (-ndcBottomLeft.y);

                if (autoFocusZoomAdjustment == 0.0f)
                    autoFocusZoomAdjustment = adj;
                else
                    autoFocusZoomAdjustment = std::min(adj, autoFocusZoomAdjustment);
            }

            if (ndcTopRight.y > 0.0f)
            {
                float const adj = NdcFractionZoomTarget / (ndcTopRight.y);

                if (autoFocusZoomAdjustment == 0.0f)
                    autoFocusZoomAdjustment = adj;
                else
                    autoFocusZoomAdjustment = std::min(adj, autoFocusZoomAdjustment);
            }

            // TODOTEST
            //LogMessage("NDC: Horz=(", ndcBottomLeft.x, ", ", ndcTopRight.x, ") Vert=(", ndcBottomLeft.y, ", ", ndcTopRight.y, ") ZA=", autoFocusZoomAdjustment);

            if (autoFocusZoomAdjustment == 0.0f)
                autoFocusZoomAdjustment = 1.0f;

            // Calculate new auto-focus zoom
            newAutoFocusZoom = mAutoFocus->CurrentAutoFocusZoom * autoFocusZoomAdjustment;
            // TODOTEST: TOTAL OVERRIDE with original algo
            newAutoFocusZoom = std::min(
                mRenderContext.CalculateZoomForWorldWidth(unionAABB->GetWidth() / NdcFractionZoomTarget),
                mRenderContext.CalculateZoomForWorldHeight(unionAABB->GetHeight() / NdcFractionZoomTarget));
            mAutoFocus->CurrentAutoFocusZoom = newAutoFocusZoom;
        }
        else
        {
            newAutoFocusZoom = mAutoFocus->CurrentAutoFocusZoom;
            newAutoFocusCameraWorldPosition = mAutoFocus->CurrentAutoFocusCameraWorldPosition;
        }

        //
        // Set zoom
        //

        mZoomParameterSmoother->SetValue(newAutoFocusZoom * mAutoFocus->UserZoomOffset);

        // If we've clamped the zoom, erode lost zoom from user offset
        {
            float const impliedUserOffset = mZoomParameterSmoother->GetValue() / newAutoFocusZoom;

            mAutoFocus->UserZoomOffset = Clamp(
                impliedUserOffset,
                std::min(mAutoFocus->UserZoomOffset, 1.0f),
                std::max(mAutoFocus->UserZoomOffset, 1.0f));
        }

        //
        // Set pan
        //

        mCameraWorldPositionParameterSmoother->SetValue(newAutoFocusCameraWorldPosition + mAutoFocus->UserCameraWorldPositionOffset);

        // If we've clamped the pan, erode lost panning from user offset
        {
            vec2f const impliedUserOffset = mCameraWorldPositionParameterSmoother->GetValue() - newAutoFocusCameraWorldPosition;

            mAutoFocus->UserCameraWorldPositionOffset = vec2f(
                Clamp(
                impliedUserOffset.x,
                std::min(0.0f, mAutoFocus->UserCameraWorldPositionOffset.x),
                std::max(0.0f, mAutoFocus->UserCameraWorldPositionOffset.x)),
                Clamp(
                impliedUserOffset.y,
                std::min(0.0f, mAutoFocus->UserCameraWorldPositionOffset.y),
                std::max(0.0f, mAutoFocus->UserCameraWorldPositionOffset.y)));
        }
    }

    mZoomParameterSmoother->Update();
    mCameraWorldPositionParameterSmoother->Update();
}

void ViewManager::InternalFocusOnShip(Geometry::AABBSet const & allAABBs)
{
    auto const unionAABB = allAABBs.MakeUnion();
    if (unionAABB.has_value())
    {
        float const newAutoFocusZoom = std::min(
            mRenderContext.CalculateZoomForWorldWidth(unionAABB->GetWidth() / NdcFractionZoomTarget),
            mRenderContext.CalculateZoomForWorldHeight(unionAABB->GetHeight() / NdcFractionZoomTarget));

        mZoomParameterSmoother->SetValue(newAutoFocusZoom);

        vec2f const newWorldCenter = unionAABB->CalculateCenter();

        mCameraWorldPositionParameterSmoother->SetValue(newWorldCenter);
    }
}