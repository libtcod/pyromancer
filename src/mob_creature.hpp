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

class Game;
class Creature;
class Skill;
class SkillType;

enum CreatureTypeId {
	// the cave chapter 1
	CREATURE_FRIEND,
	CREATURE_FISH,
	CREATURE_DEER,
	// pyromancer
	CREATURE_MINION,
	CREATURE_ZEEPOH,
	CREATURE_DARK_WANDERER,
	CREATURE_ICE_SHRIEKER,
	// TreeBurner
	CREATURE_VILLAGER,
	CREATURE_VILLAGE_HEAD,
	CREATURE_ARCHER,
	CREATURE_PLAYER,
	NB_CREATURE_TYPES
};

enum CreatureTypeFlags {
	CREATURE_REPLACABLE = 1, // if too far, put it back near the player
	CREATURE_OFFSCREEN  = 2, // updated even if out of console screen
	CREATURE_SAVE       = 4, // save this creature in savegame
	CREATURE_NOTBLOCK   = 8, // does not block path
	CREATURE_CATCHABLE  =16, // can catch a creature by clicking on it when adjacent
};

#define CREATURE_NAME_SIZE 32
#define CREATURE_TALK_SIZE 64
#define VISIBLE_HEIGHT 0.05f
#define MIN_VISIBLE_HEIGHT 0.02f


class SkillType;

class CreatureType {
	TCODColor col;
	int ch; // character
	float maxStatusResource[StatusResource::NB_TYPES];
	float speed;
	float height; // in meters
	float attackDamages;
	int flags;
	char name[CREATURE_NAME_SIZE];
	Light *light; // light producing creature

	static TCODList<CreatureType *> list;
	TCODList<SkillType *> skills;
public :
	// properties
	inline const char *getName() const { return name; }
	void setName(const char *name);
	inline const TCODColor & getColor() const { return col; }
	inline float getResource(StatusResource::Type typ) { return maxStatusResource[typ]; } 
	inline float getLife() const { return maxStatusResource[StatusResource::LIFE]; }
	inline float getMana() const { return maxStatusResource[StatusResource::MANA]; }
	inline float getSpeed() const { return speed; }
	inline float getHeight() const { return height; }
	inline int getFlags() const { return flags; }
	inline int getChar() const { return ch; }
	inline int getAttackDamages() const { return (int)attackDamages; }
	inline const Light *getLight() const { return light; }
	inline void setLight(Light *light) { this->light=light; }
	// resources
	inline void setLife(float value) { maxStatusResource[StatusResource::LIFE] = value; }
	inline void setMana(float value) { maxStatusResource[StatusResource::MANA] = value; }
	// flags
	bool isReplacable() const { return ( flags & CREATURE_REPLACABLE ) != 0; }
	bool isUpdatedOffscreen() const { return ( flags & CREATURE_OFFSCREEN ) != 0; }
	bool mustSave() const { return ( flags & CREATURE_SAVE) != 0; }
	bool isBlockingPath() const { return (flags & CREATURE_NOTBLOCK) == 0; }
	bool isCatchable() const { return (flags & CREATURE_CATCHABLE) != 0; }
	// skills
	void addSkill(SkillType *type) { skills.push(type); }

	static CreatureTypeId getType(const char *name);
	inline static CreatureType * getType(CreatureTypeId id) { return list.get(id); }
protected :
	static void init();
	static int create(int ch, const TCODColor &col, float life, float height, float speed, float attackDamages, const char *name, int flags=0);
};

enum CreatureFlags {
	BURNING=1, // is burning
	SEEN=2, // has been seen by the player
};

class Creature : public DynamicEntity, public ITCODPathCallback, public NoisyThing, public SaveListener {
public :
	CreatureTypeId type;
	TCODColor col;
	int ch; // character
	StatusResource resources[StatusResource::NB_TYPES];
	TCODPath *path;
	Item *mainHand, *offHand;
	Item *asItem; // an item corresponding to this creature
	TCODList<Condition *> conditions;
	Light *light; // light producing creature	

	Creature();
	Creature(const char *typeName);
	virtual ~Creature();
	
	void initFromType();
	void die();
	inline bool isDead() {return getLife() <= 0.0f; }

