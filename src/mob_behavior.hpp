/*
* Copyright (c) 2009,2010 Jice
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * The name of Jice may not be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY Jice ``AS IS'' AND ANY
* EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL Jice BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

enum EWalkFlags {
	WATER_ONLY=1, // fish
	AVOID_WATER=2, // tries not to walk in water
	FLEE_PLAYER=4, // get away from player
	IGNORE_CREATURES=8, // can walk where there is another creature
};

class WalkPattern : public ITCODPathCallback {
protected :
	int flags;
public :
	WalkPattern(int flags=IGNORE_CREATURES) : flags(flags) {}
	float getWalkCost( int xFrom, int yFrom, int xTo, int yTo, void *userData ) const;
};

class WaterOnlyWalkPattern : public WalkPattern {
public :
	WaterOnlyWalkPattern() : WalkPattern(WATER_ONLY|IGNORE_CREATURES) {}
};

class AvoidWaterWalkPattern : public WalkPattern {
public :
	AvoidWaterWalkPattern() : WalkPattern(AVOID_WATER|IGNORE_CREATURES) {}
};

class Creature;

class Behavior {
public :
	virtual bool update(Creature *crea, float elapsed);
};

class AttackOnSee : public Behavior {
public :
	AttackOnSee(float pathDelay);
	bool update(Creature *crea, float elapsed);
protected :
	float pathTimer;
	float pathDelay;
};

class WalkBehavior  : public Behavior {
public :
	WalkBehavior(WalkPattern *walkPattern, float pathDelay, bool trackPlayer=false);
	bool update(Creature *crea, float elapsed);
protected :
	WalkPattern *walkPattern;	
	float pathTimer;
	float pathDelay;
	bool trackPlayer;
};

class FollowBehavior : public WalkBehavior {
public :
	FollowBehavior(WalkPattern *walkPattern, float pathDelay) : WalkBehavior(walkPattern,pathDelay),leader(NULL),standDelay(0.0f) {}
	void setLeader(Creature *leader) { this->leader=leader;}
	bool update(Creature *crea, float elapsed);	
protected :
	Creature *leader;
	float standDelay;
};

// default duration of a scare point in seconds
#define SCARE_LIFE 1.0

class ScarePoint : public Entity {
public :
	float life;
	ScarePoint(float x, float y, float life=SCARE_LIFE) : Entity(x,y),life(life) {}
};


class HerdBehavior : public WalkBehavior {
public :
	HerdBehavior(WalkPattern *walkPattern, float pathDelay) : WalkBehavior(walkPattern,pathDelay) {}
	virtual ~HerdBehavior() {}
	bool update(Creature *crea, float elapsed);
	static void addScarePoint(int x, int y, float life=SCARE_LIFE);	
	static void updateScarePoints(float elapsed);
	static void recomputeHerds();
protected :
	static TCODList<ScarePoint *> scare;
	TCODList<Creature *>herd; // current herd for this creature
};
