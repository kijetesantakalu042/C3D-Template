#include <3ds.h>
#include <citro3d.h>
#include <tex3ds.h>
#include <string.h>
#include "vshader_shbin.h"
#include <stdio.h>

#include "vars.h"
#include "vertex.h"
#include "bird_t3x.h"

// Helper function for loading a texture from memory
static bool loadTextureFromMem(C3D_Tex* tex, C3D_TexCube* cube, const void* data, size_t size)
{
    Tex3DS_Texture t3x = Tex3DS_TextureImport(data, size, tex, cube, false);
    if (!t3x)
        return false;

    // Delete the t3x object since we don't need it
    Tex3DS_TextureFree(t3x);
    return true;
}

void init() {
    // Do the standard init stuff
    gfxInit(GSP_RGBA8_OES, GSP_RGBA8_OES, false);
    consoleInit(GFX_BOTTOM, NULL);
    printf("Initialized LibCTRU\n");

    // Enable 3D
    printf("Enabling 3D\n");
    gfxSet3D(true);
    printf("Enabled 3D\n");

    //Initializing Citro3D graphics.
    printf("Initializing C3D\n");
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    printf("Initialized C3D\n");

    //Initializing render targets.
    printf("Making render targets\n");
    leftTarget = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    rightTarget = C3D_RenderTargetCreate(240, 400, GPU_RB_RGBA8, GPU_RB_DEPTH24_STENCIL8);
    printf("Made render targets");

    //Clearing render targets;
    printf("Clearing screen\n");
    C3D_RenderTargetClear(leftTarget, C3D_CLEAR_ALL, 0x68B0D8FF, 0);
    C3D_RenderTargetClear(rightTarget, C3D_CLEAR_ALL, 0x68B0D8FF, 0);
    printf("Screen cleared\n");

    //Setting render outputs.
    printf("Seting RenderTargets\n");
    C3D_RenderTargetSetOutput(leftTarget, GFX_TOP, GFX_LEFT, COMMON_DISPLAY_TRANSFER_FLAGS);
    C3D_RenderTargetSetOutput(rightTarget, GFX_TOP, GFX_RIGHT, COMMON_DISPLAY_TRANSFER_FLAGS);
    printf("Graphics init done!\n");

    return;
}

