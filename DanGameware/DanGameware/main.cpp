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

#include "imgui_impl_opengl3.h"
#include "MapManager.h"

#include "SoundEngine.h"
#include "soloud.h"
#include "soloud_wav.h"
#include "ai.h"
/*extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}*/
#include "debug_draw.h";
#include "filesystem.h"
Tween g_tween;



#define WINDOW_WIDTH (1600)
#define WINDOW_HEIGHT (900)
#define FULL_SCREEN (0)
#define MSAA_C (0)
#define V_SYNC (0)

H3DNode main_camera = 0;
gentity_t g_entities = nullptr;
size_t g_entities_len = 0;
bool game_pause = false;
H3DRes background_mat, crosshairmat;

ArchiveReader g_archive_reader;

extern int player_health;

GLFWwindow* _winHandle;
float prev_x = 0;
float prev_y = 0;
bool running = true;
float timescale = 1;
bool edit_mode = false;
bool fisrt_mosue = true;

bool pause_after_current_frame=false;
void dw_console_log(const char* fmt, ...);
void windowCloseListener(GLFWwindow* win)
{
	running = false;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_P && action == GLFW_PRESS)
	{
		game_pause = !game_pause;
		pause_after_current_frame = false;
	}
	if (key == GLFW_KEY_T && action == GLFW_PRESS)
	{
		flyCamEnabled = !flyCamEnabled;
	}
	if (key == GLFW_KEY_O && action == GLFW_PRESS)
	{
		pause_after_current_frame = true;
		game_pause = false;
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

std::vector<H3DRes> matres;
int specunifind;

std::vector<H3DRes> watermats;
int waterparamsunifind;

H3DRes postmatres;
struct PostmatUniformIDs
{
	int exposure, thres, offset;
};
PostmatUniformIDs postmatuniforms;
void initWaterShader()
{
	H3DRes mat = 0;
	watermats.clear();
	while (mat = h3dGetNextResource(H3DResTypes::Material, mat))
	{
		std::string nm = h3dGetResName(mat);
		if (nm.find("materials/water/dirtywater/mat.material.xml") == std::string::npos)
			continue;
		auto m_samplerIndex = h3dFindResElem(mat, H3DMatRes::UniformElem, H3DMatRes::UnifNameStr, "waterParams");
		if (m_samplerIndex != -1)
		{
			printf("%d,%s\n", m_samplerIndex, h3dGetResName(mat));
			waterparamsunifind = m_samplerIndex;
			watermats.push_back(mat);
		}
	}
}
void detectMaterials()
{
	H3DRes mat = 0;
	matres.clear();
	while (mat=h3dGetNextResource(H3DResTypes::Material, mat))
	{
		std::string nm = h3dGetResName(mat);
		if (nm.find("materials/water/dirtywater/mat.material.xml") == std::string::npos)
			continue;
		auto m_samplerIndex = h3dFindResElem(mat, H3DMatRes::UniformElem, H3DMatRes::UnifNameStr, "matSpecParams");
		if (m_samplerIndex != -1)
		{
			printf("%d,%s\n", m_samplerIndex, h3dGetResName(mat));
			specunifind = m_samplerIndex;
			matres.push_back(mat);
		}
	}
	/*postmatres  = h3dFindResource(H3DResTypes::Material, "pipelines/postHDR.material.xml");
	postmatuniforms.exposure = h3dFindResElem(postmatres, H3DMatRes::UniformElem, H3DMatRes::UnifNameStr, "hdrExposure");
	postmatuniforms.thres = h3dFindResElem(postmatres, H3DMatRes::UniformElem, H3DMatRes::UnifNameStr, "hdrBrightThres");
	postmatuniforms.offset = h3dFindResElem(postmatres, H3DMatRes::UniformElem, H3DMatRes::UnifNameStr, "hdrBrightOffset");*/

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
#ifdef KH_RELEASE_PACKAGE
		g_archive_reader.addArchive("arch.bag");
#else
		g_archive_reader.addDirectory("D:/Alvahshi/game/sources/DanGameware/DanGameware/content");
		g_archive_reader.addDirectory(getenv("ALVAHSHI_CONTENT"));
#endif
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
	H3DRes overlay_shader = h3dAddResource(H3DResTypes::Shader, "shaders/overlay.shader", 0);
	crosshairmat = h3dAddResource(H3DResTypes::Material, "gui/hud/crosshair.material.xml", 0);
	background_mat = h3dAddResource(H3DResTypes::Material, "textures/backgroundfill.material.xml", 0);
	
	
	g_archive_reader.loadResourcesFromKhArchive([]() {});


	main_camera = h3dAddCameraNode(H3DRootNode, "Camera", pipeRes);
	//h3dSetNodeParamI(main_camera, H3DCamera::OccCullingI, 1);
	h3dSetNodeParamI(main_camera, H3DCamera::ViewportWidthI, winWidth);
	h3dSetNodeParamI(main_camera, H3DCamera::ViewportHeightI, winHeight);
	h3dSetupCameraView(main_camera, 70.0f, (float)winWidth / winHeight, 1, 30000);
	h3dResizePipelineBuffers(pipeRes, winWidth, winHeight);
	
}
void updateEmitters(float dt)
{
	unsigned int cnt = h3dFindNodes(H3DRootNode, "", H3DNodeTypes::Emitter);
	for (unsigned int i = 0; i < cnt; ++i)
		h3dUpdateEmitter(h3dGetNodeFindResult(i), dt);
}
float t_total_time = 0;
void gameupdate(float dt)
{
	t_total_time += dt;
	if(!edit_mode)
		g_input.capture(_winHandle);
	//ai
	if (g_entities != nullptr)
	{
		update_ents(g_entities, g_entities_len, dt);
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
	
	for (const auto& c : watermats)
	{
		h3dSetResParamF(c, H3DMatRes::UniformElem, waterparamsunifind, H3DMatRes::UnifValueF4,0, t_total_time);
	}

	khsound::updateCamPos(main_camera);

	//phdt = glfwGetTime() - t;
	//phys_time += phdt;
	//if (frames % 500 == 0)
		//dw_console_log("Physic time: avg=%.2fms (%d fps) current=%.2fms (%d fps)\n", (phys_time / frames) * 1000, (int)(frames / phys_time), phdt * 1000, (int)(1.0 / phdt));
	dumph3dMessages();
}
void gameRender()
{
	//show overlay
	const float ww = (float)h3dGetNodeParamI(main_camera, H3DCamera::ViewportWidthI) /(float)h3dGetNodeParamI(main_camera, H3DCamera::ViewportHeightI);
	const float w = 0.04;
	const float ovLogo[] = {
		(ww*0.5)-w, 0.5-w, 0, 1,
		(ww*0.5)-w, 0.5+w, 0, 0,
		(ww*0.5)+w, 0.5 + w, 1, 0,
		(ww*0.5)+w, 0.5 - w, 1, 1
	};
	h3dShowOverlays(ovLogo, 4, 1, 1, 1, 1, crosshairmat, 0);

	h3dRender(main_camera);
	h3dFinalizeFrame();
	h3dClearOverlays();
	debug_draw_frame(main_camera, false);

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwMakeContextCurrent(_winHandle);
	glfwSwapBuffers(_winHandle);
}
void imgui_frame();
std::vector<H3DNode> dynamic_lights;
int selected_light_idx = 0;
bool allow_tab=true;

/*int maadwawdin(void) {
	char buff[256];
	int error;
	lua_State *L = luaL_newstate();
	luaopen_base(L);
	luaopen_table(L);
	luaopen_io(L);   
	luaopen_string(L);
	luaopen_math(L);  

	while (fgets(buff, sizeof(buff), stdin) != NULL) {
		error = luaL_loadbuffer(L, buff, strlen(buff), "line") ||
			lua_pcall(L, 0, 0, 0);
		if (error) {
			fprintf(stderr, "%s", lua_tostring(L, -1));
			lua_pop(L, 1);  
		}
	}
	lua_close(L);
	return 0;
}*/
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
	
	const float ww = (float)h3dGetNodeParamI(main_camera, H3DCamera::ViewportWidthI) /
		(float)h3dGetNodeParamI(main_camera, H3DCamera::ViewportHeightI);

	// Show logo
	const float ovLogo[] = {
		0, 0, 0, 1,
		0, 1, 0, 0,
		ww, 1, 1, 0,
		ww, 0, 1, 1
	};
	h3dShowOverlays(ovLogo, 4, 1, 1, 1, 1, background_mat, 0);
	h3dRender(main_camera);
	h3dFinalizeFrame();
	h3dClearOverlays();

	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	glfwMakeContextCurrent(_winHandle);
	glfwSwapBuffers(_winHandle);
}
void main_load_map()
{
	g_tween = Tween();
	auto t1 = glfwGetTime();
	mapLoad("street", &mapLoadUpdate);
	detectMaterials();
	initWaterShader();
	dw_console_log("map load time = %0.2f", glfwGetTime() - t1);
}
bool do_reload_map = false;
void map_reload()
{
	do_reload_map = true;
}
//h3dsetGlobalShaderUniform(const char* name,)
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
	
	main_load_map();
	glfwSetInputMode(_winHandle, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	double last_t = glfwGetTime();
	while (running)
	{
		if (do_reload_map)
		{
			do_reload_map = false;
			main_load_map();
		}
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
		
		if (game_pause == false)
		{
			gameupdate(dt);
		}
		else
		{
			g_input.capture(_winHandle);
			::flyCam->Update(dt);
		}

		if (pause_after_current_frame)
		{
			game_pause = true;
		}
		gameRender();


	}
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
	khsound::shutdown_sound();
	return 0;
}
bool light_editor_open = true;
void alve_editor_draw_light_controls()
{
	

	if (light_editor_open)
		ImGui::Begin("Light Params", &light_editor_open);
	else
		return;


	ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar;
	ImGui::BeginChild("ChildL", ImVec2(0, 200), false, window_flags);
	for (int n = 0; n < dynamic_lights.size(); n++)
	{
		char buf[32];
		sprintf(buf, "Light %d", n);
		if (ImGui::Selectable(buf, selected_light_idx == n))
			selected_light_idx = n;
	}
	ImGui::EndChild();
	ImGui::Separator();
	bool closest = ImGui::Button("Closest");
	ImGui::SameLine();
	bool add = ImGui::Button("Add");
	if (add)
	{
		float cx, cy, cz;
		h3dGetNodeTransform(main_camera,&cx, &cy, &cz, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		addLight(Vector3df(cx, cy, cz), Vector3df(), Vector3df(1, 1, 1), 200, 2, false, 0.001, 360);
		selected_light_idx = dynamic_lights.size() - 1;
	}
	if (closest)
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
	ImGui::SameLine();
	if (dynamic_lights.size() > 0)
	{
		bool del = ImGui::Button("Delete");
		if (del)
			ImGui::OpenPopup("confirm_popup");
	}
	if (ImGui::BeginPopup("confirm_popup"))
	{
		ImGui::Text("Are you sure?");
		bool yes = ImGui::Button("Yes");
		if (yes)
		{
			auto light = dynamic_lights[selected_light_idx];
			h3dRemoveNode(light);
			dynamic_lights.erase(dynamic_lights.begin() + selected_light_idx);
			if (selected_light_idx >= dynamic_lights.size())
				selected_light_idx = dynamic_lights.size() - 1;
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	ImGui::Spacing();
	ImGui::Separator();

	if (dynamic_lights.size() == 0)
		return;
	H3DNode light = dynamic_lights[selected_light_idx];

	float pos[3],rot[3];
	h3dGetNodeTransform(light,pos, pos+1, pos+2,rot,rot+1,rot+2, nullptr, nullptr, nullptr);
	dd_sphere(Vector3df(pos[0], pos[1], pos[2]),5,DD_RED);

	ImGui::DragFloat3("Pos", pos);
	ImGui::DragFloat3("Rot", rot);
	h3dSetNodeTransform(light, pos[0], pos[1], pos[2], rot[0], rot[1], rot[2], 1, 1, 1);

	float fov = h3dGetNodeParamF(light, H3DLight::FovF, 0);
	ImGui::DragFloat("Fov", &fov, 1);
	h3dSetNodeParamF(light, H3DLight::FovF, 0, fov);
	float colx, coly, colz;
	colx = h3dGetNodeParamF(light, H3DLight::ColorF3, 0);
	coly = h3dGetNodeParamF(light, H3DLight::ColorF3, 1);
	colz = h3dGetNodeParamF(light, H3DLight::ColorF3, 2);
	
	float col[3] = { colx,coly,colz};

	ImGui::ColorEdit3("Diff", col);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 0, col[0]);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 1, col[1]);
	h3dSetNodeParamF(light, H3DLight::ColorF3, 2, col[2]);

	float rad = h3dGetNodeParamF(light, H3DLight::RadiusF, 0);
	ImGui::DragFloat("Rad", &rad);
	h3dSetNodeParamF(light, H3DLight::RadiusF, 0, rad);
	
	float mul = h3dGetNodeParamF(light, H3DLight::ColorMultiplierF, 0);
	ImGui::DragFloat("Int", &mul,0.01);
	h3dSetNodeParamF(light, H3DLight::ColorMultiplierF, 0, mul);

	float bias = h3dGetNodeParamF(light, H3DLight::ShadowMapBiasF, 0);
	ImGui::DragFloat("Bias", &bias, 0.0001);
	h3dSetNodeParamF(light, H3DLight::ShadowMapBiasF, 0, bias);
	
	bool shadow = h3dGetNodeParamI(light, H3DLight::ShadowMapCountI)>0;
	ImGui::Checkbox("Shadow", &shadow);
	if(shadow)
		h3dSetNodeParamI(light, H3DLight::ShadowMapCountI,3);
	else
		h3dSetNodeParamI(light, H3DLight::ShadowMapCountI, 0);
	
	ImGui::End();
}
void alve_editor_draw_material_controls()
{
	ImGui::Begin("Material Params");
	if (matres.size() == 0)
	{
		ImGui::Text("No material selected");
		ImGui::End();
		return;
	}
	float colx, coly, colz,colw;
	colx = h3dGetResParamF(matres[0], H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 0);
	coly = h3dGetResParamF(matres[0], H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 1);
	colz = h3dGetResParamF(matres[0], H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 2);
	colw = h3dGetResParamF(matres[0], H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 3);
	float col[3] = { colx,coly,colz };
	ImGui::ColorEdit3("Diffuse color", col);
	ImGui::DragFloat("Light intensity", &colw, 0.01);
	for (auto& mt : matres)
	{
		h3dSetResParamF(mt, H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 0, col[0]);
		h3dSetResParamF(mt, H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 1, col[1]);
		h3dSetResParamF(mt, H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 2, col[2]);
		h3dSetResParamF(mt, H3DMatRes::UniformElem, specunifind, H3DMatRes::UnifValueF4, 3, colw);
	}
	ImGui::End();
	/*ImGui::Begin("HDR params");
	float exp = h3dGetResParamF(postmatres, H3DMatRes::UniformElem, postmatuniforms.exposure, H3DMatRes::UnifValueF4, 0);
	float tres = h3dGetResParamF(postmatres, H3DMatRes::UniformElem, postmatuniforms.thres, H3DMatRes::UnifValueF4, 0);
	float offset = h3dGetResParamF(postmatres, H3DMatRes::UniformElem, postmatuniforms.offset, H3DMatRes::UnifValueF4, 0);
	ImGui::DragFloat("Exposure", &exp, 0.01);
	ImGui::DragFloat("Threshold", &tres, 0.01);
	ImGui::DragFloat("Offset", &offset, 0.01);

	h3dSetResParamF(postmatres, H3DMatRes::UniformElem, postmatuniforms.exposure, H3DMatRes::UnifValueF4, 0,exp);
	h3dSetResParamF(postmatres, H3DMatRes::UniformElem, postmatuniforms.thres, H3DMatRes::UnifValueF4, 0, tres);
	h3dSetResParamF(postmatres, H3DMatRes::UniformElem, postmatuniforms.offset, H3DMatRes::UnifValueF4, 0, offset);

	ImGui::End();*/

}
void draw_particle_editor()
{

}
f32 tttime;
int framescouint=0;
int last_fps = 0;
void imgui_stats_window()
{
	float frame_time = h3dGetStat(H3DStats::FrameTime, true);
	float batch_count = h3dGetStat(H3DStats::BatchCount, true);
	float tri_count = h3dGetStat(H3DStats::TriCount, true);
	float light_count = h3dGetStat(H3DStats::LightPassCount, true);
	float texturemem = h3dGetStat(H3DStats::TextureVMem, false);
	float geometry_mem = h3dGetStat(H3DStats::GeometryVMem, false);
	tttime += frame_time;
	framescouint++;
	if (tttime > 100)
	{
		tttime -= 100;
		last_fps = framescouint*10;
		framescouint = 0;
	}


	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	{
		ImVec2 window_pos, window_pos_pivot;
		window_pos.x = 10;
		window_pos.y = 10;
		window_pos_pivot.x = 0;
		window_pos_pivot.y = 0;
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		window_flags |= ImGuiWindowFlags_NoMove;
		ImGui::SetNextWindowBgAlpha(0.6f); // Transparent background
		bool pop = true;
		ImGui::Begin("Stats", &pop, window_flags);
		ImGui::Text("%d FPS (%d Batches,%d Lights)", last_fps, (int)batch_count, (int)light_count);
		ImGui::Text("%d Triangles", (int)tri_count);
		ImGui::Text("%d MB Textures,%d MB Models", (int)texturemem, (int)geometry_mem);
		ImGui::End();
	}
	
	{
		bool pop = true;
		ImVec2 window_pos, window_pos_pivot;
		window_pos.x = 10;
		window_pos.y = 90;
		window_pos_pivot.x = 0;
		window_pos_pivot.y = 0;
		ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
		ImGui::SetNextWindowBgAlpha(0.6f); // Transparent background
		ImGui::Begin("Player", &pop, window_flags);
		int ss = player_health;
		ImGui::Text("Health = %d", ss);
		ImGui::End();
	}
	
}
void alve_draw_options_menu()
{
	ImGui::Begin("Options");
	int current_item=0;
	const char* items[] = { "800x600", "1280x720", "1440x900", "1600x900", "1920x1080"};
	ImGui::Combo("Resolution", &current_item, items,5);
	bool f = true;
	ImGui::Checkbox("Fullscreen", &f);
	ImGui::Checkbox("V-sync", &f);
	ImGui::Button("Apply");
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
		//alve_draw_options_menu();
		alve_editor_draw_material_controls();
		khshowConsole();
		alve_editor_draw_light_controls();
	}
}




