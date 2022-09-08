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
#include <stdio.h>
#include <math.h>
#include "main.hpp"

AiDirector *AiDirector::instance=NULL;

AiDirector::AiDirector() : spawnCount(0),spawnTimer(0.0f),
	waitTimer(0.0f),timer(-M_PI/2),dropTimer(0.0f),nbBoss(0) {
	static int itemKillCount=config.getIntProperty("config.aidirector.itemKillCount");

	lowLevel=config.getFloatProperty("config.aidirector.lowLevel");
	medLevel=config.getFloatProperty("config.aidirector.medLevel");

	instance=this;
	status=STATUS_CALM;
	killCount=TCODRandom::getInstance()->getInt((int)(itemKillCount*0.75),(int)(itemKillCount*1.25));
	hordeTimer=config.getIntProperty("config.aidirector.hordeDelay");
	baseCreature=CREATURE_MINION;
}

void AiDirector::setLevelCoef(float coef) {
	levelCoef=coef;
	waitTimer=0.0f;
	float maxHorde=config.getIntProperty("config.aidirector.hordeDelay")* (1.0f-0.5f*coef);
	hordeTimer=MIN(maxHorde,hordeTimer);
}
void AiDirector::update(float elapsed) {
	static float waveLength=config.getFloatProperty("config.aidirector.waveLength");
	static float medRate=config.getFloatProperty("config.aidirector.medRate");
	static float highRate=config.getFloatProperty("config.aidirector.highRate");
	static int maxCreatures=config.getIntProperty("config.aidirector.maxCreatures");
	static int hordeDelay=config.getIntProperty("config.aidirector.hordeDelay");
	static bool debug=config.getBoolProperty("config.debug");

	GameEngine *game=gameEngine;
	hordeTimer-= elapsed;
	dropTimer+=elapsed;
	timer+=elapsed*(2*M_PI)/(waveLength* (1.0f-0.5f*levelCoef));
	spawnTimer+=elapsed;
	float wavePos=0.5f * (1.0f+sinf(timer));
	if ( wavePos < lowLevel ) {
		if ( wavePos >= lowLevel/2 ) status=STATUS_CALM;
		return;
	}
	if ( waitTimer > 0.0f ) {
		waitTimer-=elapsed;
		return;
	}
	if ( wavePos < medLevel ) status=STATUS_MED;
	else status=STATUS_HIGH;
	int nbCreatures=Creature::creatureByType[baseCreature].size();
	if (nbCreatures >= maxCreatures) return;
	if( spawnTimer > 60.0f ) {
		spawnTimer-=60.0f;
		spawnCount=0;
	}
	if ( wavePos < medLevel || hordeTimer > 0.0f) {
		float rate=(wavePos < medLevel) ? medRate : highRate;
		rate *= (1.0f+levelCoef);
		float curRate=spawnCount/spawnTimer;
		if ( curRate < rate ) {
			float timeRemaining=60.0f-spawnTimer;
			int nbMissing = (int)(rate - spawnCount);
			if (nbMissing > 0 ) {
				if ( debug ) game->gui.log.debug("minion (%s) wavepos %d horde in %d",
					(wavePos < medLevel ? "med":"high"),(int)(wavePos*100),(int)(hordeTimer));
				if (nbBoss < gameEngine->dungeon->level && status == STATUS_HIGH && TCODRandom::getInstance()->getInt(0,10)==1) {
					spawnMiniBosses();
				} else spawnMinion(false);
				waitTimer=timeRemaining/nbMissing;
			}
		}
	} else {
		status=STATUS_HORDE;
		float timeRemaining=60.0f-spawnTimer;
		int nbMissing = (int)(highRate*(1.0f+levelCoef) - spawnCount);
		if ( debug ) game->gui.log.debug("Horde! %d",nbMissing);
		// spawn miniboss(es)
		if ( nbBoss < gameEngine->dungeon->level )
		for (int i=1; i < levelCoef*3; i++) {
			spawnMiniBosses();
		}
		while ( nbMissing> 0 && game->dungeon->creatures.size() < maxCreatures ) {
			spawnMinion(true);
			nbMissing--;
		}
		hordeTimer= hordeDelay;
		waitTimer=timeRemaining;
	}
}

