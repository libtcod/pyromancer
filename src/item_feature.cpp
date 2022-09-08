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
#include <math.h>
#include "main.hpp"

// abstract class
ItemFeature *ItemFeature::clone() const {
	return new ItemFeature(type);
}
void ItemFeature::copy(const ItemFeature *feat) {
	if ( type != feat->type ) {
		fprintf(stderr,"Fatal : cannot copy item feature %d on %d\n",feat->type,type);
		std::abort();
	}
}

// produce
ItemFeatureProduce::ItemFeatureProduce(float delay, float chance, ItemType *type) :
	ItemFeature(PRODUCE),delay(delay),chance(chance),type(type) {}
ItemFeature *ItemFeatureProduce::clone() const {
	return new ItemFeatureProduce(delay,chance,type);
}
void ItemFeatureProduce::copy(const ItemFeature *feat) {
	ItemFeature::copy(feat);
	*this = *(const ItemFeatureProduce *)feat;
}
void ItemFeatureProduce::use(Item *it) {
	float odds=TCODRandom::getInstance()->getFloat(0.0f,1.0f) ;
	if ( it->isA("tree") ) {
		bool cut=false;
		if ( gameEngine->player->mainHand && gameEngine->player->mainHand->isA("cut") ) cut=true;
		else if ( gameEngine->player->offHand && gameEngine->player->offHand->isA("cut") ) cut=true;
		if ( cut ) {
			Item *product=it->produce(odds);
			gameEngine->gui.log.info("You cut %s off %s.",product->aName(),it->theName());
			product->putInInventory(gameEngine->player);
		} else {
			if ( TCODRandom::getInstance()->getInt(0,2) == 0 ) {
				gameEngine->gui.log.info("You kick %s.",it->theName());
				gameEngine->gui.log.warn("You feel a sharp pain in the foot.");
				Condition *cond=new Condition(ConditionType::CRIPPLED,10.0f,0.5f);
				gameEngine->hitFlash();
				gameEngine->player->addCondition(cond);
			} else {
				gameEngine->gui.log.info("Some branches of %s are interesting, but you need something to cut them.",it->theName());
			}
		}
	} else {
		Item *product=it->produce(odds);
		gameEngine->gui.log.info("You pick %s.",product->aName());
		product->putInInventory(gameEngine->player);
	}
}

// fire
ItemFeatureFire::ItemFeatureFire(float resistance, ItemType *type) :
	ItemFeature(FIRE),resistance(resistance),type(type) {}
ItemFeature *ItemFeatureFire::clone() const {
	return new ItemFeatureFire(resistance,type);
}
void ItemFeatureFire::copy(const ItemFeature *feat) {
	ItemFeature::copy(feat);
	*this = *(const ItemFeatureFire *)feat;
}
bool ItemFeatureFire::update(float elapsed, TCOD_mouse_t *mouse, Item *it) {
	if (it->fireResistance > 0.0f) return true;
	// this item has burnt away!
	Dungeon *dungeon=gameEngine->dungeon;
	if ( type ) {
		it->convertTo(type);
		// set this item to fire!
		ItemFeatureHeat *ignite=(ItemFeatureHeat *)type->getFeature(ItemFeature::HEAT);
		if ( ignite ) {
			float rad = ignite->radius;
			gameEngine->startFireZone((int)(it->x-rad),(int)(it->y-2*rad),(int)(2*rad+1),(int)(3*rad));
		}
	}
	// destroy this item
	if (! it->owner ) {
		if ( it->asCreature ) dungeon->removeCreature(it->asCreature,false);
	} else {
		if ( it->asCreature ) it->asCreature->toDelete=true;
	}
	ItemFeatureHeat *ignite=(ItemFeatureHeat *)it->getFeature(ItemFeature::HEAT);
	if ( ignite ) {
		// this item stops burning
		float rad = ignite->radius;
		gameEngine->removeFireZone((int)(it->x-rad),(int)(it->y-2*rad),(int)(2*rad+1),(int)(3*rad));
	}
	Cell *cell=dungeon->getCell(it->x,it->y);
	if (cell->building) {
		cell->building->collapseRoof();
	}
	return false;
}

// age
ItemFeatureAge::ItemFeatureAge(float delay, ItemType *type) :
	ItemFeature(AGE),delay(delay),type(type) {}
