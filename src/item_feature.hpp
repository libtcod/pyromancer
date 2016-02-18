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

class Item;
class ItemType;
class ConditionEffect;
class Creature;
class Skill;

class Effect {
public :
	// type of effect
	enum Type {
		DAMAGE,			// deal damages
		INTERRUPT,		// interrupt skills
		PARRY,			// parry melee attack
		BLAST,			// push target away
		CONDITION,		// apply condition to target
		TELEPORT,		// teleport target
		RUSH,			
		METAMORPHOSE,	// transform target
		SUMMON,			// create creature
		LIGHT,			// enlighten target
		CAST,			// create item (projectile) 
	};
	Type type;
	Effect(Type type) : type(type) {}
	virtual bool execute(int x, int y, Creature *target, Creature *caster, Skill *skill) = 0;
	static void configureParser(TCODParser *parser,TCODParserStruct *effect);
};

class ConditionType {
public :
	enum Type {
		STUNNED,	// cannot move, attack or use skills
		BLEED,		// lose health points
		HEAL,		// gain health points
		POISONED,	// lose health points
		IMMUNE,		// cannot be poisoned
		PARALIZED,	// cannot move but can attack and use skills
		CRIPPLED,	// move slower
		WOUNDED,	// max health is decreased
		FURY,
		BLOCK,
		REGEN_MANA, // gain mana
	};
	Type type;
	const char *name;
	static TCODList<ConditionType *> list;
	static ConditionType *find(const char *name);
	static ConditionType *get(Type type);
	static void init();
	// return true if condition can be applied
	bool check(Creature *cr);
private :
	ConditionType(Type type,const char *name) : type(type),name(name) {}
};

class ItemFeature {
public :
	enum Type { 
		FIRE,  // fire has an effect on item (either transform or destroy)
		PRODUCE,  // item produces other items when clicked
		FOOD, // item can be eaten and restore health and/or mana
		AGE, // time has an effect on item (either transform or destroy)
		ATTACK, // when wielded, deal damages, can be thrown or throws projectiles
		LIGHT, // produces light when lying on ground
		HEAT, // produces heat
		CONTAINER, // can contain other items
		SPELLTREE, // choose/upgrade a spell when using this item
		EXPLODE_ON_BOUNCE, // light animation when the item hits a wall
		NB_ITEM_FEATURES
	};
	Type type;
	ItemFeature(Type type) : type(type) {}
	virtual ItemFeature *clone() const;
	virtual void copy(const ItemFeature *feat);
	virtual void use(Item *it) {}
	virtual bool update(float elapsed, TCOD_mouse_t *mouse, Item *it) { return true; }
};
class ItemFeatureFire : public ItemFeature {
public :
	float resistance; // in seconds
	ItemType *type; // if NULL, fire destroys the item
	ItemFeatureFire(float resistance, ItemType *type);
	ItemFeature *clone() const;
	void copy(const ItemFeature *feat);
	bool update(float elapsed, TCOD_mouse_t *mouse, Item *it);
};

class ItemFeatureProduce : public ItemFeature {
public :
	float delay; // in seconds
	float chance;
	ItemType *type;
	ItemFeatureProduce(float delay, float chance, ItemType *type);
	ItemFeature *clone() const;
	void copy(const ItemFeature *feat);
	void use(Item *it);
};

class ItemFeatureFood : public ItemFeature {
public :
	int health;
	int mana;
	float delay;
	ItemFeatureFood(int health, int mana, float delay);
	ItemFeature *clone() const;
	void copy(const ItemFeature *feat);
	void use(Item *it);
};

class ItemFeatureAge : public ItemFeature  {
public :
	float delay; // in seconds
	ItemType *type; // if NULL, destroy item
	ItemFeatureAge(float delay, ItemType *type);
	ItemFeature *clone() const;
	void copy(const ItemFeature *feat);
	bool update(float elapsed, TCOD_mouse_t *mouse, Item *it);
};

class ItemFeatureLight : public ItemFeature  {
public :
	enum Flags {
		RANDOM_RADIUS=1,
		INVSQRT=2,
		MODE_MUL=4,
		MODE_MAX=8,
	};
	float range;
	HDRColor color;
	HDRColor color2;
	int flags;
	float patternDelay;
	const char *pattern;
	const char *colorPattern;
	ItemFeatureLight(float range, const HDRColor &color, int flags, float patternDelay, 
		const char *pattern,const HDRColor &color2,const char *colorPattern=NULL);
	ItemFeature *clone() const;
	void copy(const ItemFeature *feat);
};



class ItemFeatureAttack : public ItemFeature  {
public :
	enum WieldType { 
		WIELD_NONE, 
		WIELD_ONE_HAND, 
		WIELD_MAIN_HAND, 
		WIELD_OFF_HAND, 
		WIELD_TWO_HANDS };
	enum Flags {
		WEAPON_PROJECTILE=1,
		WEAPON_RANGED=2,
	};
	WieldType wield;
	float minCastDelay;
	float maxCastDelay;
	float minReloadDelay;
	float maxReloadDelay;
	float minDamagesCoef;
	float maxDamagesCoef;
	int flags;
	float speed; // for projectiles
	ItemType * ammunition; // example : arrows for a bow
	ItemType * casts; // example : fireball for a pyromancer wand
	StatusResource::Type resourceType;
	float resourceCost;
	ItemFeatureAttack(WieldType wield, float minCastDelay, float maxCastDelay, 
		float minReloadDelay, float maxReloadDelay, float minDamagesCoef, float maxDamagesCoef, 
		int flags, float speed, ItemType *ammunition, ItemType * casts,StatusResource::Type resourceType,float resourceCost);
	ItemFeature *clone() const;
	void copy(const ItemFeature *feat);
	void use(Item *it);
	bool update(float elapsed, TCOD_mouse_t *mouse, Item *it);
};

class ItemFeatureHeat : public ItemFeature  {
public :
	float intensity;
	float radius;
	ItemFeatureHeat(float intensity, float radius);
	ItemFeature *clone() const;
	void copy(const ItemFeature *feat);
	bool update(float elapsed, TCOD_mouse_t *mouse, Item *it);
};

class ItemFeatureContainer : public ItemFeature  {
public :
	int size; 
	ItemFeatureContainer(int size);
	ItemFeature *clone() const;
	void copy(const ItemFeature *feat);
	void use(Item *it);
};

class ItemFeatureExplodeOnBounce : public ItemFeature  {
public :
	float delay;
	float startRange;
	float startLightCoef;

	float middleRange;
	float middleLightCoef;
	float middleTime;

	float endRange;
	float endLightCoef;
	
	int particleCount;
	ParticleType particleType;
	TCODList<Effect *> effects;
	ItemFeatureExplodeOnBounce(float delay,float startRange,float endRange,
		float middleRange,float middleLightCoef,float middleTime,
		float startLightCoef,float endLightCoef,int particleCount,float particleSpeed,
		float particleDuration,
		float particleMinDamage,float particleMaxDamage,const HDRColor &particleStartColor,const HDRColor &particleEndColor);
	ItemFeature *clone() const;
	void copy(const ItemFeature *feat);
	bool update(float elapsed, TCOD_mouse_t *mouse, Item *it);
	virtual ~ItemFeatureExplodeOnBounce() {}
	Effect *getEffect(Effect::Type type) const;
	ConditionEffect *getConditionEffect(ConditionType::Type type) const;
};

class ItemFeatureSpellTree : public ItemFeature {
public :
	ItemFeatureSpellTree();
	void use(Item *it);
};
