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
#include <math.h>
#include "main.hpp"

float Villager::talkDelay = 0.0f;

Archer::Archer() : Creature("archer") {
	pathTimer=0.0f;
}

bool Archer::update(float elapsed) {
	static float arrowSpeed=config.getFloatProperty("config.gameplay.arrowSpeed");
	pathTimer+=elapsed;
	if ( ! Creature::update(elapsed) ) return false;
	if ( gameEngine->dungeon->map->isInFov((int)x,(int)y) ) {
		// see player
		if ( pathTimer > 1.0f ) {
			pathTimer=0.0f;
			Item *arrow=Item::getItem("arrow",x+0.5f,y+0.5f,false);
			arrow->dx = gameEngine->player->x-x;
			arrow->dy = gameEngine->player->y-y;
			arrow->speed = arrowSpeed;
			float angle=atan2(-arrow->dy,arrow->dx)/3.14159f;   // between -1 and 1
			int iangle=(int)((angle-1.0f/16)*8);
			if ( iangle < 0 ) iangle += 16; // between 0 and 15
			if ((unsigned)iangle >= 16) *(int *)(NULL)=0; // triggers sigsegv
			static int angleChar[] = {
				TCOD_CHAR_BOLT_E,
				TCOD_CHAR_BOLT_ENE,
				TCOD_CHAR_BOLT_NE,
				TCOD_CHAR_BOLT_NNE,
				TCOD_CHAR_BOLT_N,
				TCOD_CHAR_BOLT_SSE,
				TCOD_CHAR_BOLT_SE,
				TCOD_CHAR_BOLT_ESE,
				TCOD_CHAR_BOLT_E,
				TCOD_CHAR_BOLT_ENE,
				TCOD_CHAR_BOLT_NE,
				TCOD_CHAR_BOLT_NNE,
				TCOD_CHAR_BOLT_N,
				TCOD_CHAR_BOLT_SSE,
				TCOD_CHAR_BOLT_SE,
				TCOD_CHAR_BOLT_ESE,
			};
			float l=sqrt(arrow->dx*arrow->dx + arrow->dy*arrow->dy);
			arrow->dx /= l;
			arrow->dy /= l;
			arrow->duration = l / arrow->speed;
			arrow->ch=angleChar[iangle];
			gameEngine->dungeon->addItem(arrow);
		}
	} 
	return true;
}

Villager::Villager() : Creature("villager") {
}

bool Villager::update(float elapsed) {
	static TextGenerator talkGenerator("data/cfg/villager.txg");
	bool oldSeen=hasBeenSeen();
	if (!Creature::update(elapsed)) return false;
	if ( !oldSeen && hasBeenSeen() && talkDelay > 10.0f ) {
		talkDelay=0.0f;
		talk(talkGenerator.generate("villager","${SPOTTED}"));
	}
	if (Creature::creatureByType[CREATURE_VILLAGER].get(0) == this ) {
		talkDelay += elapsed;
	}
	return true;
}

