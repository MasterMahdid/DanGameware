#include <algorithm>
#include <math.h>
#include "Tween.h"
namespace easing_functions
{
	float PI = std::atan(1) * 4;
	float HALFPI = PI / 2;
	float Linear(float p)
	{
		return p;
	}
	float QuadraticEaseIn(float p)
	{
		return p * p;
	}
	float QuadraticEaseOut(float p)
	{
		return -(p * (p - 2));
	}
	float QuadraticEaseInOut(float p)
	{
		if (p < 0.5f)
		{
			return 2 * p * p;
		}
		else
		{
			return (-2 * p * p) + (4 * p) - 1;
		}
	}
	float CubicEaseIn(float p)
	{
		return p * p * p;
	}
	float CubicEaseOut(float p)
	{
		float f = (p - 1);
		return f * f * f + 1;
	}
	float CubicEaseInOut(float p)
	{
		if (p < 0.5f)
		{
			return 4 * p * p * p;
		}
		else
		{
			float f = ((2 * p) - 2);
			return 0.5f * f * f * f + 1;
		}
	}
	float QuarticEaseIn(float p)
	{
		return p * p * p * p;
	}
	float QuarticEaseOut(float p)
	{
		float f = (p - 1);
		return f * f * f * (1 - p) + 1;
	}
	float QuarticEaseInOut(float p)
	{
		if (p < 0.5f)
		{
			return 8 * p * p * p * p;
		}
		else
		{
			float f = (p - 1);
			return -8 * f * f * f * f + 1;
		}
	}
	float QuinticEaseIn(float p)
	{
		return p * p * p * p * p;
	}
	float QuinticEaseOut(float p)
	{
		float f = (p - 1);
		return f * f * f * f * f + 1;
	}
	float QuinticEaseInOut(float p)
	{
		if (p < 0.5f)
		{
			return 16 * p * p * p * p * p;
		}
		else
		{
			float f = ((2 * p) - 2);
			return 0.5f * f * f * f * f * f + 1;
		}
	}
	float SineEaseIn(float p)
	{
		return (float)sinf((p - 1) * HALFPI) + 1;
	}
	float SineEaseOut(float p)
	{
		return (float)sinf(p * HALFPI);
	}
	float SineEaseInOut(float p)
	{
		return 0.5f * (1 - (float)cosf(p * PI));
	}
	float CircularEaseIn(float p)
	{
		return 1 - (float)sqrtf(1 - (p * p));
	}
	float CircularEaseOut(float p)
	{
		return (float)sqrtf((2 - p) * p);
	}
	float CircularEaseInOut(float p)
	{
		if (p < 0.5f)
		{
			return 0.5f * (1 - (float)sqrtf(1 - 4 * (p * p)));
		}
		else
		{
			return 0.5f * ((float)sqrtf(-((2 * p) - 3) * ((2 * p) - 1)) + 1);
		}
	}
	float ExponentialEaseIn(float p)
	{
		return (p == 0.0f) ? p : (float)pow(2, 10 * (p - 1));
	}
	float ExponentialEaseOut(float p)
	{
		return (p == 1.0f) ? p : 1 - (float)pow(2, -10 * p);
	}
	float ExponentialEaseInOut(float p)
	{
		if (p == 0.0 || p == 1.0) return p;

		if (p < 0.5f)
		{
			return 0.5f * (float)pow(2, (20 * p) - 10);
		}
		else
		{
			return -0.5f * (float)pow(2, (-20 * p) + 10) + 1;
		}
	}
	float ElasticEaseIn(float p)
	{
		return (float)sinf(13 * HALFPI * p) * (float)pow(2, 10 * (p - 1));
	}
	float ElasticEaseOut(float p)
	{
		return (float)sinf(-13 * HALFPI * (p + 1)) * (float)pow(2, -10 * p) + 1;
	}
	float ElasticEaseInOut(float p)
	{
		if (p < 0.5f)
		{
			return 0.5f * (float)sinf(13 * HALFPI * (2 * p)) * (float)pow(2, 10 * ((2 * p) - 1));
		}
		else
		{
			return 0.5f * ((float)sinf(-13 * HALFPI * ((2 * p - 1) + 1)) * (float)pow(2, -10 * (2 * p - 1)) + 2);
		}
	}
	float BackEaseIn(float p)
	{
		return p * p * p - p * (float)sinf(p * PI);
	}
	float BackEaseOut(float p)
	{
		float f = (1 - p);
		return 1 - (f * f * f - f * (float)sinf(f * PI));
	}
	float BackEaseInOut(float p)
	{
		if (p < 0.5f)
		{
			float f = 2 * p;
			return 0.5f * (f * f * f - f * (float)sinf(f * PI));
		}
		else
		{
			float f = (1 - (2 * p - 1));
			return 0.5f * (1 - (f * f * f - f * (float)sinf(f * PI))) + 0.5f;
		}
	}

