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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main.hpp"

TCODList<SkillType *> SkillType::list;

class SkillParser : public ITCODParserListener {
public :
	SkillParser() : redirect(false) {}
	// IParserListener interface
	bool parserNewStruct(TCODParser *parser, const TCODParserStruct *def,const char *name);
	bool parserFlag(TCODParser *parser, const char *name);
	bool parserProperty(TCODParser *parser, const char *propname, TCOD_value_type_t type, TCOD_value_t value);
	bool parserEndStruct(TCODParser *parser, const TCODParserStruct *def, const char *name);
	void error(const char *msg);
private :
	SkillType *currentSkill;
	union {
		DamageEffect *damage;
		BlastEffect *blast;
		ConditionEffect *condition;
		RushEffect *rush;
		SummonEffect *summon;
		LightEffect *light;
		CastEffect *cast;
	} currentEffect;
	EffectParser effectParser;
	bool redirect;
	TCODList<Effect *> *effectList;
};

bool SkillParser::parserNewStruct(TCODParser *parser, const TCODParserStruct *def,const char *name) {
	const char *defname=def->getName();
	if ( redirect ) return effectParser.parserNewStruct(effectList,parser,def,name);
	if ( strcmp(defname,"skill")==0) {
		// new skill definition
		currentSkill = new SkillType(strdup(name));
		effectList=NULL;
	} else if (strcmp(defname,"targetEffects") == 0 ) {
		effectList = &currentSkill->targetEffects;
		redirect=true;
	} else if (strcmp(defname,"adrenalinEffects") == 0 ) {
		effectList = &currentSkill->adrenalinEffects;
		redirect=true;
	} else if (strcmp(defname,"wielderEffects") == 0 ) {
		effectList = &currentSkill->wielderEffects;
		redirect=true;
	}
	return true;
}

bool SkillParser::parserFlag(TCODParser *parser, const char *name) {
	if ( redirect ) return effectParser.parserFlag(parser,name);
	if ( strcmp(name,"needMainHand") == 0 ) {
		currentSkill->flags |= SkillType::NEED_MAIN;
	} else if ( strcmp(name,"needWalk") == 0 ) {
		currentSkill->flags |= SkillType::NEED_WALK;
	} else if ( strcmp(name,"noInterrupt") == 0 ) {
		currentSkill->flags |= SkillType::NO_INTERRUPT;
	} else if ( strcmp(name,"notPrinted") == 0 ) {
		currentSkill->flags |= SkillType::NOT_PRINTED;
	} else if ( strcmp(name,"opponentOnly") == 0 ) {
		currentSkill->flags |= SkillType::OPPONENT_ONLY;
	} else if ( strcmp(name,"useAllStamina") == 0 ) {
		currentSkill->flags |= SkillType::USE_ALL_STAMINA;
	} else if ( strcmp(name,"onoff") == 0 ) {
		currentSkill->flags |= SkillType::ONOFF;
	} else if ( strcmp(name,"executeWhenCasted") == 0 ) {
		currentSkill->flags |= SkillType::EXECUTE_WHEN_CASTED;
	} else if ( strcmp(name,"ticktock") == 0 ) {
		currentSkill->flags |= SkillType::TICKTOCK;
	}
	return true;
}

bool SkillParser::parserProperty(TCODParser *parser, const char *propname, TCOD_value_type_t type, TCOD_value_t value) {
	if ( redirect ) return effectParser.parserProperty(parser,propname,type,value);
	if ( strcmp(propname,"castTime") == 0 ) {
		currentSkill->castTime = value.f;
	} else if ( strcmp(propname,"reloadTime") == 0 ) {
		currentSkill->reloadTime = value.f;
	} else if ( strcmp(propname,"description") == 0 ) {
		currentSkill->description = strdup(value.s);
	} else if ( strcmp(propname,"requires") == 0 ) {
		SkillType *required=SkillType::find(value.s);
		if ( ! required ) parser->error("unknown skill %s",value.s);
		currentSkill->required = required;
	} else if ( strcmp(propname,"resourceType") == 0 ) {
		if ( strcmp(value.s,"mana") == 0 ) currentSkill->resourceType=StatusResource::MANA;
		else parser->error("Unknown resource type %s",value.s);
	} else if ( strcmp(propname,"resourceCost") == 0 ) {
		currentSkill->resourceCost=value.f;
	// targetType value list
	} else if ( strcmp(propname,"targetType") == 0 ) {
		if ( strcmp(value.s,"caster") == 0 ) {
			currentSkill->targetType = SkillType::CASTER;
		} else if ( strcmp(value.s,"opponent") == 0 ) {
			currentSkill->targetType = SkillType::OPPONENT;
		} else if ( strcmp(value.s,"range1") == 0 ) {
			currentSkill->targetType = SkillType::RANGE1;
		} else if ( strcmp(value.s,"range2") == 0 ) {
			currentSkill->targetType = SkillType::RANGE2;
		} else if ( strcmp(value.s,"range3") == 0 ) {
			currentSkill->targetType = SkillType::RANGE3;
		}
	}
	return true;
}

