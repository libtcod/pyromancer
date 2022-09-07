/*
* Copyright (c) 2011 Jice
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

void Effect::configureParser(TCODParser *parser,TCODParserStruct *effect) {
	effect->addFlag("parry");
	effect->addFlag("teleport");
	effect->addFlag("interrupt");
	// the damage effect
	TCODParserStruct *damage=parser->newStructure("damage");
	damage->addProperty("minDmg",TCOD_TYPE_INT,true);
	damage->addProperty("maxDmg",TCOD_TYPE_INT,true);
	damage->addProperty("comboCoef",TCOD_TYPE_FLOAT,true);
	effect->addStructure(damage);
	// the blast effect
	TCODParserStruct *pushaway=parser->newStructure("blast");
	pushaway->addProperty("range",TCOD_TYPE_INT,true);
	effect->addStructure(pushaway);
	// the metamorphose effect
	TCODParserStruct *metamorphose=parser->newStructure("metamorphose");
	metamorphose->addProperty("into",TCOD_TYPE_STRING,true);
	effect->addStructure(metamorphose);
	// the summon effect
	TCODParserStruct *summon=parser->newStructure("summon");
	summon->addProperty("creature",TCOD_TYPE_STRING,true);
	summon->addProperty("minCount",TCOD_TYPE_INT,true);
	summon->addProperty("maxCount",TCOD_TYPE_INT,true);
	effect->addStructure(summon);
	// the rush effect
	TCODParserStruct *rush=parser->newStructure("rush");
	rush->addProperty("rushRange",TCOD_TYPE_INT,true);
	rush->addProperty("stunningChance",TCOD_TYPE_FLOAT,true);
	rush->addProperty("minStunningDelay",TCOD_TYPE_FLOAT,true);
	rush->addProperty("maxStunningDelay",TCOD_TYPE_FLOAT,true);
	rush->addProperty("minRushDmg",TCOD_TYPE_INT,true);
	rush->addProperty("maxRushDmg",TCOD_TYPE_INT,true);
	effect->addStructure(rush);
	// the condition effect
	TCODParserStruct *condition=parser->newStructure("condition");
	condition->addProperty("alias",TCOD_TYPE_STRING,false);
	condition->addProperty("duration",TCOD_TYPE_FLOAT,true);
	condition->addProperty("chance",TCOD_TYPE_FLOAT,true);
	condition->addProperty("amount",TCOD_TYPE_FLOAT,false);
	effect->addStructure(condition);
	// the light effect
	TCODParserStruct *light=parser->newStructure("light");
	light->addProperty("range",TCOD_TYPE_FLOAT,true);
	light->addProperty("col",TCOD_TYPE_STRING,true);
	light->addProperty("col2",TCOD_TYPE_STRING,false);
	light->addProperty("patternDelay",TCOD_TYPE_FLOAT,false);
	light->addProperty("pattern",TCOD_TYPE_STRING,false);
	light->addProperty("colorPattern",TCOD_TYPE_STRING,false);
	light->addProperty("mode",TCOD_TYPE_STRING,false);
	light->addFlag("randomRadius");
	light->addFlag("invsqrt");
	effect->addStructure(light);
	// the cast effect
	TCODParserStruct *cast=parser->newStructure("cast");
	cast->addProperty("item",TCOD_TYPE_STRING,true);
	effect->addStructure(cast);
}

bool EffectParser::parserNewStruct(TCODList<Effect *> *effectList, TCODParser *parser, const TCODParserStruct *def,const char *name) {
	const char *defname=def->getName();
	this->effectList=effectList;
	if (strcmp(defname,"damage") == 0 ) {
		// current skill deals damages
		currentEffect.damage=new DamageEffect();
		effectList->push(currentEffect.damage);
	} else if (strcmp(defname,"blast") == 0 ) {
		// current skill pushes the target away
		currentEffect.blast=new BlastEffect();
		effectList->push(currentEffect.blast);
	} else if (strcmp(defname,"metamorphose") == 0 ) {
		// current skill metamorphoses the target
		currentEffect.summon=new SummonEffect(true);
		effectList->push(currentEffect.summon);
	} else if (strcmp(defname,"summon") == 0 ) {
		// current skill summons new creatures
		currentEffect.summon=new SummonEffect(false);
		effectList->push(currentEffect.summon);
	} else if (strcmp(defname,"light") == 0 ) {
		// current skill creates a light
		currentEffect.light=new LightEffect();
		effectList->push(currentEffect.light);
	} else if (strcmp(defname,"cast") == 0 ) {
		// current skill creates an item
		currentEffect.cast=new CastEffect();
		effectList->push(currentEffect.cast);
	} else if (strcmp(defname,"condition") == 0 ) {
		// current skill adds condition
		currentEffect.condition=new ConditionEffect();
		effectList->push(currentEffect.condition);
		// look for the condition
		ConditionType *type=ConditionType::find(name);
		if (! type ) {
			parser->error("unknown condition '%s'",name);
			return false;
		}
		currentEffect.condition->conditionType = type->type;
	} else if (strcmp(defname,"rush") == 0 ) {
		// current skill is a rush
		currentEffect.rush=new RushEffect();
		effectList->push(currentEffect.rush);
	}
	return true;
}

bool EffectParser::parserFlag(TCODParser *parser, const char *name) {
	if ( strcmp(name,"interrupt") == 0 ) {
		// TODO
		//Effect *fx=new Effect();
		//fx->type=Effect::INTERRUPT;
		//effectList->push(fx);
	} else if ( strcmp(name,"parry") == 0 ) {
		// TODO
		//Effect *fx=new Effect();
		//fx->type=Effect::PARRY;
		//effectList->push(fx);
	} else if ( strcmp(name,"teleport") == 0 ) {
		// TODO
		//Effect *fx=new Effect();
		//fx->type=Effect::TELEPORT;
		//effectList->push(fx);
	} else if ( strcmp(name,"randomRadius") == 0 ) {
		currentEffect.light->light.flags|=ItemFeatureLight::RANDOM_RADIUS;
	} else if ( strcmp(name,"invsqrt") == 0 ) {
		currentEffect.light->light.flags|=ItemFeatureLight::INVSQRT;
	}
	return true;
}

bool EffectParser::parserProperty(TCODParser *parser, const char *propname, TCOD_value_type_t type, TCOD_value_t value) {
	// damage effect
	if ( strcmp(propname,"minDmg") == 0 ) {
		currentEffect.damage->minDmg = value.i;
	} else if ( strcmp(propname,"maxDmg") == 0 ) {
		currentEffect.damage->maxDmg = value.i;
	} else if ( strcmp(propname,"comboCoef") == 0 ) {
		currentEffect.damage->comboCoef = value.f;
	// condition effect
	} else if ( strcmp(propname,"alias") == 0 ) {
		currentEffect.condition->alias = strdup(value.s);
	} else if ( strcmp(propname,"chance") == 0 ) {
		currentEffect.condition->chance = value.f;
	} else if ( strcmp(propname,"duration") == 0 ) {
		currentEffect.condition->duration = value.f;
	} else if ( strcmp(propname,"amount") == 0 ) {
		currentEffect.condition->amount = value.f;
	// rush effect
	} else if ( strcmp(propname,"stunningChance") == 0 ) {
		currentEffect.rush->stunningChance = value.f;
	} else if ( strcmp(propname,"minStunningDelay") == 0 ) {
		currentEffect.rush->minStunningDelay = value.f;
	} else if ( strcmp(propname,"maxStunningDelay") == 0 ) {
		currentEffect.rush->maxStunningDelay = value.f;
	} else if ( strcmp(propname,"minRushDmg") == 0 ) {
		currentEffect.rush->minDmg = value.i;
	} else if ( strcmp(propname,"maxRushDmg") == 0 ) {
		currentEffect.rush->maxDmg = value.i;
	} else if ( strcmp(propname,"rushRange") == 0 ) {
		currentEffect.rush->range = value.i;
	// BLAST effect
	} else if ( strcmp(propname,"range") == 0 ) {
		if ( currentEffect.blast->type == Effect::BLAST )
			currentEffect.blast->range = value.i;
		else
			currentEffect.light->light.range=value.f;
	// metamorphose effect
	} else if ( strcmp(propname,"into") == 0 ) {
		currentEffect.summon->creatureTypeName = strdup(value.s);
	// summon effect
	} else if ( strcmp(propname,"creature") == 0 ) {
		currentEffect.summon->creatureTypeName = strdup(value.s);
		currentEffect.summon->creatureType = CreatureType::getType(CreatureType::getType(value.s)); //ooh this sucks
	} else if ( strcmp(propname,"minCount") == 0 ) {
		currentEffect.summon->minCount=value.i;
	} else if ( strcmp(propname,"maxCount") == 0 ) {
		currentEffect.summon->maxCount=value.i;
	// light effect
	} else if ( strcmp(propname,"col") == 0 ) {
		currentEffect.light->light.color=getHDRColorProperty(value.s);
	} else if ( strcmp(propname,"col2") == 0 ) {
		currentEffect.light->light.color2=getHDRColorProperty(value.s);
	} else if ( strcmp(propname,"patternDelay") == 0 ) {
		currentEffect.light->light.patternDelay=value.f;
	} else if ( strcmp(propname,"pattern") == 0 ) {
		currentEffect.light->light.pattern=strdup(value.s);
	} else if ( strcmp(propname,"mode") == 0 ) {
		if ( strcmp(value.s,"mul") == 0 ) currentEffect.light->light.flags|=ItemFeatureLight::MODE_MUL;
		else if ( strcmp(value.s,"max") == 0 ) currentEffect.light->light.flags|=ItemFeatureLight::MODE_MAX;
		else if ( strcmp(value.s,"add") == 0 ) ;
		else parser->error("Unknown light mode '%s'. Expect add,mul or max",value.s);
	} else if ( strcmp(propname,"colorPattern") == 0 ) {
		currentEffect.light->light.colorPattern=strdup(value.s);
	// cast effect
	} else if ( strcmp(propname,"item") == 0 ) {
		currentEffect.cast->itemType = ItemType::getType(value.s);
		if (!currentEffect.cast->itemType ) {
			parser->error("Unknown item type '%s'",value.s);
		}
	}
	return true;
}

bool EffectParser::parserEndStruct(TCODParser *parser, const TCODParserStruct *def, const char *name) {
	return true;
}

DamageEffect::DamageEffect() : Effect(DAMAGE) {}
BlastEffect::BlastEffect() : Effect(BLAST) {}
SummonEffect::SummonEffect(bool meta) : Effect(meta?METAMORPHOSE:SUMMON),
	creatureTypeName(NULL),creatureType(NULL),minCount(1),maxCount(1) {}
LightEffect::LightEffect() : Effect(LIGHT),light(0,TCODColor::white,false,0.0f,NULL,TCODColor::black,NULL) {}
CastEffect::CastEffect() : Effect(CAST),itemType(NULL) {}
ConditionEffect::ConditionEffect() : Effect(CONDITION),alias(NULL),chance(1.0f),amount(0.0f) {}
RushEffect::RushEffect() : Effect(RUSH) {}

bool DamageEffect::execute(int x, int y, Creature *cr, Creature *caster, Skill *skill) {
	// deal damages to the target
	int dmg = TCODRandom::getInstance()->getInt(minDmg,maxDmg);
	// combo ?
	// TODO
	//if ( combo ) {
	//	dmg = (int)(dmg * comboCoef);
	//}
	// fury ?
	// TODO
	//Condition *fury=caster->getCondition(ConditionType::FURY);
	//if ( fury ) {
	//	dmg=(int)(dmg*(1.0f+fury->amount));
	//}

	if (cr->hasCondition(ConditionType::BLOCK)) {
		Condition *cond=cr->getCondition(ConditionType::BLOCK);
		float rnd=TCODRandom::getInstance()->getFloat(0.0f,1.0f);
		if ( rnd <= cond->amount ) {
			// attack blocked
			// TODO
			//new Bubble(cr->x,cr->y-1, cr->isPlayer ? TCODColor::lightRed : TCODColor::lightBlue, "blocked");
			dmg=0;
			//cr->curSta+=5;
			//cr->curSta = MIN(cr->maxSta,cr->curSta);
		}
	}
	if ( dmg > 0 ) {
		//Player *player = gameEngine->player;
		// TODO
		//if ( player->adrenalin < adrenalinCount ) player->adrenalin++;
		cr->takeDamage(dmg);
		//cr->hit = hitDelay;
		//if ( type->weapon ) gameEngine->addBloodStain(cr->x,cr->y,dmg);
	}
	return false;
}
	//static float giveDamageStamina=config.getFloatProperty("config.gameplay.giveDamageStamina");
	//static float criticalFailureChance=config.getFloatProperty("config.gameplay.criticalFailureChance");
	//static int adrenalinCount=config.getIntProperty("config.gameplay.adrenalinCount");
	//static float hitDelay=config.getFloatProperty("config.display.creatureHitDelay");

// TODO InterruptEffect

		// interrupt every target skills currently casted
		/*
		for ( Skill **sk=cr->skills.begin(); sk != cr->skills.end(); sk++) {
			if ( (*sk)->castTime > 0.0f && ! ((*sk)->type->flags & SkillType::NO_INTERRUPT)) {
				(*sk)->interrupt();
			}
		}
		*/