	// resources
	inline float getResourceValue(StatusResource::Type typ) { return resources[typ].currentValue;}
	inline float getResourceCurrentMax(StatusResource::Type typ) { return resources[typ].currentMaxValue;}
	inline float getResourceRatio(StatusResource::Type typ) { return resources[typ].getRatio();}
	inline void addResource(StatusResource::Type typ,float val) {resources[typ].add(val); }
	inline void setResourceValue(StatusResource::Type typ,float val) {resources[typ].currentValue=val; }
	inline void setResourceCurrentMax(StatusResource::Type typ,float val) {resources[typ].currentMaxValue=val; }
	inline float getLife() {return getResourceValue(StatusResource::LIFE);}
	inline float getMana() {return getResourceValue(StatusResource::MANA);}
	inline float getLifeRatio() {return getResourceRatio(StatusResource::LIFE);}
	inline float getManaRatio() {return getResourceRatio(StatusResource::MANA);}
	inline float getMaxLife() {return getResourceCurrentMax(StatusResource::LIFE);}
	inline float getMaxMana() {return getResourceCurrentMax(StatusResource::MANA);}
	inline void addLife(float val) {addResource(StatusResource::LIFE,val); }
	inline void addMana(float val) {addResource(StatusResource::MANA,val); }
	inline void setLife(float val) {setResourceValue(StatusResource::LIFE,val); }
	inline void setMana(float val) {setResourceValue(StatusResource::MANA,val); }
	inline void setMaxLife(float val) {setResourceCurrentMax(StatusResource::LIFE,val); }
	inline void setMaxMana(float val) {setResourceCurrentMax(StatusResource::MANA,val); }

	// flags
	inline bool isBurning() const { return (flags & BURNING) != 0 ; }
	inline bool hasBeenSeen() const { return (flags & SEEN) != 0 ; }
	inline void setBurning(bool b) { if (b) flags |= BURNING; else flags &= ~ BURNING; }
	inline void setSeen(bool b) { if (b) flags |= SEEN; else flags &= ~ SEEN; }

	// behaviors
	inline void addBehavior(Behavior *b) { behaviors.push(b); }

	// type properties
	inline CreatureType *getType() const { return CreatureType::getType(type); }
	inline const char *getName() const { return getType()->getName(); }
	inline void setName(const char *name) { getType()->setName(name); }
	// type flags
	bool isReplacable() const { return getType()->isReplacable(); }
	bool isUpdatedOffscreen() const { return getType()->isUpdatedOffscreen(); }
	bool mustSave() const { return getType()->mustSave(); }
	bool isBlockingPath() const { return getType()->isBlockingPath(); }
	bool isCatchable() const { return getType()->isCatchable(); }

	//skills
	virtual void addSkill(Skill *skill) { skills.push(skill); }
	void addSkillType(SkillType *skillType);
	void addSkillType(const char *skillTypeName);

	// conditions
	void addCondition(Condition *cond);
	bool hasCondition(ConditionType::Type type, const char *alias=NULL) const;
	Condition *getCondition(ConditionType::Type type, const char *alias=NULL) const;
	float getMaxConditionDuration(ConditionType::Type type,const char *alias=NULL) const;
	float getMinConditionAmount(ConditionType::Type type,const char *alias=NULL) const;
	float getMaxConditionAmount(ConditionType::Type type,const char *alias=NULL) const;

	// factory
	static Creature *getCreature(CreatureTypeId id);
	static TCODList<Creature *> creatureByType[NB_CREATURE_TYPES];

	void attack(Creature *target, float elapsed);

	virtual bool update(float elapsed);
	virtual void render(LightMap *lightMap);
	void renderTalk();
	virtual void takeDamage(float amount);
	virtual void stun(float delay);
	virtual float getWalkCost( int xFrom, int yFrom, int xTo, int yTo, void *userData ) const;
	void talk(const char *text);
	bool isTalking() const { return talkText.delay > 0.0f; }
	bool isInRange(int x, int y) const;
	bool isPlayer() const;
	
	
	// items
	Item * addToInventory(Item *it); // in case of stackable items, returned item might be != it
	Item * removeFromInventory(Item *it, int count=1); // same as addToInventory
	Item **inventoryBegin() { return inventory.begin(); }
	Item **inventoryEnd() { return inventory.end(); }
	void equip(Item *it);
	void unequip(Item *it);
	// same as equip/unequip but with messages (you're wielding...)
	void wield(Item *it);
	void unwield(Item *it);
	virtual void initItem() {} // build asItem member

	// SaveListener
	bool loadData(uint32 chunkId, uint32 chunkVersion, TCODZip *zip);
	void saveData(uint32 chunkId, TCODZip *zip);

	float fovRange;
	bool toDelete;
protected :
	friend class Behavior;
	friend class WalkBehavior;
	friend class FollowBehavior;
	friend class HerdBehavior;
	friend class AttackOnSee;
	friend class ForestScreen;
	friend class Condition;
	friend class Game;
	TCODList<Behavior *> behaviors;
	TCODList<Item *> inventory;
	float walkTimer,pathTimer;
	int flags;
	struct TalkText : public Rect {
		char text[CREATURE_TALK_SIZE];
		float delay;
	} talkText;
	TCODList<Skill *> skills;

	bool walk(float elapsed);
	void randomWalk(float elapsed);
	void updateBehaviors(float elapsed);
	void updateConditions(float elapsed);
	void updateSkills(float elapsed);
};