ItemFeature *ItemFeatureAge::clone() const {
	return new ItemFeatureAge(delay,type);
}
void ItemFeatureAge::copy(const ItemFeature *feat) {
	ItemFeature::copy(feat);
	*this = *(const ItemFeatureAge *)feat;
}
bool ItemFeatureAge::update(float elapsed, TCOD_mouse_t *mouse, Item *it) {
	return it->age(elapsed,this);
}

// food
ItemFeatureFood::ItemFeatureFood(int health, int mana, float delay) :
	ItemFeature(FOOD),health(health),mana(mana),delay(delay) {}
ItemFeature *ItemFeatureFood::clone() const {
	return new ItemFeatureFood(health,mana,delay);
}
void ItemFeatureFood::copy(const ItemFeature *feat) {
	ItemFeature::copy(feat);
	*this = *(const ItemFeatureFood *)feat;
}
void ItemFeatureFood::use(Item *it) {
	if ( health ) it->owner->addCondition(new Condition(ConditionType::HEAL, delay,health));
	if ( mana ) it->owner->addCondition(new Condition(ConditionType::REGEN_MANA, delay,mana));
}


// light
ItemFeatureLight::ItemFeatureLight(float range, const HDRColor &color, int flags,
	float patternDelay, const char *pattern,const HDRColor &color2,const char *colorPattern) :
	ItemFeature(LIGHT),range(range),color(color),color2(color2),flags(flags),patternDelay(patternDelay) {
	if ( pattern ) this->pattern=strdup(pattern);
	else this->pattern=NULL;
	if ( colorPattern ) this->colorPattern=strdup(colorPattern);
	else this->colorPattern=NULL;
}
ItemFeature *ItemFeatureLight::clone() const {
	return new ItemFeatureLight(range,color,flags,patternDelay,pattern,color2,colorPattern);
}
void ItemFeatureLight::copy(const ItemFeature *feat) {
	ItemFeature::copy(feat);
	*this = *(const ItemFeatureLight *)feat;
}

// attack
ItemFeatureAttack::ItemFeatureAttack(WieldType wield, float minCastDelay, float maxCastDelay,
		float minReloadDelay, float maxReloadDelay, float minDamagesCoef, float maxDamagesCoef,
		int flags, float speed, ItemType *ammunition, ItemType *casts,StatusResource::Type resourceType,float resourceCost) :
		ItemFeature(ATTACK),wield(wield),minCastDelay(minCastDelay),
		maxCastDelay(maxCastDelay),minReloadDelay(minReloadDelay),maxReloadDelay(maxReloadDelay),
		minDamagesCoef(minDamagesCoef),maxDamagesCoef(maxDamagesCoef),flags(flags),
		speed(speed),ammunition(ammunition),casts(casts),resourceType(resourceType),
		resourceCost(resourceCost){}
