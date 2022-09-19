#pragma once
#include <functional>
#include <vector>

enum class EASING_FUNCTION
{
	Linear,
	QuadraticEaseIn,
	QuadraticEaseOut,
	QuadraticEaseInOut,
	CubicEaseIn,
	CubicEaseOut,
	CubicEaseInOut,
	QuarticEaseIn,
	QuarticEaseOut,
	QuarticEaseInOut,
	QuinticEaseIn,
	QuinticEaseOut,
	QuinticEaseInOut,
	SineEaseIn,
	SineEaseOut,
	SineEaseInOut,
	CircularEaseIn,
	CircularEaseOut,
	CircularEaseInOut,
	ExponentialEaseIn,
	ExponentialEaseOut,
	ExponentialEaseInOut,
	ElasticEaseIn,
	ElasticEaseOut,
	ElasticEaseInOut,
	BackEaseIn,
	BackEaseOut,
	BackEaseInOut,
	BounceEaseIn,
	BounceEaseOut,
	BounceEaseInOut
};


struct FuncTween
{
	float startVal;
	float endVal;
	std::function<void(float)> target;
	float time;
	float delay;
	int repeatCount;
	bool reflect;
	EASING_FUNCTION easing;
	float currentTime;
	int tag;
};
struct DelayCall
{
	float delay;
	std::function<void()> function;
	int tag;
};

class DisplayObject;
class Tween
{
	std::vector<FuncTween> calls;
	std::vector<DelayCall> delayed_calls;
public:
	void update(float dt);
	void removeByTag(int tag);
	void delayCall(float delay, const std::function<void()>& func, int tag = 0);
	void callFuncPeriodic(float start_value, float end_value, std::function<void(float)>  target, float time, EASING_FUNCTION easing, int tag = 0, float delay = 0, int repeat_count = 1, bool reflect = false);
};