bool SkillParser::parserEndStruct(TCODParser *parser, const TCODParserStruct *def, const char *name) {
	if (redirect) {
		if ( strcmp(def->getName(),"targetEffects" ) != 0
			&& strcmp(def->getName(),"adrenalinEffects" ) != 0
			&& strcmp(def->getName(),"wielderEffects" ) != 0 ) {
			return effectParser.parserEndStruct(parser,def,name);
		} else {
			redirect=false;
		}
	}
	return true;
}

void SkillParser::error(const char *msg) {
	fprintf(stderr,"%s",msg);
	std::abort();
}

SkillType::SkillType(char *name) :
	name(name),castTime(0.0f),targetType(OPPONENT),
	required(NULL), description(""),flags(0),nbCast(0)
	{
	id = list.size();
	list.push(this);
}

bool SkillType::hasEffect(Effect::Type type) {
	return ( getEffect(type) != NULL );
}

bool SkillType::hasConditionEffect(ConditionType::Type type) {
	for ( Effect **it=targetEffects.begin(); it != targetEffects.end(); it ++) {
		if ( (*it)->type == Effect::CONDITION && ((ConditionEffect *)(*it))->conditionType == type ) return true;
	}
	return false;
}

Effect *SkillType::getEffect(Effect::Type type) {
	for ( Effect **it=targetEffects.begin(); it != targetEffects.end(); it ++) {
		if ( (*it)->type == type ) return *it;
	}
	for ( Effect **it=wielderEffects.begin(); it != wielderEffects.end(); it ++) {
		if ( (*it)->type == type ) return *it;
	}
	for ( Effect **it=adrenalinEffects.begin(); it != adrenalinEffects.end(); it ++) {
		if ( (*it)->type == type ) return *it;
	}
	return NULL;
}


SkillType *SkillType::find(const char *name) {
	if ( list.size() == 0 ) init();
	// find a skill from its name
	for (SkillType **sk=list.begin(); sk != list.end(); sk++) {
		if ( strcmp((*sk)->name,name) == 0 ) return *sk;
	}
	return NULL;
}

const char *SkillType::getUndefinedArticle() {
	if ( strchr("aeiou",name[0]) ) return "an ";
	return "a ";
}

bool SkillType::init() {
	static const char *targetTypes[] = {
		"caster",
		"opponent",
		"range1",
		"range2",
		"range3",
		NULL
	};
	Item::init(); // ensure item types are defined
	TCODParser parser;
	// the skill entity
	TCODParserStruct *skill=parser.newStructure("skill");
	skill->addProperty("castTime",TCOD_TYPE_FLOAT,true);
	skill->addProperty("reloadTime",TCOD_TYPE_FLOAT,true);
	skill->addProperty("description",TCOD_TYPE_STRING,false);
	skill->addProperty("resourceType",TCOD_TYPE_STRING,false);
	skill->addProperty("resourceCost",TCOD_TYPE_FLOAT,false);
	skill->addProperty("requires",TCOD_TYPE_STRING,false);
	skill->addValueList("targetType",targetTypes,true);
	skill->addFlag("needMainHand");
	skill->addFlag("needWalk");
	skill->addFlag("noInterrupt");
	skill->addFlag("notPrinted");
	skill->addFlag("opponentOnly");
	skill->addFlag("useAllStamina");
	skill->addFlag("ticktock");
	skill->addFlag("onoff");
	skill->addFlag("executeWhenCasted");
	// the effects & adrenalinEffects sub-entities
	TCODParserStruct *targetEffects=parser.newStructure("targetEffects");
	Effect::configureParser(&parser,targetEffects);
	skill->addStructure(targetEffects);
	TCODParserStruct *adrenalinEffects=parser.newStructure("adrenalinEffects");
	Effect::configureParser(&parser,adrenalinEffects);
	skill->addStructure(adrenalinEffects);
	TCODParserStruct *wielderEffects=parser.newStructure("wielderEffects");
	Effect::configureParser(&parser,wielderEffects);
	skill->addStructure(wielderEffects);
	parser.run("data/cfg/skills.txt",new SkillParser());

	return true;
}

