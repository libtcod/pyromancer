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

void IndoorRenderer::renderSubcells() {
	static TCODColor memoryWallColor=config.getColorProperty("config.display.memoryWallColor");


	gameEngine->ground->clear(TCODColor::black);
	Dungeon *dungeon=gameEngine->dungeon;
	// variables to store the part of the dungeon than needs
	// subcell rendering from lightmap (part which gets light)
	int minx2x=dungeon->width*2,maxx2x=0,miny2x=dungeon->height*2,maxy2x=0;
	// render static lights
	gameEngine->lightMap->clear(TCODColor::black);
	for (int x=0; x < 2*CON_W; x++) {
		int dungeonx=x+gameEngine->xOffset*2;
		if ( dungeonx >= 0 && dungeonx < dungeon->width*2) {
			for (int y=0; y < 2*CON_H; y++) {
				int dungeony=y+gameEngine->yOffset*2;
				if ( dungeony >= 0 && dungeony < dungeon->height*2 && dungeon->map2x->isInFov(dungeonx,dungeony)) {
					SubCell *sc = dungeon->getSubCell(dungeonx,dungeony);
					if ( sc->r > 0 || sc->g > 0 || sc->b > 0 ) {
						if ( minx2x > x ) minx2x=x;
						if ( miny2x > y ) miny2x=y;
						if ( maxx2x < x ) maxx2x=x;
						if ( maxy2x < y ) maxy2x=y;
						gameEngine->lightMap->setColor2x(x,y,HDRColor(sc->r,sc->g,sc->b));
					}
				}
			}
		}
	}
	minx2x+=2*gameEngine->xOffset;
	maxx2x+=2*gameEngine->xOffset;
	miny2x+=2*gameEngine->yOffset;
	maxy2x+=2*gameEngine->yOffset;

	// render the memory map
	for (int x=0; x < CON_W; x++) {
		int dungeonx=x+gameEngine->xOffset;
		if ( dungeonx >= 0 && dungeonx < dungeon->width) {
			for (int y=0; y < CON_H; y++) {
				int dungeony=y+gameEngine->yOffset;
				if ( dungeony >= 0 && dungeony < dungeon->height) {
					if ( dungeon->getMemory(dungeonx,dungeony) ) {
						int dungeonx2x=dungeonx*2;
						int dungeony2x=dungeony*2;
						if (!gameEngine->dungeon->map2x->isTransparent(dungeonx2x,dungeony2x))
							gameEngine->ground->putPixel(x*2,y*2,memoryWallColor );
						if (!dungeon->map2x->isTransparent(dungeonx2x+1,dungeony2x))
							gameEngine->ground->putPixel(x*2+1,y*2,memoryWallColor );
						if (!dungeon->map2x->isTransparent(dungeonx2x,dungeony2x+1))
							gameEngine->ground->putPixel(x*2,y*2+1,memoryWallColor );
						if (!dungeon->map2x->isTransparent(dungeonx2x+1,dungeony2x+1))
							gameEngine->ground->putPixel(x*2+1,y*2+1,memoryWallColor );
					}
				}
			}
		}
	}

	// render the lights on 2x lightmap
	dungeon->renderLightsToLightMap(gameEngine->lightMap,&minx2x, &miny2x, &maxx2x, &maxy2x,false);

	// render the fireballs
	for (Particle **it=gameEngine->particles.begin();it!=gameEngine->particles.end(); it++) {
		(*it)->render(gameEngine->lightMap);
	}
	// render the subcell items
	dungeon->renderItems(gameEngine->lightMap,true);

	// convert it to lightmap (consolex2) coordinates
	minx2x-=2*gameEngine->xOffset;
	maxx2x-=2*gameEngine->xOffset;
	miny2x-=2*gameEngine->yOffset;
	maxy2x-=2*gameEngine->yOffset;
	minx2x=std::max(0,minx2x);
	maxx2x=std::min(CON_W*2-1,maxx2x);
	miny2x=std::max(0,miny2x);
	maxy2x=std::min(CON_H*2-1,maxy2x);

	// shade the 2xground
	gameEngine->lightMap->applyToImage(gameEngine->ground,minx2x,miny2x,maxx2x,maxy2x);
	//gameEngine->lightMap->applyToImage(gameEngine->ground,0,0,CON_W*2,CON_H*2);


}

void IndoorRenderer::renderCells() {
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
