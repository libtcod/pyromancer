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

class Creature;

class SkillType {
public :
	enum Flags {
		NEED_MAIN = 1,
		NO_INTERRUPT = 2,
		NOT_PRINTED = 4,
		OPPONENT_ONLY=8,
		USE_ALL_STAMINA=16,
		NEED_WALK=32,
		TICKTOCK=64,
		ONOFF=128,
		EXECUTE_WHEN_CASTED=256,
	};
	enum TargetType {
		CASTER,
		OPPONENT,
		RANGE1,
		RANGE2,
		RANGE3,
	};

	char *name;
	int id;
	float castTime, reloadTime;
	StatusResource::Type resourceType;
	float resourceCost;
	TargetType targetType;
	TCODList<Effect *> targetEffects; // effect on target when casted
	TCODList<Effect *> wielderEffects; // for weapons/items, effect on the wielder
	TCODList<Effect *> adrenalinEffects; // effect on target when casted under adrenaline

	SkillType *required;
	const char *description;
	int flags; // combination of Flags
	int nbCast;

	static TCODList<SkillType *> list;

	static SkillType *find(const char *name);

	bool hasEffect(Effect::Type type);
	bool hasConditionEffect(ConditionType::Type type);
	Effect *getEffect(Effect::Type type);
	// returns "a " or "an " depending on the skill name
	const char *getUndefinedArticle();
	const char *getDescription();
	// return true if the skill can be used
	bool check(Creature *cr);
protected :
	friend class SkillParser;
	static bool init();
	SkillType(char *name);
};

class Skill {
public :
	SkillType *type;
	// if currently casting, type->castTime >= castTime > 0
	// else if currently reloading, -type->reloadTime <= castTime < 0
	float castTime;
	bool combo;
	bool isOn; // for onoff skills
	int durability; // for weapons
	TCODList<Condition *> conditions; // for weapons, conditions when wielded

	Creature *caster;
	Creature *target;

	char *getName();
	Skill(SkillType *type);
	bool isReady();
	inline bool isCasting() { return castTime > 0.0f; }
	inline bool isReloading() { return castTime < 0.0f; }
	// return true if skill destroyed (broken weapon)
	bool update(float elapsed);
	bool cast();
	void interrupt();
private :
	// return true if skill destroyed (broken weapon)
	bool execute();
	bool executeOnTarget(Creature *cr);
	bool applyEffectOnTarget(Effect *fx,Creature *cr);
};
