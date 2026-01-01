#pragma once

#include "Invader.h"

class Mothership : public Invader {
public:
	Mothership(Game* game, int index);
	~Mothership();

	/// Update the mothership
	void Update(double t) override;

private:
	// Time of the last update
	double update_frame_start;

	// Direction mothership is flying in
	float direction;
};