// TODO BlastEffect
bool BlastEffect::execute(int x, int y, Creature *cr, Creature *caster, Skill *skill) {
	// pushes the target away
	float dx=cr->x - (caster ? caster->x : (float)x);
	float dy=cr->y - (caster ? caster->y : (float)y);
	int range=abs(dx)+abs(dy);
	// check range
	if ( range <= range ) {
		float l = Entity::fastInvSqrt(dx*dx+dy*dy);
		cr->speed=10.0f;
		cr->duration = 1.0f/(cr->speed*l);
		cr->dx = dx * l;
		cr->dy = dy * l;
	}
	return true;
}

bool ConditionEffect::execute(int x, int y, Creature *cr, Creature *caster, Skill *skill) {
	// adds a condition on the target
	float rnd=TCODRandom::getInstance()->getFloat(0.0f,1.0f);
	if ( rnd <= chance ) {
		ConditionType *condType=ConditionType::get(conditionType);
		if ( condType->check(cr) ) {
			Condition *cond=new Condition(conditionType,duration, amount, alias);
			cond->applyTo(cr);
		}
	}
	return false;
}

	// TODO TeleportEffect
		// teleport the target at a random place
		/*
		int rx=TCODRandom::getInstance()->getInt(0,ARENA_SIZE-1);
		int ry=TCODRandom::getInstance()->getInt(0,ARENA_SIZE-1);
		Creature *cr2=gameEngine->getCreature(rx,ry);
		while (cr2) {
			rx=rx+1;
			if ( rx == ARENA_SIZE ) {
				rx=0;ry=ry+1;
				if (ry == ARENA_SIZE ) {
					ry=0;
				}
			}
			cr2=gameEngine->getCreature(rx,ry);
		}
		cr->x=rx;
		cr->y=ry;
		*/
	// TODO RushEffect