bool SkillType::check(Creature *cr) {
	// TODO
	/*
	if ( classNeeded ) {
		// check if the creature has a weapon of class classNeeded
		if ( ( ! cr->mainHand || ! cr->mainHand->isA(classNeeded) ) &&
		  ( ! cr->offHand || ! cr->offHand->isA(classNeeded) ) ) return false;
	}
	*/
	if ( flags & NEED_WALK ) {
		// check if the creature can walk
		if ( cr->hasCondition(ConditionType::PARALIZED)
			|| cr->hasCondition(ConditionType::CRIPPLED) ) return false;
	}
	if ( flags & NEED_MAIN ) {
		// check if the creature has a weapon in main hand
		if ( ! cr->mainHand || ! cr->mainHand->isA("weapon") ) return false;
	}
	return true;
}

const char *SkillType::getDescription() {
	static char buf[1024];
	char tmp[256];
	strcpy(buf,name);
	strcat(buf," :\n");
	sprintf(tmp," \x01b %gs \x01a %gs",
		castTime,reloadTime);
	strcat(buf,tmp);
	if ( resourceCost > 0.0f ) {
		sprintf(tmp," %s %g\n", StatusResource::getName(resourceType),resourceCost);
		strcat(buf,tmp);
	}
	strcat(buf,"\n");
	if ( description && description[0] != 0 ) {
		strcat(buf,description);
		strcat(buf,"\n");
	}
	if ( hasEffect(Effect::DAMAGE) ) {
		DamageEffect *fx=(DamageEffect *)getEffect(Effect::DAMAGE);
		int minDmg = fx->minDmg;
		int maxDmg = fx->maxDmg;
		sprintf(tmp," damages      : %d - %d\n",minDmg,maxDmg);
		strcat(buf,tmp);
	}
	bool hasCond=false;
	for ( Effect **fx=targetEffects.begin(); fx != targetEffects.end(); fx++) {
		if ((*fx)->type == Effect::CONDITION ) {
			ConditionEffect *condFx=(ConditionEffect *)(*fx);
			sprintf(tmp," %s",condFx->alias ? condFx->alias :
				ConditionType::get(condFx->conditionType)->name);
			strcat(buf,tmp);
			if ( condFx->chance != 1.0f ) {
				sprintf(tmp," %d%%%%",(int)(condFx->chance*100));
				strcat(buf,tmp);
			}
			sprintf(tmp," %gs",condFx->duration);
			strcat(buf,tmp);
			if ( condFx->amount != 0.0f ) {
				sprintf(tmp," amount %g",condFx->amount);
				strcat(buf,tmp);
			}
			hasCond=true;
		}
	}
	if ( hasCond ) strcat(buf,"\n");
	if ( !adrenalinEffects.isEmpty()) {
		strcat(buf,"\nAdrenalin additional effects :\n");
		for ( Effect **fx=adrenalinEffects.begin(); fx != adrenalinEffects.end(); fx++) {
			if ((*fx)->type == Effect::CONDITION ) {
				ConditionEffect *condFx=(ConditionEffect *)(*fx);
				sprintf(tmp," %s",condFx->alias ? condFx->alias :
					ConditionType::get(condFx->conditionType)->name);
				strcat(buf,tmp);
				if ( condFx->chance != 1.0f ) {
					sprintf(tmp," %d%%%%",(int)(condFx->chance*100));
					strcat(buf,tmp);
				}
				sprintf(tmp," %gs",condFx->duration);
				strcat(buf,tmp);
				if ( condFx->amount != 0.0f ) {
					sprintf(tmp," amount %g",condFx->amount);
					strcat(buf,tmp);
				}
				strcat(buf,"\n");
			} else if ( (*fx)->type == Effect::DAMAGE ) {
				DamageEffect *dmgFx=(DamageEffect *)(*fx);
				sprintf(tmp," damages      : %d - %d\n",dmgFx->minDmg,dmgFx->maxDmg);
				strcat(buf,tmp);
			}
		}
	}
	return buf;
}