ItemFeature *ItemFeatureAttack::clone() const {
	return new ItemFeatureAttack(wield, minCastDelay, maxCastDelay,
		minReloadDelay, maxReloadDelay, minDamagesCoef, maxDamagesCoef,
		flags, speed, ammunition, casts,resourceType,resourceCost);
}
void ItemFeatureAttack::copy(const ItemFeature *feat) {
	ItemFeature::copy(feat);
	*this = *(const ItemFeatureAttack *)feat;
}
void ItemFeatureAttack::use(Item *it) {
	if ( it->owner ) {
		if ( it->isEquiped() ) it->owner->unwield(it);
		else it->owner->wield(it);
	}
}
bool ItemFeatureAttack::update(float elapsed, TCOD_mouse_t *mouse, Item *it) {
	// bows, wands, ... : the longer you press the button, the more powerful the projectile is
	static float attackCoef=0.0f;

	switch (it->phase) {
	case EXPLODE : break;
	case CAST : {
		it->phaseTimer -= elapsed;
		if ( it->phaseTimer <= 0.0f && (flags & ItemFeatureAttack::WEAPON_RANGED )) {
			if ( resourceCost > 0.0f ) {
				if ( it->owner->getResourceValue(resourceType) < resourceCost ) break;
				it->owner->addResource(resourceType,-resourceCost);
			}
			it->phaseTimer=it->reloadDelay;
			if ( it->phaseTimer > 0.0f ) it->phase=RELOAD;
			else it->phase=IDLE;
			Item *projectile=NULL;
			if ( casts ) {
				projectile=Item::getItem(casts,it->owner->x,it->owner->y);
				projectile->owner=it->owner;
				projectile->dx=it->targetx-it->owner->x;
				projectile->dy=it->targety-it->owner->y;
				float l=Entity::fastInvSqrt(projectile->dx*projectile->dx+projectile->dy*projectile->dy);
				projectile->dx*=l;
				projectile->dy*=l;
				projectile->duration=0.0f; // TODO parameter
				projectile->x+=projectile->dx;
				projectile->y+=projectile->dy;
			} else if ( ammunition ) {
				// TODO
			}

			ItemFeatureAttack *projectileAttack=(ItemFeatureAttack *)projectile->getFeature(ItemFeature::ATTACK);
			if ( ! projectileAttack ) {
				fprintf(stderr,"Item type %s cannot have non projectile ammunition/casts type %s",it->typeData->name,projectile->typeData->name);
				std::abort();
			}
			projectile->speed=projectileAttack->speed;
			projectile->damages *= attackCoef;
			if ( projectile->light ) {
				projectile->light->range *= attackCoef;
			}
			gameEngine->dungeon->addItem(projectile);
		} else {
			if ( flags & ItemFeatureAttack::WEAPON_PROJECTILE ) {
				// keep targetting while the mouse button is pressed
				int dx=mouse->cx+gameEngine->xOffset;
				int dy=mouse->cy+gameEngine->yOffset;
				it->targetx=dx;
				it->targety=dy;
				if ( !mouse->lbutton ) {
					// fire when mouse button released
					it->phaseTimer=MAX(it->phaseTimer,0.0f);
					float ispeed=(it->castDelay-it->phaseTimer)/it->castDelay;
					it->speed=MIN(ispeed,1.0f);
					it->phase=RELOAD;
					it->phaseTimer=it->reloadDelay;
					if ( (int)it->targetx == (int)it->owner->x && (int)it->targety == (int)it->owner->y ) return true;
					it->x=it->owner->x;
					it->y=it->owner->y;
					Item *newIt=it->owner->removeFromInventory(it);
					newIt->dx = it->targetx - it->x;
					newIt->dy = it->targety - it->y;
					float l=Entity::fastInvSqrt(newIt->dx*newIt->dx+newIt->dy*newIt->dy);
					newIt->dx*=l;
					newIt->dy*=l;
					newIt->x = it->x;
					newIt->y = it->y;
					newIt->speed=it->speed*12;
					newIt->duration=1.5f;
					gameEngine->dungeon->addItem(newIt);
				}
			}
		}
	}
		break;
	case RELOAD :
		it->phaseTimer -= elapsed;
		if ( it->phaseTimer <= 0.0f ) {
			it->phase=IDLE;
		}
		break;
	case IDLE:
		if ( it->owner->isPlayer() && mouse->lbutton && it->isEquiped()
			&& !it->owner->hasCondition(ConditionType::STUNNED)) {
			static float longSpellDelay=config.getFloatProperty("config.creatures.player.longSpellDelay");
			it->phaseTimer=it->castDelay;
			it->phase=CAST;
			int dx=mouse->cx+gameEngine->xOffset;
			int dy=mouse->cy+gameEngine->yOffset;
			it->targetx=dx;
			it->targety=dy;
			attackCoef=1.0f ; //+((Player *)it->owner)->lbuttonDelay/longSpellDelay;
			attackCoef=MIN(2.0f,attackCoef);
			((Player *)it->owner)->lbuttonDelay=0.0f;
//printf ("delay %g coef %g\n",((Player *)owner)->lbuttonDelay,attackCoef);
		}
	break;
	}
	return true;
}

// heat
ItemFeatureHeat::ItemFeatureHeat(float intensity, float radius) :
	ItemFeature(HEAT),intensity(intensity),radius(radius) {}
