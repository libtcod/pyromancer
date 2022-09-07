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

Particle::Particle(const ParticleType *type) : type(type) {
	speed=type->speed;
	duration=type->duration;
}

void Particle::render(LightMap *lightMap) {
	int lmx=(int)(x-gameEngine->xOffset)*2;
	int lmy=(int)(y-gameEngine->yOffset)*2;
	if ( IN_RECTANGLE(lmx,lmy,lightMap->width,lightMap->height)) {
		if ( gameEngine->dungeon->map2x->isInFov((int)(x*2),(int)(y*2))) {
			HDRColor lcol=lightMap->getColor2x(lmx,lmy);
			lcol=lcol+HDRColor::lerp(type->endColor,type->startColor,(duration/type->duration));
			lightMap->setColor2x(lmx,lmy,lcol);
		}
	}
}

void Particle::render(TCODImage *ground) {
	int lmx=(int)(x-gameEngine->xOffset)*2;
	int lmy=(int)(y-gameEngine->yOffset)*2;
	TCODColor lcol=ground->getPixel(lmx,lmy);
	lcol=lcol+HDRColor::lerp(type->startColor,type->endColor,(duration/type->duration));
	ground->putPixel(lmx,lmy,lcol);
}

bool Particle::update(float elapsed) {
	Dungeon *dungeon=gameEngine->dungeon;
	float foldx=x;
	float foldy=y;
	int oldx=(int)(x*2);
	int oldy=(int)(y*2);
	x += speed*dx*elapsed;
	y += speed*dy*elapsed;
	int newx=(int)(x*2);
	int newy=(int)(y*2);

	if (!IN_RECTANGLE(newx,newy,dungeon->width*2,dungeon->height*2) ) return false;
	duration -= elapsed;
	if ( newx==oldx && newy==oldy ) return duration > 0.0f;
	if ( dungeon->hasCreature((int)x,(int)y)) {
		// hit a creature
		Creature *cr=dungeon->getCreature((int)x,(int)y);
		cr->takeDamage(TCODRandom::getInstance()->getFloat(type->minDamage,type->maxDamage));
		if (! type->throughCreatures ) return false;
	}
	if (! dungeon->map2x->isWalkable(newx,newy)) {
		// hit a wall
		if (! type->bounce ) return false;
		int cdx=newx-oldx;
		int cdy=newy-oldy;
		if ( cdx == 0 ) {
			// hit horizontal wall
			dy=-dy;
		} else if (cdy == 0 ) {
			// hit vertical wall
			dx=-dx;
		} else {
			bool xwalk=dungeon->map2x->isWalkable(oldx+cdx,oldy);
			bool ywalk=dungeon->map2x->isWalkable(oldx,oldy+cdy);
			if ( xwalk && ywalk ) {
				// outer corner bounce. detect which side of the cell is hit
				// TODO : this does not work
				float fdx=newx+0.5f - foldx;
				float fdy=newy+0.5f - foldy;
				fdx=ABS(fdx);
				fdy=ABS(fdy);
				if ( fdx >= fdy ) dx=-dx;
				if ( fdy >= fdx ) dy=-dy;
			} else if (! xwalk ) {
				if ( ywalk ) {
					// vertical wall bounce
					dx=-dx;
				} else {
					// inner corner bounce
					dx=-dx;dy=-dy;
				}
			} else {
				// horizontal wall bounce
				dy=-dy;
			}
		}
	}
	return duration > 0.0f;
}
