/*
* Copyright (c) 2009 Jice
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

#include "main.hpp"

VillageHead::VillageHead() : Boss("village head") {
	summonMinions=true;
}

Boss::Boss() : Creature("Zeepoh") {
	init();
}

void Boss::init() {
	summonTimer=0.0f;
	summonMinions=true;
}

Boss::Boss(const char *typeName) : Creature(typeName) {
	init();
}

// boss can't be stunned
void Boss::stun(float delay) {}

bool Boss::update(float elapsed) {
	//static float summonTime=config.getFloatProperty("config.creatures.boss.summonTime");
	//static int minionCount=config.getIntProperty("config.creatures.boss.minionCount");

	if (! Creature::update(elapsed) ) return false;
	if ( !hasBeenSeen() ) {
		if ( gameEngine->dungeon->isCellInFov(x,y) && gameEngine->dungeon->getMemory(x,y) ) {
			// creature is seen by player
			setSeen(true);
		}
		return true;
	}
	/*
	summonTimer+=elapsed;
	if ( summonMinions && summonTimer > summonTime ) {
		// summon some minions to protect the boss
		summonTimer=0.0f;
		for (int i=0; i< minionCount; i++ ) {
			AiDirector::instance->spawnMinion(true,(int)x,(int)y);
		}
	}
	*/
	return getLife() > 0;
}
