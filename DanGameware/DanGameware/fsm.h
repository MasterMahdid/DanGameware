#pragma once
#include <functional>

enum class range
{
	RANGE_MELLEE,
	RANGE_NEAR,
	RANGE_MID,
	RANGE_FAR
};
class State
{
public:
	virtual ~State() {};
	virtual void enter(const State* prev_state) {};
	virtual void execute(float dt) {};
	virtual void exit(const State* next_state) {};
};
class AiServices
{
public:
	virtual void moveToTarget() = 0;
	virtual void setTarget(int target) = 0;
	virtual void stand() = 0;
	virtual void melee() = 0;
	virtual void ranged() = 0;

};
struct agent
{
	std::function<void()> think;
	float next_think;//time0
};