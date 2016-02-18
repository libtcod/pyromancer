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

#include <math.h>
#include "main.hpp"

TCODList<ScarePoint *> HerdBehavior::scare;

#define FOLLOW_DIST 5

float WalkPattern::getWalkCost( int xFrom, int yFrom, int xTo, int yTo, void *userData ) const {
	static int secureDist=config.getIntProperty("config.creatures.boss.secureDist");
	static float secureCoef=config.getFloatProperty("config.creatures.boss.secureCoef");

	Dungeon *dungeon=gameEngine->dungeon;
	if ( !dungeon->map->isWalkable(xTo,yTo)) return 0.0f;
	bool ripples=dungeon->hasRipples(xTo,yTo);
	if ( (flags & WATER_ONLY) && !ripples ) return 0.0f;
	if ( gameEngine->fireManager && gameEngine->fireManager->isBurning(xTo,yTo) ) return 100.0f;
	if ( flags & FLEE_PLAYER ) {
		float pdist=SQRDIST(gameEngine->player->x,gameEngine->player->y,xTo,yTo);
		if ( pdist < secureDist ) return 1.0f + secureCoef*(secureDist - pdist);
	}
	if ( ( flags & IGNORE_CREATURES ) == 0 ) {
		if ( gameEngine->dungeon->hasCreature(xTo,yTo) ) return 50.0f;
		if ( gameEngine->player->x == xTo && gameEngine->player->y == yTo ) return 50.0f;
	}
	float cost = terrainTypes[dungeon->getTerrainType(xTo,yTo)].walkCost;
	if ( (flags & AVOID_WATER) && ripples ) cost *= 3; // try to avoid getting wet!
	return cost;
}

WalkBehavior::WalkBehavior(WalkPattern *walkPattern, float pathDelay, bool trackPlayer) 
	: walkPattern(walkPattern), pathDelay(pathDelay), trackPlayer(trackPlayer) {
	pathTimer=TCODRandom::getInstance()->getFloat(0.0f,pathDelay);	
}

bool WalkBehavior::update(Creature *crea, float elapsed) {
	Behavior::update(crea,elapsed);
	if ( crea->speed > 0.0f ) return true; // cannot walk while 'flying'
	pathTimer+=elapsed;
	if ( pathTimer > pathDelay ){
		if ( ! crea->path || crea->path->isEmpty() ) {
			// stay away from player
			// while staying in lair
			int destx,desty;
			if ( trackPlayer ) {
				destx=(int)gameEngine->player->x+TCODRandom::getInstance()->getInt(-15,15);
				desty=(int)gameEngine->player->y+TCODRandom::getInstance()->getInt(-15,15);
			} else {
				destx = (int)(crea->x+TCODRandom::getInstance()->getInt(-15,15));
				desty = (int)(crea->y+TCODRandom::getInstance()->getInt(-15,15));
			}
			destx=CLAMP(0,gameEngine->dungeon->width-1,destx);
			desty=CLAMP(0,gameEngine->dungeon->height-1,desty);
			gameEngine->dungeon->getClosestWalkable(&destx,&desty,true,true);
			if (! crea->path) {
				crea->path=new TCODPath(gameEngine->dungeon->width,gameEngine->dungeon->height,walkPattern,gameEngine);
			}
			crea->path->compute((int)crea->x,(int)crea->y,destx,desty);
			pathTimer=0.0f;
		} else crea->walk(elapsed);
	} else {
		crea->walk(elapsed);
	}
	return true;
}

bool FollowBehavior::update(Creature *crea, float elapsed) {
	int pdist=(int)crea->distance(*leader);
	Dungeon *dungeon=gameEngine->dungeon;
	standDelay+=elapsed;
	if ( ( pdist > FOLLOW_DIST || standDelay > 10.0f ) && (! crea->path || crea->path->isEmpty()) ) {
			// go near the leader
			int destx = (int)(leader->x + TCODRandom::getInstance()->getInt(-FOLLOW_DIST,FOLLOW_DIST));
			int desty = (int)(leader->y + TCODRandom::getInstance()->getInt(-FOLLOW_DIST,FOLLOW_DIST));
			destx=CLAMP(0,dungeon->width-1,destx);
			desty=CLAMP(0,dungeon->height-1,desty);
			dungeon->getClosestWalkable(&destx,&desty,true,true,false);
			if (! crea->path) {
				crea->path=new TCODPath(dungeon->width,dungeon->height,walkPattern,NULL);
			}
			crea->path->compute((int)crea->x,(int)crea->y,destx,desty);
			crea->pathTimer=0.0f;
	} else {
		if (crea->walk(elapsed)) {
			standDelay=0.0f;
		}
	}		
	return true;
}

#define SCARE_RANGE 10.0f
// range below which fishes try to get away from each other
#define CLOSE_RANGE 2.0f
// range below which fishes try to get closer from each other
#define FAR_RANGE 10.0f


void HerdBehavior::recomputeHerds() {
}

void HerdBehavior::updateScarePoints(float elapsed) {
	for (ScarePoint **spit=scare.begin(); spit != scare.end(); spit++) {
		(*spit)->life -= elapsed;
		if ((*spit)->life <= 0.0f) {
			delete *spit;
			spit=scare.remove(spit);
		}
	}
}