ItemFeature *ItemFeatureHeat::clone() const {
	return new ItemFeatureHeat(intensity,radius);
}
void ItemFeatureHeat::copy(const ItemFeature *feat) {
	ItemFeature::copy(feat);
	*this = *(const ItemFeatureHeat *)feat;
}
bool ItemFeatureHeat::update(float elapsed, TCOD_mouse_t *mouse, Item *it) {
	it->heatTimer += elapsed;
	if ( it->heatTimer > 1.0f) {
		Dungeon *dungeon=gameEngine->dungeon;
		// warm up adjacent items
		it->heatTimer = 0.0f;
		for (int tx=-(int)floor(radius); tx <= (int)ceil(radius); tx++) {
			if ( (int)(it->x)+tx >= 0 && (int)(it->x)+tx < dungeon->width) {
				int dy=(int)(sqrtf(radius*radius - tx*tx));
				for (int ty=-dy; ty <= dy; ty++) {
					if ( (int)(it->y)+ty >= 0 && (int)(it->y)+ty < dungeon->height ) {
						TCODList<Item *> *items=dungeon->getItems((int)(it->x)+tx,(int)(it->y)+ty);
						for ( Item **it2=items->begin(); it2!=items->end(); it2++) {
							// found an adjacent item
							if ( (*it2)->hasFeature(ItemFeature::FIRE) ) {
								// item is affected by fire
								(*it2)->fireResistance -= intensity;
							}
						}
						if ( it->owner->isPlayer()) {
							Creature *cr=dungeon->getCreature((int)(it->x)+tx,(int)(it->y)+ty);
							if ( cr ) {
								cr->takeDamage(intensity);
								cr->setBurning(true);
							}
						} else if ((int)(gameEngine->player->x) == (int)(it->x+tx) && (int)(gameEngine->player->y) == (int)(it->y+ty)) {
							gameEngine->player->takeDamage(intensity);
						}
					}
				}
			}
		}
	}
	return true;
}

// container
ItemFeatureContainer::ItemFeatureContainer(int size) :
	ItemFeature(CONTAINER),size(size) {}
ItemFeature *ItemFeatureContainer::clone() const {
	return new ItemFeatureContainer(size);
}
void ItemFeatureContainer::copy(const ItemFeature *feat) {
	ItemFeature::copy(feat);
	*this = *(const ItemFeatureContainer *)feat;
}
void ItemFeatureContainer::use(Item *it) {
	if (! it->isPickable()) {
		gameEngine->openCloseLoot(it);
	} else if (it->isA("bottle")) {
		if( it->stack.size() > 0 ) {
			Item *content=it->stack.get(0);
			// content owner might not be up to date
			content->owner=it->owner;
			content->use();
			it->computeBottleName();
		}
	}
}

// explode
ItemFeatureExplodeOnBounce::ItemFeatureExplodeOnBounce(float delay,float startRange,float endRange,
		float middleRange,float middleLightCoef,float middleTime,
		float startLightCoef,float endLightCoef,int particleCount,float particleSpeed,
		float particleDuration,
		float particleMinDamage,float particleMaxDamage,const HDRColor & particleStartColor,
		const HDRColor &particleEndColor) :
	ItemFeature(EXPLODE_ON_BOUNCE),delay(delay),startRange(startRange),startLightCoef(startLightCoef),
	middleRange(middleRange),middleLightCoef(middleLightCoef),middleTime(middleTime),
	endRange(endRange),endLightCoef(endLightCoef),particleCount(particleCount),
	particleType(particleStartColor,particleEndColor,particleMinDamage,particleMaxDamage,
		particleDuration,particleSpeed) {}
ItemFeature *ItemFeatureExplodeOnBounce::clone() const {
	ItemFeatureExplodeOnBounce *ret=new ItemFeatureExplodeOnBounce(delay,startRange,endRange,
	middleRange,middleLightCoef,middleTime,startLightCoef,endLightCoef,
	particleCount,particleType.speed,particleType.duration,particleType.minDamage,particleType.maxDamage,
	particleType.startColor,particleType.endColor);
	for ( Effect **it=effects.begin(); it != effects.end(); it++) {
		ret->effects.push(*it);
	}
	return ret;
}
void ItemFeatureExplodeOnBounce::copy(const ItemFeature *feat) {
	ItemFeature::copy(feat);
	const ItemFeatureExplodeOnBounce *eob=(const ItemFeatureExplodeOnBounce *)feat;
	this->delay=eob->delay;
	this->startRange=eob->startRange;
	this->endRange=eob->endRange;
	this->middleRange=eob->middleRange;
	this->middleLightCoef=eob->middleLightCoef;
	this->middleTime=eob->middleTime;
	this->startLightCoef=eob->startLightCoef;
	this->endLightCoef=eob->endLightCoef;
	this->particleCount=eob->particleCount;
	this->particleType=eob->particleType;
	for ( Effect **it=eob->effects.begin(); it != eob->effects.end(); it++) {
		effects.push(*it);
	}
}

