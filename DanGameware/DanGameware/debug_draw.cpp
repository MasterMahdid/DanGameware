#include "debug_draw.h"
#include <Horde3D.h>
#include "utMath.h"
#include "GLFW\glfw3.h"
#include "imgui.h"
namespace debug_draw_internal
{
	struct ddline_s
	{
		Vector3df a, b, color;
		f64 expire_time;
	};
	struct ddpoint_s
	{
		Vector3df pos, color;
		f32 size;
		f64 expire_time;
	};

	struct ddlable_s
	{
		Vector3df pos;
		const char* str;
		f64 expire_time;
	};

	ddline_s* dd_lines;
	size_t dd_lines_count;

	ddpoint_s* dd_points;
	size_t dd_points_count;

	ddlable_s* dd_labels;
	size_t dd_labels_count;

	f64 getCurrentTime()
	{
		return glfwGetTime();
	}

	template<typename T>
	static void clearDebugQueue(f64 current_time,T * queue, size_t &queueCount)
	{
		int index = 0;
		T * pElem = queue;
		for (int i = 0; i < queueCount; ++i, ++pElem)
		{
			if (pElem->expire_time > current_time)
			{
				if (index != i)
				{
					queue[index] = *pElem;
				}
				++index;
			}
		}
		queueCount = index;
	}
}

using namespace debug_draw_internal;

void draw_lines()
{
	glBegin(GL_LINES);
	for (size_t i = 0; i < dd_lines_count; i++)
	{
		glColor3f(dd_lines[i].color.x, dd_lines[i].color.y, dd_lines[i].color.z);
		glVertex3f(dd_lines[i].a.x, dd_lines[i].a.y, dd_lines[i].a.z);
		glVertex3f(dd_lines[i].b.x, dd_lines[i].b.y, dd_lines[i].b.z);
	}
	glEnd();
}
void draw_points()
{
	for (size_t i = 0; i < dd_points_count; i++)
	{
		glPointSize(dd_points[i].size);
		glBegin(GL_POINTS);
		glColor3f(dd_points[i].color.x, dd_points[i].color.y, dd_points[i].color.z);
		glVertex3f(dd_points[i].pos.x, dd_points[i].pos.y, dd_points[i].pos.z);
		glEnd();
	}
	
	
}

void dd_line(Vector3df a, Vector3df b, Vector3df color, f32 duration)
{
	auto & ln = dd_lines[dd_lines_count++];
	ln.a = a;
	ln.b = b;
	ln.color = color;
	ln.expire_time = getCurrentTime() + duration;
}
void dd_point(Vector3df pos, Vector3df color, f32 size, f32 duration)
{
	auto & pt = dd_points[dd_points_count++];
	pt.pos = pos;
	pt.color = color;
	pt.size = size;
	pt.expire_time = getCurrentTime() + duration;
}
void dd_sphere(Vector3df center, f32 radius, Vector3df color, f32 duration)
{
	static const int stepSize = 15;
	static const int cz = 360 / stepSize;
	static const float pi = 4 * atan(1);
	static const float deg_to_rad = (1.0f / 180.0f)*pi;
	Vector3df cache[cz];
	Vector3df radiusVec(0,0,radius);
	cache[0] = center + radiusVec;

	for (int n = 1; n < cz; ++n)
	{
		cache[n] = cache[0];
	}
	Vector3df lastPoint, temp;
	for (int i = stepSize; i <= 360; i += stepSize)
	{
		const float s = sinf(i*deg_to_rad);
		const float c = cosf(i*deg_to_rad);
		
		lastPoint.x = center.x;
		lastPoint.y = center.y + radius * s;
		lastPoint.z = center.z + radius * c;

		for (int n = 0, j = stepSize; j <= 360; j += stepSize, ++n)
		{
			temp.x = center.x + sinf(j*deg_to_rad) * radius * s;
			temp.y = center.y + cosf(j*deg_to_rad) * radius * s;
			temp.z = lastPoint.z;

			dd_line(lastPoint, temp, color, duration);
			dd_line(lastPoint, cache[n], color, duration);

			cache[n] = lastPoint;
			lastPoint = temp;
		}
	}
}
void dd_axes(float* mat,f32 duration)
{
	Horde3D::Vec3f xa(100, 0, 0);
	Horde3D::Vec3f ya(0, 100, 0);
	Horde3D::Vec3f za(0, 0, 100);

	Horde3D::Vec3f c(0, 0, 0);

	Horde3D::Vec3f xa2, ya2, za2,c2;

	Horde3D::Matrix4f mt2(mat);
	//mt2 = mt2.inverted();

	xa2 = mt2*xa;
	ya2 = mt2*ya;
	za2 = mt2*za;
	c2 = mt2*c;

	auto conv = [](const Horde3D::Vec3f v) -> Vector3df
	{
		return Vector3df(v.x, v.y, v.z);
	};

	dd_line(conv(c2), conv(xa2), Vector3df(1, 0, 0), duration);
	dd_line(conv(c2), conv(ya2), Vector3df(0, 1, 0), duration);
	dd_line(conv(c2), conv(za2), Vector3df(0, 0, 1), duration);
}
void dd_box(const Vector3df points[8], Vector3df color, float duration)
{
	for (int i = 0; i < 4; ++i)
	{
		dd_line(points[i], points[(i + 1) & 3], color, duration);
		dd_line(points[4 + i], points[4 + ((i + 1) & 3)], color, duration);
		dd_line(points[i], points[4 + i], color, duration);
	}
}

