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
#include <ctype.h>
#include <assert.h>
#include "main.hpp"

// maximum sprint : 2 times faster
#define MIN_SPRINT_COEF 0.5f

static TCODColor healthColor(127,127,255);

Player::Player() : Creature("player"), lbuttonDelay(0.0f),rbuttonDelay(0.0f),lWalkDelay(0.0f),lbutton(false),
	rbutton(false),initDungeon(true) {
	static float sprintLength=config.getFloatProperty("config.creatures.player.sprintLength");

	noLight.color=TCODColor(30,30,30);
	noLight.range=5;
	noLight.drawMode=Light::MODE_MAX;
	noLight.setup(TCODColor(30,30,30),0.0f,NULL,NULL);

	fovRange=0;
	sprintDelay=sprintLength;
	stealth=1.0f;
	crouch=false;
	isSprinting=false;
	memset(quickSlots,0,sizeof(void *)*10);
	rmbSkill=NULL;
}

Player::~Player() {
}

void Player::addSkill(Skill *skill) {
	Creature::addSkill(skill);
	for (int i=0; i < 10; i++) {
		// affects the skill to the first available quickSlot
		if (! quickSlots[i]) {
			quickSlots[i]=skill;
			break;
		}
	}
}

void Player::init() {
	static float sprintLength=config.getFloatProperty("config.creatures.player.sprintLength");

	setLife(getMaxLife());
	setMaxMana(getType()->getMana());
	setMana(getType()->getMana());
	up=down=left=right=false;
	averageSpeed=speedElapsed=speedDist=0.0f;
	sprintDelay=sprintLength;
}

void Player::initLevel() {
	up=down=left=right=false;
}

void Player::termLevel() {
	if ( path ) delete path;
	path=NULL;
	walkTimer=0.0f;
	initDungeon=true;
}

void Player::takeDamage(float amount) {
	float oldLife=getLife();
	Creature::takeDamage(amount);
	if ( (int)getLife() < (int)oldLife ) gameEngine->hitFlash();
}

bool Player::setPath(int xDest,int yDest, bool limitPath) {
	static int maxPathFinding=config.getIntProperty("config.creatures.player.maxPathFinding");

	Dungeon *dungeon=gameEngine->dungeon;
	if ( !IN_RECTANGLE(xDest,yDest,dungeon->width,dungeon->height)) return false;
	// check if right clicked on a wall
	if ( !dungeon->map->isWalkable(xDest,yDest) ) {
		// walk toward the player and see if no other wall blocks the path
		float dx=x-xDest;
		float dy=y-yDest;
		if ( xDest < dungeon->width-1 && dx > 0 && dungeon->map->isWalkable(xDest+1,yDest) ) {
			xDest++;
		} else if ( xDest > 0 && dx < 0 && dungeon->map->isWalkable(xDest-1,yDest)) {
			xDest--;
		} else if ( yDest < dungeon->height-1 && dy > 0 && dungeon->map->isWalkable(xDest,yDest+1) ) {
			yDest++;
		} else if ( yDest > 0 && dy < 0 && dungeon->map->isWalkable(xDest,yDest-1)) {
			yDest--;
		}
		// to a known cell
		TCODLine::init(xDest,yDest,(int)x,(int)y);
		bool wall=true;
		int wx,wy;
		while (! TCODLine::step(&wx,&wy)) {
			if ( dungeon->getMemory(wx,wy) && ! wall ) break;
			if ( dungeon->map->isWalkable(wx,wy) ) {
				// found a ground cell
				if ( wall ) {
					wall=false;
					xDest=wx;yDest=wy;
				}
			} else if ( ! wall ) return false; // hit another wall. no path
		}
	}
	if (! path) path=new TCODPath(dungeon->width,dungeon->height,this,NULL);
	bool ok=path->compute((int)x,(int)y,xDest,yDest);
	if ( ! ok ) return false;
	if ( limitPath && ! dungeon->getMemory(xDest,yDest) && path->size() > maxPathFinding) {
		delete path;
		path=NULL;
		return false;
	}
	return true;
}