void sceneinit() {
    printf("Initializing scene");

    vertexShader_dvlb = DVLB_ParseFile((u32*) vshader_shbin, vshader_shbin_size);

    shaderProgramInit(&program);
    shaderProgramSetVsh(&program, &vertexShader_dvlb->DVLE[0]);

    //Binding.
    C3D_BindProgram(&program);

    //Get location of uniforms used in the vertex shader.
    uLoc_projection = shaderInstanceGetUniformLocation(program.vertexShader, "projection");
    uLoc_modelView = shaderInstanceGetUniformLocation(program.vertexShader, "modelView");

    //Initialize attributes, and then configure them for use with vertex shader.
    C3D_AttrInfo* attributeInfo = C3D_GetAttrInfo();
    AttrInfo_Init(attributeInfo);
    AttrInfo_AddLoader(attributeInfo, 0, GPU_FLOAT, 3); //First float array = vertex position.
    AttrInfo_AddLoader(attributeInfo, 1, GPU_FLOAT, 2); //Second float array = texture coordinates.
    AttrInfo_AddLoader(attributeInfo, 2, GPU_FLOAT, 3); //Third float array = normals.

    //Create vertex buffer objects.
    vbo_data = linearAlloc(sizeof(vertexList));
    memcpy(vbo_data, vertexList, sizeof(vertexList));
    static C3D_Tex bird_tex;

    //Initialize and configure buffers.
    bufferInfo = C3D_GetBufInfo();
    BufInfo_Init(bufferInfo);
    BufInfo_Add(bufferInfo, vbo_data, sizeof(Vertex), 3, 0x210);

    // Load the texture and bind it to the first texture unit
    if (!loadTextureFromMem(&bird_tex, NULL, bird_t3x, bird_t3x_size))
        svcBreak(USERBREAK_PANIC);
    C3D_TexSetFilter(&bird_tex, GPU_LINEAR, GPU_NEAREST);
    C3D_TexBind(0, &bird_tex);

    // Configure the first fragment shading substage to blend the fragment primary color
    // with the fragment secondary color.
    // See https://www.opengl.org/sdk/docs/man2/xhtml/glTexEnv.xml for more insight
    environment = C3D_GetTexEnv(0);
    C3D_TexEnvSrc(environment, C3D_Both, GPU_TEXTURE0, GPU_FRAGMENT_PRIMARY_COLOR, 0);
    C3D_TexEnvOpRgb(environment, C3D_RGB, 0, 0);
    C3D_TexEnvOpAlpha(environment, C3D_Alpha, 0, 0);
    C3D_TexEnvFunc(environment, C3D_Both, GPU_ADD);

    const C3D_Material material =
    {
        { 0.2f, 0.2f, 0.2f }, //ambient
        { 0.4f, 0.4f, 0.4f }, //diffuse
        { 0.8f, 0.8f, 0.8f }, //specular0
        { 0.0f, 0.0f, 0.0f }, //specular1
        { 0.0f, 0.0f, 0.0f }, //emission
    };

    //Lighting setup
    C3D_LightEnvInit(&lightEnv);
    C3D_LightEnvBind(&lightEnv);
    C3D_LightEnvMaterial(&lightEnv, &material);

    LightLut_Phong(&lut_Spec, 30);
    C3D_LightEnvLut(&lightEnv, GPU_LUT_D0, GPU_LUTINPUT_LN, false, &lut_Spec);

    C3D_FVec lightVector = {{ 16.0, 0.5, 0.0, 0.0 }};
    C3D_LightInit(&light, &lightEnv);
    C3D_LightColor(&light, 1.0, 1.0, 1.0);
    C3D_LightPosition(&light, &lightVector);
}

void SceneRender(float interOcularDistance) {
    C3D_RenderTargetClear(leftTarget, C3D_CLEAR_ALL, 0x68B0D8FF, 0);
    C3D_RenderTargetClear(rightTarget, C3D_CLEAR_ALL, 0x68B0D8FF, 0);

    //Compute projection matrix.
    Mtx_PerspStereoTilt(&projection, C3D_AngleFromDegrees(40.0f), C3D_AspectRatioTop, 0.01f, 1000.0f, interOcularDistance, 2.0f, false);

    //Calculate model view matrix.
    C3D_Mtx modelView;
    Mtx_Identity(&modelView);
    Mtx_Scale(&modelView, 0.75f, 0.75f, 0.75f);
    Mtx_RotateX(&modelView, angleX, false);
    Mtx_RotateY(&modelView, angleY, false);
    Mtx_Translate(&modelView, 0.0, 0.0, -2.0, false);

    if (interOcularDistance >= 0.0f){
        angleX += radian;
        angleY += radian;
    }

    //Update uniforms
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_projection, &projection);
    C3D_FVUnifMtx4x4(GPU_VERTEX_SHADER, uLoc_modelView, &modelView);

    //Draw the vertex buffer objects.
    C3D_DrawArrays(GPU_TRIANGLES, 0, sizeof(vertexList)/sizeof(vertexList[0]));
}

int main(int argc, char* argv[]) {
    init();
    sceneinit();

    // Main loop
    while (aptMainLoop())
    {
        hidScanInput();

        // Your code goes here
        u32 kDown = hidKeysDown();
        if (kDown & KEY_START)
            break; // break in order to return to hbmenu

        float slider = osGet3DSliderState();

        float iod = slider / 3;

        //Rendering scene
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        {
            C3D_FrameDrawOn(leftTarget);
            SceneRender(-iod);
            if (iod > 0.0f){
                C3D_FrameDrawOn(rightTarget);
                SceneRender(iod);
            }
        }
        C3D_FrameEnd(0);
    }

    gfxExit();
    return 0;
}
