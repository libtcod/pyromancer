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

#define PANEL_HEIGHT (CON_H-15)

StatusPanel::StatusPanel() {
	con = new TCODConsole(12,PANEL_HEIGHT);
	con->setDefaultBackground(guiBackground);
	con->setDefaultForeground(guiText);
	con->clear();
	flags=DIALOG_MULTIPOS|DIALOG_DRAGGABLE;
	possiblePos.push(new UmbraRect(CON_W-12,0,12,PANEL_HEIGHT));
	possiblePos.push(new UmbraRect(CON_W-12,0,12,PANEL_HEIGHT));
	possiblePos.push(new UmbraRect(CON_W-12,0,12,PANEL_HEIGHT));
	
	possiblePos.push(new UmbraRect(CON_W-12,CON_H-PANEL_HEIGHT,12,PANEL_HEIGHT));
	possiblePos.push(new UmbraRect(CON_W-12,CON_H-PANEL_HEIGHT,12,PANEL_HEIGHT));
	possiblePos.push(new UmbraRect(CON_W-12,CON_H-PANEL_HEIGHT,12,PANEL_HEIGHT));
	
	possiblePos.push(new UmbraRect(0,0,12,PANEL_HEIGHT));
	possiblePos.push(new UmbraRect(0,0,12,PANEL_HEIGHT));
	possiblePos.push(new UmbraRect(0,0,12,PANEL_HEIGHT));
	
	possiblePos.push(new UmbraRect(0,CON_H-PANEL_HEIGHT,12,PANEL_HEIGHT));
	possiblePos.push(new UmbraRect(0,CON_H-PANEL_HEIGHT,12,PANEL_HEIGHT));
	possiblePos.push(new UmbraRect(0,CON_H-PANEL_HEIGHT,12,PANEL_HEIGHT));
	minimizedRect = *possiblePos.get(0);
	minimizedRect.x=userPref.statusx;
	minimizedRect.y=userPref.statusy;
	setMinimized();
	titleBarAlpha=0.0f;
}