void Player::render(LightMap *lightMap) {
	//static float longButtonDelay=config.getFloatProperty("config.creatures.player.longButtonDelay");
	//static float longSpellDelay=config.getFloatProperty("config.creatures.player.longSpellDelay");
	static float sprintLength=config.getFloatProperty("config.creatures.player.sprintLength");
	//static bool blink=false;

	Creature::render(lightMap);
	/*
	if ( lbuttonDelay> longButtonDelay || rbuttonDelay>longButtonDelay ) {
		// spell charging progress bar
		int barLength=0;
		float delay=MAX(rbuttonDelay,lbuttonDelay);
		if ( delay >= longSpellDelay ) {
			barLength=3;
		} else barLength=1+(int)((delay-longButtonDelay)*1.99/(longSpellDelay-longButtonDelay));
		blink=!blink;
		if ( barLength == 3 && blink ) barLength = 0;
		int bary=CON_H/2-1;
		if ( gameEngine->mousey<=CON_H/2 ) bary=CON_H/2+1;
		for (int i=CON_W/2-1; i < CON_W/2+barLength-1; i++ ) {
			TCODConsole::root->putCharEx(i,bary,TCOD_CHAR_PROGRESSBAR,TCODColor::lightRed,TCODColor::darkRed);
		}
	}
	*/

	// sprint bar
	if ( isSprinting && ! hasCondition(ConditionType::CRIPPLED) ) {
		if ( sprintDelay < sprintLength ) {
			float sprintCoef = sprintDelay/sprintLength;
			static TCODImage sprintBar(10,2);
			for (int x=0; x < 10; x++ ) {
				float coef = ( x * 0.1f - sprintCoef ) * 5;
				coef=CLAMP(0.0f,1.0f,coef);
				TCODColor col=TCODColor::lerp(TCODColor::blue,TCODColor::white,coef);
				sprintBar.putPixel(x,0,col);
				sprintBar.putPixel(x,1,col);
			}
			transpBlit2x(&sprintBar,0,0,10,2, TCODConsole::root, CON_W/2-2, CON_H/2+2, 0.4f);
		}
	}

	// stealth bar
	if ( crouch || stealth < 1.0f ) {
		static TCODImage stealthBar(2,10);
		for (int y=0; y < 10; y++ ) {
			float coef=(y*0.1f - stealth )* 5;
			coef=CLAMP(0.0f,1.0f,coef);
			TCODColor col=TCODColor::lerp(TCODColor::white,TCODColor::darkViolet,coef);
			stealthBar.putPixel(0,y,col);
			stealthBar.putPixel(1,y,col);
			transpBlit2x(&stealthBar, 0, 0, 2, 10, TCODConsole::root, CON_W/2-3, CON_H/2-3, 0.4f);
		}

	}
}

// handle movement keys
// supported layouts:
// hjklyubn (vi keys)
// arrows
// numpad 12346789
// WSAD / ZSQD (fps keys)
bool Player::getMoveKey(TCOD_key_t key,bool *up, bool *down, bool *left, bool *right) {
	static int moveUpKey=toupper(config.getCharProperty("config.creatures.player.moveUpKey"));
	static int moveDownKey=toupper(config.getCharProperty("config.creatures.player.moveDownKey"));
	static int moveLeftKey=toupper(config.getCharProperty("config.creatures.player.moveLeftKey"));
	static int moveRightKey=toupper(config.getCharProperty("config.creatures.player.moveRightKey"));

	bool ret=false;
	int kc=toupper(key.c);
	if ( kc == moveUpKey || key.vk == TCODK_UP || kc == 'Z' || kc =='W' || kc == 'K' || key.vk == TCODK_KP8) {
		*up = key.pressed;
		if ( key.pressed ) ret=true;
	} else if ( kc == moveDownKey || key.vk == TCODK_DOWN || kc == 'S' || kc == 'J' || key.vk == TCODK_KP2) {
		*down = key.pressed;
		if ( key.pressed ) ret=true;
	} else if ( kc == moveLeftKey || key.vk == TCODK_LEFT || kc == 'Q' || kc == 'A' || kc == 'H' || key.vk == TCODK_KP4) {
		*left = key.pressed;
		if ( key.pressed ) ret=true;
	} else if ( kc == moveRightKey || key.vk == TCODK_RIGHT || kc == 'D' || kc == 'L' || key.vk == TCODK_KP6) {
		*right = key.pressed;
		if ( key.pressed ) ret=true;
	} else if ( kc == 'Y' || key.vk == TCODK_KP7 ) {
		*up = key.pressed;
		*left = key.pressed;
		if ( key.pressed ) ret=true;
	} else if ( kc == 'U' || key.vk == TCODK_KP9 ) {
		*up = key.pressed;
		*right = key.pressed;
		if ( key.pressed ) ret=true;
	} else if ( kc == 'B' || key.vk == TCODK_KP1 ) {
		*down = key.pressed;
		*left = key.pressed;
		if ( key.pressed ) ret=true;
	} else if ( kc == 'N' || key.vk == TCODK_KP3 ) {
		*down = key.pressed;
		*right = key.pressed;
		if ( key.pressed ) ret=true;
	}

	return ret;
}

