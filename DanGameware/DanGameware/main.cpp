#include <Horde3D.h>
#include <GLFW/glfw3.h>
#include <math.h>
#include <stdio.h>
#include <thread>

#include "Horde3DUtils.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Tween.h"
#include "Gameplay.h"
#include "khmath.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "Horde3DOverlays.h"

#include "imgui_impl_opengl3.h"
#include "MapManager.h"

#include "SoundEngine.h"
#include "soloud.h"
#include "soloud_wav.h"
#include "ai.h"
extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include "debug_draw.h";

Tween g_tween;



#define WINDOW_WIDTH (1600)
#define WINDOW_HEIGHT (900)
#define FULL_SCREEN (0)
#define MSAA_C (8)
#define V_SYNC (1)

H3DNode main_camera = 0;
gentity_t g_entities = nullptr;
bool game_pause = false;


GLFWwindow* _winHandle;
float prev_x = 0;
float prev_y = 0;
bool running = true;
float timescale = 1;
bool edit_mode = false;
bool fisrt_mosue = true;

void dw_console_log(const char* fmt, ...);
void windowCloseListener(GLFWwindow* win)
{
	running = false;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		dw_console_log("pressed P");
		game_pause = !game_pause;
	}
}
void mouseMoveListener(GLFWwindow* win, double x, double y)
{
	if (edit_mode)
	{
		prev_x = x = prev_y = y = 0;
		fisrt_mosue = true;
		return;
	}
	if (fisrt_mosue)
	{
		fisrt_mosue = false;
	}
	else
	{
		
		float dx = x - prev_x;
		float dy = prev_y - y;
		g_input.drx = dx;
		g_input.dry = dy;
	}
	prev_x = x;
	prev_y = y;
}
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (edit_mode)
		return;
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		g_input.attack = true;
	}
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		g_input.attack = false;
	}

}
bool init()
{
	glfwInit();
	glfwWindowHint(GLFW_RED_BITS, 8);
	glfwWindowHint(GLFW_GREEN_BITS, 8);
	glfwWindowHint(GLFW_BLUE_BITS, 8);
	glfwWindowHint(GLFW_ALPHA_BITS, 8);
	glfwWindowHint(GLFW_DEPTH_BITS, 24);
	glfwWindowHint(GLFW_SAMPLES, MSAA_C);

	_winHandle = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "DanGameware", FULL_SCREEN ? glfwGetPrimaryMonitor() : NULL, NULL);
	glfwMakeContextCurrent(_winHandle);
	glfwSetInputMode(_winHandle, GLFW_STICKY_KEYS, GL_TRUE);
	if(V_SYNC)
		glfwSwapInterval(1);
	else
		glfwSwapInterval(0);
	glfwSetWindowCloseCallback(_winHandle, windowCloseListener);
	glfwSetKeyCallback(_winHandle, key_callback);
	glfwSetCursorPosCallback(_winHandle, mouseMoveListener);
	glfwSetMouseButtonCallback(_winHandle, mouse_button_callback);
	glfwSetInputMode(_winHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	edit_mode = true;

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	ImGui::StyleColorsDark();
	//io.MouseDrawCursor = true;
	// Setup Platform/Renderer backends
	ImGui_ImplGlfw_InitForOpenGL(_winHandle, true);
	ImGui_ImplOpenGL3_Init();

	ImVec4* colors = ImGui::GetStyle().Colors;
	colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 1.00f);
	colors[ImGuiCol_FrameBg] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	colors[ImGuiCol_CheckMark] = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	colors[ImGuiCol_Button] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	colors[ImGuiCol_ResizeGrip] = ImVec4(0.17f, 0.17f, 0.17f, 1.00f);
	colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.14f, 0.13f, 1.00f);
	colors[ImGuiCol_Text] = ImVec4(0.79f, 0.75f, 0.63f, 1.00f);


	auto& st = ImGui::GetStyle();
	st.WindowRounding = 3;
	st.FrameRounding = 3;
	st.FramePadding.x = 7;
	st.FramePadding.y = 6;
	


	khsound::init_sound();


	return true;
}
bool isKeyDown(int key)
{
	return glfwGetKey(_winHandle, key) == GLFW_PRESS;
}