Effect *ItemFeatureExplodeOnBounce::getEffect(Effect::Type type) const {
	for ( Effect **effect=effects.begin(); effect != effects.end(); effect++) {
		if ( (*effect)->type == type ) return *effect;
	}
	return NULL;
}

ConditionEffect *ItemFeatureExplodeOnBounce::getConditionEffect(ConditionType::Type type) const {
	for ( Effect **effect=effects.begin(); effect != effects.end(); effect++) {
		if ( (*effect)->type == Effect::CONDITION && ((ConditionEffect *)(*effect))->conditionType == type )
			return (ConditionEffect *)*effect;
	}
	return NULL;
}

bool ItemFeatureExplodeOnBounce::update(float elapsed, TCOD_mouse_t *mouse, Item *it) {
	if (it->phase != EXPLODE) return true;
	bool first=it->phaseTimer == delay;
	it->phaseTimer-= elapsed;
	if ( it->phaseTimer < 0.0f ) {
		// apply effects to all creatures in range
		if ( effects.size() > 0 ) {
			float maxRange=MAX(startRange,middleRange);
			maxRange=MAX(maxRange,endRange);
			Dungeon *dungeon=gameEngine->dungeon;
			Player *player=gameEngine->player;
			TCODList<Creature *> creatures;
			// find all creatures at range
			for (int x=(int)(it->x-maxRange); x < (int)(it->x+maxRange); x++) {
				if ( x >= 0 && x < dungeon->width) {
					int dy=(int)(sqrtf(maxRange*maxRange - (it->x-x)*(it->x-x)));
					for (int y=(int)(it->y-dy); y < (int)(it->y+dy); y++) {
						if ( y >=0 && y < dungeon->width) {
							if ( it->owner->isPlayer()) {
								Creature *cr=dungeon->getCreature(x,y);
								if ( cr ) {
									creatures.push(cr);
								}
							} else if (player != it->owner && (int)player->x == x && (int)player->y==y) {
								creatures.push(player);
							}
						}
					}
				}
			}
			// apply effects
			for ( Effect **effect=effects.begin(); effect != effects.end(); effect++) {
				for (Creature **cr=creatures.begin(); cr!= creatures.end(); cr++) {
					(*effect)->execute((int)it->x,(int)it->y,*cr,it->owner,NULL);
				}
			}
		}
		return false;
	}
	ItemFeatureLight *l = (ItemFeatureLight *)it->getFeature(ItemFeature::LIGHT);
	float blastcoef=first ? 10.0f : 1.0f;
	if (middleTime > 0.0f ) {
		if ( it->phaseTimer >= delay - middleTime ) {
			float coef=(delay - it->phaseTimer) / middleTime;
			it->light->range=(startRange
				-(startRange-middleRange)*coef);
			it->light->color=l->color*((startLightCoef*blastcoef
				-(startLightCoef-middleLightCoef)*coef));
		} else {
			float coef=(delay-it->phaseTimer-middleTime)/(delay-middleTime);
			it->light->range=(middleRange
				-(middleRange-endRange)*coef);
			it->light->color=l->color*((middleLightCoef
				-(middleLightCoef-endLightCoef)*coef));
		}
	} else {
		it->light->range=(endRange
			-(endRange-startRange)*it->phaseTimer/delay);
		it->light->color=l->color*((endLightCoef
			-(endLightCoef-startLightCoef*blastcoef)*it->phaseTimer/delay));
	}
	return true;
}

//spell tree
ItemFeatureSpellTree::ItemFeatureSpellTree() : ItemFeature(SPELLTREE) {}
void ItemFeatureSpellTree::use(Item *it) {
	gameEngine->gui.setMode(GUI_POWERUP);
}