void Player::computeStealth(float elapsed) {
	Dungeon *dungeon=gameEngine->dungeon;
	float shadow = dungeon->getShadow(x*2,y*2);
	float cloud = dungeon->getCloudCoef(x*2,y*2);
	shadow = MIN(shadow,cloud);
	// increase shadow. TODO should be in outdoor only!
	float shadowcoef = crouch ? 4.0f : 2.0f;
	shadow = 1.0f - shadowcoef*(1.0f-shadow);
	stealth -= (stealth-shadow) * elapsed;
	float speedcoef = crouch ? 0.6f : 1.0f;
	stealth += speedcoef * averageSpeed * elapsed * 0.1f;
	stealth = CLAMP(0.0f,3.0f,stealth);
//printf ("shadow %g stealth %g\n",shadow,stealth);
}

bool Player::activateCell(int dungeonx, int dungeony, bool lbut_pressed, bool walk, bool *activated) {
	// click on adjacent non walkable item = activate it
	// clink on adjacent pickable item = pick it up
	bool useWeapon=true;
	Dungeon *dungeon=gameEngine->dungeon;
	TCODList<Item *> *items=dungeon->getItems(dungeonx,dungeony);
	if ( activated ) *activated=false;
	if ( items->size() > 0 ) {
		useWeapon=false;
		//if ( lbut_pressed ) {
			TCODList<Item *>toPick;
			TCODList<Item *>toUse;
			for ( Item **it=items->begin();it != items->end(); it++) {
				if ( (*it)->isPickable() && (*it)->speed == 0.0f
					&& (lbut_pressed || (*it)->hasAutoPick()) ) toPick.push(*it);
				else if ( (*it)->isActivatedOnBump() ) toUse.push(*it);
			}
			for ( Item **it=toPick.begin();it != toPick.end(); it++) {
				Item *newIt = (*it)->putInInventory(this);
				if ( newIt->isActivatedOnBump() ) toUse.push(newIt);
			}
			for ( Item **it=toUse.begin();it != toUse.end(); it++) {
				(*it)->use();
				if ( activated ) *activated=true;
			}
		//}
	} else if (dungeon->hasCreature(dungeonx,dungeony) ) {
		// click on adjacent catchable creature = catch it
		Creature *crea=dungeon->getCreature(dungeonx,dungeony);
		if ( crea && crea->isCatchable()) {
			useWeapon=false;
			if ( lbut_pressed ) {
				dungeon->removeCreature(crea,false);
				crea->initItem();
				assert(crea->asItem);
				crea->asItem->putInInventory(this,0,"catch");
			}
		}
	}
	return useWeapon;
}

