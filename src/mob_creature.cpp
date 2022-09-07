/*
* Copyright (c) 2009,2010 Jice
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
#include "main.hpp"

TCODList<Creature *> Creature::creatureByType[NB_CREATURE_TYPES];

TCODList<CreatureType *> CreatureType::list;

int CreatureType::create(int ch, const TCODColor &col, float life, float height, float speed, float attackDamages,const char *name, int flags) {
	CreatureType *type=new CreatureType();
	type->ch=ch;
	type->col=col;
	type->maxStatusResource[StatusResource::LIFE]=life;
	type->flags=flags;
	type->height=height;
	type->speed=speed;
	type->attackDamages=attackDamages;
	strncpy(type->name,name,CREATURE_NAME_SIZE);
	type->name[CREATURE_NAME_SIZE-1]=0;
	type->light=NULL;
	list.push(type);
	return list.size()-1;
}

void CreatureType::init() {
	static char minionChar=config.getCharProperty("config.creatures.minion.ch");
	static TCODColor minionColor=config.getColorProperty("config.creatures.minion.col");
	static int minionLife=config.getIntProperty("config.creatures.minion.life");
	static float minionSpeed=config.getFloatProperty("config.creatures.minion.speed");
	static float minionDamage=config.getFloatProperty("config.creatures.minion.damage");

	static char bossChar=config.getCharProperty("config.creatures.boss.ch");
	static TCODColor bossColor=config.getColorProperty("config.creatures.boss.col");
	static int bossLife=config.getIntProperty("config.creatures.boss.life");
	static float bossSpeed=config.getFloatProperty("config.creatures.boss.speed");
	static float treasureLightRange=config.getFloatProperty("config.display.treasureLightRange");
	static TCODColor treasureLightColor=config.getColorProperty("config.display.treasureLightColor");
	static float treasureIntensityDelay=config.getFloatProperty("config.display.treasureIntensityDelay");
	static const char *treasureIntensityPattern=strdup(config.getStringProperty("config.display.treasureIntensityPattern"));

	static TCODColor villagerColor=config.getColorProperty("config.creatures.villager.col");
	static char villagerChar=config.getCharProperty("config.creatures.villager.ch");
	static int villagerLife=config.getIntProperty("config.creatures.villager.life");

	static TCODColor villageHeadColor=config.getColorProperty("config.creatures.villageHead.col");
	static char villageHeadChar=config.getCharProperty("config.creatures.villageHead.ch");
	static int villageHeadLife=config.getIntProperty("config.creatures.villageHead.life");

	static TCODColor archerColor=config.getColorProperty("config.creatures.archer.col");
	static char archerChar=config.getCharProperty("config.creatures.archer.ch");
	static int archerLife=config.getIntProperty("config.creatures.archer.life");

	static char playerChar=config.getCharProperty("config.creatures.player.ch");
	static TCODColor playerColor=config.getColorProperty("config.creatures.player.col");

	// the cave creatures
	create('@',TCODColor(210,210,255),100,1.2f,12.0f,0.0f,"Aidan",CREATURE_OFFSCREEN | CREATURE_SAVE);
	create(0,TCODColor::desaturatedSky,10,0.5f,12.0f,0.0f,"fish",CREATURE_NOTBLOCK|CREATURE_CATCHABLE);
	create('d',TCODColor::darkerYellow,10,1.0f,20.0f,0.0f,"deer",CREATURE_SAVE);

	// pyromancer creatures
	create(minionChar,minionColor,minionLife,1.0f,minionSpeed,minionDamage,"minion");

	ExtendedLight *treasureLight=new ExtendedLight();
	treasureLight->color=treasureLightColor;
	treasureLight->range=treasureLightRange*2;
	treasureLight->setup(treasureLightColor,treasureIntensityDelay,treasureIntensityPattern,NULL);
	int bossIdx=create(bossChar,bossColor,bossLife,1.0f,bossSpeed,0.0f,"Zeepoh");
	list.get(bossIdx)->setLight(treasureLight);
	list.get(bossIdx)->setMana(50.0f);

   	int idx=create(minionChar,TCODColor::darkBlue,50.0,1.0f,minionSpeed,minionDamage,"dark wanderer");
	list.get(idx)->setMana(5.0f);
   	idx=create(minionChar,TCODColor::lighterBlue,30.0,1.0f,minionSpeed,minionDamage,"ice shrieker");
	list.get(idx)->setMana(5.0f);
	ExtendedLight *shriekLight=new ExtendedLight();
	shriekLight->color=HDRColor(140,140,300);
	shriekLight->range=4;
	shriekLight->setup(shriekLight->color,0.5,"9865495649",NULL);
	list.get(idx)->setLight(shriekLight);

	// treeburner creatures
	create(villagerChar,villagerColor,villagerLife,1.0f,minionSpeed,minionDamage,"villager");
	int vhIdx=create(villageHeadChar,villageHeadColor,villageHeadLife,1.0f,bossSpeed,0.0f,"village head");
	list.get(vhIdx)->setLight(treasureLight);
	create(archerChar,archerColor,archerLife,1.0f,minionSpeed,minionDamage,"archer");

	// common creatures
	int playerIdx=create(playerChar,playerColor,100.0f,1.0f,gameEngine->getFloatParam("playerSpeed"),0.0f,"player");
	CreatureType *playerType=list.get(playerIdx);
	playerType->maxStatusResource[StatusResource::MANA]=100;
}

CreatureTypeId CreatureType::getType(const char *name) {
	if ( list.size() == 0 ) init();
	int id=0;
	for (CreatureType **it=list.begin(); it != list.end(); it++,id++) {
		if ( strcmp((*it)->getName(),name) == 0 ) {
			return (CreatureTypeId)id;
		}
	}
	fprintf(stderr,"Fatal : unknown creature type %s",name);
	exit(0);
}

void CreatureType::setName(const char *pname) {
	strncpy(name,pname,CREATURE_NAME_SIZE);
	name[CREATURE_NAME_SIZE-1]=0;
}

Creature::Creature(const char *typeName) {
	type = CreatureType::getType(typeName);
	initFromType();
}

void Creature::initFromType() {
	talkText.text[0]=0;
	talkText.delay=0.0f;
	toDelete=false;
	path=NULL;
	flags=IGNORE_CREATURES;
	mainHand=offHand=NULL;
	asItem=NULL;
	fovRange=0;
	walkTimer=0.0f;
	pathTimer=0.0f;
	ch=getType()->getChar();
	col=getType()->getColor();
	for (int i=0; i <StatusResource::NB_TYPES; i++) {
		setResourceValue((StatusResource::Type)i,getType()->getResource((StatusResource::Type)i));
		setResourceCurrentMax((StatusResource::Type)i,getType()->getResource((StatusResource::Type)i));
	}
	light=NULL;
	if ( getType()->getLight() ) {
		light=getType()->getLight()->clone();
		gameEngine->dungeon->addLight(light);
	}
}

Creature::Creature() {
}

Creature::~Creature() {
	if ( path ) delete path;
}


void Creature::addCondition(Condition *cond) {
	conditions.push(cond);
	cond->target=this;
}

bool Creature::hasCondition(ConditionType::Type type, const char *alias) const {
	return (getCondition(type,alias) != NULL);
}

float Creature::getMaxConditionDuration(ConditionType::Type type,const char *alias) const {
	float maxVal=-1E8f;
	for ( Condition **it=conditions.begin(); it != conditions.end(); it ++) {
		if ( (*it)->equals(type,alias) && (*it)->duration > maxVal) maxVal=(*it)->duration;
	}
	return maxVal;
}

float Creature::getMinConditionAmount(ConditionType::Type type,const char *alias) const {
	float minVal=1E8f;
	for ( Condition **it=conditions.begin(); it != conditions.end(); it ++) {
		if ( (*it)->equals(type,alias) && (*it)->amount < minVal) minVal=(*it)->amount;
	}
	return minVal;
}

float Creature::getMaxConditionAmount(ConditionType::Type type,const char *alias) const {
	float maxVal=-1E8f;
	for ( Condition **it=conditions.begin(); it != conditions.end(); it ++) {
		if ( (*it)->equals(type,alias) && (*it)->amount > maxVal) maxVal=(*it)->amount;
	}
	return maxVal;
}

Condition *Creature::getCondition(ConditionType::Type type, const char *alias) const {
	for ( Condition **it=conditions.begin(); it != conditions.end(); it ++) {
		if ( (*it)->equals(type,alias) ) return *it;
	}
	return NULL;
}

void Creature::attack(Creature *target, float elapsed) {
	target->takeDamage(getType()->getAttackDamages()*elapsed);
}

void Creature::addSkillType(SkillType *skillType) {
	getType()->addSkill(skillType);
	Skill *skill=new Skill(skillType);
	skill->caster=this;
	addSkill(skill);
}

void Creature::addSkillType(const char *skillTypeName) {
	addSkillType(SkillType::find(skillTypeName));
}

Creature *Creature::getCreature(CreatureTypeId id) {
	static float pathDelay=config.getFloatProperty("config.creatures.pathDelay");
	Creature *ret=NULL;
	switch(id) {
		// the cave
		case CREATURE_DEER :
			 ret=new Creature("deer");
			 ret->addBehavior(new HerdBehavior(new AvoidWaterWalkPattern(),pathDelay));
		break;
		case CREATURE_FRIEND :
			ret = new Friend();
		break;
		case CREATURE_FISH :
			ret = new Fish(NULL);
		break;
		// pyromancer
		case CREATURE_MINION :
			ret = new Creature("minion");
			ret->addBehavior(new AttackOnSee(pathDelay));
		break;
		case CREATURE_DARK_WANDERER :
			ret = new Creature("dark wanderer");
			ret->addBehavior(new WalkBehavior(new WalkPattern(AVOID_WATER|FLEE_PLAYER),pathDelay,true));
		break;
		case CREATURE_ICE_SHRIEKER :
			ret = new Creature("ice shrieker");
			ret->addBehavior(new WalkBehavior(new WalkPattern(AVOID_WATER|FLEE_PLAYER),pathDelay,true));
		break;
		case CREATURE_ZEEPOH : {
			ret = new Creature("Zeepoh");
			ret->addSkillType("summon minions");
			ret->addSkillType("summon ice shriekers");
			ret->addSkillType("shockfrost");
			Item *amulet=Item::getItem("amulet",0,0,false);
			amulet->name=strdup("Amulet of Zeepoh");
			amulet->article=Item::THE;
			ret->addToInventory(amulet);
			ret->addBehavior(new WalkBehavior(new WalkPattern(AVOID_WATER|FLEE_PLAYER|IGNORE_CREATURES),pathDelay,false));
		}
		break;
		// treeburner
		case CREATURE_VILLAGER :
			ret = new Villager();
			ret->addBehavior(new AttackOnSee(pathDelay));
		break;
		case CREATURE_ARCHER :
			ret = new Archer();
			ret->addBehavior(new AttackOnSee(pathDelay));
		break;
		case CREATURE_VILLAGE_HEAD :
			ret = new VillageHead();
			ret->addBehavior(new WalkBehavior(new WalkPattern(AVOID_WATER|FLEE_PLAYER),pathDelay,false));
		break;
		default:break;
	}
	if ( ret ) {
		creatureByType[id].push(ret);
	}
	return ret;
}

bool Creature::isInRange(int px, int py) const {
	int dx=(int)(px-x);
	int dy=(int)(py-y);
	return ( ABS(dx) <= fovRange
		&& ABS(dy) <= fovRange
		&& dx*dx + dy*dy <= fovRange*fovRange );
}

bool Creature::isPlayer() const {
	return this == gameEngine->player;
}

void Creature::talk(const char *text) {
	strncpy(talkText.text,(char*)text,CREATURE_TALK_SIZE-1);
	talkText.text[CREATURE_TALK_SIZE-1]=0;
	talkText.delay=strlen(text) * 0.1f;
	talkText.delay=MAX(0.5f,talkText.delay);
	// compute text size
	char *ptr=(char*)text;
	talkText.h=1;
	talkText.w=0;
	char *end=strchr(ptr,'\n');
	while ( end ) {
		int len = end - ptr;
		if ( talkText.w < len ) talkText.w=len;
		talkText.h++;
		ptr=end+1;
		end=strchr(ptr,'\n');
	}
	if ( end ) {
		int len = end - ptr;
		if ( talkText.w < len ) talkText.w=len;
	}
}

void Creature::renderTalk() {
	int conx=(int)(x-gameEngine->xOffset);
	int cony=(int)(y-gameEngine->yOffset);
	if ( !IN_RECTANGLE(conx,cony,CON_W,CON_H) ) return; // creature out of console
	talkText.x=conx;
	talkText.y=cony - talkText.h;
	if ( talkText.y < 0 ) talkText.y = cony+1;
	gameEngine->packer->addRect(&talkText);
	TCODConsole::root->setDefaultBackground(TCODColor::lighterYellow);
	TCODConsole::root->setDefaultForeground(TCODColor::darkGrey);
	TCODConsole::root->printEx((int)talkText.x,(int)talkText.y,TCOD_BKGND_SET,TCOD_CENTER,talkText.text);
}

void Creature::render(LightMap *lightMap) {
	static int penumbraLevel=config.getIntProperty("config.gameplay.penumbraLevel");
	static int darknessLevel=config.getIntProperty("config.gameplay.darknessLevel");
	static float fireSpeed=config.getFloatProperty("config.display.fireSpeed");
	static TCODColor corpseColor=config.getColorProperty("config.display.corpseColor");
	static TCODColor lowFire(255,0,0);
	static TCODColor midFire(255,204,0);
	static TCODColor highFire(255,255,200);
	static TCODColor fire[64];
	static bool fireInit=false;
	if (! fireInit ) {
		for (int i=0; i < 32; i++) {
			fire[i]=TCODColor::lerp(lowFire,midFire,i/32.0f);
		}
		for (int i=32; i < 64; i++) {
			fire[i]=TCODColor::lerp(midFire,highFire,(i-32)/32.0f);
		}
		fireInit=true;
	}

	// position on console
	int conx=(int)(x-gameEngine->xOffset);
	int cony=(int)(y-gameEngine->yOffset);
	if ( !IN_RECTANGLE(conx,cony,CON_W,CON_H) ) return; // out of console

	float playerDist = distance(*gameEngine->player);
	float apparentHeight = getType()->getHeight() / playerDist;
	if ( apparentHeight < MIN_VISIBLE_HEIGHT ) return; // too small to see at that distance

	TCODColor c;
	int displayChar=ch;
	TCODColor lightColor=lightMap->getColor(conx,cony)*1.5f;
	Dungeon *dungeon=gameEngine->dungeon;
	float shadow = dungeon->getShadow(x*2,y*2);
	float clouds = dungeon->getCloudCoef(x*2,y*2);
	shadow = MIN(shadow,clouds);
	lightColor = lightColor * shadow;
	if ( getLife() <= 0 ) {
		ch='%';
		c=corpseColor*lightColor;
	} else if ( isBurning() ) {
		float fireX = gameEngine->getCurrentDate() * fireSpeed + noiseOffset;
		int fireIdx = (int)((0.5f+0.5f*noise1d.get(&fireX))*64.0f);
		c=fire[fireIdx];
		int r=(int)(c.r*1.5f*lightColor.r/255);
		int g=(int)(c.g*1.5f*lightColor.g/255);
		int b=(int)(c.b*1.5f*lightColor.b/255);
		c.r=CLAMP(0,255,r);
		c.g=CLAMP(0,255,g);
		c.b=CLAMP(0,255,b);
	} else {
		c=col*lightColor;
	}
	int intensity = c.r+c.g+c.b;
	if (intensity < darknessLevel ) return; // creature not seen
	if (intensity < penumbraLevel ) displayChar='?';
	if ( apparentHeight < VISIBLE_HEIGHT ) displayChar = '?'; // too small to distinguish
	TCODConsole::root->setChar(conx,cony,displayChar);
	TCODConsole::root->setCharForeground(conx,cony,c);
}

void Creature::stun(float delay) {
	walkTimer=MIN(-delay,walkTimer);
}

bool Creature::walk(float elapsed) {
	float coef=1.0f;
	if (hasCondition(ConditionType::CRIPPLED)) {
		coef = getMinConditionAmount(ConditionType::CRIPPLED);
	}
	walkTimer+=elapsed*coef;
	if ( walkTimer >= 0 && ! hasCondition(ConditionType::STUNNED) && ! hasCondition(ConditionType::PARALIZED)) {
		TerrainId terrainId=gameEngine->dungeon->getTerrainType((int)x,(int)y);
		float walkTime = terrainTypes[terrainId].walkCost / getType()->getSpeed();
		walkTimer=-walkTime;
		if ( path && ! path->isEmpty()) {
			int newx,newy;
			GameEngine *game=gameEngine;
			path->get(0,&newx,&newy);
			if ( (game->player->x != newx || game->player->y != newy)
				&& !game->dungeon->hasCreature(newx,newy) ) {
				int oldx=(int)x,oldy=(int)y;
				int newx=oldx,newy=oldy;
				if (path->walk(&newx,&newy,false)) {
					setPos(newx,newy);
					game->dungeon->moveCreature(this,oldx,oldy,newx,newy);
					if ( game->dungeon->hasRipples(newx,newy) ) {
						gameEngine->startRipple(newx,newy);
					}
					return true;
				}
			}
		}
	}
	return false;
}

void Creature::randomWalk(float elapsed) {
	walkTimer+=elapsed;
	if ( walkTimer >= 0 && ! hasCondition(ConditionType::STUNNED) && ! hasCondition(ConditionType::PARALIZED)) {
		walkTimer=-1.0f/getType()->getSpeed();
		static int dirx[]={-1,0,1,-1,1,-1,0,1};
		static int diry[]={-1,-1,-1,0,0,1,1,1};
		int d=TCODRandom::getInstance()->getInt(0,7);
		int count=8;
		GameEngine *game=gameEngine;
		do {
			int newx=(int)(x+dirx[d]),newy=(int)(y+diry[d]);
			if (IN_RECTANGLE(newx,newy,game->dungeon->width,game->dungeon->height)
				&& game->dungeon->map->isWalkable(newx,newy)
				&& (game->player->x != newx || game->player->y != newy)
				&& !game->dungeon->hasCreature(newx,newy) ) {
				game->dungeon->moveCreature(this,(int)x,(int)y,newx,newy);
				x=newx;y=newy;
				return;
			}
			d = (d+1)%8;
			count --;
		} while (count > 0);
	}
}

float Creature::getWalkCost( int xFrom, int yFrom, int xTo, int yTo, void *userData ) const {
	GameEngine *game=gameEngine;
	if ( !game->dungeon->map->isWalkable(xTo,yTo)) return 0.0f;
	if ( game->fireManager && game->fireManager->isBurning(xTo,yTo) ) return 100.0f;
	//if ( doesIgnoreCreatures() ) return 1.0f;
	if ( game->dungeon->hasCreature(xTo,yTo) ) return 50.0f;
	if ( game->player->x == xTo || game->player->y == yTo ) return 50.0f;
	return 1.0f;
}

void Creature::takeDamage(float amount) {
	addLife(-amount);
	if ( getLife() <= 0 && !isPlayer()) {
		if (AiDirector::instance) AiDirector::instance->killCreature(this);
		gameEngine->stats.creatureDeath[type]++;
	}
	for ( Skill **it=skills.begin(); it!=skills.end(); it++) {
		(*it)->interrupt();
	}
}

Item * Creature::addToInventory(Item *item) {
	item = item->addToList(&inventory);
	item->owner=this;
	item->x=x;
	item->y=y;
	return item;
}

Item * Creature::removeFromInventory(Item *item, int count) {
	if ( count == 0 ) count = item->count;
	item = item->removeFromList(&inventory,count);
	if ( item == mainHand || item == offHand ) unwield(item);
	item->owner=NULL;
	return item;
}

void Creature::updateConditions(float elapsed) {
	for (Condition **it=conditions.begin(); it != conditions.end(); it++) {
		if ((*it)->update(elapsed) ) {
			delete *it;
			it=conditions.remove(it);
		}
	}
}

void Creature::updateSkills(float elapsed) {
	for (Skill **it=skills.begin(); it != skills.end(); it++) {
		if ((*it)->update(elapsed) ) {
		}
	}
}

void Creature::die() {
	if (inventory.size() > 0 ) {
		// drop a random item from inventory
		int num=TCODRandom::getInstance()->getInt(0,inventory.size()-1);
		Item *it=inventory.get(num);
		it = removeFromInventory(it);
		it->x=x;
		it->y=y;
		gameEngine->dungeon->addItem(it);
		gameEngine->gui.log.info("The %s dropped %s",getName(),it->aName());
	}
}

bool Creature::update(float elapsed) {
	static float burnDamage=config.getFloatProperty("config.creatures.burnDamage");
	static int distReplace=config.getIntProperty("config.aidirector.distReplace")*config.getIntProperty("config.aidirector.distReplace");

	Cell *cell=gameEngine->dungeon->getCell(x,y);
	cell->trampleDate=gameEngine->getCurrentDate();
	if ( light ) {
		light->setPos(x*2,y*2);
	}

	if ( getLife() <= 0 ) {
		if ( light ) {
			gameEngine->dungeon->removeLight(light);
		}
		creatureByType[type].removeFast(this);
		return false;
	}
	if (speed > 0.0f) {
		//DynamicEntity::CollisionType type=
		int oldx=(int)x;
		int oldy=(int)y;
		updateMove(elapsed,0.0f,false,false,false);
		if ( (int)x != oldx || (int)y != oldy ) {
			gameEngine->dungeon->moveCreature(this,oldx,oldy,(int)x,(int)y);
		}
	}
	if (talkText.delay > 0.0f) {
		talkText.delay -= elapsed;
		if (talkText.delay < 0.0f) talkText.delay=-10.0f;
	} else if (talkText.delay < 0.0f) {
		talkText.delay += elapsed;
		if (talkText.delay > 0.0f) talkText.delay=0.0f;
	}

	updateConditions(elapsed);
	updateSkills(elapsed);

	if ( isReplacable() ) {
		int pdist=(int)squaredDistance(*gameEngine->player);
		if ( pdist > distReplace ) {
			AiDirector::instance->replace(this);
			return true;
		}
	}

	if ( isBurning() ) {
		takeDamage(burnDamage*elapsed);
	}
	// update items in inventory
	for ( Item ** it=inventory.begin(); it != inventory.end(); it++) {
		if (!(*it)->age(elapsed)) {
			it=inventory.removeFast(it); // from inventory
		}
	}
	// ai
	updateBehaviors(elapsed);
	if (skills.size()>0 && gameEngine->dungeon->isCellInFov(x,y)) {
		// TODO move this in a behavior
		for (Skill **sk=skills.begin(); sk!=skills.end(); sk++) {
			if ( (*sk)->isReady() ) (*sk)->cast();
		}
	}
	return getLife()> 0;
}

void Creature::updateBehaviors(float elapsed) {
	for ( Behavior **it = behaviors.begin(); it != behaviors.end(); it++) {
		if (!(*it)->update(this,elapsed)) {
			delete *it;
			it = behaviors.remove(it);
		}
	}
}

void Creature::equip(Item *it) {
	ItemFeatureAttack *feat=(ItemFeatureAttack *)it->getFeature(ItemFeature::ATTACK);
	switch ( feat->wield ) {
		case ItemFeatureAttack::WIELD_NONE : break;
		case ItemFeatureAttack::WIELD_MAIN_HAND :
			if ( offHand && offHand == mainHand ) offHand=NULL; // unequip two hands weapon
			mainHand=it;
		break;
		case ItemFeatureAttack::WIELD_ONE_HAND :
			if ( ! mainHand ) mainHand=it;
			else if (! offHand ) offHand = it;
			else {
				if ( offHand == mainHand ) offHand=NULL;
				mainHand=it;
			}
		break;
		case ItemFeatureAttack::WIELD_OFF_HAND :
			if ( mainHand && offHand == mainHand ) mainHand=NULL; // unequip two hands weapon
			offHand=it;
		break;
		case ItemFeatureAttack::WIELD_TWO_HANDS :
			mainHand = offHand = it;
		break;
	}
}

void Creature::unequip(Item *it) {
	if ( it == mainHand ) mainHand=NULL;
	if ( it == offHand ) offHand=NULL; // might be both for two hands items
}

void Creature::wield(Item *it) {
	ItemFeatureAttack *feat=(ItemFeatureAttack *)it->getFeature(ItemFeature::ATTACK);
	switch ( feat->wield ) {
		case ItemFeatureAttack::WIELD_NONE :
			gameEngine->gui.log.warn("You cannot wield %s",it->aName());
			return;
		break;
		case ItemFeatureAttack::WIELD_MAIN_HAND :
			if ( mainHand ) unwield(mainHand);
		break;
		case ItemFeatureAttack::WIELD_OFF_HAND :
			if ( offHand ) unwield(offHand);
		break;
		case ItemFeatureAttack::WIELD_ONE_HAND :
			if ( mainHand && offHand ) unwield(mainHand);
		break;
		case ItemFeatureAttack::WIELD_TWO_HANDS :
			if ( mainHand ) unwield(mainHand);
			if ( offHand ) unwield(offHand);
		break;
	}
	equip(it);
	if ( isPlayer() ) {
		gameEngine->gui.log.info("You're wielding %s",it->aName());
	}
}

void Creature::unwield(Item *it) {
	unequip(it);
	if ( isPlayer() ) {
		gameEngine->gui.log.info("You were wielding %s",it->aName());
	}
}

#define CREA_CHUNK_VERSION 9
void Creature::saveData(uint32_t chunkId, TCODZip *zip) {
	saveGame.saveChunk(CREA_CHUNK_ID,CREA_CHUNK_VERSION);
	zip->putString(getType()->getName());
	zip->putFloat(x);
	zip->putFloat(y);
	for (int i=0; i < StatusResource::NB_TYPES; i++) {
		zip->putFloat(getResourceCurrentMax((StatusResource::Type)i));
		zip->putFloat(getResourceValue((StatusResource::Type)i));
	}
	// save inventory
	zip->putInt(inventory.size());
	for (Item **it = inventory.begin(); it != inventory.end(); it++) {
		zip->putString((*it)->typeData->name);
		(*it)->saveData(ITEM_CHUNK_ID,zip);
	}
	// save conditions
	zip->putInt(conditions.size());
	for ( Condition **it=conditions.begin(); it != conditions.end(); it++) {
		(*it)->save(zip);
	}

}

bool Creature::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip *zip) {
	if ( chunkVersion != CREA_CHUNK_VERSION ) return false;
	const char *typeName=zip->getString();
	type = CreatureType::getType(typeName);
	initFromType();
	x=zip->getFloat();
	y=zip->getFloat();
	for (int i=0; i < StatusResource::NB_TYPES; i++) {
		setResourceCurrentMax((StatusResource::Type)i,zip->getFloat());
		setResourceValue((StatusResource::Type)i,zip->getFloat());
	}
	// load inventory
	int nbItems = zip->getInt();
	while (nbItems > 0 ) {
		const char * itemTypeName=zip->getString();
		ItemType *itemType=ItemType::getType(itemTypeName);
		if (!itemType) return false;
		uint32_t itemChunkId ,itemChunkVersion;
		saveGame.loadChunk(&itemChunkId, &itemChunkVersion);
		Item *it=Item::getItem(itemType, 0,0);
		if (!it->loadData(itemChunkId, itemChunkVersion, zip)) return false;
		addToInventory(it);
		nbItems--;
	}
	// load conditions
	int nbConditions = zip->getInt();
	while (nbConditions > 0 ) {
		Condition *cond= new Condition();
		cond->target=this;
		cond->load(zip);
		conditions.push(cond);
		nbConditions--;
	}
	return true;
}
