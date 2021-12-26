/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2021-08-28
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "View.h"

#include <GameCore/GameException.h>
#include <GameCore/Log.h>
#include <GameCore/SysSpecifics.h>

namespace ShipBuilder {

View::View(
    ShipSpaceSize initialShipSpaceSize,
    DisplayLogicalSize initialDisplaySize,
    int logicalToPhysicalPixelFactor,
    std::function<void()> swapRenderBuffersFunction,
    ResourceLocator const & resourceLocator)
    : mViewModel(
        initialShipSpaceSize,
        initialDisplaySize,
        logicalToPhysicalPixelFactor)
    , mShaderManager()
    , mSwapRenderBuffersFunction(swapRenderBuffersFunction)
    //////////////////////////////////
    , mHasBackgroundTexture(false)
    , mHasGameVisualizationTexture(false)
    , mHasStructuralLayerVisualizationTexture(false)
    , mHasElectricalLayerVisualizationTexture(false)
    , mRopeCount(false)
    , mHasTextureLayerVisualizationTexture(false)
    , mOtherVisualizationsOpacity(0.75f)
    , mIsGridEnabled(false)
    , mCircleOverlayCenter(0, 0) // Will be overwritten
    , mCircleOverlayColor(vec3f::zero()) // Will be overwritten
    , mHasCircleOverlay(false)
    , mRectOverlayRect({0, 0}, {1, 1}) // Will be overwritten
    , mRectOverlayColor(vec3f::zero()) // Will be overwritten
    , mHasRectOverlay(false)
    , mDashedLineOverlayColor(vec3f::zero()) // Will be overwritten
    //////////////////////////////////
    , mPrimaryVisualization(VisualizationType::StructuralLayer)
{
    //
    // Initialize global OpenGL settings
    //

    // Set anti-aliasing for lines
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    // Enable blending for alpha transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBlendEquation(GL_FUNC_ADD);

    // Disable depth test
    glDisable(GL_DEPTH_TEST);

    // Disable scissor test
    glDisable(GL_SCISSOR_TEST);

    //
    // Load shader manager
    //

    mShaderManager = ShaderManager<ShaderManagerTraits>::CreateInstance(resourceLocator.GetShipBuilderShadersRootPath());

    // Set texture samplers in programs
    mShaderManager->ActivateProgram<ProgramType::Texture>();
    mShaderManager->SetTextureParameters<ProgramType::Texture>();
    mShaderManager->ActivateProgram<ProgramType::TextureNdc>();
    mShaderManager->SetTextureParameters<ProgramType::TextureNdc>();

    //
    // Initialize Background texture and VAO
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mBackgroundTextureOpenGLHandle = tmpGLuint;

        // Configure texture
        glBindTexture(GL_TEXTURE_2D, *mBackgroundTextureOpenGLHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CheckOpenGLError();

        //
        // VAO
        //

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mBackgroundTextureVAO = tmpGLuint;
        glBindVertexArray(*mBackgroundTextureVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mBackgroundTextureVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mBackgroundTextureVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::TextureNdc));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::TextureNdc), 4, GL_FLOAT, GL_FALSE, sizeof(TextureNdcVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize canvas VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mCanvasVAO = tmpGLuint;
        glBindVertexArray(*mCanvasVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mCanvasVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mCanvasVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Canvas));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Canvas), 4, GL_FLOAT, GL_FALSE, sizeof(CanvasVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize game layer visualization and VAO
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mGameVisualizationTextureOpenGLHandle = tmpGLuint;

        // Configure texture
        glBindTexture(GL_TEXTURE_2D, *mGameVisualizationTextureOpenGLHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        CheckOpenGLError();

        //
        // VAO
        //

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mGameVisualizationTextureVAO = tmpGLuint;
        glBindVertexArray(*mGameVisualizationTextureVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mGameVisualizationTextureVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mGameVisualizationTextureVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize structural layer visualization and VAO
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mStructuralLayerVisualizationTextureOpenGLHandle = tmpGLuint;

        // Configure texture
        glBindTexture(GL_TEXTURE_2D, *mStructuralLayerVisualizationTextureOpenGLHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        CheckOpenGLError();

        //
        // VAO
        //

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mStructuralLayerVisualizationTextureVAO = tmpGLuint;
        glBindVertexArray(*mStructuralLayerVisualizationTextureVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mStructuralLayerVisualizationTextureVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mStructuralLayerVisualizationTextureVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize electrical layer visualization and VAO
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mElectricalLayerVisualizationTextureOpenGLHandle = tmpGLuint;

        // Configure texture
        glBindTexture(GL_TEXTURE_2D, *mElectricalLayerVisualizationTextureOpenGLHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        CheckOpenGLError();

        //
        // VAO
        //

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mElectricalLayerVisualizationTextureVAO = tmpGLuint;
        glBindVertexArray(*mElectricalLayerVisualizationTextureVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mElectricalLayerVisualizationTextureVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mElectricalLayerVisualizationTextureVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize ropes layer visualization VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mRopesVAO = tmpGLuint;
        glBindVertexArray(*mRopesVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mRopesVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mRopesVBO);
        static_assert(sizeof(RopeVertex) == (2 + 4) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Rope1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Rope1), 2, GL_FLOAT, GL_FALSE, sizeof(RopeVertex), (void *)(0));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Rope2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Rope2), 4, GL_FLOAT, GL_FALSE, sizeof(RopeVertex), (void *)(2 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize texture layer visualization and VAO
    //

    {
        GLuint tmpGLuint;

        //
        // Texture
        //

        // Create texture OpenGL handle
        glGenTextures(1, &tmpGLuint);
        mTextureLayerVisualizationTextureOpenGLHandle = tmpGLuint;

        // Configure texture
        glBindTexture(GL_TEXTURE_2D, *mTextureLayerVisualizationTextureOpenGLHandle);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        CheckOpenGLError();

        //
        // VAO
        //

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mTextureLayerVisualizationTextureVAO = tmpGLuint;
        glBindVertexArray(*mTextureLayerVisualizationTextureVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mTextureLayerVisualizationTextureVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mTextureLayerVisualizationTextureVBO);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Texture));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Texture), 4, GL_FLOAT, GL_FALSE, sizeof(TextureVertex), (void *)0);
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize Grid
    //

    {
        GLuint tmpGLuint;

        //
        // VAO
        //

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mGridVAO = tmpGLuint;
        glBindVertexArray(*mGridVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mGridVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mGridVBO);
        static_assert(sizeof(GridVertex) == (2 + 2 + 1) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Grid1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Grid1), 4, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::Grid2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::Grid2), 1, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize circle overlay VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mCircleOverlayVAO = tmpGLuint;
        glBindVertexArray(*mCircleOverlayVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mCircleOverlayVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mCircleOverlayVBO);
        static_assert(sizeof(CircleOverlayVertex) == (4 + 3) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::CircleOverlay1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::CircleOverlay1), 4, GL_FLOAT, GL_FALSE, sizeof(CircleOverlayVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::CircleOverlay2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::CircleOverlay2), 3, GL_FLOAT, GL_FALSE, sizeof(CircleOverlayVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize rect overlay VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mRectOverlayVAO = tmpGLuint;
        glBindVertexArray(*mRectOverlayVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mRectOverlayVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mRectOverlayVBO);
        static_assert(sizeof(RectOverlayVertex) == (4 + 3) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::RectOverlay1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::RectOverlay1), 4, GL_FLOAT, GL_FALSE, sizeof(RectOverlayVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::RectOverlay2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::RectOverlay2), 3, GL_FLOAT, GL_FALSE, sizeof(RectOverlayVertex), (void *)(4 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }

    //
    // Initialize dashed line overlay VAO
    //

    {
        GLuint tmpGLuint;

        // Create VAO
        glGenVertexArrays(1, &tmpGLuint);
        mDashedLineOverlayVAO = tmpGLuint;
        glBindVertexArray(*mDashedLineOverlayVAO);
        CheckOpenGLError();

        // Create VBO
        glGenBuffers(1, &tmpGLuint);
        mDashedLineOverlayVBO = tmpGLuint;

        // Describe vertex attributes
        glBindBuffer(GL_ARRAY_BUFFER, *mDashedLineOverlayVBO);
        static_assert(sizeof(DashedLineOverlayVertex) == (3 + 3) * sizeof(float));
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::DashedLineOverlay1));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::DashedLineOverlay1), 3, GL_FLOAT, GL_FALSE, sizeof(DashedLineOverlayVertex), (void *)0);
        glEnableVertexAttribArray(static_cast<GLuint>(VertexAttributeType::DashedLineOverlay2));
        glVertexAttribPointer(static_cast<GLuint>(VertexAttributeType::DashedLineOverlay2), 3, GL_FLOAT, GL_FALSE, sizeof(DashedLineOverlayVertex), (void *)(3 * sizeof(float)));
        CheckOpenGLError();

        glBindVertexArray(0);
    }
}

