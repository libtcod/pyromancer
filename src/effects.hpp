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

class CreatureType;
class Creature;

// due to dependency hell, Effect is declared in item_feature.hpp

class DamageEffect : public Effect {
public :
	int minDmg, maxDmg;
	float comboCoef;
	DamageEffect();
	bool execute(int x, int y, Creature *cr, Creature *caster, Skill *skill);
};

class BlastEffect : public Effect {
public :
	int range;
	BlastEffect();
	bool execute(int x, int y, Creature *cr, Creature *caster, Skill *skill);
};

class ConditionEffect : public Effect {
public :
	const char *alias;
	ConditionType::Type conditionType;
	float duration;
	float chance;
	float amount;
	ConditionEffect();
	bool execute(int x, int y, Creature *cr, Creature *caster, Skill *skill);
}; 

class RushEffect : public Effect {
public :
	int range;
	float stunningChance;
	float minStunningDelay,maxStunningDelay;
	int minDmg, maxDmg;
	RushEffect();
	bool execute(int x, int y, Creature *cr, Creature *caster, Skill *skill);
};

class SummonEffect : public Effect {
public :
	const char *creatureTypeName;
	CreatureType *creatureType;
	int minCount, maxCount; // used only by summon
	SummonEffect(bool meta);
	bool execute(int x, int y, Creature *cr, Creature *caster, Skill *skill);
};

class LightEffect : public Effect {
public : 
	ItemFeatureLight light;
	LightEffect();
	bool execute(int x, int y, Creature *cr, Creature *caster, Skill *skill);
};

class CastEffect : public Effect {
public :
	ItemType *itemType;
	CastEffect();
	bool execute(int x, int y, Creature *cr, Creature *caster, Skill *skill);
};

class EffectParser {
public :
	bool parserNewStruct(TCODList<Effect *> *effectList, TCODParser *parser, const TCODParserStruct *def,const char *name);
	bool parserFlag(TCODParser *parser, const char *name);
	bool parserProperty(TCODParser *parser, const char *propname, TCOD_value_type_t type, TCOD_value_t value);
	bool parserEndStruct(TCODParser *parser, const TCODParserStruct *def, const char *name);
private :
	union {
		DamageEffect *damage;
		BlastEffect *blast;
		ConditionEffect *condition;
		RushEffect *rush;
		SummonEffect *summon;
		LightEffect *light;
		CastEffect *cast;
	} currentEffect;
	TCODList<Effect *> *effectList;
};