bool RushEffect::execute(int x, int y, Creature *cr, Creature *caster, Skill *skill) { return false; }
		//caster->rush=true;
		//caster->rushEffect=fx;

bool SummonEffect::execute(int x, int y, Creature *cr, Creature *caster, Skill *skill) {
	if ( type == Effect::METAMORPHOSE ) {
		// metamorphose = summon + the caster dies
		cr->setLife(0);
	}
	int count=TCODRandom::getInstance()->getInt(minCount,maxCount);
	for (int i=0; i < count; i++ ) {
		// TODO replace by getCreature(CreatureType *)
		Creature *summoned=Creature::getCreature(CreatureType::getType(creatureType->getName()));
		if ( x == -1 ) {
			x=(int)cr->x;
			y=(int)cr->y;
		}
		gameEngine->dungeon->getClosestWalkable(&x,&y,true,false);
		summoned->setPos(x,y);
		gameEngine->dungeon->addCreature(summoned);
	}
	return false;
}

bool LightEffect::execute(int x, int y, Creature *cr, Creature *caster, Skill *skill) {
	// put a light on target
	if (! cr->light) {
		ExtendedLight *el=new ExtendedLight();
		cr->light=el;
		if ( skill->type->flags & SkillType::EXECUTE_WHEN_CASTED ) {
			// light grows during casting phase
			if ( cr->isPlayer() ) {
				el->range=gameEngine->player->noLight.range;
			} else {
				el->range=0.5f;
			}
			el->color=(TCODColor)(light.color)*0.02f;
		} else {
			// light appears suddenly
			el->range=light.range;
			el->color=(TCODColor)light.color;
		}
		el->setup(cr->light->color,light.patternDelay,light.pattern,NULL);
		el->flags=0;
		if ( light.flags & ItemFeatureLight::RANDOM_RADIUS ) el->flags|=Light::RANDOM_RAD;
		if ( light.flags & ItemFeatureLight::INVSQRT ) el->flags|=Light::INVSQRT;
		if ( light.flags & ItemFeatureLight::MODE_MUL ) el->drawMode = Light::MODE_MUL;
		else if ( light.flags & ItemFeatureLight::MODE_MAX ) el->drawMode = Light::MODE_MAX;

		gameEngine->dungeon->addLight(cr->light);
		if ( cr->isPlayer())  {
			gameEngine->dungeon->removeLight(&gameEngine->player->noLight);
		}
	} else {
		gameEngine->dungeon->removeLight(cr->light);
		delete cr->light;
		cr->light=NULL;
		if ( cr->isPlayer() ) {
			gameEngine->dungeon->addLight(&gameEngine->player->noLight);
		}
	}
	return false;
}


