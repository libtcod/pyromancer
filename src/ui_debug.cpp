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

const char *MapDebugger::mapNames[NB_DEBUGMAPS] = {
	"lightmap","heightmap","shadowheight","shadowmap","fov",
	"normalmap","clouds", "waterCoef","firezones"
};

MapDebugger::MapDebugger() {
	img=NULL;
}

MapDebugger::~MapDebugger() {
	if ( img ) delete img;
}

void MapDebugger::onInitialise() {
	mapNum=(EDebugMapType)0;
	img=new TCODImage(CON_W*2,CON_H*2);
}

void MapDebugger::onActivate() {
	toOpen=true;
	toExit=false;
}

void MapDebugger::render() {
	Dungeon *dungeon=gameEngine->dungeon;
	for (int x=0; x < CON_W*2; x++) {
		for (int y=0; y < CON_H*2; y++) {
			int dungeon2x=x+gameEngine->xOffset*2;
			int dungeon2y=y+gameEngine->yOffset*2;
			TCODColor col;
			if ( IN_RECTANGLE(dungeon2x,dungeon2y,dungeon->width*2,dungeon->height*2)) {
				// debug maps
				switch (mapNum) {
					case DBG_LIGHTMAP : {
						col = gameEngine->lightMap->getColor2x(x,y);
						} break;
					case DBG_HEIGHTMAP : {
						float h = dungeon->hmap->getValue(dungeon2x,dungeon2y);
						col = h*TCODColor::white;
						} break;
					case DBG_SHADOWMAP : {
						float h = dungeon->getShadow(dungeon2x,dungeon2y);
						col = h*TCODColor::white;
						} break;
					case DBG_SHADOWHEIGHT : {
						float h = dungeon->getShadowHeight(dungeon2x,dungeon2y);
						col = h*TCODColor::white;
						} break;
					case DBG_FOV : {
						col = dungeon->map2x->isInFov(dungeon2x,dungeon2y) ? TCODColor::lightGrey : TCODColor::darkGrey;
						} break;
					case DBG_NORMALMAP : {
						float n[3];
						dungeon->hmap->getNormal(dungeon2x,dungeon2y,n);
						col=TCODColor((int)(128+n[0]*128),(int)(128+n[1]*128),(int)(128+n[2]*128));
						} break;
					case DBG_CLOUDS : {
						float h = dungeon->getInterpolatedCloudCoef(dungeon2x, dungeon2y);
						h = (h-0.5f)/1.2f;
						col = h*TCODColor::white;
					} break;
					case DBG_WATERCOEF : {
						float h = dungeon->getWaterCoef(dungeon2x, dungeon2y);
						col = h*TCODColor::white;
					} break;
					case DBG_FIREZONES : {
						if (gameEngine->fireManager ) {
							col= gameEngine->fireManager->isBurning(dungeon2x/2,dungeon2y/2)
								? TCODColor::white:TCODColor::black;
						}
					} break;
					default : break;
				}
			}
			img->putPixel(x,y,col);
		}
	}
	img->blit2x(TCODConsole::root,0,0);
	TCODConsole::root->setBackgroundFlag(TCOD_BKGND_MULTIPLY);
	TCODConsole::root->setDefaultForeground(guiText);
	TCODConsole::root->print(2,1,"Debug maps. Shit-Tab to exit");
	for (EDebugMapType map=(EDebugMapType)0; map != NB_DEBUGMAPS; map = (EDebugMapType)(map+1)) {
		if ( map == mapNum ) {
			TCODConsole::root->setDefaultBackground(guiHighlightedBackground);
			TCODConsole::root->setDefaultForeground(TCODColor::white);
		} else {
			TCODConsole::root->setDefaultBackground(guiBackground);
			TCODConsole::root->setDefaultForeground(guiText);
		}
		TCODConsole::root->print(2,3+map,mapNames[map]);
	}
	TCODConsole::root->setBackgroundFlag(TCOD_BKGND_NONE);
}

bool MapDebugger::update(float elapsed, TCOD_key_t &k, TCOD_mouse_t &mouse)  {
	if ( !k.pressed && k.vk == TCODK_DOWN ) {
		mapNum = (EDebugMapType)(mapNum+1);
		if ( mapNum == NB_DEBUGMAPS ) mapNum = (EDebugMapType)0;
	} else if ( ! k.pressed && k.vk == TCODK_UP ) {
		if ( mapNum == (EDebugMapType)0 ) mapNum = (EDebugMapType)(NB_DEBUGMAPS-1);
		else mapNum = (EDebugMapType)(mapNum-1);
	} else if ( k.vk == TCODK_TAB && ! k.pressed && k.shift ) {
		gameEngine->gui.setMode(GUI_NONE);
		k.vk=TCODK_NONE;
		return false;
	}
	return true;
}