void View::EnableVisualGrid(bool doEnable)
{
    mIsGridEnabled = doEnable;
}

void View::UploadBackgroundTexture(RgbaImageData && texture)
{
    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mBackgroundTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(std::move(texture));

    //
    // Create vertices (in NDC)
    //

    // The texture coordinate at the bottom of the quad obeys the texture's aspect ratio,
    // rather than the screen's

    float const textureBottom =
        -(static_cast<float>(texture.Size.height) - static_cast<float>(mViewModel.GetDisplayPhysicalSize().height))
            / static_cast<float>(mViewModel.GetDisplayPhysicalSize().height);

    std::array<TextureNdcVertex, 4> vertexBuffer;

    // Bottom-left
    vertexBuffer[0] = TextureNdcVertex(
        vec2f(-1.0f, -1.0f),
        vec2f(0.0f, textureBottom));

    // Top-left
    vertexBuffer[1] = TextureNdcVertex(
        vec2f(-1.0f, 1.0f),
        vec2f(0.0f, 1.0f));

    // Bottom-right
    vertexBuffer[2] = TextureNdcVertex(
        vec2f(1.0f, -1.0f),
        vec2f(1.0f, textureBottom));

    // Top-right
    vertexBuffer[3] = TextureNdcVertex(
        vec2f(1.0f, 1.0f),
        vec2f(1.0f, 1.0f));

    //
    // Upload vertices
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mBackgroundTextureVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(TextureNdcVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Remember we have this texture
    //

    mHasBackgroundTexture = true;
}

void View::UploadGameVisualizationTexture(RgbaImageData const & texture)
{
    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mGameVisualizationTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(texture);

    //
    // Upload vertices
    //

    UploadTextureVertices(mGameVisualizationTextureVBO);

    //
    // Remember we have this texture
    //

    mHasGameVisualizationTexture = true;
}

void View::UpdateGameVisualizationTexture(
    RgbaImageData const & subTexture,
    ImageCoordinates const & origin)
{
    assert(mHasGameVisualizationTexture);

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mGameVisualizationTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture region
    GameOpenGL::UploadTextureRegion(
        subTexture.Data.get(),
        origin.x,
        origin.y,
        subTexture.Size.width,
        subTexture.Size.height);
}

void View::RemoveGameVisualizationTexture()
{
    mHasGameVisualizationTexture = false;
}

void View::UploadStructuralLayerVisualizationTexture(RgbaImageData const & texture)
{
    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mStructuralLayerVisualizationTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(texture);

    //
    // Upload vertices
    //

    UploadTextureVertices(mStructuralLayerVisualizationTextureVBO);

    //
    // Remember we have this texture
    //

    mHasStructuralLayerVisualizationTexture = true;
}

void View::RemoveStructuralLayerVisualizationTexture()
{
    mHasStructuralLayerVisualizationTexture = false;
}

void View::UploadElectricalLayerVisualizationTexture(RgbaImageData const & texture)
{
    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mElectricalLayerVisualizationTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(texture);

    //
    // Upload vertices
    //

    UploadTextureVertices(mElectricalLayerVisualizationTextureVBO);

    //
    // Remember we have this texture
    //

    mHasElectricalLayerVisualizationTexture = true;
}

void View::RemoveElectricalLayerVisualizationTexture()
{
    mHasElectricalLayerVisualizationTexture = false;
}

void View::UploadRopesLayerVisualization(RopeBuffer const & ropeBuffer)
{
    //
    // Create vertices
    //

    std::vector<RopeVertex> vertexBuffer;
    vertexBuffer.reserve(ropeBuffer.GetSize());

    for (auto const & e : ropeBuffer)
    {
        vertexBuffer.emplace_back(
            vec2f(
                static_cast<float>(e.StartCoords.x) + 0.5f,
                static_cast<float>(e.StartCoords.y) + 0.5f),
            e.RenderColor.toVec4f());

        vertexBuffer.emplace_back(
            vec2f(
                static_cast<float>(e.EndCoords.x) + 0.5f,
                static_cast<float>(e.EndCoords.y) + 0.5f),
            e.RenderColor.toVec4f());
    }

    //
    // Upload vertices
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mRopesVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(RopeVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Remember we have ropes
    //

    mRopeCount = ropeBuffer.GetSize();
}

void View::RemoveRopesLayerVisualization()
{
    mRopeCount = 0;
}

void View::UploadTextureLayerVisualizationTexture(RgbaImageData const & texture)
{
    //
    // Upload texture
    //

    // Bind texture
    glBindTexture(GL_TEXTURE_2D, *mTextureLayerVisualizationTextureOpenGLHandle);
    CheckOpenGLError();

    // Upload texture
    GameOpenGL::UploadTexture(texture);

    //
    // Create vertices
    //
    // We draw the texture with the same "0.5 ship offset" that we use at ShipFactory
    //

    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);
    float const offsetX = 0.5f / shipWidth;
    float const offsetY = 0.5f / shipHeight;

    std::array<TextureVertex, 4> vertexBuffer;

    // Bottom-left
    vertexBuffer[0] = TextureVertex(
        vec2f(0.0f, 0.0f),
        vec2f(offsetX, offsetY));

    // Top-left
    vertexBuffer[1] = TextureVertex(
        vec2f(0.0f, shipHeight - 1),
        vec2f(offsetX, 1.0f - offsetY));

    // Bottom-right
    vertexBuffer[2] = TextureVertex(
        vec2f(shipWidth - 1, 0.0f),
        vec2f(1.0f - offsetX, offsetY));

    // Top-right
    vertexBuffer[3] = TextureVertex(
        vec2f(shipWidth - 1, shipHeight - 1),
        vec2f(1.0f - offsetX, 1.0f - offsetY));

    //
    // Upload vertices
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mTextureLayerVisualizationTextureVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(TextureVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Remember we have this texture
    //

    mHasTextureLayerVisualizationTexture = true;
}

void View::RemoveTextureLayerVisualizationTexture()
{
    mHasTextureLayerVisualizationTexture = false;
}

void View::UploadCircleOverlay(
    ShipSpaceCoordinates const & center,
    OverlayMode mode)
{
    // Store center
    mCircleOverlayCenter = center;

    // Store color
    mCircleOverlayColor = GetOverlayColor(mode);

    mHasCircleOverlay = true;

    // Update overlay
    UpdateCircleOverlay();
}

void View::RemoveCircleOverlay()
{
    assert(mHasCircleOverlay);

    mHasCircleOverlay = false;
}

void View::UploadRectOverlay(
    ShipSpaceRect const & rect,
    OverlayMode mode)
{
    // Store rect
    mRectOverlayRect = rect;

    // Store color
    mRectOverlayColor = GetOverlayColor(mode);

    mHasRectOverlay = true;

    // Update overlay
    UpdateRectOverlay();
}

void View::RemoveRectOverlay()
{
    assert(mHasRectOverlay);

    mHasRectOverlay = false;
}

void View::UploadDashedLineOverlay(
    ShipSpaceCoordinates const & start,
    ShipSpaceCoordinates const & end,
    OverlayMode mode)
{
    // Store line
    mDashedLineOverlaySet.clear();
    mDashedLineOverlaySet.emplace_back(start, end);

    // Store color
    mDashedLineOverlayColor = GetOverlayColor(mode);

    // Update overlay
    UpdateDashedLineOverlay();
}

void View::RemoveDashedLineOverlay()
{
    assert(!mDashedLineOverlaySet.empty());

    mDashedLineOverlaySet.clear();
}

void View::Render()
{
    //
    // Initialize
    //

    // Set viewport
    glViewport(0, 0, mViewModel.GetDisplayPhysicalSize().width, mViewModel.GetDisplayPhysicalSize().height);

    //
    // Following is with scissor test disabled
    //

    glDisable(GL_SCISSOR_TEST);

    // Background texture
    if (mHasBackgroundTexture)
    {
        // Set this texture in the shader's sampler
        mShaderManager->ActivateTexture<ProgramParameterType::BackgroundTextureUnit>();
        glBindTexture(GL_TEXTURE_2D, *mBackgroundTextureOpenGLHandle);

        // Bind VAO
        glBindVertexArray(*mBackgroundTextureVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::TextureNdc>();

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }
    else
    {
        // Just clear canvas
        glClearColor(0.985f, 0.985f, 0.985f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // Canvas
    {
        // Bind VAO
        glBindVertexArray(*mCanvasVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::Canvas>();

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }

    //
    // Visualizations
    //

    // Ropes - when they're not the primary viz (in which case we render them on top of structural)
    if (mRopeCount > 0 && mPrimaryVisualization != VisualizationType::RopesLayer)
    {
        RenderRopesLayerVisualization();
    }

    // Game, structural, and texture visualizations - whichever is primary goes first
    if (mPrimaryVisualization == VisualizationType::Game)
    {
        if (mHasGameVisualizationTexture)
        {
            RenderGameVisualizationTexture();
        }
    }
    else if (mPrimaryVisualization == VisualizationType::StructuralLayer)
    {
        if (mHasStructuralLayerVisualizationTexture)
        {
            RenderStructuralLayerVisualizationTexture();
        }
    }
    else if (mPrimaryVisualization == VisualizationType::TextureLayer)
    {
        if (mHasTextureLayerVisualizationTexture)
        {
            RenderTextureLayerVisualizationTexture();
        }
    }

    // Game, structural, and texture visualizations - when they're not primary

    if (mPrimaryVisualization != VisualizationType::Game)
    {
        if (mHasGameVisualizationTexture)
        {
            RenderGameVisualizationTexture();
        }
    }

    if (mPrimaryVisualization != VisualizationType::StructuralLayer)
    {
        if (mHasStructuralLayerVisualizationTexture)
        {
            RenderStructuralLayerVisualizationTexture();
        }
    }

    if (mPrimaryVisualization != VisualizationType::TextureLayer)
    {
        if (mHasTextureLayerVisualizationTexture)
        {
            RenderTextureLayerVisualizationTexture();
        }
    }

    // Electrical layer visualization texture
    if (mHasElectricalLayerVisualizationTexture)
    {
        RenderElectricalLayerVisualizationTexture();
    }

    // Ropes layer, but only when it's primary viz
    if (mRopeCount > 0 && mPrimaryVisualization == VisualizationType::RopesLayer)
    {
        RenderRopesLayerVisualization();
    }

    //
    // Misc stuff on top of visualizations
    //

    // Grid
    if (mIsGridEnabled)
    {
        // Bind VAO
        glBindVertexArray(*mGridVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::Grid>();

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }

    // Circle overlay
    if (mHasCircleOverlay)
    {
        // Bind VAO
        glBindVertexArray(*mCircleOverlayVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::CircleOverlay>();

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }

    // Rect overlay
    if (mHasRectOverlay)
    {
        // Bind VAO
        glBindVertexArray(*mRectOverlayVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::RectOverlay>();

        // Draw
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        CheckOpenGLError();
    }

    //
    // Following is with scissor test enabled
    //

    glEnable(GL_SCISSOR_TEST);

    // Dashed line overlay
    if (!mDashedLineOverlaySet.empty())
    {
        // Bind VAO
        glBindVertexArray(*mDashedLineOverlayVAO);

        // Activate program
        mShaderManager->ActivateProgram<ProgramType::DashedLineOverlay>();

        // Set line width
        glLineWidth(1.5f);

        // Draw
        glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mDashedLineOverlaySet.size() * 2));
        CheckOpenGLError();
    }

    // Unbind VAOs
    glBindVertexArray(0);

    // Flip the back buffer onto the screen
    mSwapRenderBuffersFunction();
}

///////////////////////////////////////////////////////////////////////////////

void View::OnViewModelUpdated()
{
    UpdateCanvas();

    UpdateGrid();

    if (mHasCircleOverlay)
    {
        UpdateCircleOverlay();
    }

    if (mHasRectOverlay)
    {
        UpdateRectOverlay();
    }

    if (!mDashedLineOverlaySet.empty())
    {
        UpdateDashedLineOverlay();
    }

    //
    // Ortho matrix
    //

    auto const orthoMatrix = mViewModel.GetOrthoMatrix();

    mShaderManager->ActivateProgram<ProgramType::Canvas>();
    mShaderManager->SetProgramParameter<ProgramType::Canvas, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::CircleOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::CircleOverlay, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::DashedLineOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::DashedLineOverlay, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Grid>();
    mShaderManager->SetProgramParameter<ProgramType::Grid, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::RectOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::RectOverlay, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Rope>();
    mShaderManager->SetProgramParameter<ProgramType::Rope, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ProgramType::Texture>();
    mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    //
    // Scissor test
    //

    auto const physicalCanvasRect = mViewModel.GetPhysicalVisibleShipRegion();
    glScissor(
        physicalCanvasRect.origin.x, 
        mViewModel.GetDisplayPhysicalSize().height - 1 - (physicalCanvasRect.origin.y + physicalCanvasRect.size.height), // Origin is bottom
        physicalCanvasRect.size.width, 
        physicalCanvasRect.size.height);
    CheckOpenGLError();
}

void View::UpdateCanvas()
{
    //
    // Upload vertices
    //

    // Calculate space size of 1 pixel
    float const borderSize = mViewModel.GetShipSpaceForOnePhysicalDisplayPixel();

    // Ship space size
    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);

    std::array<CanvasVertex, 4> vertexBuffer;

    // Left, Top
    vertexBuffer[0] = CanvasVertex(
        vec2f(-borderSize, shipHeight + borderSize),
        vec2f(0.0f, 0.0f));

    // Left, Bottom
    vertexBuffer[1] = CanvasVertex(
        vec2f(-borderSize, -borderSize),
        vec2f(0.0f, 1.0f));

    // Right, Top
    vertexBuffer[2] = CanvasVertex(
        vec2f(shipWidth + borderSize, shipHeight + borderSize),
        vec2f(1.0f, 0.0f));

    // Right, Bottom
    vertexBuffer[3] = CanvasVertex(
        vec2f(shipWidth + borderSize, -borderSize),
        vec2f(1.0f, 1.0f));

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, *mCanvasVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(CanvasVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Set pixel size parameter - normalized size (i.e. in the 0->1 space) of 1 pixel (w, h separately)
    //

    DisplayPhysicalSize const canvasPhysSize = mViewModel.ShipSpaceSizeToPhysicalDisplaySize({
        static_cast<int>(shipWidth + 2.0f * borderSize),
        static_cast<int>(shipHeight + 2.0f * borderSize) });

    vec2f const pixelSize = vec2f(
        1.0f / canvasPhysSize.width,
        1.0f / canvasPhysSize.height);

    mShaderManager->ActivateProgram<ProgramType::Canvas>();
    mShaderManager->SetProgramParameter<ProgramType::Canvas, ProgramParameterType::PixelSize>(pixelSize.x, pixelSize.y);
}

void View::UpdateGrid()
{
    //
    // Calculate vertex attributes
    //

    // Ship space
    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);
    DisplayPhysicalSize const shipPixelSize = mViewModel.ShipSpaceSizeToPhysicalDisplaySize(mViewModel.GetShipSize());
    float const pixelWidth = static_cast<float>(shipPixelSize.width);
    float const pixelHeight = static_cast<float>(shipPixelSize.height);
    float const pixelMidX = pixelWidth / 2.0f;

    std::array<GridVertex, 4> vertexBuffer;

    // Notes:
    //  - Grid origin is in upper-left corner

    // Bottom-left
    vertexBuffer[0] = GridVertex(
        vec2f(0.0f, 0.0f),
        vec2f(0.0f, pixelHeight),
        pixelMidX);

    // Top-left
    vertexBuffer[1] = GridVertex(
        vec2f(0.0f, shipHeight),
        vec2f(0.0f, 0.0f),
        pixelMidX);

    // Bottom-right
    vertexBuffer[2] = GridVertex(
        vec2f(shipWidth, 0.0f),
        vec2f(pixelWidth, pixelHeight),
        pixelMidX);

    // Top-right
    vertexBuffer[3] = GridVertex(
        vec2f(shipWidth, shipHeight),
        vec2f(pixelWidth, 0.0f),
        pixelMidX);

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, *mGridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(GridVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Calculate step size
    //

    float const pixelStepSize = mViewModel.CalculateGridPhysicalPixelStepSize();

    mShaderManager->ActivateProgram<ProgramType::Grid>();
    mShaderManager->SetProgramParameter<ProgramType::Grid, ProgramParameterType::PixelStep>(
        pixelStepSize);
}

void View::UpdateCircleOverlay()
{
    assert(mHasCircleOverlay);

    //
    // Upload vertices
    //

    std::array<CircleOverlayVertex, 4> vertexBuffer;

    // Left, Top
    vertexBuffer[0] = CircleOverlayVertex(
        vec2f(
            static_cast<float>(mCircleOverlayCenter.x),
            static_cast<float>(mCircleOverlayCenter.y + 1.0f)),
        vec2f(0.0f, 0.0f),
        mCircleOverlayColor);

    // Left, Bottom
    vertexBuffer[1] = CircleOverlayVertex(
        vec2f(
            static_cast<float>(mCircleOverlayCenter.x),
            static_cast<float>(mCircleOverlayCenter.y)),
        vec2f(0.0f, 1.0f),
        mCircleOverlayColor);

    // Right, Top
    vertexBuffer[2] = CircleOverlayVertex(
        vec2f(
            static_cast<float>(mCircleOverlayCenter.x + 1.0f),
            static_cast<float>(mCircleOverlayCenter.y + 1.0f)),
        vec2f(1.0f, 0.0f),
        mCircleOverlayColor);

    // Right, Bottom
    vertexBuffer[3] = CircleOverlayVertex(
        vec2f(
            static_cast<float>(mCircleOverlayCenter.x + 1.0f),
            static_cast<float>(mCircleOverlayCenter.y)),
        vec2f(1.0f, 1.0f),
        mCircleOverlayColor);

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, *mCircleOverlayVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(CircleOverlayVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Set pixel size parameter - normalized size (i.e. in the 0->1 space) of 1 pixel (w, h separately)
    //

    DisplayPhysicalSize const squarePhysSize = mViewModel.ShipSpaceSizeToPhysicalDisplaySize({ 1, 1 });

    vec2f const pixelSize = vec2f(
        1.0f / std::max(squarePhysSize.width, 1),
        1.0f / std::max(squarePhysSize.height, 1));

    mShaderManager->ActivateProgram<ProgramType::CircleOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::CircleOverlay, ProgramParameterType::PixelSize>(pixelSize.x, pixelSize.y);
}

void View::UpdateRectOverlay()
{
    assert(mHasRectOverlay);

    //
    // Upload vertices
    //

    std::array<RectOverlayVertex, 4> vertexBuffer;

    // Left, Top
    vertexBuffer[0] = RectOverlayVertex(
        vec2f(
            static_cast<float>(mRectOverlayRect.origin.x),
            static_cast<float>(mRectOverlayRect.origin.y + mRectOverlayRect.size.height)),
        vec2f(0.0f, 0.0f),
        mRectOverlayColor);

    // Left, Bottom
    vertexBuffer[1] = RectOverlayVertex(
        vec2f(
            static_cast<float>(mRectOverlayRect.origin.x),
            static_cast<float>(mRectOverlayRect.origin.y)),
        vec2f(0.0f, 1.0f),
        mRectOverlayColor);

    // Right, Top
    vertexBuffer[2] = RectOverlayVertex(
        vec2f(
            static_cast<float>(mRectOverlayRect.origin.x + mRectOverlayRect.size.width),
            static_cast<float>(mRectOverlayRect.origin.y + mRectOverlayRect.size.height)),
        vec2f(1.0f, 0.0f),
        mRectOverlayColor);

    // Right, Bottom
    vertexBuffer[3] = RectOverlayVertex(
        vec2f(
            static_cast<float>(mRectOverlayRect.origin.x + mRectOverlayRect.size.width),
            static_cast<float>(mRectOverlayRect.origin.y)),
        vec2f(1.0f, 1.0f),
        mRectOverlayColor);

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, *mRectOverlayVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(RectOverlayVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Set pixel size parameter - normalized size (i.e. in the 0->1 space) of 1 pixel (w, h separately)
    //

    DisplayPhysicalSize const squarePhysSize = mViewModel.ShipSpaceSizeToPhysicalDisplaySize(mRectOverlayRect.size);

    vec2f const pixelSize = vec2f(
        1.0f / std::max(squarePhysSize.width, 1),
        1.0f / std::max(squarePhysSize.height, 1));

    mShaderManager->ActivateProgram<ProgramType::RectOverlay>();
    mShaderManager->SetProgramParameter<ProgramType::RectOverlay, ProgramParameterType::PixelSize>(pixelSize.x, pixelSize.y);
}

void View::UpdateDashedLineOverlay()
{
    assert(!mDashedLineOverlaySet.empty());

    //
    // Upload vertices
    //

    std::vector<DashedLineOverlayVertex> vertexBuffer;

    for (auto const & p : mDashedLineOverlaySet)
    {
        //
        // Calculate length, in pixels
        //

        ShipSpaceSize const shipRect(std::abs(p.first.x - p.second.x), std::abs(p.first.y - p.second.y));
        DisplayPhysicalSize physRec = mViewModel.ShipSpaceSizeToPhysicalDisplaySize(shipRect);
        float pixelLength = physRec.ToFloat().length();

        // Normalize length so it's a multiple of the period + 1/2 period
        float constexpr DashPeriod = 8.0f; // 4 + 4
        float const leftover = std::fmod(pixelLength + DashPeriod / 2.0f, DashPeriod);
        pixelLength += (DashPeriod - leftover);

        //
        // Populate vertices
        //

        vertexBuffer.emplace_back(
            p.first.ToFloat() + vec2f(0.5f, 0.5f),
            0.0f,
            mDashedLineOverlayColor);

        vertexBuffer.emplace_back(
            p.second.ToFloat() + vec2f(0.5f, 0.5f),
            pixelLength,
            mDashedLineOverlayColor);
    }

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, *mDashedLineOverlayVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(DashedLineOverlayVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void View::UploadTextureVertices(GameOpenGLVBO const & vbo)
{
    //
    // Create vertices
    //

    float const shipWidth = static_cast<float>(mViewModel.GetShipSize().width);
    float const shipHeight = static_cast<float>(mViewModel.GetShipSize().height);

    std::array<TextureVertex, 4> vertexBuffer;

    // Bottom-left
    vertexBuffer[0] = TextureVertex(
        vec2f(0.0f, 0.0f),
        vec2f(0.0f, 0.0f));

    // Top-left
    vertexBuffer[1] = TextureVertex(
        vec2f(0.0f, shipHeight),
        vec2f(0.0f, 1.0f));

    // Bottom-right
    vertexBuffer[2] = TextureVertex(
        vec2f(shipWidth, 0.0f),
        vec2f(1.0f, 0.0f));

    // Top-right
    vertexBuffer[3] = TextureVertex(
        vec2f(shipWidth, shipHeight),
        vec2f(1.0f, 1.0f));

    //
    // Upload vertices
    //

    glBindBuffer(GL_ARRAY_BUFFER, *vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(TextureVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void View::RenderGameVisualizationTexture()
{
    // Set this texture in the shader's sampler
    mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
    glBindTexture(GL_TEXTURE_2D, *mGameVisualizationTextureOpenGLHandle);

    // Bind VAO
    glBindVertexArray(*mGameVisualizationTextureVAO);

    // Activate program
    mShaderManager->ActivateProgram<ProgramType::Texture>();

    // Set opacity
    mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::Opacity>(
        mPrimaryVisualization == VisualizationType::Game ? 1.0f : mOtherVisualizationsOpacity);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CheckOpenGLError();
}

void View::RenderStructuralLayerVisualizationTexture()
{
    // Set this texture in the shader's sampler
    mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
    glBindTexture(GL_TEXTURE_2D, *mStructuralLayerVisualizationTextureOpenGLHandle);

    // Bind VAO
    glBindVertexArray(*mStructuralLayerVisualizationTextureVAO);

    // Activate program
    mShaderManager->ActivateProgram<ProgramType::Texture>();

    // Set opacity
    mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::Opacity>(
        mPrimaryVisualization == VisualizationType::StructuralLayer ? 1.0f : mOtherVisualizationsOpacity);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CheckOpenGLError();
}

void View::RenderElectricalLayerVisualizationTexture()
{
    // Set this texture in the shader's sampler
    mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
    glBindTexture(GL_TEXTURE_2D, *mElectricalLayerVisualizationTextureOpenGLHandle);

    // Bind VAO
    glBindVertexArray(*mElectricalLayerVisualizationTextureVAO);

    // Activate program
    mShaderManager->ActivateProgram<ProgramType::Texture>();

    // Set opacity
    mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::Opacity>(
        mPrimaryVisualization == VisualizationType::ElectricalLayer ? 1.0f : mOtherVisualizationsOpacity);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CheckOpenGLError();
}

void View::RenderRopesLayerVisualization()
{
    // Bind VAO
    glBindVertexArray(*mRopesVAO);

    // Activate program
    mShaderManager->ActivateProgram<ProgramType::Rope>();

    // Set opacity
    mShaderManager->SetProgramParameter<ProgramType::Rope, ProgramParameterType::Opacity>(
        mPrimaryVisualization == VisualizationType::RopesLayer ? 1.0f : mOtherVisualizationsOpacity);

    // Set line width
    glLineWidth(2.5f);

    // Draw
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(mRopeCount * 2));
    CheckOpenGLError();
}

void View::RenderTextureLayerVisualizationTexture()
{
    // Set this texture in the shader's sampler
    mShaderManager->ActivateTexture<ProgramParameterType::TextureUnit1>();
    glBindTexture(GL_TEXTURE_2D, *mTextureLayerVisualizationTextureOpenGLHandle);

    // Bind VAO
    glBindVertexArray(*mTextureLayerVisualizationTextureVAO);

    // Activate program
    mShaderManager->ActivateProgram<ProgramType::Texture>();

    // Set opacity
    mShaderManager->SetProgramParameter<ProgramType::Texture, ProgramParameterType::Opacity>(
        mPrimaryVisualization == VisualizationType::TextureLayer ? 1.0f : mOtherVisualizationsOpacity);

    // Draw
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    CheckOpenGLError();
}

vec3f View::GetOverlayColor(OverlayMode mode) const
{
    switch (mode)
    {
        case OverlayMode::Default:
        {
            return vec3f(0.05f, 0.05f, 0.05f);
        }

        case OverlayMode::Error:
        {
            return vec3f(1.0f, 0.0f, 0.0f);
        }
    }

    assert(false);
    return vec3f::zero();
}

}