bool Player::update(float elapsed, TCOD_key_t key,TCOD_mouse_t *mouse) {
	//static float longSpellDelay=config.getFloatProperty("config.creatures.player.longSpellDelay");
	//static float playerSpeed=config.getFloatProperty("config.creatures.player.speed");
	static float sprintLength=config.getFloatProperty("config.creatures.player.sprintLength");
	static char quickslot1Key=config.getCharProperty("config.creatures.player.quickslot1");
	static char quickslot2Key=config.getCharProperty("config.creatures.player.quickslot2");
	static char quickslot3Key=config.getCharProperty("config.creatures.player.quickslot3");
	static char quickslot4Key=config.getCharProperty("config.creatures.player.quickslot4");
	static char quickslot5Key=config.getCharProperty("config.creatures.player.quickslot5");
	static char quickslot6Key=config.getCharProperty("config.creatures.player.quickslot6");
	static char quickslot7Key=config.getCharProperty("config.creatures.player.quickslot7");
	static char quickslot8Key=config.getCharProperty("config.creatures.player.quickslot8");
	static char quickslot9Key=config.getCharProperty("config.creatures.player.quickslot9");
	static char quickslot10Key=config.getCharProperty("config.creatures.player.quickslot10");

	Dungeon *dungeon=gameEngine->dungeon;
	Cell *cell=dungeon->getCell(x,y);
	cell->trampleDate=gameEngine->getCurrentDate();

	if ( initDungeon ) {
		initDungeon=false;
		if (light ) dungeon->addLight(light);
		else dungeon->addLight(&noLight);
	}

	if (getLife() <= 0) return false;
	updateConditions(elapsed);
	updateSkills(elapsed);

	// special key status
	bool ctrl=TCODConsole::isKeyPressed(TCODK_CONTROL);
	//if ( key.vk == TCODK_SHIFT ) isSprinting=key.pressed;
	isSprinting=TCODConsole::isKeyPressed(TCODK_SHIFT);

	// user input
	if ( gameEngine->isGamePaused()) {
		up=down=left=right=false;
		return true;
	}
	// update items in inventory
	for ( Item ** it=inventory.begin(); it != inventory.end(); it++) {
		if (!(*it)->age(elapsed)) {
			it=inventory.removeFast(it); // from inventory
		}
	}

	// crouching
	crouch=ctrl;

	// update breath recovery after sprint
	updateSprintDelay(elapsed, isSprinting);
	// compute average speed during last second
	computeAverageSpeed(elapsed);
	// update fov according to breath
	computeFovRange(elapsed);

	bool tryToMove=Player::getMoveKey(key,&up,&down,&left,&right);

	// mouse coordinates
	int dungeonx=mouse->cx+gameEngine->xOffset;
	int dungeony=mouse->cy+gameEngine->yOffset;

	bool useWeapon=true;
	bool isStunned=hasCondition(ConditionType::STUNNED);
	Condition *frozen=getCondition(ConditionType::STUNNED,"frozen");
	if ( frozen && ( tryToMove || mouse->lbutton_pressed ) ) {
		// wriggle to reduce frozen length
		frozen->duration -= 0.1f;
		frozen->duration=MAX(0.0001f,frozen->duration);
	}
	if ( !isStunned && mouse->lbutton_pressed && ABS(dungeonx-x) <= 1 && ABS(dungeony-y) <= 1 ) {
		// click on the player or near him in water=ripples
		if ( mouse->lbutton_pressed && dungeon->hasRipples(dungeonx,dungeony) ) gameEngine->startRipple(dungeonx,dungeony);
		if ( dungeonx != x || dungeony != y ) {
			useWeapon=activateCell(dungeonx,dungeony,mouse->lbutton_pressed,false);
		}
	}
	if ( useWeapon ) {
		if ( mainHand ) mainHand->update(elapsed,key,mouse);
		if ( offHand ) offHand->update(elapsed,key,mouse);
	}

	if (!isStunned && mouse->lbutton) {
		lbuttonDelay+=elapsed;
		lWalkDelay+=elapsed;
	}
	if (!isStunned && mouse->rbutton) rbuttonDelay+=elapsed;

	// right mouse button
	if (!isStunned && mouse->rbutton_pressed && rmbSkill) {
		rmbSkill->cast();
		rbuttonDelay=0.0f;
	}

	if (!isStunned && ! key.pressed ) {
		if ( ( key.c=='1' || key.c == quickslot1Key ) && quickSlots[0]) quickSlots[0]->cast();
		else if ( ( key.c=='2' || key.c == quickslot2Key ) && quickSlots[1]) quickSlots[1]->cast();
		else if ( ( key.c=='3' || key.c == quickslot3Key ) && quickSlots[2]) quickSlots[2]->cast();
		else if ( ( key.c=='4' || key.c == quickslot4Key ) && quickSlots[3]) quickSlots[3]->cast();
		else if ( ( key.c=='5' || key.c == quickslot5Key ) && quickSlots[4]) quickSlots[4]->cast();
		else if ( ( key.c=='6' || key.c == quickslot6Key ) && quickSlots[5]) quickSlots[5]->cast();
		else if ( ( key.c=='7' || key.c == quickslot7Key ) && quickSlots[6]) quickSlots[6]->cast();
		else if ( ( key.c=='8' || key.c == quickslot8Key ) && quickSlots[7]) quickSlots[7]->cast();
		else if ( ( key.c=='9' || key.c == quickslot9Key ) && quickSlots[8]) quickSlots[8]->cast();
		else if ( ( key.c=='0' || key.c == quickslot10Key ) && quickSlots[9]) quickSlots[9]->cast();
	}

	// walk
	float maxInvSpeed=1.0f/getType()->getSpeed();
	if ( isSprinting && sprintDelay> 0.0f && sprintDelay < sprintLength ) {
		float sprintCoef=1.0f - 4*(sprintLength-sprintDelay)/sprintLength;
		sprintCoef=MAX(MIN_SPRINT_COEF,sprintCoef);
		maxInvSpeed *= sprintCoef;
	}
	if (hasCondition(ConditionType::CRIPPLED)) {
		float crippleCoef = getMinConditionAmount(ConditionType::CRIPPLED);
		maxInvSpeed /= crippleCoef;
	}
	if ( crouch ) {
		maxInvSpeed *= 2.0f;
	}
	// update stealth level
	if ( crouch || stealth < 1.0f ) {
		computeStealth(elapsed);
	}
	if ( ! hasCondition(ConditionType::STUNNED) && ! hasCondition(ConditionType::PARALIZED) ) {
		walkTimer+=elapsed;
	}
	if ( walkTimer >= 0 ) {
		TerrainId terrainId=dungeon->getTerrainType((int)x,(int)y);
		float walkTime = terrainTypes[terrainId].walkCost * maxInvSpeed;
		bool hasWalked=false;
		int newx=(int)x,oldx=(int)x,newy=(int)y,oldy=(int)y;
		if ( up ) newy--;
		else if ( down ) newy++;
		if ( left ) newx--;
		else if ( right) newx++;
		int dx=newx-(int)x;
		int dy=newy-(int)y;
		if ( dx!=0 || dy != 0 ) {
			int oldnewx=newx;
			int oldnewy=newy;
			if ( path ) {
				delete path;
				path = NULL;
			}
			if ( IN_RECTANGLE(newx,newy,dungeon->width,dungeon->height ) && ! dungeon->hasCreature(newx,newy) && dungeon->map->isWalkable(newx,newy)) {
				x=newx;y=newy;
				if ( dx != 0 && dy != 0 ) {
					speedDist += 1.41f;
				} else {
					speedDist += 1.0f;
				}
				gameEngine->stats.nbSteps++;
				hasWalked=true;
			} else if ( IN_RECTANGLE(newx,newy,dungeon->width,dungeon->height) && ! dungeon->hasCreature(newx,newy)
				&& dungeon->hasActivableItem(newx,newy) ) {
				// activate some item by bumping on it (like a chest)
				activateCell(newx,newy,false,true);
				up=down=left=right=false;
				hasWalked=true;
			} else {
				// try to slide against walls
				if ( dx != 0 && dy != 0 ) {
					newx=(int)x+dx;
					newy=(int)y;
					// horizontal slide
					if ( IN_RECTANGLE(newx,newy,dungeon->width,dungeon->height)
						&& dungeon->map->isWalkable(newx,newy)
						&& (! dungeon->hasCreature(newx,newy) || ! dungeon->getCreature(newx,newy)->isBlockingPath())
						) {
						x=newx;y=newy;
						gameEngine->stats.nbSteps++;
						speedDist+=1.0f;
						hasWalked=true;
					} else {
						// vertical slide
						newx=(int)x;
						newy=(int)y+dy;
						if ( IN_RECTANGLE(newx,newy,dungeon->width,dungeon->height )
							&& dungeon->map->isWalkable(newx,newy)
							&& (! dungeon->hasCreature(newx,newy) || ! dungeon->getCreature(newx,newy)->isBlockingPath())
							) {
							x=newx;y=newy;
							gameEngine->stats.nbSteps++;
							speedDist+=1.0f;
							hasWalked=true;
						}
					}
				} else if ( dx != 0 ) {
					static int dy=1;
					if ( IN_RECTANGLE(x+dx,y+dy,dungeon->width,dungeon->height )
						&& dungeon->map->isWalkable((int)x+dx,(int)y+dy)
						&& (! dungeon->hasCreature((int)x+dx,(int)y+dy)
							|| ! dungeon->getCreature((int)x+dx,(int)y+dy)->isBlockingPath())
						) {
						newx = (int)x + dx;
						newy = (int)y + dy;
						x=newx;y=newy;
						gameEngine->stats.nbSteps++;
						dy=-dy;
						speedDist+=1.41f;
						hasWalked=true;
					} else if ( IN_RECTANGLE(x+dx,y-dy,dungeon->width,dungeon->height )
						&& dungeon->map->isWalkable((int)x+dx,(int)y-dy)
						&& (! dungeon->hasCreature((int)x+dx,(int)y-dy)
							|| ! dungeon->getCreature((int)x+dx,(int)y-dy)->isBlockingPath())
						) {
						newx = (int)x + dx;
						newy = (int)y - dy;
						x=newx;y=newy;
						gameEngine->stats.nbSteps++;
						dy=-dy;
						speedDist+=1.41f;
						hasWalked=true;
					}
				} else if ( dy != 0 ) {
					static int dx=1;
					if ( IN_RECTANGLE(x+dx,y+dy,dungeon->width,dungeon->height )
						&& dungeon->map->isWalkable((int)x+dx,(int)y+dy)
						&& (! dungeon->hasCreature((int)x+dx,(int)y+dy)
							|| ! dungeon->getCreature((int)x+dx,(int)y+dy)->isBlockingPath())
						) {
						newx = (int)x + dx;
						newy = (int)y + dy;
						x=newx;y=newy;
						gameEngine->stats.nbSteps++;
						dx=-dx;
						speedDist+=1.41f;
						hasWalked=true;
					} else if ( IN_RECTANGLE(x-dx,y+dy,dungeon->width,dungeon->height )
						&& dungeon->map->isWalkable((int)x-dx,(int)y+dy)
						&& (! dungeon->hasCreature((int)x-dx,(int)y+dy)
							|| ! dungeon->getCreature((int)x-dx,(int)y+dy)->isBlockingPath())
						) {
						newx = (int)x - dx;
						newy = (int)y + dy;
						x=newx;y=newy;
						gameEngine->stats.nbSteps++;
						dx=-dx;
						speedDist+=1.41f;
						hasWalked=true;
					}
				}
				if ( oldx == x && oldy ==y && IN_RECTANGLE(oldnewx,oldnewy,dungeon->width,dungeon->height)) {
					// could not walk. activate item ?
					bool activated=false;
					activateCell(oldnewx,oldnewy,false,false, &activated);
					if (activated) {
						up=down=left=right=false;
						hasWalked=true;
					}
				}
			}
		} else if ( path && ! path->isEmpty()) {
			path->get(0,&newx,&newy);
			if ( ! dungeon->hasCreature(newx,newy) ) {
				path->walk(&newx,&newy,false);
				setPos(newx,newy);
				gameEngine->stats.nbSteps++;
				hasWalked=true;
			} else {
				// the path is obstructed. cancel it
				delete path;
				path=NULL;
			}
		}
		// auto pickup items
		if ( oldx != x || oldy != y ) {
			TCODList<Item *> *items=dungeon->getItems((int)x,(int)y);
			TCODList<Item *>toPick;
			TCODList<Item *>toUse;
			for ( Item **it=items->begin();it != items->end(); it++) {
				if ( (*it)->isPickable() && (*it)->speed == 0.0f && (*it)->hasAutoPick() ) toPick.push(*it);
				else if ( (*it)->isActivatedOnBump() ) toUse.push(*it);
			}
			for ( Item **it=toPick.begin();it != toPick.end(); it++) {
				Item *newIt = (*it)->putInInventory(this);
				if ( newIt->isActivatedOnBump() ) toUse.push(newIt);
			}
			for ( Item **it=toUse.begin();it != toUse.end(); it++) {
				(*it)->use();
			}

			if ( dungeon->hasRipples(x,y) ) {
				gameEngine->startRipple(x,y);
			}
		}
		if ( hasWalked ) walkTimer=-walkTime;
	}
	if ( light ) light->setPos(x*2,y*2);
	else noLight.setPos(x*2,y*2);
	return true;
}

