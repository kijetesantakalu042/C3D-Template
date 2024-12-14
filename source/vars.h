static const u32 COMMON_DISPLAY_TRANSFER_FLAGS = \
	(GX_TRANSFER_FLIP_VERT(0) | GX_TRANSFER_OUT_TILED(0) | GX_TRANSFER_RAW_COPY(0) | \
	GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGBA8) | GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGBA8) | \
	GX_TRANSFER_SCALING(GX_TRANSFER_SCALE_NO));

DVLB_s* vertexShader_dvlb;
shaderProgram_s program;

C3D_BufInfo* bufferInfo;
C3D_TexEnv* environment;

int uLoc_projection, uLoc_modelView;
void* vbo_data;

static C3D_LightEnv lightEnv;
static C3D_Light light;
static C3D_LightLut lut_Spec;
static C3D_Mtx projection;

C3D_RenderTarget* leftTarget;
C3D_RenderTarget* rightTarget;

static const float radian = acos(-1) / 180.0f;
static float angleX = 0.0, angleY = 0.0;
