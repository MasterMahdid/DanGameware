#pragma once
#include "khmath.h"
#include "Horde3D.h"
void debug_draw_init();
void debug_draw_frame(H3DNode camera, bool depth_test);
void debug_draw_imgui(H3DNode camera);
void dd_line(Vector3df a, Vector3df b,Vector3df color, f32 duration=0);
void dd_sphere(Vector3df center, f32 radius, Vector3df color, f32 duration=0);
void dd_point(Vector3df pos,Vector3df color,f32 size,f32 duration=0);
void dd_axes(float* mat, f32 duration = 0);
void dd_box(const Vector3df points[8], Vector3df color, float duration=0);
void dd_aabb(Vector3df min, Vector3df max,Vector3df color,float duration=0);
void dd_string(const char* str, Vector3df pos, Vector3df color,float duration);