H3DRes matres;
int specunifind;
void detectMaterials()
{
	H3DRes mat = 0;
	while (mat=h3dGetNextResource(H3DResTypes::Material, mat))
	{
		auto m_samplerIndex = h3dFindResElem(mat, H3DMatRes::UniformElem, H3DMatRes::UnifNameStr, "matSpecParams");
		if (m_samplerIndex != -1)
		{
			printf("%d,%s\n", m_samplerIndex, h3dGetResName(mat));
			specunifind = m_samplerIndex;
			matres = mat;
			break;
		}
	}
}
void dumph3dMessages()
{
	int level;
	float time;
	const char* text = h3dGetMessage(&level, &time);

	while (strlen(text)!=0)
	{
		dw_console_log(text);
		text = h3dGetMessage(&level, &time);
	}
}
void initGame(int winWidth, int winHeight)
{
	h3dInit(H3DRenderDevice::OpenGL2);
	h3dSetOption(H3DOptions::SampleCount, (float)MSAA_C);
	h3dSetOption(H3DOptions::FastAnimation, (float)0);
	h3dSetOption(H3DOptions::SRGBLinearization, (float)0);
	h3dSetOption(H3DOptions::SRGBLinearization, (float)0);
	h3dSetOption(H3DOptions::MaxAnisotropy, (float)16);
	h3dSetOption(H3DOptions::WireframeMode, (float)0);
	h3dSetOption(H3DOptions::LoadTextures, 1);
	h3dSetOption(H3DOptions::ShadowMapSize, 2048);



	H3DRes pipeRes = h3dAddResource(H3DResTypes::Pipeline, "pipelines/alve.pipeline.xml", 0);
	H3DRes model_shader = h3dAddResource(H3DResTypes::Shader, "shaders/model.shader", 0);
	H3DRes model_shader2 = h3dAddResource(H3DResTypes::Shader, "shaders/model_org.shader", 0);
	H3DRes model_shader3 = h3dAddResource(H3DResTypes::Shader, "shaders/model_org2.shader", 0);
	H3DRes skybox_shader = h3dAddResource(H3DResTypes::Shader, "shaders/skybox.shader", 0);
	H3DRes particle_shader = h3dAddResource(H3DResTypes::Shader, "shaders/particle.shader", 0);

	h3dutLoadResourcesFromDisk("./enginecontent/");


	main_camera = h3dAddCameraNode(H3DRootNode, "Camera", pipeRes);
	h3dSetNodeParamI(main_camera, H3DCamera::OccCullingI, 1);
	h3dSetNodeParamI(main_camera, H3DCamera::ViewportWidthI, winWidth);
	h3dSetNodeParamI(main_camera, H3DCamera::ViewportHeightI, winHeight);
	h3dSetupCameraView(main_camera, 80.0f, (float)winWidth / winHeight, 1, 30000);
	h3dResizePipelineBuffers(pipeRes, winWidth, winHeight);
	
}
void updateEmitters(float dt)
{
	unsigned int cnt = h3dFindNodes(H3DRootNode, "", H3DNodeTypes::Emitter);
	for (unsigned int i = 0; i < cnt; ++i)
		h3dUpdateEmitter(h3dGetNodeFindResult(i), dt);
}
void gameupdate(float dt)
{
	if(!edit_mode)
		g_input.capture(_winHandle);
	//ai
	if (g_entities != nullptr)
	{
		update_ents(g_entities, 2, dt);
	}
	//physic update
	commitGameObjectListChanges();
	for (const auto& go : gameObjects_array)
	{
		go->PhysicUpdate(dt);
	}
	double phdt;

	g_phyis.update(dt);
	for (const auto& go : gameObjects_array)
	{
		go->Update(dt);
	}
	g_tween.update(dt);
	updateEmitters(dt);

	//phdt = glfwGetTime() - t;
	//phys_time += phdt;
	//if (frames % 500 == 0)
		//dw_console_log("Physic time: avg=%.2fms (%d fps) current=%.2fms (%d fps)\n", (phys_time / frames) * 1000, (int)(frames / phys_time), phdt * 1000, (int)(1.0 / phdt));
	dumph3dMessages();
}
void gameRender()
{
	//if (game_pause == false)
	{
		h3dRender(main_camera);
		debug_draw_frame(main_camera, false);
		h3dFinalizeFrame();
		h3dClearOverlays();
	}
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwMakeContextCurrent(_winHandle);
	glfwSwapBuffers(_winHandle);
}
void imgui_frame();
std::vector<H3DNode> dynamic_lights;
int selected_light_idx = 0;
bool allow_tab=true;