void AiDirector::spawnMiniBoss(Creature *cr, bool withItem) {
	cr->setSeen(true);
	if ( withItem ) {
		Item *it=getDropItem();
		cr->addToInventory(it);
	}
	spawnCreature(cr);
	nbBoss++;
}
void AiDirector::spawnMiniBosses() {
	#define MAXNUM 2
	int level=gameEngine->dungeon->level;
	level=MIN(MAXNUM,level);
	int typ=TCODRandom::getInstance()->getInt(0,level);
	Creature *cr=NULL;
	switch (typ) {
	case 0:
		cr=Creature::getCreature(CREATURE_ICE_SHRIEKER);
		cr->addSkillType("iceball");
		spawnMiniBoss(cr,true);
		break;
	case 1:
		cr=Creature::getCreature(CREATURE_ICE_SHRIEKER);
		cr->addSkillType("iceball");
		spawnMiniBoss(cr,true);
		cr=Creature::getCreature(CREATURE_ICE_SHRIEKER);
		cr->addSkillType("iceball");
		spawnMiniBoss(cr,false);
		break;
	case 2:
		cr=Creature::getCreature(CREATURE_DARK_WANDERER);
		cr->addSkillType("darkness");
		cr->addSkillType("minifrost");
		spawnMiniBoss(cr,true);
		break;
	}
}

void AiDirector::spawnCreature(Creature *cr) {
	int sx,sy;
	GameEngine *game=gameEngine;
	game->dungeon->getClosestSpawnSource(game->player->x,game->player->y,&sx,&sy);
	game->dungeon->getClosestWalkable(&sx,&sy,true,false);
	cr->setPos(sx,sy);
	game->dungeon->addCreature(cr);
}

void AiDirector::spawnMinion(bool chase, int x, int y) {
	int sx,sy;
	GameEngine *game=gameEngine;

	spawnCount++;
	if (x != -1 && y != -1 ) {
		sx=x;sy=y;
	} else {
		game->dungeon->getClosestSpawnSource(game->player->x,game->player->y,&sx,&sy);
	}
	Creature *cr=Creature::getCreature(baseCreature);
	game->dungeon->getClosestWalkable(&sx,&sy,true,false);
	cr->setPos(sx,sy);
	if ( chase ) cr->setSeen(true);
	game->dungeon->addCreature(cr);
}

// remove a creature that is too far from player
void AiDirector::replace(Creature *cr) {
	int oldx=(int)cr->x,oldy=(int)cr->y;
	cr->setBurning(false);
	cr->setSeen(false);
	int newx,newy;
	gameEngine->dungeon->getClosestSpawnSource(gameEngine->player->x,gameEngine->player->y,&newx,&newy);
	cr->setPos(newx,newy);
	gameEngine->dungeon->moveCreature(cr,oldx,oldy,newx,newy);
}

void AiDirector::dropItem(int x, int y) {
	Item *it=getDropItem();
	it->setPos(x,y);
	gameEngine->dungeon->addItem(it);
	dropTimer=0.0f;
}

Item *AiDirector::getDropItem() {
	TCODList<Powerup *>list;
	Item *it=NULL;
	Powerup::getAvailable(&list);

	bool hp = false;
	bool mana=false;
	int nbScrolls=list.size();
	ItemType *scrollType=ItemType::getType("scroll");
	for (Item **it2=gameEngine->dungeon->items.begin();it2!=gameEngine->dungeon->items.end(); it2++) {
		if ( (*it2)->isA(scrollType) ) nbScrolls--;
	}
	if ( gameEngine->player->getLifeRatio() < gameEngine->player->getManaRatio() ) {
		hp=TCODRandom::getInstance()->getFloat(0.0f,1.0f) > gameEngine->player->getLifeRatio() ;
		if (! hp ) mana = TCODRandom::getInstance()->getFloat(0.0f,1.0f) > gameEngine->player->getManaRatio();
	} else {
		mana=TCODRandom::getInstance()->getFloat(0.0f,1.0f) > gameEngine->player->getManaRatio() ;
		if (! mana ) hp = TCODRandom::getInstance()->getFloat(0.0f,1.0f) > gameEngine->player->getLifeRatio();
	}
	if ( list.size() == 0 || hp || mana || nbScrolls <= 0) {
		if ( hp) {
			// health potion
			it=Item::getItem("health",0,0);
		} else {
			// mana potion
			it=Item::getItem("mana",0,0);
		}
		Item *bottle=Item::getItem("bottle",0,0);
		it->putInContainer(bottle);
		bottle->col = it->col;
		bottle->computeBottleName();
		return bottle;
	} else {
		Item *it=Item::getItem("scroll",0,0);
		return it;
	}
}

void AiDirector::killCreature(Creature *cr) {
	static int itemKillCount=config.getIntProperty("config.aidirector.itemKillCount");
	gameEngine->stats.nbCreatureKilled++;
	if ( cr->type != CREATURE_MINION ) nbBoss--;
	if ( gameEngine->player->getLifeRatio() < 0.5f || gameEngine->player->getManaRatio() < 0.5f ) killCount --;
	if ( killCount <= 0 || (dropTimer > 60.0f && TCODRandom::getInstance()->getInt(0,4)==0)) {
		dropItem((int)cr->x,(int)cr->y);
		killCount=itemKillCount;
	}
}