void StatusPanel::drawBar(int x, int y, float lvl, const TCODColor &col1, const TCODColor &col2) {
	drawBar(x,y,lvl,0.0f,col1,TCODColor::black,col2);
}
void StatusPanel::drawBar(int cx, int cy, 
	float lvl1, float lvl2, 
	const TCODColor &col1, const TCODColor &col2, const TCODColor &col3) {
	static TCODImage img(20,4);
	int ilvl1 = (int)(20*lvl1);
	float coef1=lvl1*20-ilvl1;
	for (int x=0; x < ilvl1; x++ ) {
		img.putPixel(x,0,col1);
		img.putPixel(x,1,col1);
	}
	if ( lvl2 > lvl1 ) {
		TCODColor lcol=TCODColor::lerp(col2,col1,coef1);
		img.putPixel(ilvl1,0,lcol);
		img.putPixel(ilvl1,1,lcol);
		int ilvl2 = (int)(20*lvl2);
		float coef2=20*lvl2-ilvl2;
		for (int x =ilvl1+1; x < ilvl2; x++) {
			img.putPixel(x,0,col2);
			img.putPixel(x,1,col2);
		}
		ilvl1=ilvl2;
		if (ilvl1 < 20) {
			lcol=TCODColor::lerp(col3,col2,coef2);
			img.putPixel(ilvl1,0,lcol);
			img.putPixel(ilvl1,1,lcol);
		}
	} else if ( ilvl1 < 20 ) {
		TCODColor lcol=TCODColor::lerp(col3,col1,coef1);
		img.putPixel(ilvl1,0,lcol);
		img.putPixel(ilvl1,1,lcol);
	}
	for (int x =ilvl1+1; x < 20; x++) {
		img.putPixel(x,0,col3);
		img.putPixel(x,1,col3);
	}
	img.blit2x(TCODConsole::root, cx, cy, 0, 0, 20, 2);
}
void StatusPanel::render() {
	Player *player=gameEngine->player;
	int y,dy;
	if ( rect.y == 0) {
		y=1;dy=1;
	} else {
		y=rect.h-2;dy=-1;
	}

	bool skills=false;
	for (int i=0;i< 10; i++) {
		if ( player->quickSlots[i]) {
			skills=true;
			break;
		}
	}
	if ( titleBarAlpha > 0.0f ) {
		con->clear();
		con->printEx(rect.w/2,y,TCOD_BKGND_NONE, TCOD_CENTER,"HP %d/%d",(int)player->getLife(),(int)player->getType()->getLife());
		con->printEx(rect.w/2,y+2*dy,TCOD_BKGND_NONE, TCOD_CENTER,"MANA %d/%d",(int)player->getMana(),(int)player->getType()->getMana());
		con->setDefaultBackground(guiText);
		con->setDefaultForeground(guiBackground);		
		if ( skills ) con->printEx(rect.w/2,rect.y+PANEL_HEIGHT-12*dy,TCOD_BKGND_SET, TCOD_CENTER,"   Actions  ");
		if (!player->conditions.isEmpty()) {
			con->printEx(rect.w/2,y+4*dy,TCOD_BKGND_SET, TCOD_CENTER," Conditions ");
		}
		con->setDefaultBackground(guiBackground);
		con->setDefaultForeground(guiText);
		blitSemiTransparent(con,0,0,rect.w,rect.h,TCODConsole::root,rect.x,rect.y,titleBarAlpha,titleBarAlpha);
	}
	if ( titleBarAlpha > 0.0f ) {
		renderFrame(titleBarAlpha,"Status");
	}

	drawBar(rect.x+1,rect.y+y+dy,player->getLifeRatio(),TCODColor::red,TCODColor::darkerGrey);
	drawBar(rect.x+1,rect.y+y+3*dy,player->getManaRatio(),TCODColor::blue,TCODColor::darkerGrey);
	
	y += 5*dy;
	TCODList<Condition *> conds;
	// extract conditions from the player.
	// every condition is displayed only once (the longer)
	for (Condition **it=player->conditions.begin(); it != player->conditions.end(); it++) {
		Condition **it2=NULL;
		for( it2=conds.begin(); it2!=conds.end(); it2++) {
			if ( (*it)->equals((*it2)->type->type, (*it2)->alias) ) {
				break;
			}
		}
		if (it2 == conds.end()) {
			// new condition
			conds.push(*it);
		} else {
			// replace only if longer
			if ( (*it)->duration > (*it2)->duration ) *it2 = *it;
		}
	}
	for (Condition **it=conds.begin(); it != conds.end(); it++) {
		if (! (*it)->alias || strcmp((*it)->alias,"") != 0 ) {
			float coef=(*it)->duration / (*it)->initialDuration;
			drawBar(rect.x+1,rect.y+y,coef,TCODColor::azure,TCODColor::black);
			TCODConsole::root->setDefaultForeground(guiText);
			TCODConsole::root->printEx(rect.x+rect.w/2,rect.y+y,TCOD_BKGND_NONE, TCOD_CENTER,(*it)->alias ? (*it)->alias : (*it)->type->name);
			y+=dy;
		}
	}
	// skills
	if ( skills ) {
		y=PANEL_HEIGHT-11*dy;
		for (int i=0; i < 10; i++) {
			Skill *skill=player->quickSlots[i];
			if ( skill ){
				float coef=0.0f;
				TCODColor col1,col2;
				if ( skill->castTime > 0.0f ) {
					// casting phase
					coef = (skill->type->castTime-skill->castTime) / skill->type->castTime;
					col1=TCODColor::darkGreen;
					col2=TCODColor::black;
				} else if (skill->castTime < 0.0f) {
					// reloading phase
					coef = (skill->type->reloadTime+skill->castTime) / skill->type->reloadTime;
					col1=TCODColor::black;  
					col2=TCODColor::darkRed;
				} 			
				if ( coef != 0.0f ) drawBar(rect.x+1,rect.y+y,coef,col1,col2);
				TCODConsole::root->setDefaultForeground(guiText);
				TCODConsole::root->printEx(rect.x+1,rect.y+y,TCOD_BKGND_NONE, TCOD_LEFT, "%d %s",i==10 ? 0:i+1,skill->getName());
				y+=dy;
			}
		}
	}
}

bool StatusPanel::update(float elapsed, TCOD_key_t &k, TCOD_mouse_t &mouse) {
	static bool lookOn=false;
	if ( k.vk == TCODK_ALT || k.lalt ) lookOn=k.pressed;
	if ( !gameEngine->isGamePaused() && rect.mouseHover && ! lookOn) {
		titleBarAlpha+=elapsed;
		titleBarAlpha=MIN(1.0f,titleBarAlpha);
	} else if ( !isDragging ) {
		titleBarAlpha-=elapsed;
		titleBarAlpha=MAX(0.0f,titleBarAlpha);
	}	
	return true;
}

void StatusPanel::setPos(int x,int y) {
	MultiPosDialog::setPos(x,y);
	userPref.statusx=x;
	userPref.statusy=y;
}
