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

// John Carmack's magic fast inverse sqrt
float Entity::fastInvSqrt(float x){
    float xhalf = 0.5f*x;
    int i = *(int*)&x;
    i = 0x5f3759df - (i>>1);
    x = *(float*)&i;
    x = x*(1.5f - xhalf*x*x);
    return x;
}

float Entity::distance(const Entity &p) const {
	return sqrtf((float)squaredDistance(p));
}

bool Entity::isOnScreen() const {
	return IN_RECTANGLE(x-gameEngine->xOffset,y-gameEngine->yOffset,CON_W,CON_H);
}

DynamicEntity::CollisionType DynamicEntity::updateMove(float elapsed, float bounceCoef, bool hitPlayer,
	bool hitCreatures, bool hitItems) {
	Dungeon *dungeon=gameEngine->dungeon;
	float oldx=x;
	float oldy=y;
	x += speed*dx*elapsed;
	y += speed*dy*elapsed;
	if ( ! IN_RECTANGLE(x,y,dungeon->width,dungeon->height) ) {
		x=oldx;y=oldy;
		return WALL;
	}
	int curoldx=(int)oldx;
	int curoldy=(int)oldy;
	TCODLine::init(curoldx,curoldy,(int)x,(int)y);
	while ( ! TCODLine::step(&curoldx,&curoldy) ) {
		if (! dungeon->isCellWalkable(curoldx,curoldy)) {
			// bounce against a wall
			float newx=curoldx;
			float newy=curoldy;
			int cdx=(int)(curoldx-oldx);
			int cdy=(int)(curoldy-oldy);
			curoldx=(int)oldx;curoldy=(int)oldy;
			speed *= bounceCoef;
			x=oldx;y=oldy;
			if ( bounceCoef == 0.0f ) {
				return WALL;
			}
			if ( cdx == 0 ) {
				// hit horizontal wall
				dy=-dy;
			} else if (cdy == 0 ) {
				// hit vertical wall
				dx=-dx;
			} else {
				bool xwalk=dungeon->isCellWalkable(newx,oldy);
				bool ywalk=dungeon->isCellWalkable(oldx,newy);
				if ( xwalk && ywalk ) {
					// outer corner bounce. detect which side of the cell is hit
					//  ##
					//  ##
					// .
					float fdx=std::abs(dx);
					float fdy=std::abs(dy);
					if ( fdx >= fdy ) dy=-dy;
					if ( fdy >= fdx ) dx=-dx;
				} else if (! xwalk ) {
					if ( ywalk ) {
						// vertical wall bounce
						dx=-dx;
					} else {
						// inner corner bounce
						// ##
						// .#
						dx=-dx;dy=-dy;
					}
				} else {
					// horizontal wall bounce
					dy=-dy;
				}
			}
		} else {
			// on a walkable cell
			if ( hitPlayer && std::abs(curoldx - (int)gameEngine->player->x) < 2
					&& std::abs(curoldy - (int)gameEngine->player->y) < 2 ) {
				x=oldx;y=oldy;
				speed=0;
				return PLAYER;
			} else if ( hitCreatures && (
					dungeon->hasCreature(curoldx,curoldy)
					|| dungeon->hasCreature(curoldx+1,curoldy)
					|| dungeon->hasCreature(curoldx-1,curoldy)
					|| dungeon->hasCreature(curoldx,curoldy+1)
					|| dungeon->hasCreature(curoldx,curoldy-1))
				) {
				x=curoldx;y=curoldy;
				speed=0;
				return CREATURE;
			}
			if ( hitItems ) {
				// item hit ?
				TCODList<Item *> *items=dungeon->getItems(curoldx,curoldy);
				if (items && items->size()>0) {
					for (Item **it=items->begin();it!=items->end();it++) {
						if (*it != this) {
							x=oldx;y=oldy;
							speed=0;
							return ITEM;
						}
					}
				}
			}
		}
		oldx=curoldx;
		oldy=curoldy;
	}
	if ( duration > 0.0f ) {
		duration -= elapsed;
		if ( duration <= 0.0f ) {
			speed=0.0f;
			return END_DURATION;
		}
	}
	return NONE;
}