bool CastEffect::execute(int x, int y, Creature *cr, Creature *caster, Skill *skill) {
	Item *projectile=Item::getItem(itemType,cr->x,cr->y);
	ItemFeatureAttack *projectileAttack=(ItemFeatureAttack *)itemType->getFeature(ItemFeature::ATTACK);
	if (projectileAttack && (projectileAttack->flags & ItemFeatureAttack::WEAPON_PROJECTILE)) {
		float targetx,targety;
		if ( cr->isPlayer()) {
			targetx=gameEngine->mousex+gameEngine->xOffset;
			targety=gameEngine->mousey+gameEngine->yOffset;
		} else {
			targetx=gameEngine->player->x;
			targety=gameEngine->player->y;
		}
		projectile->owner=cr;
		projectile->dx=targetx-cr->x;
		projectile->dy=targety-cr->y;
		float l=Entity::fastInvSqrt(projectile->dx*projectile->dx+projectile->dy*projectile->dy);
		projectile->dx*=l;
		projectile->dy*=l;
		projectile->duration=0.0f; // TODO parameter
		projectile->x+=projectile->dx;
		projectile->y+=projectile->dy;
		projectile->speed=projectileAttack->speed;
	}
	gameEngine->dungeon->addItem(projectile);
	projectile->owner=cr;
	return false;
}