bool HerdBehavior::update(Creature *crea1, float elapsed) {
	TCODList<Creature *> *herd=&Creature::creatureByType[crea1->type];
//	printf ("=> %d\n",crea1);
	for (Creature **f2=herd->begin(); f2 != herd->end(); f2++) {
		Creature *crea2=*f2;
		if ( crea1 != crea2 ) {
			float dx=crea2->x-crea1->x;
			if ( fabs(dx)>= FAR_RANGE ) continue;
			float dy=crea2->y-crea1->y;
			if ( fabs(dy) >= FAR_RANGE ) continue; // too far to interact
			float invDist=crea1->fastInvDistance(*crea2);
//	printf ("==> %d\n",crea2);
			if (invDist > 1E4f) {
			} else if ( invDist > 1.0f/CLOSE_RANGE ) {
				// get away from other creature
				crea1->dx -= elapsed*5.0f * dx * invDist;
				crea1->dy -= elapsed*5.0f * dy * invDist;
			} else if ( invDist > 1.0f/FAR_RANGE ) {
				// get closer to other creature
				crea1->dx += elapsed*1.2f * dx * invDist;
				crea1->dy += elapsed*1.2f * dy * invDist;
			}
		}
	}
	
	float speed=crea1->getType()->getSpeed();
	crea1->dx=CLAMP(-speed,speed,crea1->dx);
	crea1->dy=CLAMP(-speed,speed,crea1->dy);
	// interaction with scare points
	for (ScarePoint **spit=scare.begin(); spit != scare.end(); spit++) {
		float dx = (*spit)->x - crea1->x;
		float dy = (*spit)->y - crea1->y;
		float dist=Entity::fastInvSqrt(dx*dx+dy*dy);
		if ( dist < 1E4f && dist > 1.0f/SCARE_RANGE ) {
			float coef=(SCARE_RANGE-1.0f/dist)*SCARE_RANGE;
			crea1->dx -= elapsed*speed*10*coef*dx*dist;
			crea1->dy -= elapsed*speed*10*coef*dy*dist;
		}
	}
	crea1->dx=CLAMP(-speed*2,speed*2,crea1->dx);
	crea1->dy=CLAMP(-speed*2,speed*2,crea1->dy);

	float newx=crea1->x+crea1->dx;
	float newy=crea1->y+crea1->dy;
	Dungeon *dungeon=gameEngine->dungeon;
	newx=CLAMP(0.0, dungeon->width-1,newx);
	newy=CLAMP(0.0, dungeon->height-1,newy);
	crea1->walkTimer+=elapsed;
	if ((int)crea1->x != (int)newx
		|| (int)crea1->y != (int)newy ) {
		if (dungeon->isCellWalkable(newx,newy)) {
			TerrainId terrainId=gameEngine->dungeon->getTerrainType((int)newx,(int)newy);
			float walkTime = terrainTypes[terrainId].walkCost / speed;
			if ( crea1->walkTimer >= walkTime ) {
				crea1->walkTimer=0;
				dungeon->moveCreature(crea1,(int)crea1->x,(int)crea1->y,(int)newx,(int)newy);
				crea1->x = newx;
				crea1->y = newy;
			}
		}
	}
	return true;	
}

void HerdBehavior::addScarePoint(int x, int y, float life) {
	scare.push(new ScarePoint(x,y,life));
}

bool Behavior::update(Creature *crea, float elapsed) {
	GameEngine *game=gameEngine;
	if ( !crea->hasBeenSeen() && game->dungeon->map->isInFov((int)crea->x,(int)crea->y) && game->dungeon->getMemory(crea->x,crea->y)) {
		float dist=crea->squaredDistance(*game->player);
		if (dist < 1.0f || game->player->stealth >= 1.0f - 1.0f/dist) {
			// creature is seen by player
			crea->setSeen(true);
		}
	}
	return true;
}

AttackOnSee::AttackOnSee(float pathDelay) {
	this->pathDelay=pathDelay;
	pathTimer=TCODRandom::getInstance()->getFloat(0.0f,pathDelay);
};

bool AttackOnSee::update(Creature *crea, float elapsed) {
	GameEngine *game=gameEngine;
	Behavior::update(crea,elapsed);
	pathTimer+=elapsed;
	if ( crea->isBurning() || ! crea->hasBeenSeen() ) {
		crea->randomWalk(elapsed);
	} else {
		// track player
		if (! crea->path) {
			crea->path=new TCODPath(game->dungeon->width,game->dungeon->height,crea,game);
		} 
		if ( pathTimer > pathDelay ) {
			int dx,dy;
			crea->path->getDestination(&dx,&dy);
			if (dx!=game->player->x || dy != game->player->y ) {
				// path is no longer valid (the player moved)
					crea->path->compute((int)crea->x,(int)crea->y,(int)game->player->x,(int)game->player->y);
					pathTimer=0.0f;
			}
		}
		crea->walk(elapsed);
	}
	float dx=ABS(game->player->x-crea->x);
	float dy=ABS(game->player->y-crea->y);
	if ( dx <= 1.0f && dy <= 1.0f ) {
		// at melee range. attack
		crea->attack(game->player,elapsed);
	}
	return true;
}