	float BounceEaseOut(float p)
	{
		if (p < 4 / 11.0f)
		{
			return (121 * p * p) / 16.0f;
		}
		else if (p < 8 / 11.0f)
		{
			return (363 / 40.0f * p * p) - (99 / 10.0f * p) + 17 / 5.0f;
		}
		else if (p < 9 / 10.0f)
		{
			return (4356 / 361.0f * p * p) - (35442 / 1805.0f * p) + 16061 / 1805.0f;
		}
		else
		{
			return (54 / 5.0f * p * p) - (513 / 25.0f * p) + 268 / 25.0f;
		}
	}
	float BounceEaseIn(float p)
	{
		return 1 - BounceEaseOut(1 - p);
	}
	float BounceEaseInOut(float p)
	{
		if (p < 0.5f)
		{
			return 0.5f * BounceEaseIn(p * 2);
		}
		else
		{
			return 0.5f * BounceEaseOut(p * 2 - 1) + 0.5f;
		}
	}
	float Interpolate(float p, EASING_FUNCTION f)
	{
		switch (f)
		{
		case EASING_FUNCTION::Linear: return Linear(p);
		case EASING_FUNCTION::QuadraticEaseOut: return QuadraticEaseOut(p);
		case EASING_FUNCTION::QuadraticEaseIn: return QuadraticEaseIn(p);
		case EASING_FUNCTION::QuadraticEaseInOut: return QuadraticEaseInOut(p);
		case EASING_FUNCTION::CubicEaseIn: return CubicEaseIn(p);
		case EASING_FUNCTION::CubicEaseOut: return CubicEaseOut(p);
		case EASING_FUNCTION::CubicEaseInOut: return CubicEaseInOut(p);
		case EASING_FUNCTION::QuarticEaseIn: return QuarticEaseIn(p);
		case EASING_FUNCTION::QuarticEaseOut: return QuarticEaseOut(p);
		case EASING_FUNCTION::QuarticEaseInOut: return QuarticEaseInOut(p);
		case EASING_FUNCTION::QuinticEaseIn: return QuinticEaseIn(p);
		case EASING_FUNCTION::QuinticEaseOut: return QuinticEaseOut(p);
		case EASING_FUNCTION::QuinticEaseInOut: return QuinticEaseInOut(p);
		case EASING_FUNCTION::SineEaseIn: return SineEaseIn(p);
		case EASING_FUNCTION::SineEaseOut: return SineEaseOut(p);
		case EASING_FUNCTION::SineEaseInOut: return SineEaseInOut(p);
		case EASING_FUNCTION::CircularEaseIn: return CircularEaseIn(p);
		case EASING_FUNCTION::CircularEaseOut: return CircularEaseOut(p);
		case EASING_FUNCTION::CircularEaseInOut: return CircularEaseInOut(p);
		case EASING_FUNCTION::ExponentialEaseIn: return ExponentialEaseIn(p);
		case EASING_FUNCTION::ExponentialEaseOut: return ExponentialEaseOut(p);
		case EASING_FUNCTION::ExponentialEaseInOut: return ExponentialEaseInOut(p);
		case EASING_FUNCTION::ElasticEaseIn: return ElasticEaseIn(p);
		case EASING_FUNCTION::ElasticEaseOut: return ElasticEaseOut(p);
		case EASING_FUNCTION::ElasticEaseInOut: return ElasticEaseInOut(p);
		case EASING_FUNCTION::BackEaseIn: return BackEaseIn(p);
		case EASING_FUNCTION::BackEaseOut: return BackEaseOut(p);
		case EASING_FUNCTION::BackEaseInOut: return BackEaseInOut(p);
		case EASING_FUNCTION::BounceEaseIn: return BounceEaseIn(p);
		case EASING_FUNCTION::BounceEaseOut: return BounceEaseOut(p);
		case EASING_FUNCTION::BounceEaseInOut: return BounceEaseInOut(p);
		default:return Linear(p);
		}
	}
}
void Tween::callFuncPeriodic(float start_value, float end_value, std::function<void(float)> target, float time, EASING_FUNCTION easing, int tag, float delay, int repeat_count, bool reflect)
{
	FuncTween f;
	f.startVal = start_value;
	f.endVal = end_value;
	f.target = target;
	f.time = time;
	f.easing = easing;
	f.currentTime = 0;
	f.tag = tag;
	f.delay = delay;
	f.repeatCount = repeat_count;
	f.reflect = reflect;
	calls.push_back(f);
}

void Tween::update(float dt)
{
	for (auto& c : calls)
	{
		if (c.delay > 0)
		{
			c.delay -= dt;
			continue;
		}
		c.currentTime += dt;
		float percent = c.currentTime / c.time;
		if (percent>1)
			percent = 1;
		if (percent<0)
			percent = 0;
		float intp = easing_functions::Interpolate(percent, c.easing);
		float val;
		if (c.startVal<c.endVal)
			val = c.startVal + (c.endVal - c.startVal) * intp;
		else
			val = c.startVal - (c.startVal - c.endVal) * intp;
		c.target(val);

		if (c.currentTime > c.time)
		{
			if (c.repeatCount > 1 || c.repeatCount == 0)
			{
				if (c.repeatCount != 0)
					c.repeatCount--;
				c.currentTime = 0;
				if (c.reflect)
				{
					auto t = c.startVal;
					c.startVal = c.endVal;
					c.endVal = t;
				}
			}
		}
	}
	for (auto& c : delayed_calls)
	{
		c.delay -= dt;
		if (c.delay <= 0)
			c.function();
	}
	calls.erase(std::remove_if(calls.begin(), calls.end(), [](const FuncTween& c)
	{
		return c.currentTime > c.time;
	}), calls.end());
	delayed_calls.erase(std::remove_if(delayed_calls.begin(), delayed_calls.end(), [](const DelayCall& c)
	{
		return c.delay <= 0;
	}), delayed_calls.end());

}

void Tween::removeByTag(int tag)
{
	calls.erase(std::remove_if(calls.begin(), calls.end(), [=](const FuncTween& c)
	{
		return c.tag == tag;
	}), calls.end());
}

void Tween::delayCall(float delay, const std::function<void()>& func, int tag)
{
	DelayCall c;
	c.delay = delay;
	c.function = func;
	c.tag = tag;
	delayed_calls.push_back(c);
}