void dd_aabb(Vector3df min, Vector3df max, Vector3df color, float duration)
{
	Vector3df bb[2];
	Vector3df points[8];
	bb[0] = min;
	bb[1] = max;
	// Expand min/max bounds:
	for (int i = 0; i < 8; ++i)
	{
		points[i].x = bb[(i ^ (i >> 1)) & 1].x;
		points[i].y = bb[(i >> 1) & 1].y;
		points[i].z = bb[(i >> 2) & 1].z;
	}

	// Build the lines:
	dd_box(points, color, duration);
}

void debug_draw_init()
{
	size_t dd_lines_size = 10000;
	dd_lines_count = 0;
	dd_lines = new ddline_s[dd_lines_size];

	size_t dd_points_size = 10000;
	dd_points_count = 0;
	dd_points = new ddpoint_s[dd_points_size];


	size_t dd_labels_size = 10000;
	dd_labels_count = 0;
	dd_labels = new ddlable_s[dd_labels_size];

}
void showpopuptext(const char* txt, float x, float y)
{
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
	ImVec2 window_pos, window_pos_pivot;
	window_pos.x = x;
	window_pos.y = y;
	window_pos_pivot.x = 0;
	window_pos_pivot.y = 0;
	ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
	window_flags |= ImGuiWindowFlags_NoMove;
	ImGui::SetNextWindowBgAlpha(0.1f); // Transparent background
	bool pop = true;
	ImGui::Begin(txt, &pop, window_flags);
	ImGui::Text(txt);
	ImGui::End();
}
void dd_string(const char* str, Vector3df pos, Vector3df color, float duration)
{
	auto & lb = dd_labels[dd_labels_count++];
	lb.pos = pos;
	lb.str = str;
	lb.expire_time = getCurrentTime() + duration;
}
void debug_draw_imgui(H3DNode camera)
{
	float* proj_mat = new float[16];
	h3dGetCameraProjMat(camera, proj_mat);
	Horde3D::Matrix4f prj(proj_mat);

	const float* view_mat = new float[16];
	h3dGetNodeTransMats(camera, NULL, &view_mat);
	Horde3D::Matrix4f view(view_mat);
	view = view.inverted();

	auto awd = prj*view;




	




	for (size_t i = 0; i < dd_labels_count; i++)
	{
		Horde3D::Vec3f vec;
		vec.x = dd_labels[i].pos.x;
		vec.y = dd_labels[i].pos.y;
		vec.z = dd_labels[i].pos.z;


		auto screen_pos = awd*vec;

		//Horde3D::Vec3f screen_pos = pjm*(mt2*vec);
		//Horde3D::Vec3f screen_pos = pjm*(mt2*vec);

		showpopuptext(dd_labels[i].str,screen_pos.x,screen_pos.y);
	}
	clearDebugQueue(getCurrentTime(), dd_labels, dd_labels_count);
}
void debug_draw_frame(H3DNode camera,bool depth_test)
{
	float* proj_mat = new float[16];
	h3dGetCameraProjMat(camera, proj_mat);
	const float* view_mat = new float[16];
	h3dGetNodeTransMats(camera, NULL, &view_mat);
	Horde3D::Matrix4f mt2(view_mat);
	mt2 = mt2.inverted();
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf(proj_mat);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf(mt2.x);
	if(depth_test)
		glEnable(GL_DEPTH_TEST);
	else
		glDisable(GL_DEPTH_TEST);

	draw_lines();
	draw_points();

	clearDebugQueue(getCurrentTime(), dd_lines, dd_lines_count);
	clearDebugQueue(getCurrentTime(), dd_points, dd_points_count);
	//clearqueue
}