Skill::Skill(SkillType *type) :
	type(type), castTime(0.0f), combo(false),isOn(false),caster(NULL),target(NULL) {
	// TODO
	//for ( Effect **it=type->wielderEffects.begin();it!=type->wielderEffects.end();it++) {
	//	Condition *cond=new Condition((*it)->condition.type,
	//		(*it)->condition.duration,(*it)->condition.amount);
	//	conditions.push(cond);
	//}
}

char *Skill::getName() {
	static char buf[128];
	strcpy(buf,type->name);
	return buf;
}

bool Skill::isReady() {
	if ( castTime != 0.0f ) return false;
	if ( ( type->flags & SkillType::NEED_MAIN ) ) {
		if ( ! caster->mainHand
			// TODO
			//|| ! caster->mainHand->isReady()
		) return false;
	}
	return true;
}

bool Skill::update(float elapsed) {
	for (Condition **it=conditions.begin(); it!=conditions.end(); it++) {
		(*it)->target=caster;
		if ((*it)->update(elapsed)) {
			// reset the condition (permanent condition)
			(*it)->duration=(*it)->initialDuration;
			(*it)->curAmount=0.0f;
		}
	}
	if ( isOn && type->resourceCost > 0.0f ) {
		float cost=type->resourceCost * elapsed;
		caster->addResource(type->resourceType,-cost);
		if (caster->getResourceValue(type->resourceType)<= 0.0f) {
			cast(); // no more resource. cast again to turn it off
		}
	}
	if ( castTime > 0.0f ) {
		// casting
		castTime -= elapsed;
		if ( castTime <= 0.0f ) {
			// TODO
			/*
			if ( type->flags & SkillType::TICKTOCK ) {
				caster->ticktockSkill=NULL;
			}
			*/
			castTime = 0.0f;
			/*
			if ( type->staminaCost < 0 ) {
				caster->curSta -= type->staminaCost;
				caster->curSta = MIN(caster->maxSta,caster->curSta);
			}
			*/
			if (type->flags & SkillType::ONOFF) {
				isOn=!isOn;
			}
			if ( (type->flags & SkillType::EXECUTE_WHEN_CASTED ) &&
				(isOn || (type->flags & SkillType::ONOFF) ==0 ) ) {
				castTime=-type->reloadTime;
			} else return execute();
			return true;
		}
		// TODO make something clean!!
		// patterns when turning light on/off should be handled directly in Light class
		if (type->flags & SkillType::EXECUTE_WHEN_CASTED) {
			LightEffect *fx=(LightEffect *)type->getEffect(Effect::LIGHT);
			if ( fx && caster->light ) {
				float coef=(1.0f-castTime/type->castTime);
				if (isOn) coef = 1.0f-coef;
				caster->light->range = (fx->light.range-gameEngine->player->noLight.range) * coef+gameEngine->player->noLight.range;
				caster->light->color=(TCODColor)(fx->light.color)*MIN(1.0f,coef+0.2f);
			}
		}
	} else if ( castTime < 0.0f ) {
		// reloading
		castTime += elapsed;
		if ( castTime > 0.0f ) {
			castTime = 0.0f;
		}
	}
	return false;
}