int maadwawdin(void) {
	char buff[256];
	int error;
	lua_State *L = luaL_newstate();
	luaopen_base(L);             /* opens the basic library */
	luaopen_table(L);            /* opens the table library */
	luaopen_io(L);               /* opens the I/O library */
	luaopen_string(L);           /* opens the string lib. */
	luaopen_math(L);             /* opens the math lib. */

	while (fgets(buff, sizeof(buff), stdin) != NULL) {
		error = luaL_loadbuffer(L, buff, strlen(buff), "line") ||
			lua_pcall(L, 0, 0, 0);
		if (error) {
			fprintf(stderr, "%s", lua_tostring(L, -1));
			lua_pop(L, 1);  /* pop error message from the stack */
		}
	}

	lua_close(L);
	return 0;
}
int ldupdate_cnt = 0;
void mapLoadUpdate()
{
	glfwPollEvents();
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	ImVec2 window_pos, window_pos_pivot;
	window_pos.x = 10;
	window_pos.y = 10;
	window_pos_pivot.x = 0;
	window_pos_pivot.y = 0;
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
	window_flags |= ImGuiWindowFlags_NoMove;
	ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
	bool pop;
	ImGui::Begin("Stats", &pop, window_flags);
	ImGui::Text("Loading %d models", ldupdate_cnt * 2);
	ImGui::End();
	ldupdate_cnt++;
	ImGui::Render();
	glClearColor(ldupdate_cnt * 0.02, 0, 0, 255);
	glClear(GL_COLOR_BUFFER_BIT);
	
	//float points[] = { 0,0,0,1,     1,0,1,1,           1,0.562,1,0,      0,0.562,0,0 };
	//h3dShowOverlays(points, 4, 1, 1, 1, 1, background_mat, 0);
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwMakeContextCurrent(_winHandle);
	glfwSwapBuffers(_winHandle);
}
void main_load_map()
{
	mapLoad("finalook", &mapLoadUpdate);
}
int main(int argc, char** argv);
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	main(0, nullptr);
}
int main(int argc, char** argv)
{
	//maadwawdin();
	init();
	initGame(WINDOW_WIDTH, WINDOW_HEIGHT);
	debug_draw_init();
	
	mapLoad("finalook", &mapLoadUpdate);
	double last_t = glfwGetTime();
	while (running)
	{
		//dt
		double t = glfwGetTime();
		float dt = t - last_t;
		last_t = t;
		//input
		g_input.dry = 0;
		g_input.drx = 0;
		glfwPollEvents();
		
		//imgui
		imgui_frame();
		if (glfwGetKey(_winHandle, GLFW_KEY_TAB) == GLFW_PRESS && allow_tab)
		{
			edit_mode = !edit_mode;
			allow_tab = false;
			if (edit_mode)
				glfwSetInputMode(_winHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			else
				glfwSetInputMode(_winHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			allow_tab = false;

		}
		if (glfwGetKey(_winHandle, GLFW_KEY_TAB) == GLFW_RELEASE)
		{
			allow_tab = true;
		}


		if (dt > 0.1f) dt = 0.1f;
		
		if(game_pause==false)
			gameupdate(dt);
		gameRender();

	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	khsound::shutdown_sound();
	return 0;
}

void alve_editor_draw_light_controls(H3DNode light, bool& out_select_closest_light)
{
	ImGui::Begin("Light Params");
	float lx, ly, lz,rx,ry,rz;
	h3dGetNodeTransform(light,&lx, &ly, &lz,&rx,&ry,&rz, nullptr, nullptr, nullptr);

	ImGui::DragFloat("Pos X", &lx, 1);
	ImGui::DragFloat("Pos Y", &ly, 1);
	ImGui::DragFloat("Pos Z", &lz, 1);
	ImGui::DragFloat("Rot X", &rx, 1);
	ImGui::DragFloat("Rot Y", &ry, 1);
	ImGui::DragFloat("Rot Z", &rz, 1);
	h3dSetNodeTransform(light, lx, ly, lz, rx, ry, rz, 1, 1, 1);

	float fov = h3dGetNodeParamF(light, H3DLight::FovF, 0);
	ImGui::DragFloat("Fov", &fov, 1);
	h3dSetNodeParamF(light, H3DLight::FovF, 0, fov);
	float colx, coly, colz;
	colx = h3dGetNodeParamF(light, H3DLight::ColorF3, 0);
	coly = h3dGetNodeParamF(light, H3DLight::ColorF3, 1);
	colz = h3dGetNodeParamF(light, H3DLight::ColorF3, 2);
	
	float col[3] = { colx,coly,colz};

	ImGui::ColorEdit3("Diffuse color", col);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 0, col[0]);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 1, col[1]);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 2, col[2]);

	float rad = h3dGetNodeParamF(light, H3DLight::RadiusF, 0);
	ImGui::DragFloat("Radius", &rad);
	h3dSetNodeParamF(light, H3DLight::RadiusF, 0, rad);
	
	float mul = h3dGetNodeParamF(light, H3DLight::ColorMultiplierF, 0);
	ImGui::DragFloat("Light intensity", &mul,0.01);
	h3dSetNodeParamF(light, H3DLight::ColorMultiplierF, 0, mul);
	out_select_closest_light = ImGui::Button("Select closest light");
	ImGui::End();
}
void alve_editor_draw_material_controls()
{
	ImGui::Begin("Material Params");

	float colx, coly, colz,colw;
	colx = h3dGetResParamF(matres, H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 0);
	coly = h3dGetResParamF(matres, H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 1);
	colz = h3dGetResParamF(matres, H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 2);
	colw = h3dGetResParamF(matres, H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 3);
	float col[3] = { colx,coly,colz };
	ImGui::ColorEdit3("Diffuse color", col);
	ImGui::DragFloat("Light intensity", &colw, 0.01);
	h3dSetResParamF(matres, H3DMatRes::UniformElem, specunifind,H3DMatRes::UnifValueF4, 0, col[0]);
	h3dSetResParamF(matres, H3DMatRes::UniformElem, specunifind,H3DMatRes::UnifValueF4, 1, col[1]);
	h3dSetResParamF(matres, H3DMatRes::UniformElem, specunifind,H3DMatRes::UnifValueF4, 2, col[2]);
	h3dSetResParamF(matres, H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 3, colw);


	ImGui::End();
}
void imgui_stats_window()
{
	float frame_time = h3dGetStat(H3DStats::FrameTime, true);
	float batch_count = h3dGetStat(H3DStats::BatchCount, true);
	float tri_count = h3dGetStat(H3DStats::TriCount, true);
	float light_count = h3dGetStat(H3DStats::LightPassCount, true);
	float texturemem = h3dGetStat(H3DStats::TextureVMem, false);
	float geometry_mem = h3dGetStat(H3DStats::GeometryVMem, false);
	int frame_rate = 1000 / frame_time;


	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	ImVec2 window_pos, window_pos_pivot;
	window_pos.x = 10;
	window_pos.y = 10;
	window_pos_pivot.x = 0;
	window_pos_pivot.y = 0;
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
	window_flags |= ImGuiWindowFlags_NoMove;
	ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
	bool pop = true;
	ImGui::Begin("Stats", &pop, window_flags);
	ImGui::Text("%d FPS (%d Batches,%d Lights)", frame_rate, (int)batch_count, (int)light_count);
	ImGui::Text("%d Triangles",(int)tri_count);
	ImGui::Text("%d MB Textures,%d MB Models", (int)texturemem, (int)geometry_mem);

	ImGui::End();
}
void khshowConsole();
void imgui_frame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	//debug_draw_imgui(main_camera);

	imgui_stats_window();
	
	if (edit_mode)
	{
		//ImGui::ShowDemoWindow();
		//alve_editor_draw_material_controls();
		khshowConsole();
		bool select_closest_light = false;
		if (dynamic_lights.size() > 0)
		{
			alve_editor_draw_light_controls(dynamic_lights[selected_light_idx], select_closest_light);
		}

		if (select_closest_light)
		{
			float min_dis = 0;
			for (int i = 0; i < dynamic_lights.size(); i++)
			{

				float camx, camy, camz, lx, ly, lz;
				h3dGetNodeTransform(main_camera, &camx, &camy, &camz, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
				h3dGetNodeTransform(dynamic_lights[i], &lx, &ly, &lz, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
				auto dist = (Vector3df(camx, camy, camz) - Vector3df(lx, ly, lz)).getLengthSQ();
				if (i == 0 || dist < min_dis)
				{
					min_dis = dist;
					selected_light_idx = i;
				}
			}
		}
	}
}




