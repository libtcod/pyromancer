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

#include <stdio.h>
#include "main.hpp"

#define POSX CON_W-1
#define POSY CON_H-10

Descriptor::Descriptor() : item(NULL),creature(NULL) {tooltip[0]=0;}

void Descriptor::getTerrainDescriptor(int x, int y) {
	Dungeon *dungeon=gameEngine->dungeon;
	TerrainId id=dungeon->getTerrainType(x,y);
	Cell *cell=dungeon->getCell(x,y);
	if ( terrainTypes[id].trampleDelay > 0.0f ) {
		float coef=gameEngine->getCurrentDate()-cell->trampleDate / terrainTypes[id].trampleDelay;
		if ( coef  < 0.8f ) {
			sprintf(tooltip,"trampled %s",terrainTypes[id].name);
			return;
		} else if ( coef < 1.0f ) {
			sprintf(tooltip,"lightly trampled %s",terrainTypes[id].name);
			return;
		}
	}
	strcpy(tooltip,terrainTypes[id].name);

}


void Descriptor::setFocus(int mousex, int mousey, int x, int y, bool lookOn) {
	this->x=x;this->y=y;
	this->mousex=mousex;this->mousey=mousey;
	Dungeon *dungeon=gameEngine->dungeon;
	Player *player=gameEngine->player;
	tooltip[0]=0;
	item=NULL;
	creature=NULL;
	this->lookOn=lookOn;
	if ( IN_RECTANGLE(x,y,dungeon->width,dungeon->height)
		&& player->isInRange(x,y) && dungeon->map->isInFov(x,y)) {
		if ( x == player->x && y == player->y ) creature=gameEngine->player;
		else creature=dungeon->getCreature(x,y);
		if (! creature ) item = dungeon->getFirstItem(x,y);
		if (lookOn && !creature  && ! item) {
			getTerrainDescriptor(x,y);
		}
	} else if ( lookOn ) {
		getTerrainDescriptor(x,y);
	}
}

void Descriptor::render() {
	TCODConsole::root->setDefaultForeground(guiText);
	// descriptor
	if ( creature ) {
		TCODConsole::root->printEx(POSX,POSY,TCOD_BKGND_NONE,TCOD_RIGHT,creature->getName());
	} else if ( item ) {
		TCODConsole::root->printEx(POSX,POSY,TCOD_BKGND_NONE,TCOD_RIGHT,item->aName());
		if( lookOn ) item->renderDescription(mousex,mousey-1,false);
	}
	// tooltip
	if (tooltip[0] != 0 ) {
		int my=mousey-1;
		if ( my < 0 ) my = 2;
		if ( IN_RECTANGLE(mousex,my,CON_W,CON_H)) {
			TCODConsole::root->printEx(mousex,my,TCOD_BKGND_NONE,TCOD_CENTER,tooltip);
		}
	}
}