bool Skill::cast() {
	static float comboDelay=config.getFloatProperty("config.gameplay.comboDelay");
	//static float parryDelay=config.getFloatProperty("config.gameplay.parryDelay");
	static float castHandicap=config.getFloatProperty("config.gameplay.creatureCastHandicap");

	// check conditions
	// combo ?
	if ( castTime < 0.0f && ABS(-castTime-type->reloadTime/2) < comboDelay && type->hasEffect(Effect::DAMAGE) ) {
		combo = true;
		castTime=0.0f;
	} else if ( castTime != 0.0f ) return false; // already casting or reloading
	if ( (type->flags & SkillType::NEED_MAIN) && caster->mainHand == NULL ) return false; // need a weapon in main hand
	// TODO
	//if ( type->staminaCost > caster->curSta ) return false; // not enough stamina
	if ( type->targetType == SkillType::OPPONENT && target == NULL ) return false; // no target selected
	if ( type->targetType == SkillType::OPPONENT ) {
		if ( ABS(target->x - caster->x) > 1 ) return false; // target too far away
		if ( ABS(target->y - caster->y) > 1 ) return false; // target too far away
	}
	// TODO
	/*
	if ( type->classNeeded ) {
		// check if the caster has a weapon of class classNeeded
		if (( ! caster->mainHand || ! caster->mainHand->isA(type->classNeeded) )
			&& ( ! caster->offHand || ! caster->offHand->isA(type->classNeeded) ) ) return false;
	}
	*/

	//caster->idle=0.0f;
	// TODO
	/*
	if ( type->flags & SkillType::TICKTOCK ) {
		caster->ticktockSkill=this;
	}
	*/
	if ( (type->flags & SkillType::ONOFF) == 0 &&  type->resourceCost > 0.0f ) {
		if ( caster->getResourceValue(type->resourceType) >= type->resourceCost ) {
			caster->addResource(type->resourceType,-type->resourceCost);
		} else return false; // not enough resource to cast
	}
	// start casting
	castTime=type->castTime;
	if ( ! caster->isPlayer() ) castTime*=castHandicap;
	if ( type->flags & SkillType::EXECUTE_WHEN_CASTED ) {
		if ( !isOn || (type->flags & SkillType::ONOFF) == 0 ) execute();
	}
	// TODO
	/*
	if ( type->hasEffect(Effect::PARRY) ) {
		// check if we parry an attack
		for ( Skill **sk=target->skills.begin(); sk != target->skills.end(); sk ++ ) {
			if ( (*sk)->type->weapon && (*sk)->castTime > 0.0
				// can only parry in the last parryDelay seconds of the casting phase
				&& (*sk)->castTime <= parryDelay
				) {
				// parry this attack
				(*sk)->interrupt();
				this->interrupt();
				if ( caster->isPlayer ) {
					gameEngine->stats.playerParry();
				} else {
					gameEngine->stats.creatureParry(caster);
				}
				new Bubble(target->x,target->y-1, target->isPlayer ? TCODColor::lightRed :
					TCODColor::lightBlue, "parry");
				break;
			}
		}
	}
	*/
	return true;
}

void Skill::interrupt() {
	if ( ( type->flags & SkillType::NO_INTERRUPT ) == 0 ) {
		castTime=0.0f;
		// TODO
		/*
		if ( type->flags & SkillType::TICKTOCK ) {
			caster->ticktockSkill=NULL;
		}
		*/
	}
}

// private stuff

bool Skill::applyEffectOnTarget(Effect *fx, Creature *cr) {

	return fx->execute(-1,-1,cr,caster,this);
}