void Player::computeFovRange(float elapsed) {
	static float rangeAccomodation=config.getFloatProperty("config.creatures.player.rangeAccomodation");
	static float playerSpeed=gameEngine->getFloatParam("playerSpeed");
	float fovRangeTarget = maxFovRange;
	if ( averageSpeed > playerSpeed/2 ) {
		float fovSpeed=averageSpeed-playerSpeed/2;
		float fovRefSpeed=playerSpeed/2;
		fovRangeTarget = maxFovRange - (fovSpeed*0.5/fovRefSpeed)*0.8*maxFovRange ;
	}
	if ( crouch ) fovRangeTarget *= 1.15f;
	if ( fovRange > fovRangeTarget ) fovRange += (fovRangeTarget-fovRange) * elapsed ;
	else fovRange += (fovRangeTarget-fovRange) * elapsed / rangeAccomodation;
}

void Player::computeAverageSpeed(float elapsed) {
	speedElapsed += elapsed;
	if ( speedElapsed > 0.5f ) {
		averageSpeed = speedDist * 2;
		speedElapsed = speedDist = 0.0f;
	}
}

// update breath recovery after sprint
// sprintDelay < 0 : recovery. cannot sprint
void Player::updateSprintDelay(float elapsed, bool isSprinting) {
	static float sprintLength=config.getFloatProperty("config.creatures.player.sprintLength");
	static float sprintRecovery=config.getFloatProperty("config.creatures.player.sprintRecovery");
	if ( sprintDelay > 0.0f && isSprinting && averageSpeed > 0.1f ) {
		sprintDelay -= elapsed;
		if ( sprintDelay < 0.0f ) {
			// exhausted
			Condition *cond=new Condition(ConditionType::CRIPPLED,sprintRecovery,0.5f,"exhausted");
			addCondition(cond);
		}
	} else if ( sprintDelay < 0.0f ) {
		if (! hasCondition(ConditionType::CRIPPLED,"exhausted") ) sprintDelay=sprintLength;
	} else if ( sprintDelay > 0.0f && sprintDelay < sprintLength ) {
		sprintDelay += elapsed;
		if ( sprintDelay > sprintLength ) sprintDelay=sprintLength;
	}
}

#define PLAY_CHUNK_VERSION 4
void Player::saveData(uint32_t chunkId, TCODZip *zip) {
	saveGame.saveChunk(PLAY_CHUNK_ID,PLAY_CHUNK_VERSION);

	// save player specific data
	zip->putFloat(stealth);
	Creature::saveData(CREA_CHUNK_ID,zip);
}

bool Player::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip *zip) {
	if ( chunkVersion != PLAY_CHUNK_VERSION ) return false;

	// load player specific data
	stealth=zip->getFloat();

	saveGame.loadChunk(&chunkId,&chunkVersion);
	bool ret = Creature::loadData(chunkId,chunkVersion,zip);
	if ( ret ) {
		TextGenerator::addGlobalValue("PLAYER_NAME",getType()->getName());
	}
	return ret;
}
