#include"Gameplay.h"
#include <GLFW/glfw3.h>
playerInput g_input;
std::vector<GameObject*> gameObjects_array;
std::vector<GameObject*> to_add_gameobjects;
inline bool isKeyDown(int key, GLFWwindow* _winHandle)
{
	return glfwGetKey(_winHandle, key) == GLFW_PRESS;
}
bool always_run = true;
void playerInput::capture(GLFWwindow* _winHandle)
{
	this->dx = this->dy = 0;
	if (glfwGetKey(_winHandle,GLFW_KEY_W) == GLFW_PRESS)
		this->dy += 1;
	if (glfwGetKey(_winHandle, GLFW_KEY_S) == GLFW_PRESS)
		this->dy -= 1;
	if (glfwGetKey(_winHandle, GLFW_KEY_D) == GLFW_PRESS)
		this->dx += 1;
	if (glfwGetKey(_winHandle, GLFW_KEY_A) == GLFW_PRESS)
		this->dx -= 1;
	if (glfwGetKey(_winHandle, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || always_run)
	{
		this->dx *= 2;
		this->dy *= 2;
	}
	this->jumpPressed = glfwGetKey(_winHandle, GLFW_KEY_SPACE) == GLFW_PRESS;
	//this->attack= glfwGetKey(_winHandle, GLFW_KEY_E) == GLFW_PRESS;
	
}

void addGameObject(GameObject* go)
{
	to_add_gameobjects.push_back(go);
}
void commitGameObjectListChanges()
{
	for (auto go : to_add_gameobjects)
		gameObjects_array.push_back(go);
	to_add_gameobjects.clear();
}

void flyThroughCam::Update(float dt)
{
	float x, y, z, rx, ry;
	h3dGetNodeTransform(_cam, &x, &y, &z, &rx, &ry, nullptr, nullptr, nullptr, nullptr);

	float speed = 150* g_input.dy;
	float speedstr = 150 * -g_input.dx;
	{
		float dir_rad_y = ry*H3D_DEG2RAD;
		float dir_rad_x = rx*H3D_DEG2RAD;
		auto move_dir = Vector3df(-sinf(dir_rad_y), sinf(dir_rad_x), -cosf(dir_rad_y))*speed*dt;
		x += move_dir.x;
		y += move_dir.y;
		z += move_dir.z;
	}
	{
		float dir_rad_y = (ry+90)*H3D_DEG2RAD;
		auto move_dir = Vector3df(-sinf(dir_rad_y),0, -cosf(dir_rad_y))*speedstr*dt;
		x += move_dir.x;
		z += move_dir.z;
	}

	float sens = 5;
	ry -= g_input.drx*sens*dt;
	// Loop up/down but only in a limited range
	rx += g_input.dry*sens*dt;
	if (rx > 90) rx = 90;
	if (rx < -90) rx = -90;

	h3dSetNodeTransform(_cam, x, y, z, rx, ry, 0, 1, 1, 1);
}

float SmoothDamp(float current, float target, float& currentVelocity, float smoothTime, float maxSpeed, float deltaTime)
{
	smoothTime = khmax(0.0001f, smoothTime);
	float num = 2.0f / smoothTime;
	float num2 = num * deltaTime;
	float num3 = 1.0f / (1.0f + num2 + 0.48f * num2 * num2 + 0.235f * num2 * num2 * num2);
	float num4 = current - target;
	float num5 = target;
	float num6 = maxSpeed * smoothTime;
	num4 = clamp(num4, -num6, num6);
	target = current - num4;
	float num7 = (currentVelocity + num * num4) * deltaTime;
	currentVelocity = (currentVelocity - num * num7) * num3;
	float num8 = target + (num4 + num7) * num3;
	if (num5 - current > 0 == num8 > num5)
	{
		num8 = num5;
		currentVelocity = (num8 - num5) / deltaTime;
	}
	return num8;
}