bool Skill::executeOnTarget(Creature *cr) {
	bool ret=false;
	for ( Effect **fx=type->targetEffects.begin(); fx!=type->targetEffects.end(); fx++) {
		if (applyEffectOnTarget(*fx,cr)) ret=true;
	}
	// TODO
	/*
	if ( caster->isPlayer && ((Player *)caster)->adrenalinAttack ) {
		for ( Effect **fx=type->adrenalinEffects.begin(); fx!=type->adrenalinEffects.end(); fx++) {
			if (applyEffectOnTarget(*fx,cr)) ret=true;
		}
		((Player *)caster)->adrenalin = 0;
		((Player *)caster)->adrenalinAttack = false;
	}
	*/
	combo=false;

	return ret;
}
bool Skill::execute() {
	// TODO
	//if ( type->flags & SkillType::USE_ALL_STAMINA ) caster->curSta=0;
	if ( caster->isPlayer() && (!(type->flags & SkillType::ONOFF) || isOn)) type->nbCast++;
	if ( type->targetType == SkillType::CASTER ) {
		if (executeOnTarget(caster) ) return true;
	} else if ( type->targetType == SkillType::OPPONENT ) {
		if ( ABS(target->x - caster->x) > 2 ) return true; // target too far away
		if ( ABS(target->y - caster->y) > 2 ) return true; // target too far away
		if (executeOnTarget(target)) return true;
	} else if ( type->targetType >= SkillType::RANGE1 && type->targetType <= SkillType::RANGE3 ) {
		// range 1 creatures
		Creature *cr;
		int cx=(int)(caster->x);
		int cy=(int)(caster->y);
		cr = gameEngine->dungeon->getCreature(cx-1,cy);
		if ( cr && executeOnTarget(cr) ) return true;
		cr = gameEngine->dungeon->getCreature(cx+1,cy);
		if ( cr && executeOnTarget(cr) ) return true;
		cr = gameEngine->dungeon->getCreature(cx,cy-1);
		if ( cr && executeOnTarget(cr) ) return true;
		cr = gameEngine->dungeon->getCreature(cx,cy+1);
		if ( cr && executeOnTarget(cr) ) return true;
		cr = gameEngine->dungeon->getCreature(cx+1,cy+1);
		if ( cr && executeOnTarget(cr) ) return true;
		cr = gameEngine->dungeon->getCreature(cx-1,cy+1);
		if ( cr && executeOnTarget(cr) ) return true;
		cr = gameEngine->dungeon->getCreature(cx+1,cy-1);
		if ( cr && executeOnTarget(cr) ) return true;
		cr = gameEngine->dungeon->getCreature(cx-1,cy-1);
		if ( cr && executeOnTarget(cr) ) return true;
		// range 2 creatures
		if (type->targetType >= SkillType::RANGE2 ) {
			cr = gameEngine->dungeon->getCreature(cx+2,cy);
			if ( cr && executeOnTarget(cr) ) return true;
			cr = gameEngine->dungeon->getCreature(cx-2,cy);
			if ( cr && executeOnTarget(cr) ) return true;
			cr = gameEngine->dungeon->getCreature(cx,cy-2);
			if ( cr && executeOnTarget(cr) ) return true;
			cr = gameEngine->dungeon->getCreature(cx,cy+2);
			if ( cr && executeOnTarget(cr) ) return true;
			// range 3 creatures
			if (type->targetType >= SkillType::RANGE3 ) {
				cr = gameEngine->dungeon->getCreature(cx+2,cy-1);
				if ( cr && executeOnTarget(cr) ) return true;
				cr = gameEngine->dungeon->getCreature(cx+2,cy+1);
				if ( cr && executeOnTarget(cr) ) return true;
				cr = gameEngine->dungeon->getCreature(cx-2,cy-1);
				if ( cr && executeOnTarget(cr) ) return true;
				cr = gameEngine->dungeon->getCreature(cx-2,cy+1);
				if ( cr && executeOnTarget(cr) ) return true;
				cr = gameEngine->dungeon->getCreature(cx+1,cy-2);
				if ( cr && executeOnTarget(cr) ) return true;
				cr = gameEngine->dungeon->getCreature(cx-1,cy-2);
				if ( cr && executeOnTarget(cr) ) return true;
				cr = gameEngine->dungeon->getCreature(cx+1,cy+2);
				if ( cr && executeOnTarget(cr) ) return true;
				cr = gameEngine->dungeon->getCreature(cx-1,cy+2);
				if ( cr && executeOnTarget(cr) ) return true;
				cr = gameEngine->dungeon->getCreature(cx+3,cy);
				if ( cr && executeOnTarget(cr) ) return true;
				cr = gameEngine->dungeon->getCreature(cx-3,cy);
				if ( cr && executeOnTarget(cr) ) return true;
				cr = gameEngine->dungeon->getCreature(cx,cy+3);
				if ( cr && executeOnTarget(cr) ) return true;
				cr = gameEngine->dungeon->getCreature(cx,cy-3);
				if ( cr && executeOnTarget(cr) ) return true;
			}
		}
	}

	if ( (type->flags & SkillType::EXECUTE_WHEN_CASTED) == 0 ) castTime=-type->reloadTime;
	return false;
}
