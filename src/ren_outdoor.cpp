/*
* Copyright (c) 2010 Jice
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

void OutdoorRenderer::renderSubcells() {
	// draw subcell ground
	int squaredFov=(int)(gameEngine->player->fovRange*gameEngine->player->fovRange*4);
	int minx,maxx,miny,maxy;
	Dungeon *dungeon=gameEngine->dungeon;
	gameEngine->ground->clear(TCODColor::black);
	Rect r1(gameEngine->xOffset*2,gameEngine->yOffset*2,CON_W*2,CON_H*2);
	Rect r2(0,0,dungeon->width*2,dungeon->height*2);
	r1.intersect(r2);
	minx = (int)(r1.x - gameEngine->xOffset*2);
	maxx = (int)(r1.x + r1.w - gameEngine->xOffset*2);
	miny = (int)(r1.y - gameEngine->yOffset*2);
	maxy = (int)(r1.y + r1.h - gameEngine->yOffset*2);
	float fovRatio=1.0f/(gameEngine->aspectRatio*gameEngine->aspectRatio);
	float curDate=gameEngine->getCurrentDate();
	for (int x=minx; x < maxx; x++) {
		for (int y=miny; y < maxy; y++) {
			int dungeon2x=x+gameEngine->xOffset*2;
			int dungeon2y=y+gameEngine->yOffset*2;
			TCODColor col=dungeon->getGroundColor(dungeon2x,dungeon2y);

			gameEngine->ground->putPixel(x,y,col);
			float intensity = dungeon->getShadow(dungeon2x,dungeon2y);
			float cloudIntensity = dungeon->getInterpolatedCloudCoef(dungeon2x,dungeon2y);
			intensity=std::min(intensity,cloudIntensity);
			TCODColor lightCol=dungeon->getAmbient() ;
			if ( intensity < 1.0f ) {
				lightCol = lightCol * intensity;
			}
			gameEngine->lightMap->setColor2x(x,y,lightCol);
			if ( ( dungeon2x & 1 ) == 0 && ( dungeon2y & 1 ) == 0 ) {
				Cell *cell=dungeon->getCell(dungeon2x/2,dungeon2y/2);
				float delay=terrainTypes[cell->terrain].trampleDelay;
				if ( delay > 0.0f ) {
					float trampleCoef= ( curDate - cell->trampleDate ) / delay;
					if (  trampleCoef < 1.0f ) {
						col = TCODColor::lerp(col*terrainTypes[cell->terrain].trampleColorCoef, col, trampleCoef);
						gameEngine->ground->putPixel(x,y,col);
					}
				}
			}
		}
	}
	// render the subcell creatures
	dungeon->renderSubcellCreatures(gameEngine->lightMap);
	// draw ripples
	if (gameEngine->rippleManager) gameEngine->rippleManager->renderRipples(gameEngine->ground);

	// render the lights
	dungeon->renderLightsToLightMap(gameEngine->lightMap);

	// render the fireballs
	for (Particle **it=gameEngine->particles.begin();it!=gameEngine->particles.end(); it++) {
		(*it)->render(gameEngine->lightMap);
	}
	// render the items
	dungeon->renderItems(gameEngine->lightMap,true);

	// apply light map
	gameEngine->lightMap->applyToImageOutdoor(gameEngine->ground);

	// render canopy
	Building *playerBuilding = dungeon->getCell(gameEngine->player->x,gameEngine->player->y)->building;
	for (int x=minx; x < maxx; x++) {
		for (int y=miny; y < maxy; y++) {
			int dungeon2x=x+gameEngine->xOffset*2;
			int dungeon2y=y+gameEngine->yOffset*2;
			TCODColor col;
			int dx=(int)(dungeon2x-gameEngine->player->x*2);
			int dy=(int)(dungeon2y-gameEngine->player->y*2);
			if ( (( (!playerBuilding || dungeon->getCell(dungeon2x/2,dungeon2y/2)->building != playerBuilding)
					&& dx*dx+dy*dy*fovRatio > squaredFov )
					|| ! dungeon->map2x->isInFov(dungeon2x,dungeon2y)) ) {
				col=dungeon->canopy->getPixel(dungeon2x,dungeon2y);
				if (col.r != 0) {
					col = col * dungeon->getInterpolatedCloudCoef(dungeon2x,dungeon2y);
					col = col * dungeon->getAmbient();
					gameEngine->ground->putPixel(x,y,col);
				}
			}
		}
	}

	if (gameEngine->fireManager) gameEngine->fireManager->renderFire(gameEngine->ground);

}

void OutdoorRenderer::renderCells() {
	Dungeon *dungeon=gameEngine->dungeon;
	// render the corpses
	dungeon->renderCorpses(gameEngine->lightMap);
	// render the items
	dungeon->renderItems(gameEngine->lightMap,false);
	// render the creatures
	dungeon->renderCreatures(gameEngine->lightMap);
	// render the player
	gameEngine->player->render(gameEngine->lightMap);

	gameEngine->gui.descriptor.render();

	if ( gameEngine->isGamePaused() ) {
		TCODConsole::root->setDefaultForeground(TCODColor::lightestGrey);
		TCODConsole::root->print(CON_W-10,CON_H-1,"= pause =");
	}

}
