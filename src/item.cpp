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

const char *StatusResource::names[NB_TYPES] = {
	"life",
	"mana",
};

TCODConsole *Item::descCon=NULL;

TCODList<ItemActionId> featureActions[ItemFeature::NB_ITEM_FEATURES];

TCODList<ItemType *>ItemType::types;
// text generator for item names
static TextGenerator *textgen=NULL;

static ItemAction itemActions[NB_ITEM_ACTIONS] = {
	{"Take", ITEM_ACTION_LOOT },
	{"Take all", ITEM_ACTION_LOOT },
	{"Use", ITEM_ACTION_LOOT|ITEM_ACTION_INVENTORY },
	{"Drop", ITEM_ACTION_INVENTORY },
	{"Drop all", ITEM_ACTION_INVENTORY },
	{"Throw", ITEM_ACTION_LOOT|ITEM_ACTION_INVENTORY },
	{"Disassemble", ITEM_ACTION_LOOT|ITEM_ACTION_INVENTORY },
	{"Fill", ITEM_ACTION_LOOT|ITEM_ACTION_INVENTORY|ITEM_ACTION_ONWATER },
};

ItemAction *ItemAction::getFromId(ItemActionId id) {
	return &itemActions[id];
}

TCODList<ItemCombination *> Item::combinations;

TCODColor Item::classColor[NB_ITEM_CLASSES] = {
	TCODColor::white,
	TCODColor::chartreuse,
	TCODColor::orange,
	TCODColor::red,
	TCODColor::lightGrey,
	TCODColor::lightYellow,
};

Item *ItemType::produce(float rng) const {
	float odds=0.0f;
	TCODList <ItemFeatureProduce *> produceFeatures;
	// select one of the produce features
	for (ItemFeature **it=features.begin(); it!=features.end(); it++) {
		if ( (*it)->type == ItemFeature::PRODUCE ) {
			ItemFeatureProduce *prod=(ItemFeatureProduce *)(*it);
			produceFeatures.push(prod);
			odds += prod->chance;
		}
	}
	rng *= odds;
	for (ItemFeatureProduce **it=produceFeatures.begin(); it!=produceFeatures.end(); it++) {
		rng -= (*it)->chance;
		if ( rng < 0 ) {
			// this one wins!
			return Item::getItem((*it)->type,0,0);
		}
	}
	return NULL;
}

bool ItemType::isA(const char *typeName) const {
	return isA(ItemType::getType(typeName));
}
bool ItemType::hasAction(ItemActionId id) const {
	return actions.contains(id);
}

bool ItemType::isA(const ItemType *type) const {
	if ( type == this ) return true;
	if ( type == NULL ) return false;
	for (ItemType **father=inherits.begin(); father!=inherits.end(); father ++) {
		if ( (*father)->isA(type) ) return true;
	}
	return false;
}

ItemFeature *ItemType::getFeature(ItemFeature::Type id) const {
	for (ItemFeature **it=features.begin(); it!=features.end(); it++) {
		if ( (*it)->type == id ) return *it;
	}
	return NULL;
}

void ItemType::addFeature(const char *typeName, ItemFeature *feat) {
	ItemType::getType(typeName)->features.push(feat);
}

// to handle item type forward references in items.cfg
TCODList<ItemType *> toDefine;

class ItemFileListener : public ITCODParserListener {
public :
	ItemFileListener() : redirect(false) {}
	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
		if ( redirect ) return effectParser.parserNewStruct(&feat.explodeOnBounce->effects,parser,str,name);
		if( strcmp(str->getName(),"itemType") == 0 ) {
			type=NULL;
			for ( ItemType **it=toDefine.begin(); it!=toDefine.end(); it++) {
				if ( strcmp((*it)->name,name) == 0 ) {
					type = *it;
					break;
				}
			}
			if (! type ) {
				type = new ItemType();
				ItemType::types.push(type);
			}
			type->inventoryTab=INV_MISC;
			type->flags=0;
			type->character=0;
			type->trailLength=0;
			type->bounceCoef=0.5f;
			type->onPick=NULL;
			// inheritance
			if (types.size() > 0 ) {
				ItemType *father=types.peek();
				type->inherits.push(father);
				// copy features
				type->inventoryTab=father->inventoryTab;
				type->flags |= (father->flags & ~ITEM_ABSTRACT);
				type->character=father->character;
				type->color = father->color;
				type->trailLength=father->trailLength;
				type->bounceCoef=father->bounceCoef;
				for (ItemFeature **it=father->features.begin(); it != father->features.end(); it++) {
					ItemFeature *feat=(*it)->clone();
					type->features.push(feat);
				}
			}
			type->name=strdup(name);

			types.push(type);
			feat.spell=NULL;
		} else  if ( strcmp(str->getName(),"fire") == 0 ) {
			ItemFeatureFire *curfeat=(ItemFeatureFire *)type->getFeature(ItemFeature::FIRE);
			if ( curfeat ) feat.fire=curfeat;
			else feat.fire=new ItemFeatureFire(0.0f,NULL);
		} else  if ( strcmp(str->getName(),"heat") == 0 ) {
			ItemFeatureHeat *curfeat=(ItemFeatureHeat *)type->getFeature(ItemFeature::HEAT);
			if ( curfeat ) feat.heat=curfeat;
			else feat.heat=new ItemFeatureHeat(0.0f,0.0f);
		} else  if ( strcmp(str->getName(),"produces") == 0 ) {
			ItemFeatureProduce *curfeat=(ItemFeatureProduce *)type->getFeature(ItemFeature::PRODUCE);
			if ( curfeat ) feat.produce=curfeat;
			else feat.produce=new ItemFeatureProduce(0.0f,0.0f,NULL);
		} else  if ( strcmp(str->getName(),"food") == 0 ) {
			ItemFeatureFood *curfeat=(ItemFeatureFood *)type->getFeature(ItemFeature::FOOD);
			if ( curfeat ) feat.food=curfeat;
			else feat.food=new ItemFeatureFood(0,0,1.0f);
		} else  if ( strcmp(str->getName(),"age") == 0 ) {
			ItemFeatureAge *curfeat=(ItemFeatureAge *)type->getFeature(ItemFeature::AGE);
			if ( curfeat ) feat.age=curfeat;
			else feat.age=new ItemFeatureAge(0.0f,NULL);
		} else  if ( strcmp(str->getName(),"container") == 0 ) {
			ItemFeatureContainer *curfeat=(ItemFeatureContainer *)type->getFeature(ItemFeature::CONTAINER);
			if ( curfeat ) feat.container=curfeat;
			else feat.container=new ItemFeatureContainer(0);
		} else  if ( strcmp(str->getName(),"spellTree") == 0 ) {
			feat.spell=new ItemFeatureSpellTree();
		} else  if ( strcmp(str->getName(),"attack") == 0 ) {
			ItemFeatureAttack *curfeat=(ItemFeatureAttack *)type->getFeature(ItemFeature::ATTACK);
			if ( curfeat ) feat.attack=curfeat;
			else feat.attack=new ItemFeatureAttack(ItemFeatureAttack::WIELD_NONE,
				0.0f,0.0f,0.0f,0.0f,0.0f,0.0f,0,0.0f,NULL,NULL,StatusResource::MANA,0.0f);
		} else  if ( strcmp(str->getName(),"light") == 0 ) {
			ItemFeatureLight *curfeat=(ItemFeatureLight *)type->getFeature(ItemFeature::LIGHT);
			if ( curfeat ) feat.light=curfeat;
			else feat.light=new ItemFeatureLight(0,TCODColor::white,0,0.0f,NULL,TCODColor::black,NULL);
		} else  if ( strcmp(str->getName(),"explodeOnBounce") == 0 ) {
			ItemFeatureExplodeOnBounce *curfeat=(ItemFeatureExplodeOnBounce *)type->getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
			if ( curfeat ) feat.explodeOnBounce=curfeat;
			else feat.explodeOnBounce=new ItemFeatureExplodeOnBounce(0.0f,1.0f,1.0f,1.0f,1.0f,0.0f,1.0f,1.0f,0,1.0f,1.0f,0.0f,0.0f,
				TCODColor::white,TCODColor::white);
		} else  if ( strcmp(str->getName(),"effects") == 0 ) {
			redirect=true;
			return effectParser.parserNewStruct(&feat.explodeOnBounce->effects,parser,str,name);
		}
		return true;
	}
	bool parserFlag(TCODParser *parser,const char *name) {
		if ( redirect ) return effectParser.parserFlag(parser,name);
		if ( strcmp(name,"projectile") == 0 ) feat.attack->flags |= ItemFeatureAttack::WEAPON_PROJECTILE;
		else if ( strcmp(name,"randomRadius") == 0 ) feat.light->flags|=ItemFeatureLight::RANDOM_RADIUS;
		else if ( strcmp(name,"invsqrt") == 0 ) feat.light->flags|=ItemFeatureLight::INVSQRT;
		return true;
	}
	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t vtype, TCOD_value_t value) {
		if ( redirect ) return effectParser.parserProperty(parser,name,vtype,value);
		if ( strcmp(name,"inventoryTab") == 0 ) {
			if ( strcmp(value.s,"armor") == 0 ) type->inventoryTab=INV_ARMOR;
			else if ( strcmp(value.s,"weapon") == 0 ) type->inventoryTab=INV_WEAPON;
			else if ( strcmp(value.s,"food") == 0 ) type->inventoryTab=INV_FOOD;
			else if ( strcmp(value.s,"misc") == 0 ) type->inventoryTab=INV_MISC;
		} else if ( strcmp(name,"col") == 0 ) {
			if ( feat.light == NULL ) type->color = value.col;
			else feat.light->color=getHDRColorProperty(value.s);
		} else if ( strcmp(name,"col2") == 0 ) {
			feat.light->color2=getHDRColorProperty(value.s);
		} else if ( strcmp(name,"onPick") == 0 ) {
			type->onPick = strdup(value.s);
		} else if ( strcmp(name,"character") == 0 ) {
			type->character = value.c;
		} else if ( strcmp(name,"length") == 0 ) {
			feat.explodeOnBounce->delay = value.f;
		} else if ( strcmp(name,"startRange") == 0 ) {
			feat.explodeOnBounce->startRange = value.f;
		} else if ( strcmp(name,"endRange") == 0 ) {
			feat.explodeOnBounce->endRange = value.f;
		} else if ( strcmp(name,"startLightCoef") == 0 ) {
			feat.explodeOnBounce->startLightCoef = value.f;
		} else if ( strcmp(name,"endLightCoef") == 0 ) {
			feat.explodeOnBounce->endLightCoef = value.f;
		} else if ( strcmp(name,"middleRange") == 0 ) {
			feat.explodeOnBounce->middleRange = value.f;
		} else if ( strcmp(name,"middleLightCoef") == 0 ) {
			feat.explodeOnBounce->middleLightCoef = value.f;
		} else if ( strcmp(name,"middleTime") == 0 ) {
			feat.explodeOnBounce->middleTime = value.f;
		} else if ( strcmp(name,"particleCount") == 0 ) {
			feat.explodeOnBounce->particleCount = value.i;
		} else if ( strcmp(name,"particleSpeed") == 0 ) {
			feat.explodeOnBounce->particleType.speed = value.f;
		} else if ( strcmp(name,"particleDuration") == 0 ) {
			feat.explodeOnBounce->particleType.duration = value.f;
		} else if ( strcmp(name,"particleDamages") == 0 ) {
			getAttackCoef(value.s,&feat.explodeOnBounce->particleType.minDamage,
			&feat.explodeOnBounce->particleType.maxDamage);
		} else if ( strcmp(name,"particleStartColor") == 0 ) {
			feat.explodeOnBounce->particleType.startColor = getHDRColorProperty(value.s);
		} else if ( strcmp(name,"particleEndColor") == 0 ) {
			feat.explodeOnBounce->particleType.endColor = getHDRColorProperty(value.s);
		} else if ( strcmp(name,"trailLength") == 0 ) {
			type->trailLength = value.i;
		} else if ( strcmp(name,"bounceCoef") == 0 ) {
			type->bounceCoef = value.f;
		} else if ( strcmp(name,"flags") == 0 ) {
			TCODList<const char *> l(value.list);
			for ( const char **it=l.begin(); it!=l.end(); it++) {
				if ( strcmp(*it,"notWalkable") == 0 ) type->flags |= ITEM_NOT_WALKABLE;
				else if ( strcmp(*it,"notTransparent") == 0 ) type->flags |= ITEM_NOT_TRANSPARENT;
				else if ( strcmp(*it,"notPickable") == 0 ) type->flags |= ITEM_NOT_PICKABLE;
				else if ( strcmp(*it,"autoPick") == 0 ) type->flags |= ITEM_AUTOPICK;
				else if ( strcmp(*it,"stackable") == 0 ) type->flags |= ITEM_STACKABLE;
				else if ( strcmp(*it,"softStackable") == 0 ) type->flags |= ITEM_SOFT_STACKABLE;
				else if ( strcmp(*it,"deleteOnUse") == 0 ) type->flags |= ITEM_DELETE_ON_USE;
				else if ( strcmp(*it,"useWhenPicked") == 0 ) type->flags |= ITEM_USE_WHEN_PICKED;
				else if ( strcmp(*it,"container") == 0 ) type->flags |= ITEM_CONTAINER;
				else if ( strcmp(*it,"activateOnBump") == 0 ) type->flags |= ITEM_ACTIVATE_ON_BUMP;
				else if ( strcmp(*it,"an") == 0 ) type->flags |= ITEM_AN;
				else if ( strcmp(*it,"abstract") == 0 ) type->flags |= ITEM_ABSTRACT;
				else if ( strcmp(*it,"notBlock") == 0 ) type->flags |= ITEM_BUILD_NOT_BLOCK;
				else if ( strcmp(*it,"volatile") == 0 ) type->flags |= ITEM_VOLATILE;
				else if ( strcmp(*it,"explosion") == 0 ) type->flags |= ITEM_EXPLOSION;
				else parser->error("unknown flag '%s'",*it);
			}
		} else if ( strcmp(name,"inherits") == 0 ) {
			TCODList<const char *> l(value.list);
			for ( const char **it=l.begin(); it!=l.end(); it++) {
				ItemType *result=ItemType::getType(*it);
				// forward reference to a type not already existing
				if (! result ) {
					parser->error("inherits cannot forward reference an itemType");
					/*
					result = new ItemType();
					ItemType::types.push(result);
					toDefine.push(result);
					result->name=strdup(value.s);
					type->inherits.push(result);
					*/
				} else if (! type->inherits.contains(result)) {
					type->inherits.push(result);
					if (!toDefine.contains(result)) {
						// copy features
						if (result->inventoryTab != INV_MISC ) type->inventoryTab=result->inventoryTab;
						type->flags |= (result->flags & ~ITEM_ABSTRACT);
						if ( type->character == 0 ) type->character=result->character;
						if ( type->color == TCODColor::black ) type->color = result->color;
						for (ItemFeature **it=result->features.begin(); it != result->features.end(); it++) {
							ItemFeature *feat=type->getFeature((*it)->type);
							if (! feat) {
								feat=(*it)->clone();
								type->features.push(feat);
							} else feat->copy(*it);
						}
					}
				}
			}
		} else if ( strcmp(name,"resistance") == 0 ) {
			feat.fire->resistance=value.f;
		} else if ( strcmp(name,"itemType") == 0 ) {
			ItemType *result=ItemType::getType(value.s);
			// forward reference to a type not already existing
			if (! result ) {
				result = new ItemType();
				ItemType::types.push(result);
				toDefine.push(result);
				result->name=strdup(value.s);
			}
			if ( feat.spell->type == ItemFeature::FIRE ) {
				feat.fire->type = result;
			} else if ( feat.spell->type == ItemFeature::PRODUCE ) {
				feat.produce->type=result;
			} else if ( feat.spell->type == ItemFeature::AGE ) {
				feat.age->type=result;
			}
		} else if ( strcmp(name,"ammunition") == 0 ) {
			ItemType *result=ItemType::getType(value.s);
			// forward reference to a type not already existing
			if (! result ) {
				result = new ItemType();
				ItemType::types.push(result);
				toDefine.push(result);
				result->name=strdup(value.s);
			}
			feat.attack->ammunition = result;
			feat.attack->flags |= ItemFeatureAttack::WEAPON_RANGED;
		} else if ( strcmp(name,"casts") == 0 ) {
			ItemType *result=ItemType::getType(value.s);
			// forward reference to a type not already existing
			if (! result ) {
				result = new ItemType();
				ItemType::types.push(result);
				toDefine.push(result);
				result->name=strdup(value.s);
			}
			feat.attack->casts = result;
			feat.attack->flags |= ItemFeatureAttack::WEAPON_RANGED;
		} else if ( strcmp(name,"intensity") == 0 ) {
			feat.heat->intensity=value.f;
		} else if ( strcmp(name,"radius") == 0 ) {
			feat.heat->radius=value.f;
		} else if ( strcmp(name,"delay") == 0 ) {
			if ( feat.spell->type == ItemFeature::PRODUCE ) {
				feat.produce->delay=getDelay(value.s);
			} else if ( feat.spell->type == ItemFeature::AGE ) {
				feat.age->delay=getDelay(value.s);
			} else if ( feat.spell->type == ItemFeature::FOOD ) {
				feat.food->delay=value.f;
			}
		} else if ( strcmp(name,"chance") == 0 ) {
			feat.produce->chance=value.f;
		} else if ( strcmp(name,"health") == 0 ) {
			feat.food->health=value.i;
		} else if ( strcmp(name,"mana") == 0 ) {
			feat.food->mana=value.i;
		} else if ( strcmp(name,"size") == 0 ) {
			feat.container->size=value.i;
		} else if ( strcmp(name,"wield") == 0 ) {
			feat.attack->wield=getWieldType(value.s);
		} else if ( strcmp(name,"castDelay") == 0 ) {
			getAttackCoef(value.s,&feat.attack->minCastDelay, &feat.attack->maxCastDelay);
		} else if ( strcmp(name,"reloadDelay") == 0 ) {
			getAttackCoef(value.s,&feat.attack->minReloadDelay, &feat.attack->maxReloadDelay);
		} else if ( strcmp(name,"damageCoef") == 0 ) {
			getAttackCoef(value.s,&feat.attack->minDamagesCoef, &feat.attack->maxDamagesCoef);
		} else if ( strcmp(name,"range") == 0 ) {
			feat.light->range=value.f;
		} else if ( strcmp(name,"speed") == 0 ) {
			feat.attack->speed=value.f;
		} else if ( strcmp(name,"resourceType") == 0 ) {
			if ( strcmp(value.s,"mana") == 0 ) feat.attack->resourceType=StatusResource::MANA;
			else if ( strcmp(value.s,"health") == 0 ) feat.attack->resourceType=StatusResource::LIFE;
			else parser->error("Unknown resource type %s",value.s);
		} else if ( strcmp(name,"resourceCost") == 0 ) {
			feat.attack->resourceCost=value.f;
		} else if ( strcmp(name,"patternDelay") == 0 ) {
			feat.light->patternDelay=value.f;
		} else if ( strcmp(name,"pattern") == 0 ) {
			feat.light->pattern=strdup(value.s);
		} else if ( strcmp(name,"mode") == 0 ) {
			if ( strcmp(value.s,"mul") == 0 ) feat.light->flags|=ItemFeatureLight::MODE_MUL;
			else if ( strcmp(value.s,"max") == 0 ) feat.light->flags|=ItemFeatureLight::MODE_MAX;
			else if ( strcmp(value.s,"add") == 0 ) ;
			else parser->error("Unknown light mode '%s'. Expect add,mul or max",value.s);
		} else if ( strcmp(name,"colorPattern") == 0 ) {
			feat.light->colorPattern=strdup(value.s);
		}
		return true;
	}
	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str, const char *name) {
		if (redirect) {
			if ( strcmp(str->getName(),"effects" ) != 0 ) {
				return effectParser.parserEndStruct(parser,str,name);
			} else {
				redirect=false;
			}
		} else if( strcmp(str->getName(),"itemType") == 0 ) {
			if ( toDefine.contains(type) ) {
				toDefine.removeFast(type);
				// this type has been forward referenced. apply inheritance
				// doesn't work as we might override the son values by its father's
				// we need a way to know if a property has been overloaded or not
				/*
				for (ItemType **sonit=ItemType::types.begin(); sonit!=ItemType::types.end(); sonit++) {
					if ( (*sonit)->inherits.contains(type) ) {
						// copy features
						if (type->inventoryTab != INV_MISC ) (*sonit)->inventoryTab=type->inventoryTab;
						(*sonit)->flags |= (type->flags & ~ITEM_ABSTRACT);
						if ( (*sonit)->character == 0 ) (*sonit)->character=type->character;
						if ( (*sonit)->color == TCODColor::black ) (*sonit)->color = type->color;
						for (ItemFeature **fit=type->features.begin(); fit != type->features.end(); fit++) {
							ItemFeature *feat=(*sonit)->getFeature((*fit)->type);
							if (! feat) {
								feat=(*fit)->clone();
								(*sonit)->features.push(feat);
							} else {
								feat->copy(*fit);
							}
						}
					}
				}
				*/
			}
			types.pop();
			type = types.isEmpty() ? NULL : types.peek();
		} else if( strcmp(str->getName(),"fire") == 0 ) {
			ItemFeature *itemFeat=type->getFeature(ItemFeature::FIRE);
			if ( itemFeat ) type->features.remove(itemFeat);
			type->features.push(feat.fire);
			feat.spell=NULL;
		} else if( strcmp(str->getName(),"heat") == 0 ) {
			ItemFeature *itemFeat=type->getFeature(ItemFeature::HEAT);
			if ( itemFeat ) type->features.remove(itemFeat);
			type->features.push(feat.heat);
			feat.spell=NULL;
		} else if( strcmp(str->getName(),"produces") == 0 ) {
			ItemFeature *itemFeat=type->getFeature(ItemFeature::PRODUCE);
			if ( itemFeat ) type->features.remove(itemFeat);
			type->features.push(feat.produce);
			feat.spell=NULL;
		} else if( strcmp(str->getName(),"food") == 0 ) {
			ItemFeature *itemFeat=type->getFeature(ItemFeature::FOOD);
			if ( itemFeat ) type->features.remove(itemFeat);
			type->features.push(feat.food);
			feat.spell=NULL;
		} else if( strcmp(str->getName(),"age") == 0 ) {
			ItemFeature *itemFeat=type->getFeature(ItemFeature::AGE);
			if ( itemFeat ) type->features.remove(itemFeat);
			type->features.push(feat.age);
			feat.spell=NULL;
		} else if( strcmp(str->getName(),"spellTree") == 0 ) {
			ItemFeature *itemFeat=type->getFeature(ItemFeature::SPELLTREE);
			if ( itemFeat ) type->features.remove(itemFeat);
			type->features.push(feat.spell);
			feat.spell=NULL;
		} else if( strcmp(str->getName(),"container") == 0 ) {
			ItemFeature *itemFeat=type->getFeature(ItemFeature::CONTAINER);
			if ( itemFeat ) type->features.remove(itemFeat);
			type->features.push(feat.container);
			feat.spell=NULL;
		} else if( strcmp(str->getName(),"attack") == 0 ) {
			ItemFeature *itemFeat=type->getFeature(ItemFeature::ATTACK);
			if ( itemFeat ) type->features.remove(itemFeat);
			type->features.push(feat.attack);
			feat.spell=NULL;
		} else if( strcmp(str->getName(),"light") == 0 ) {
			ItemFeature *itemFeat=type->getFeature(ItemFeature::LIGHT);
			if ( itemFeat ) type->features.remove(itemFeat);
			type->features.push(feat.light);
			feat.spell=NULL;
		} else if( strcmp(str->getName(),"explodeOnBounce") == 0 ) {
			ItemFeature *itemFeat=type->getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
			if ( itemFeat ) type->features.remove(itemFeat);
			type->features.push(feat.explodeOnBounce);
			feat.spell=NULL;
		}
		return true;
	}
	void error(const char *msg) {
		std::cerr << "Fatal error while loading items.cfg : " << msg << std::endl;
		std::abort();
	}

protected :

	union {
		ItemFeature *spell;
		ItemFeatureFire *fire;
		ItemFeatureProduce *produce;
		ItemFeatureFood *food;
		ItemFeatureAge *age;
		ItemFeatureAttack *attack;
		ItemFeatureLight *light;
		ItemFeatureHeat *heat;
		ItemFeatureContainer *container;
		ItemFeatureExplodeOnBounce *explodeOnBounce;
	} feat;
	bool redirect; // redirect to effect parser
	TCODList<ItemType *> types;
	ItemType *type;
	EffectParser effectParser;
	float getDelay(const char *s) {
		float ret;
		char unit[32]="";
		if (sscanf(s,"%f%s",&ret,unit) != 2 ) {
			char buf[1024];
			sprintf(buf,"bad format for delay '%s'. [0-9]*(s|m|h|d) expected",s);
			error(buf);
		}
		if ( strcmp(unit,"m") == 0 ) ret *= 60;
		else if ( strcmp(unit,"h") == 0 ) ret *= 3600;
		else if ( strcmp(unit,"d") == 0 ) ret *= 3600*24;
		else if ( strcmp(unit,"s") != 0 ) {
			char buf[1024];
			sprintf(buf,"bad format for delay '%s'. [0-9]*(s|m|h|d) expected",s);
			error(buf);
		}
		return ret;
	}

	ItemFeatureAttack::WieldType getWieldType(const char *s) {
		if ( strcmp(s,"mainHand") == 0 ) return ItemFeatureAttack::WIELD_MAIN_HAND;
		else if ( strcmp(s,"oneHand") == 0 ) return ItemFeatureAttack::WIELD_ONE_HAND;
		else if ( strcmp(s,"offHand") == 0 ) return ItemFeatureAttack::WIELD_OFF_HAND;
		else if ( strcmp(s,"twoHands") == 0 ) return ItemFeatureAttack::WIELD_TWO_HANDS;
		return ItemFeatureAttack::WIELD_NONE;
	}

	void getAttackCoef(const char *s, float *fmin, float *fmax) {
		if ( strchr(s,'-') ) {
			if (sscanf(s,"%f-%f",fmin,fmax) != 2 ) {
				char buf[1024];
				sprintf(buf,"bad format '%s'.",s);
				error(buf);
			}
		} else {
			*fmin = *fmax = atof(s);
		}
	}
};


class RecipeFileListener : public ITCODParserListener {
public :
	bool parserNewStruct(TCODParser *parser,const TCODParserStruct *str,const char *name) {
		if( strcmp(str->getName(),"recipe") == 0 ) {
			recipe=new ItemCombination();
			recipe->resultType=NULL;
			recipe->nbResult=1;
			recipe->tool=NULL;
			recipe->nbIngredients=0;
			Item::combinations.push(recipe);
		} else if( strcmp(str->getName(),"tool") == 0 ) {
			mode = RECIPE_TOOL;
		} else if( strcmp(str->getName(),"result") == 0 ) {
			mode = RECIPE_RESULT;
		} else if( strcmp(str->getName(),"ingredient") == 0
			|| strcmp(str->getName(),"optionalIngredient") == 0 ) {
			if ( recipe->nbIngredients == MAX_INGREDIENTS-1 ) {
				char buf[256];
				sprintf(buf,"too many ingredients/components (max %d)",MAX_INGREDIENTS);
				error(buf);
			}
			mode = RECIPE_INGREDIENT;
			ingredient = &recipe->ingredients[recipe->nbIngredients];
			ingredient->optional=(strcmp(str->getName(),"optionalIngredient") == 0 );
			ingredient->destroy=true;
			ingredient->quantity=1;
			ingredient->revert=false;
			ingredient->type=NULL;
			recipe->nbIngredients++;
		} else if( strcmp(str->getName(),"component") == 0
			|| strcmp(str->getName(),"optionalComponent") == 0 ) {
			if ( recipe->nbIngredients == MAX_INGREDIENTS-1 ) {
				char buf[256];
				sprintf(buf,"too many ingredients/components (max %d)",MAX_INGREDIENTS);
				error(buf);
			}
			mode = RECIPE_INGREDIENT;
			ingredient = &recipe->ingredients[recipe->nbIngredients];
			ingredient->optional=(strcmp(str->getName(),"optionalComponent") == 0 );
			ingredient->destroy=true;
			ingredient->quantity=1;
			ingredient->revert=true;
			ingredient->type=NULL;
			recipe->nbIngredients++;
		}
		return true;
	}
	bool parserFlag(TCODParser *parser,const char *name) {
		return true;
	}
	bool parserProperty(TCODParser *parser,const char *name, TCOD_value_type_t vtype, TCOD_value_t value) {
		if ( strcmp(name,"itemType") == 0 ) {
			ItemType *result=ItemType::getType(value.s);
			if (! result ) {
				char buf[1024];
				sprintf(buf,"unknown item type '%s'",value.s);
				error(buf);
			}
			switch(mode) {
				case RECIPE_TOOL : recipe->tool=result; break;
				case RECIPE_INGREDIENT : ingredient->type=result; break;
				case RECIPE_RESULT : recipe->resultType=result; break;
			}
		} else if (strcmp(name,"count") == 0 ) {
			recipe->nbResult=value.i;
		}
		return true;
	}
	bool parserEndStruct(TCODParser *parser,const TCODParserStruct *str, const char *name) {
		return true;
	}
	void error(const char *msg) {
		fprintf(stderr,"Fatal error while loading recipes.cfg : %s",msg);
		std::abort();
	}

protected :
	enum { RECIPE_TOOL,RECIPE_INGREDIENT,RECIPE_RESULT } mode;
	ItemCombination *recipe;
	ItemIngredient *ingredient;
};

bool Item::init() {
	if ( ItemType::types.size() != 0 ) return true; // already done
	const char *inventoryTabs[] = {
		"armor", "weapon", "food", "misc", NULL
	};
	const char *wieldTypes[] = {
		"oneHand", "twoHands", "mainHand", "offHand", NULL
	};
	// parse item types
	TCODParser itemParser;
	TCODParserStruct *itemType=itemParser.newStructure("itemType");
	itemType->addValueList("inventoryTab",inventoryTabs,false);
	itemType->addProperty("col",TCOD_TYPE_COLOR,false);
	itemType->addProperty("character",TCOD_TYPE_CHAR,false);
	itemType->addProperty("onPick",TCOD_TYPE_STRING,false);
	itemType->addProperty("trailLength",TCOD_TYPE_INT,false);
	itemType->addProperty("bounceCoef",TCOD_TYPE_FLOAT,false);
	itemType->addListProperty("flags",TCOD_TYPE_STRING,false);
	itemType->addListProperty("inherits",TCOD_TYPE_STRING,false);
	itemType->addStructure(itemType);

	TCODParserStruct *fireEffect=itemParser.newStructure("fire");
	fireEffect->addProperty("resistance",TCOD_TYPE_FLOAT,true);
	fireEffect->addProperty("itemType",TCOD_TYPE_STRING,false);
	itemType->addStructure(fireEffect);

	TCODParserStruct *heat=itemParser.newStructure("heat");
	heat->addProperty("intensity",TCOD_TYPE_FLOAT,true);
	heat->addProperty("radius",TCOD_TYPE_FLOAT,true);
	itemType->addStructure(heat);

	TCODParserStruct *produces=itemParser.newStructure("produces");
	produces->addProperty("delay",TCOD_TYPE_STRING,true);
	produces->addProperty("chance",TCOD_TYPE_FLOAT,true);
	produces->addProperty("itemType",TCOD_TYPE_STRING,true);
	itemType->addStructure(produces);

	TCODParserStruct *food=itemParser.newStructure("food");
	food->addProperty("health",TCOD_TYPE_INT,false);
	food->addProperty("mana",TCOD_TYPE_INT,false);
	food->addProperty("delay",TCOD_TYPE_FLOAT,false);
	itemType->addStructure(food);

	TCODParserStruct *spellTree=itemParser.newStructure("spellTree");
	itemType->addStructure(spellTree);

	TCODParserStruct *ageEffect=itemParser.newStructure("age");
	ageEffect->addProperty("delay",TCOD_TYPE_STRING,true);
	ageEffect->addProperty("itemType",TCOD_TYPE_STRING,false);
	itemType->addStructure(ageEffect);

	TCODParserStruct *container=itemParser.newStructure("container");
	container->addProperty("size",TCOD_TYPE_INT,false);
	itemType->addStructure(container);

	TCODParserStruct *attack=itemParser.newStructure("attack");
	attack->addFlag("projectile");
	attack->addValueList("wield",wieldTypes,true);
	attack->addProperty("castDelay",TCOD_TYPE_STRING,true);
	attack->addProperty("reloadDelay",TCOD_TYPE_STRING,true);
	attack->addProperty("damageCoef",TCOD_TYPE_STRING,true);
	attack->addProperty("ammunition",TCOD_TYPE_STRING,false);
	attack->addProperty("speed",TCOD_TYPE_FLOAT,false);
	attack->addProperty("casts",TCOD_TYPE_STRING,false);
	attack->addProperty("resourceType",TCOD_TYPE_STRING,false);
	attack->addProperty("resourceCost",TCOD_TYPE_FLOAT,false);
	itemType->addStructure(attack);

	TCODParserStruct *light=itemParser.newStructure("light");
	light->addProperty("range",TCOD_TYPE_FLOAT,true);
	light->addProperty("col",TCOD_TYPE_STRING,true);
	light->addProperty("col2",TCOD_TYPE_STRING,false);
	light->addProperty("patternDelay",TCOD_TYPE_FLOAT,false);
	light->addProperty("pattern",TCOD_TYPE_STRING,false);
	light->addProperty("mode",TCOD_TYPE_STRING,false);
	light->addProperty("colorPattern",TCOD_TYPE_STRING,false);
	light->addFlag("randomRadius");
	light->addFlag("invsqrt");
	light->addFlag("diffuse");
	itemType->addStructure(light);

	TCODParserStruct *eob=itemParser.newStructure("explodeOnBounce");
	eob->addProperty("length",TCOD_TYPE_FLOAT,true);
	eob->addProperty("startRange",TCOD_TYPE_FLOAT,false);
	eob->addProperty("startLightCoef",TCOD_TYPE_FLOAT,false);
	eob->addProperty("middleRange",TCOD_TYPE_FLOAT,false);
	eob->addProperty("middleLightCoef",TCOD_TYPE_FLOAT,false);
	eob->addProperty("middleTime",TCOD_TYPE_FLOAT,false);
	eob->addProperty("endRange",TCOD_TYPE_FLOAT,false);
	eob->addProperty("endLightCoef",TCOD_TYPE_FLOAT,false);
	eob->addProperty("particleCount",TCOD_TYPE_INT,false);
	eob->addProperty("particleSpeed",TCOD_TYPE_FLOAT,false);
	eob->addProperty("particleDuration",TCOD_TYPE_FLOAT,false);
	eob->addProperty("particleDamages",TCOD_TYPE_STRING,false);
	eob->addProperty("particleStartColor",TCOD_TYPE_STRING,false);
	eob->addProperty("particleEndColor",TCOD_TYPE_STRING,false);
	TCODParserStruct *eobEffects=itemParser.newStructure("effects");
	eob->addStructure(eobEffects);
	Effect::configureParser(&itemParser,eobEffects);
	itemType->addStructure(eob);

	itemParser.run("data/cfg/items.cfg", new ItemFileListener());

	// check for unresolved forward references
	if ( toDefine.size() > 0 ) {
		for ( ItemType **it=toDefine.begin(); it!=toDefine.end(); it++) {
			fprintf(stderr,"FATAL : no definition for item type '%s'\n",(*it)->name);
		}
		return false;
	}

	// define available action on each item type
	for (ItemType **it=ItemType::types.begin(); it!=ItemType::types.end(); it++ ) {
		(*it)->computeActions();
	}

	// parse recipes
	TCODParser recipeParser;
	TCODParserStruct *recipe=recipeParser.newStructure("recipe");
	TCODParserStruct *tool=recipeParser.newStructure("tool");
	recipe->addStructure(tool);
	tool->addProperty("itemType",TCOD_TYPE_STRING,true);

	TCODParserStruct *ingredient=recipeParser.newStructure("ingredient");
	recipe->addStructure(ingredient);
	ingredient->addProperty("itemType",TCOD_TYPE_STRING,true);
	ingredient->addProperty("count",TCOD_TYPE_INT,false);

	TCODParserStruct *optionalIngredient=recipeParser.newStructure("optionalIngredient");
	recipe->addStructure(optionalIngredient);
	optionalIngredient->addProperty("itemType",TCOD_TYPE_STRING,true);
	optionalIngredient->addProperty("count",TCOD_TYPE_INT,false);

	TCODParserStruct *component=recipeParser.newStructure("component");
	recipe->addStructure(component);
	component->addProperty("itemType",TCOD_TYPE_STRING,true);

	TCODParserStruct *optionalComponent=recipeParser.newStructure("optionalComponent");
	recipe->addStructure(optionalComponent);
	optionalComponent->addProperty("itemType",TCOD_TYPE_STRING,true);

	TCODParserStruct *result=recipeParser.newStructure("result");
	recipe->addStructure(result);
	result->addProperty("itemType",TCOD_TYPE_STRING,true);
	result->addProperty("count",TCOD_TYPE_INT,false);

	recipeParser.run("data/cfg/recipes.cfg", new RecipeFileListener());

	return true;
}

void ItemType::computeActions() {
	if ( (flags & ITEM_NOT_PICKABLE) == 0 ) actions.push(ITEM_ACTION_TAKE);
	if ( getFeature(ItemFeature::FOOD) || getFeature(ItemFeature::ATTACK) ) actions.push(ITEM_ACTION_USE);
	if ( (flags & ITEM_NOT_PICKABLE) == 0 ) actions.push(ITEM_ACTION_DROP);
	if ( (flags & ITEM_NOT_PICKABLE) == 0 ) actions.push(ITEM_ACTION_THROW);
	if ( hasComponents() ) actions.push(ITEM_ACTION_DISASSEMBLE);
	if ( isA("liquid container") ) actions.push(ITEM_ACTION_FILL);
}

ItemType *ItemType::getType(const char *name) {
	if ( name == NULL ) return NULL;
	if ( types.size() == 0 ) {
		if (!Item::init()) std::abort(); // fatal error. cannot load items configuration
	}
	for (ItemType **it=types.begin(); it!=types.end(); it++) {
		if ( strcmp((*it)->name,name) == 0 ) return *it;
	}
	return NULL;
}

bool ItemType::hasComponents() const {
	for (ItemCombination **cur=Item::combinations.begin(); cur != Item::combinations.end();cur++) {
		if ((*cur)->resultType == this) {
			// check if ingredients can be reverted
			for (int i=0; i < (*cur)->nbIngredients; i++) {
				if ( (*cur)->ingredients[i].revert ) return true;
			}
		}
	}
	return false;
}

bool ItemCombination::isTool(const Item *item) const {
	return isTool(item->typeData);
}

bool ItemCombination::isIngredient(const Item *item) const {
	return isIngredient(item->typeData);
}

bool ItemCombination::isTool(const ItemType *itemType) const {
	return (itemType->isA(tool));
}

bool ItemCombination::isIngredient(const ItemType *itemType) const {
	for (int i=0; i < nbIngredients; i++) {
		if (itemType->isA(ingredients[i].type)) return true;
	}
	return false;
}

const ItemIngredient * ItemCombination::getIngredient(Item *item) const {
	for (int i=0; i < nbIngredients; i++) {
		if (item->isA(ingredients[i].type)) return &ingredients[i];
	}
	return NULL;
}

bool ItemType::isIngredient() const {
	for (ItemCombination **cur=Item::combinations.begin(); cur != Item::combinations.end();cur++) {
		if ((*cur)->isIngredient(this)) return true;
	}
	return false;
}

bool ItemType::isTool() const {
	for (ItemCombination **cur=Item::combinations.begin(); cur != Item::combinations.end();cur++) {
		if ( (*cur)->isTool(this) ) return true;
	}
	return false;
}

ItemCombination *ItemType::getCombination() const {
	for (ItemCombination **cur=Item::combinations.begin(); cur != Item::combinations.end();cur++) {
		if ((*cur)->resultType == this) return *cur;
	}
	return NULL;
}

Item::Item(float x,float y, const ItemType &type):
	active(false)  {
	if (descCon == NULL) {
		descCon=new TCODConsole(CON_W/2,CON_H/2);
		descCon->setAlignment(TCOD_CENTER);
		descCon->setDefaultBackground(guiBackground);
	}
	setPos(x,y);
	light=NULL;
	count=1;
	owner=NULL;
	container=NULL;
	asCreature=NULL;
	adjective=NULL;
	typeName=strdup(type.name);
	name=NULL;
	typeData=&type;
	toDelete=0;
	ItemFeatureLight *feat=(ItemFeatureLight *)getFeature(ItemFeature::LIGHT);
	if ( feat ) {
		light = new ExtendedLight();
		light->x=x*2;
		light->y=y*2;
		light->range=feat->range;
		light->color=feat->color;
		light->setup(feat->color2,feat->patternDelay,feat->pattern,feat->colorPattern);
		light->flags=0;
		if ( feat->flags & ItemFeatureLight::RANDOM_RADIUS ) light->flags |= Light::RANDOM_RAD;
		if ( feat->flags & ItemFeatureLight::INVSQRT ) light->flags |= Light::INVSQRT;
		if ( feat->flags & ItemFeatureLight::MODE_MUL ) light->drawMode = Light::MODE_MUL;
		else if ( feat->flags & ItemFeatureLight::MODE_MAX ) light->drawMode = Light::MODE_MAX;
	}
	col=type.color;
	ch=type.character;
	itemClass=ITEM_CLASS_STANDARD;
	ItemFeatureAge *featAge=(ItemFeatureAge *)typeData->getFeature(ItemFeature::AGE);
	if ( featAge ) life = featAge->delay;
	else life=0.0f;
	ItemFeatureFire *featFire=(ItemFeatureFire *)typeData->getFeature(ItemFeature::FIRE);
	if ( featFire ) fireResistance = featFire->resistance;
	else fireResistance=0.0f;
	ItemFeatureAttack *featAttack=(ItemFeatureAttack *)typeData->getFeature(ItemFeature::ATTACK);
	castDelay=reloadDelay=damages=0.0f;
	if ( featAttack ) {
		castDelay = rng->getFloat(featAttack->minCastDelay,featAttack->maxCastDelay);
		reloadDelay = rng->getFloat(featAttack->minReloadDelay,featAttack->maxReloadDelay);
		if ( reloadDelay + castDelay > 0.0f )
			damages = 15 * (reloadDelay + castDelay ) * rng->getFloat(featAttack->minDamagesCoef,featAttack->maxDamagesCoef);
		else
			damages = rng->getFloat(featAttack->minDamagesCoef,featAttack->maxDamagesCoef);
	}

	if ( type.flags & ITEM_EXPLOSION ) {
		phase=EXPLODE;
		ItemFeatureExplodeOnBounce *featExplode=(ItemFeatureExplodeOnBounce *)typeData->getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
		if ( featExplode ) {
			phaseTimer = featExplode->delay;
		}
	} else {
		phase=IDLE;
		phaseTimer=0.0f;
	}
	targetx=targety=-1;
	heatTimer=0.0f;
	onoff=false;
	article=NONE;
}

Item *Item::getItem(const char *typeName, float x, float y, bool createComponents) {
	ItemType *type=ItemType::getType(typeName);
	if (! type ) {
		printf("FATAL : unknown item type '%s'\n",typeName);
		return NULL;
	}
	return Item::getItem(type,x,y,createComponents);
}

Item *Item::getItem(const ItemType *type, float x, float y, bool createComponents) {
	Item *ret=new Item(x,y,*type);
	if (createComponents) ret->generateComponents();
	return ret;
}

#define MAX_RELOAD_BONUS 0.2f
#define MAX_CAST_BONUS 0.2f
Item *Item::getRandomWeapon(const char *typeName,ItemClass itemClass) {
	if (! textgen ) {
		textgen=new TextGenerator("data/cfg/weapon.txg",rng);
		textgen->parseFile();
	}
	ItemType *type=ItemType::getType(typeName);
	if (! type ) {
		printf ("FATAL : unknown weapon type '%s'\n",typeName);
		return NULL;
	}
	Item * weapon = Item::getItem(type,-1,-1,false);
	weapon->itemClass=itemClass;
	weapon->col = Item::classColor[itemClass];
	if ( itemClass > ITEM_CLASS_STANDARD ) {
		/*
		TODO
		int goatKey = 6 * gameEngine->player->school.type + id;
		weapon->name = strdup( textgen->generate("weapon","${%s}",
			textgen->generators.peek()->rules.get(goatKey)->name ));
		*/
	}
	enum { MOD_RELOAD, MOD_CAST, MOD_MODIFIER };
	for (int i=0; i < itemClass; i++) {
        int modType = rng->getInt(MOD_RELOAD,MOD_MODIFIER);
        switch(modType) {
            case MOD_RELOAD :
                weapon->reloadDelay -= rng->getFloat(0.05f, MAX_RELOAD_BONUS);
                weapon->reloadDelay = std::max(0.1f,weapon->reloadDelay);
            break;
            case MOD_CAST :
                weapon->castDelay -= rng->getFloat(0.05f, MAX_CAST_BONUS);
                weapon->castDelay = std::max(0.1f,weapon->reloadDelay);
            break;
            case MOD_MODIFIER :
                ItemModifierId id=(ItemModifierId)0;
                switch (gameEngine->player->school.type ) {
                    case SCHOOL_FIRE :
                        id=(ItemModifierId)rng->getInt(ITEM_MOD_FIRE_BEGIN,ITEM_MOD_FIRE_END);
                    break;
                    case SCHOOL_WATER :
                        id=(ItemModifierId)rng->getInt(ITEM_MOD_WATER_BEGIN,ITEM_MOD_WATER_END);
                    break;
                    default:break;
                }
                weapon->addModifier(id,rng->getFloat(0.1f,0.2f));
            break;
        }
	}
	weapon->damages += weapon->damages * (int)(itemClass)*0.2f; // 20% increase per color level
	weapon->damages = std::min(1.0f,weapon->damages);
	// build components
	weapon->generateComponents();
    return weapon;
}

void Item::addComponent(Item *component) {
	components.push(component);
	if ( component->adjective && ! adjective ) {
		adjective = strdup(component->adjective);
	}
}

void Item::generateComponents() {
	// check if this item type has components
	ItemCombination *combination=getCombination();
	if (! combination) return;
	int maxOptionals=itemClass-ITEM_CLASS_STANDARD;
	int i=rng->getInt(0,combination->nbIngredients-1);
	for (int count=combination->nbIngredients; count > 0 ; count--) {
		if (combination->ingredients[i].revert &&
			(!combination->ingredients[i].optional || maxOptionals > 0 )) {
			if ( combination->ingredients[i].optional ) maxOptionals--;
			ItemType *componentType=combination->ingredients[i].type;
			if ( componentType ) {
				Item *component=Item::getItem(componentType,x,y);
				addComponent(component);
			}
		}
		i = (i+1)%combination->nbIngredients;
	}
}

Item::~Item() {
	if ( light ) delete light;
	free(typeName);
	if ( adjective ) free(adjective);

}

// look for a 2 items recipe
ItemCombination *Item::getCombination(const Item *it1, const Item *it2) {
	for (ItemCombination **cur=Item::combinations.begin(); cur != Item::combinations.end();cur++) {
		if ( (*cur)->nbIngredients == 1 && (*cur)->tool != NULL) {
			// tool + 1 ingredient
			if ( it1->isA((*cur)->tool) && it2->isA((*cur)->ingredients[0].type)  ) return *cur;
			if ( it2->isA((*cur)->tool) && it1->isA((*cur)->ingredients[0].type)  ) return *cur;
		} else if ( (*cur)->nbIngredients == 2 && (*cur)->tool == NULL ) {
			// 2 ingredients (no tool)
			if ( it1->isA((*cur)->ingredients[0].type) && it2->isA((*cur)->ingredients[1].type) ) return *cur;
			if ( it2->isA((*cur)->ingredients[0].type) && it1->isA((*cur)->ingredients[1].type) ) return *cur;
		}
	}
	return NULL;
}

bool Item::hasComponents() const {
	return typeData->hasComponents();
}

ItemCombination *Item::getCombination() const {
	return typeData->getCombination();
}

// put this inside 'it' container (false if no more room)
Item *Item::putInContainer(Item *it) {
	ItemFeatureContainer *cont=(ItemFeatureContainer *)it->getFeature(ItemFeature::CONTAINER);
	if ( ! cont ) return NULL; // it is not a container
	if ( it->stack.size() >= cont->size ) return nullptr; // no more room
	Item *item = addToList(&it->stack);
	item->container=it;
	item->x=it->x;
	item->y=it->y;
	return item;
}

// remove it from this container (false if not inside)
Item *Item::removeFromContainer(int count) {
	if ( ! container || ! container->stack.contains(this) ) return NULL;
	Item *item=removeFromList(&container->stack,count);
	item->container=NULL;
	return item;
}


// add to the list, posibly stacking
Item * Item::addToList(TCODList<Item *> *list) {
	if (isStackable()) {
		// if already in the list, increase the count
		for (Item **it=list->begin(); it != list->end(); it++) {
			if ( (*it)->typeData == typeData && (! (*it)->name || (name && strcmp((*it)->name,name) == 0) ) ) {
				(*it)->count += count;
				toDelete=count;
				return *it;
			}
		}
	} else if ( isSoftStackable() ) {
		// if already in the list, add to soft stack
		for (Item **it=list->begin(); it != list->end(); it++) {
			if ( (*it)->typeData == typeData && ! (*it)->name ) {
				(*it)->stack.push(this);
				return *it;
			}
		}
	}
	// add new item in the list
	list->push(this);
	return this;
}
// remove one item, possibly unstacking
Item * Item::removeFromList(TCODList<Item *> *list, int removeCount, bool fast) {
	if (isStackable() && count > removeCount) {
		Item *newItem=Item::getItem(typeData, x, y);
		newItem->count=removeCount;
		count -= removeCount;
		return newItem;
	} else if ( isSoftStackable() ) {
		if ( stack.size() > 0) {
			Item *newStackOwner=NULL;
			// this item is the stack owner. rebuild the stack
			if ( stack.size() > removeCount ) {
				newStackOwner=stack.pop();
				while ( stack.size() > removeCount ) {
					newStackOwner->stack.push(stack.pop());
				}
			}
			// remove before adding to avoid list reallocation
			if (fast) list->removeFast(this);
			else list->remove(this);
			if ( newStackOwner ) newStackOwner->addToList(list);
			return this;
		} else {
			// this item may be in a stack. find the stack owner
			for (Item **stackOwner=list->begin(); stackOwner!=list->end(); stackOwner++) {
				if ((*stackOwner) != this && (*stackOwner)->typeData == typeData && (*stackOwner)->stack.contains(this) ) {
					return (*stackOwner)->removeFromList(list,removeCount,fast);
				}
			}
			// single softStackable item. simply remove it from the list
		}
	} else if ( container && list->contains(container)) {
		// item is inside a container
		removeFromContainer(removeCount);
	}
	if (fast) list->removeFast(this);
	else list->remove(this);
	return this;
}


void Item::computeBottleName() {
	if ( name ) free(name);
	name=NULL;
	if ( stack.isEmpty() ) {
		article=AN;
		name=strdup("empty bottle");
		return;
	}
	Item *liquid = stack.get(0);
	if ( strcmp(liquid->typeData->name,"water") == 0 ) {
		article=A; name=strdup("bottle of water");
	} else if ( strcmp(liquid->typeData->name,"poison") == 0 ) {
		article=A; name=strdup("bottle of poison");
	} else if ( strcmp(liquid->typeData->name,"sleep") == 0 ) {
		article=A; name=strdup("bottle of soporific");
	} else if ( strcmp(liquid->typeData->name,"antidote") == 0 ) {
		article=AN; name=strdup("antidote");
	} else if ( strcmp(liquid->typeData->name,"health") == 0 ) {
		article=A; name=strdup("health potion");
	} else {
		article = liquid->article;
		name=strdup(liquid->typeData->name);
	}
}

void Item::render(LightMap *lightMap, bool subCellPhase) {
	if ( subCellPhase && typeData->trailLength && speed > 0.0f ) {
		HDRColor hcol=typeData->color;
		float curx=x*2-gameEngine->xOffset*2;
		float cury=y*2-gameEngine->yOffset*2;
		for (int i=0; i < typeData->trailLength; i++ ) {
			int icurx=(int)curx;
			int icury=(int)cury;
			if ( IN_RECTANGLE(icurx,icury,lightMap->width,lightMap->height) ) {
				HDRColor lcol=lightMap->getColor2x(icurx,icury);
				lcol=lcol+hcol;
				lightMap->setColor2x(icurx,icury,lcol);
			}
			curx -= dx;
			cury -= dy;
			hcol = hcol*0.8f;
		}
	}
	if ( subCellPhase ) return;
	int conx=(int)(x-gameEngine->xOffset);
	int cony=(int)(y-gameEngine->yOffset);
	if ( !IN_RECTANGLE(conx,cony,CON_W,CON_H) ) return;
	if ( ch ) {
		TCODColor lightColor=lightMap->getColor(conx,cony);
		TCODConsole::root->setChar(conx,cony,ch);
		TCODConsole::root->setCharForeground(conx,cony,col*lightColor);
	}
}

void Item::renderDescription(int x, int y, bool below) {
	int cy=0;
	descCon->clear();
	descCon->setDefaultForeground(Item::classColor[itemClass]);
	if (name) {
		if ( count > 1 ) descCon->print(CON_W/4,cy++,"%s(%d)",name,count);
		else descCon->print(CON_W/4,cy++,name);
		descCon->setDefaultForeground(guiText);
		descCon->print(CON_W/4,cy++,typeName);
	} else {
		if ( count > 1 ) descCon->print(CON_W/4,cy++,"%s(%d)",typeName,count);
		else descCon->print(CON_W/4,cy++,typeName);
	}
	descCon->setDefaultForeground(guiText);
	ItemFeatureFood *featFood=(ItemFeatureFood *)getFeature(ItemFeature::FOOD);
	if ( featFood ) {
		if ( featFood->health ) descCon->print(CON_W/4,cy++,"Health:+%d",featFood->health);
		if ( featFood->mana ) descCon->print(CON_W/4,cy++,  "Mana  :+%d",featFood->mana);
	}
	ItemFeatureAttack *featAttack = (ItemFeatureAttack *)getFeature(ItemFeature::ATTACK);
	if ( featAttack ) {
		static const char *wieldname[] = {
			NULL, "One hand", "Main hand", "Off hand", "Two hands"
		};
		if ( featAttack->wield ) {
			descCon->print(CON_W/4,cy++,wieldname[featAttack->wield]);
		}
		if ( castDelay + reloadDelay > 0.0f ) {
			float rate=1.0f / (castDelay + reloadDelay);
			int dmgPerSec = (int)(damages *rate + 0.5f);
			descCon->print(CON_W/4,cy++,"%d damages/sec", dmgPerSec);
			descCon->print(CON_W/4,cy++,"Attack rate:%s",getRateName(rate));
		} else {
			descCon->print(CON_W/4,cy++,"%d damages", (int)damages);
		}
		ItemModifier::renderDescription(descCon,2,cy,modifiers);
	}

/*
	y--;
	if ( y < 0 ) y = 2;
	TCODConsole::root->setDefaultForeground(Item::classColor[itemClass]);
	TCODConsole::root->printEx(x,y,TCOD_BKGND_NONE,TCOD_CENTER,typeName);
*/
	renderDescriptionFrame(x,y,below);
}

const char *Item::getRateName(float rate) const {
	static const char *ratename[] = {
		"Very slow",
		"Slow",
		"Average",
		"Fast",
		"Very fast"
	};
	int rateIdx=0;
	if ( rate <= 0.5f ) rateIdx = 0;
	else if ( rate <= 1.0f ) rateIdx = 1;
	else if ( rate <= 3.0f ) rateIdx = 2;
	else if ( rate <= 5.0f ) rateIdx = 3;
	else rateIdx = 4;
	return ratename[rateIdx];
}

void Item::renderGenericDescription(int x, int y, bool below, bool frame) {
	int cy=0;
	descCon->clear();
	descCon->setDefaultForeground(Item::classColor[itemClass]);
	if (name) {
		if ( count > 1 ) descCon->print(CON_W/4,cy++,"%s(%d)",name,count);
		else descCon->print(CON_W/4,cy++,name);
		descCon->setDefaultForeground(guiText);
		descCon->print(CON_W/4,cy++,typeName);
	} else {
		if ( count > 1 ) descCon->print(CON_W/4,cy++,"%s(%d)",typeName,count);
		else descCon->print(CON_W/4,cy++,typeName);
	}
	descCon->setDefaultForeground(guiText);
	ItemFeatureFood *featFood=(ItemFeatureFood *)getFeature(ItemFeature::FOOD);
	if ( featFood ) {
		if ( featFood->health ) descCon->print(CON_W/4,cy++,"Health:+%d",featFood->health);
		if ( featFood->mana ) descCon->print(CON_W/4,cy++,  "Mana  :+%d",featFood->mana);
	}
	ItemFeatureAttack *featAttack = (ItemFeatureAttack *)getFeature(ItemFeature::ATTACK);
	if ( featAttack ) {
		static const char *wieldname[] = {
			NULL, "One hand", "Main hand", "Off hand", "Two hands"
		};
		if ( featAttack->wield ) {
			descCon->print(CON_W/4,cy++,wieldname[featAttack->wield]);
		}
		float minCast=featAttack->minCastDelay - itemClass*MAX_CAST_BONUS;
		float minReload=featAttack->minReloadDelay - itemClass*MAX_RELOAD_BONUS;
		float minDamages = 15 * (minCast + minReload ) * featAttack->minDamagesCoef;
		float maxDamages = 15 * (featAttack->maxCastDelay + featAttack->maxReloadDelay ) * featAttack->maxDamagesCoef;
    	minDamages += minDamages * (int)(itemClass)*0.2f;
    	maxDamages += maxDamages * (int)(itemClass)*0.2f;
    	minDamages=std::min<int>(1.0f,minDamages);
    	maxDamages=std::min<int>(1.0f,maxDamages);

		if ( minDamages != maxDamages ) {
			descCon->print(CON_W/4,cy++,"%d-%d damages/hit", (int)minDamages,(int)maxDamages);
		} else {
			descCon->print(CON_W/4,cy++,"%d damages/hit", (int)minDamages);
		}

		float minRate=1.0f / (featAttack->maxCastDelay + featAttack->maxReloadDelay);
		float maxRate=1.0f / (minCast + minReload);

		const char *rate1=getRateName(minRate);
		const char *rate2=getRateName(maxRate);
		if ( rate1 == rate2 ) {
			descCon->print(CON_W/4,cy++,"Attack rate:%s",rate1);
		} else {
			descCon->print(CON_W/4,cy++,"Attack rate:%s-%s",rate1,rate2);
		}
		//ItemModifier::renderDescription(descCon,2,cy,modifiers);
	}

	renderDescriptionFrame(x,y,below,frame);
}

void Item::convertTo(ItemType *newType) {
	// create the new item
	Item *newItem=Item::getItem(newType, x, y);
	newItem->speed = speed;
	newItem->dx = dx;
	newItem->dy = dy;
	if (isA("wall")) newItem->ch = ch;
	if ( owner ) {
		if ( owner->isPlayer() ) {
			if ( asCreature ) gameEngine->gui.log.info("%s died.",TheName());
			else {
				if ( strncmp(newType->name,"rotten",6) == 0 ) {
					gameEngine->gui.log.info("%s has rotted.",TheName());
				} else {
					gameEngine->gui.log.info("%s turned into %s.",TheName(), newItem->aName());
				}
			}
		}
		owner->addToInventory(newItem);
	} else gameEngine->dungeon->addItem(newItem);
}

bool Item::age(float elapsed, ItemFeatureAge *feat) {
	if (! feat ) feat=(ItemFeatureAge *)typeData->getFeature(ItemFeature::AGE);
	if ( feat ) {
		life -= elapsed;
		if (life <= 0.0f ) {
			if ( feat->type ) {
				convertTo(feat->type);
			}
			// destroy this item
			if (! owner ) {
				if ( asCreature ) gameEngine->dungeon->removeCreature(asCreature,false);
			} else {
				if ( asCreature ) asCreature->toDelete=true;
			}
			return false;
		}
	}
	return true;
}

void Item::useFeature(ItemFeature::Type type) {
	ItemFeature *feat=getFeature(type);
	if ( feat ) feat->use(this);
}

void Item::explode(bool wallCollision) {
	ItemFeatureExplodeOnBounce *eob = (ItemFeatureExplodeOnBounce *)getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
	phaseTimer=eob->delay;
	phase=EXPLODE;
	speed=0.0f;
	for (int i=0; i < eob->particleCount; i++ ) {
		Particle *p=new Particle(&eob->particleType);
		gameEngine->addParticle(p);
		p->x=x;
		p->y=y;
		float angle=atan2f(-dy,-dx);
		if ( wallCollision ) {
			angle+=TCODRandom::getInstance()->getFloat(-1.5f,1.5f);
		} else {
			angle+=TCODRandom::getInstance()->getFloat(-M_PI,M_PI);
		}
		p->dx=cosf(angle);
		p->dy=sinf(angle);
	}
}

bool Item::update(float elapsed, TCOD_key_t key, TCOD_mouse_t *mouse) {
	static int deltax[]={0,1,-1,0,0};
	static int deltay[]={0,0,0,1,-1};
	Dungeon *dungeon=gameEngine->dungeon;
	if (! owner && ! isOnScreen() ) {
		// when not on screen, update only once per second
		cumulatedElapsed += elapsed;
		if (cumulatedElapsed < 1.0f ) return true;
		elapsed=cumulatedElapsed;
		cumulatedElapsed=0.0f;
	}
	if ( speed > 0.0f ) {
		float oldx=x;
		float oldy=y;
		ItemFeatureExplodeOnBounce *eob = (ItemFeatureExplodeOnBounce *)getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
		ItemFeatureAttack *attack=(ItemFeatureAttack *)getFeature(ItemFeature::ATTACK);
		ItemFeatureHeat *heat=(ItemFeatureHeat *)getFeature(ItemFeature::HEAT);
		DynamicEntity::CollisionType type=updateMove(elapsed,typeData->bounceCoef,
			!owner->isPlayer(),owner->isPlayer(),true);
		if ( light ) {
			light->x=x*2;
			light->y=y*2;
		}
		switch (type) {
			case WALL :
				if ( eob ) {
					explode(true);
				} else if ( typeData->bounceCoef == 0.0f ) return false;
			break;
			case PLAYER :
				if ( attack ) {
					gameEngine->player->takeDamage(TCODRandom::getInstance()->getFloat
						(attack->minDamagesCoef,attack->maxDamagesCoef));
				}
				if ( eob ) {
					explode(false);
				} else return false;
			break;
			case CREATURE :
				if ( attack ) {
					for (int i=0; i < 5; i++ ) {
						Creature *cr=dungeon->getCreature(x+deltax[i],y+deltay[i]);
						if (cr) {
							cr->takeDamage(TCODRandom::getInstance()->getFloat(attack->minDamagesCoef,attack->maxDamagesCoef));
						}
					}
				}
				if ( eob ) {
					explode(false);
				} else return false;
			break;
			case ITEM :
				if ( heat ) {
					TCODList<Item *> *items=dungeon->getItems(x,y);
					if (items && items->size()>0) {
						for (Item **it=items->begin();it!=items->end();it++) {
							if (*it != this) {
								if ( (*it)->hasFeature(ItemFeature::FIRE) ) {
									float heatDmg=heat->intensity;
									(*it)->fireResistance -= heatDmg;
								}
							}
						}
					}
				}
				if ( eob ) {
					explode(false);
				} else return false;
			break;
			case END_DURATION :
				if ( dungeon->hasRipples(x,y) ) {
					// a projectile falls in water
					gameEngine->startRipple(x,y);
				}
				if ( isA("arrow") ) {
					return false;
				}
			break;
			case NONE : break;
		}
		if ( !isVolatile() && ((int)x != (int)oldx || (int)y != (int)oldy ) ) {
			dungeon->getCell(oldx,oldy)->items.removeFast(this);
			dungeon->getCell(x,y)->items.push(this);
		}
	/*
		ItemFeatureExplodeOnBounce *eob = (ItemFeatureExplodeOnBounce *)getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
		float oldx=x;
		float oldy=y;
		x += speed*dx*elapsed;
		y += speed*dy*elapsed;
		if ( ! IN_RECTANGLE(x,y,dungeon->width,dungeon->height) ) {
			x=oldx;y=oldy;
			if ( eob ) {
				explode(true);
			} else return false;
		}
		int curoldx=(int)oldx;
		int curoldy=(int)oldy;
		TCODLine::init(curoldx,curoldy,(int)x,(int)y);
		while ( ! TCODLine::step(&curoldx,&curoldy) ) {
			if (! dungeon->isCellWalkable(curoldx,curoldy)) {
				// bounce against a wall
				float newx=curoldx;
				float newy=curoldy;
				int cdx=(int)(curoldx-oldx);
				int cdy=(int)(curoldy-oldy);
				curoldx=(int)oldx;curoldy=(int)oldy;
				speed *= typeData->bounceCoef;
				if ( phase != EXPLODE ) {
					if ( eob ) {
						x=oldx;y=oldy;
						explode(true);
						break;
					}
				}
				if ( cdx == 0 ) {
					// hit horizontal wall
					dy=-dy;
				} else if (cdy == 0 ) {
					// hit vertical wall
					dx=-dx;
				} else {
					bool xwalk=dungeon->isCellWalkable(newx,oldy);
					bool ywalk=dungeon->isCellWalkable(oldx,newy);
					if ( xwalk && ywalk ) {
						// outer corner bounce. detect which side of the cell is hit
						//  ##
						//  ##
						// .
						float fdx=std::abs(dx);
						float fdy=std::abs(dy);
						if ( fdx >= fdy ) dy=-dy;
						if ( fdy >= fdx ) dx=-dx;
					} else if (! xwalk ) {
						if ( ywalk ) {
							// vertical wall bounce
							dx=-dx;
						} else {
							// inner corner bounce
							// ##
							// .#
							dx=-dx;dy=-dy;
						}
					} else {
						// horizontal wall bounce
						dy=-dy;
					}
				}
			} else {
				ItemFeatureAttack *feat=(ItemFeatureAttack *)getFeature(ItemFeature::ATTACK);
				if ( feat ) {
					if (!owner->isPlayer() && std::abs(curoldx - (int)gameEngine->player->x) < 2
						&& std::abs(curoldy - (int)gameEngine->player->y) < 2 ) {
							// a projectile hits the player
							gameEngine->player->takeDamage(TCODRandom::getInstance()->getFloat(feat->minDamagesCoef,feat->maxDamagesCoef));
							x=oldx;y=oldy;
							speed=0;
							if ( eob ) {
								explode(false);
							} else return false;
					} else if ( owner->isPlayer() && (
						dungeon->hasCreature(curoldx,curoldy)
						|| dungeon->hasCreature(curoldx+1,curoldy)
						|| dungeon->hasCreature(curoldx-1,curoldy)
						|| dungeon->hasCreature(curoldx,curoldy+1)
						|| dungeon->hasCreature(curoldx,curoldy-1))
					) {
						// creature hit
						for (int i=0; i < 5; i++ ) {
							Creature *cr=dungeon->getCreature(curoldx+deltax[i],curoldy+deltay[i]);
							if (cr) {
								cr->takeDamage(TCODRandom::getInstance()->getFloat(feat->minDamagesCoef,feat->maxDamagesCoef));
								// TODO
								//cr->stun(typeData->stunDelay);
							}
						}
						x=oldx;y=oldy;
						speed=0;
						if ( eob ) {
							explode(false);
						} else return false;
					}
				}
				ItemFeatureHeat *featHeat=(ItemFeatureHeat *)getFeature(ItemFeature::HEAT);
				if ( featHeat ) {
					// item hit ?
					bool hit=false;
					TCODList<Item *> *items=dungeon->getItems(curoldx,curoldy);
					if (items && items->size()>0) {
						for (Item **it=items->begin();it!=items->end();it++) {
							if (*it != this) {
								if ( (*it)->hasFeature(ItemFeature::FIRE) ) {
									float heatDmg=featHeat->intensity;
									(*it)->fireResistance -= heatDmg;
									hit=true;
								}
							}
						}
					}
					if ( hit ) {
						x=oldx;y=oldy;
						speed=0;
						if ( eob ) {
							explode(false);
						} else return false;
					}
				}
			}
			oldx=curoldx;
			oldy=curoldy;
		}
		*/
	}

	// update features
	for (ItemFeature **it=typeData->features.begin(); it != typeData->features.end(); it++ ) {
		if (! (*it)->update(elapsed,mouse,this) ) return false;
	}
	return true;
}

bool Item::isEquiped() {
	return ( owner && (owner->mainHand == this || owner->offHand == this) );
}

void Item::destroy(int count) {
	if ( this->count > count ) this->count -= count;
	else toDelete=true;
}

void Item::use() {
	active=true;
	useFeature(ItemFeature::PRODUCE);
	useFeature(ItemFeature::FOOD);
	useFeature(ItemFeature::ATTACK);
	useFeature(ItemFeature::CONTAINER);
	if ( isA( "door" ) ) {
		onoff=!onoff;
		gameEngine->gui.log.info("You %s %s", onoff ? "open":"close", theName());
		ch = onoff ? '/':'+';
		gameEngine->dungeon->setProperties((int)x,(int)y,onoff,onoff);
	}
	if ( isDeletedOnUse() ) {
		if ( count > 1 ) count --;
		else {
			toDelete=true;
		}
	}
	useFeature(ItemFeature::SPELLTREE);
}

Item * Item::putInInventory(Creature *owner, int putCount, const char *verb) {
	if ( putCount == 0 ) putCount=count;
	Item *it = gameEngine->dungeon->removeItem(this,putCount,false);
	it->owner=owner;
	if ( it->typeData->onPick ) {
		if ( it->name ) free(it->name);
		it->name = strdup(it->typeData->onPick);
	}
	if ( verb != NULL ) gameEngine->gui.log.info("You %s %s.", verb,it->aName());
	it=owner->addToInventory(it);
	ItemFeatureAttack *feat=(ItemFeatureAttack *)it->getFeature(ItemFeature::ATTACK);
	if ( feat ) {
		// auto equip weapon if hand is empty
		if (owner->mainHand == NULL &&
			(feat->wield == ItemFeatureAttack::WIELD_ONE_HAND || feat->wield == ItemFeatureAttack::WIELD_MAIN_HAND ) ) {
			owner->mainHand=it;
		} else if ( owner->offHand == NULL &&
			(feat->wield == ItemFeatureAttack::WIELD_ONE_HAND || feat->wield == ItemFeatureAttack::WIELD_OFF_HAND ) ) {
			owner->offHand=it;
		} else if ( owner->mainHand == NULL && owner->offHand == NULL && feat->wield == ItemFeatureAttack::WIELD_TWO_HANDS ) {
			owner->mainHand = owner->offHand = it;
		}
	}

	if ( it->isUsedWhenPicked() ) {
		it->use();
	}
	return it;
}

Item * Item::drop(int dropCount) {
	if (! owner ) return this;
	if ( dropCount == 0 ) dropCount=count;
	Creature *ownerBackup=owner; // keep the owner for once the item is removed from inventory
	Item *newItem = owner->removeFromInventory(this,dropCount);
	newItem->x=ownerBackup->x;
	newItem->y=ownerBackup->y;
	if ( asCreature ) {
		// convert item back to creature
		gameEngine->dungeon->addCreature(asCreature);
	} else {
		gameEngine->dungeon->addItem(newItem);
	}
	gameEngine->gui.log.info("You drop %s %s.", newItem->theName(),
		gameEngine->dungeon->hasRipples(newItem->x,newItem->y) ? "in the water" : "on the ground"
	);
	return newItem;
}

void Item::addModifier(ItemModifierId id, float value) {
    for ( ItemModifier **mod = modifiers.begin(); mod != modifiers.end(); mod ++) {
        if ( (*mod)->id == id ) {
            (*mod)->value += value;
            return;
        }
    }
    modifiers.push(new ItemModifier(id,value));
}

void Item::renderDescriptionFrame(int x, int y, bool below, bool frame) {
	int cx=0,cy=0,cw=CON_W/2,ch=CON_H/2;
	bool stop=false;

	// find the right border
	for ( cw = CON_W/2; cw > cx && ! stop; cw --) {
		for (int ty=0; ty < ch; ty++) {
			if (descCon->getChar(cx+cw-1,ty) != ' ') {
				stop=true;
				break;
			}
		}
	}
	// find the left border
	stop=false;
	for (cx=0; cx < CON_W/2 && ! stop; cx++,cw--) {
		for (int ty=0; ty < ch; ty++) {
			if (descCon->getChar(cx,ty) != ' ') {
				stop=true;
				break;
			}
		}
	}
	// find the bottom border
	stop=false;
	for (ch = CON_H/2; ch > 0 && ! stop; ch --) {
		for (int tx=cx; tx < cx+cw; tx++) {
			if (descCon->getChar(tx,cy+ch-1) != ' ') {
				stop=true;
				break;
			}
		}
	}
	cx-=2;cw+=4;ch+=2;
	if ( frame ) {
		// drawn the frame
		descCon->setDefaultForeground(guiText);
		descCon->putChar(cx,cy,TCOD_CHAR_NW, TCOD_BKGND_NONE);
		descCon->putChar(cx+cw-1,cy,TCOD_CHAR_NE, TCOD_BKGND_NONE);
		descCon->putChar(cx,cy+ch-1,TCOD_CHAR_SW, TCOD_BKGND_NONE);
		descCon->putChar(cx+cw-1,cy+ch-1,TCOD_CHAR_SE, TCOD_BKGND_NONE);
		for (int tx=cx+1; tx < cx+cw -1; tx ++) {
			if ( descCon->getChar(tx,cy) == ' ' ) descCon->setChar(tx,cy,TCOD_CHAR_HLINE);
		}
		descCon->hline(cx+1,cy+ch-1,cw-2,TCOD_BKGND_NONE);
		descCon->vline(cx,cy+1,ch-2,TCOD_BKGND_NONE);
		descCon->vline(cx+cw-1,cy+1,ch-2,TCOD_BKGND_NONE);
	}
	if ( ! below ) y = y -ch+1;
	if ( x-cw/2 < 0 ) x = cw/2;
	else if ( x+cw/2 > CON_W ) x = CON_W-cw/2;
	if ( y + ch > CON_H ) y = CON_H-ch;
	TCODConsole::blit(descCon,cx,cy,cw,ch,TCODConsole::root,x-cw/2,y,1.0f,frame ? 0.7f : 0.0f);
}

const char *Item::getArticle(bool capital) const {
	static const char *articles[] = {"","a","an","the"};
	static const char *Articles[] = {"","A","An","The"};
	return capital ? Articles[article] : articles[article];
}

const char *Item::AName() const {
	static char buf[64];
	if ( count == 1 && (!isSoftStackable() || stack.size() == 0) ) {
		if ( name ) {
			sprintf(buf,"%s %s%s%s", getArticle(true), adjective ? adjective : "",
				adjective ? " ": "", name);
		} else {
			sprintf(buf,"%s %s%s%s", (typeData->flags & ITEM_AN)  ? "An" : "A",
				adjective ? adjective : "", adjective ? " ": "", typeName);
		}
	} else {
		int cnt=count > 1 ? count : stack.size()+1;
		const char *nameToUse = name ? name : typeName;
		bool es = (nameToUse[strlen(nameToUse)-1] == 's');
		sprintf(buf,"%d %s%s%s%s",cnt,
			adjective ? adjective : "", adjective ? " ": "",
			nameToUse, es ? "es":"s");
	}
	return buf;
}

const char *Item::aName() const {
	static char buf[64];
	if ( count == 1 && (!isSoftStackable() || stack.size() == 0) ) {
		if ( name ) {
			sprintf(buf,"%s %s%s%s", getArticle(true), adjective ? adjective : "",
				adjective ? " ": "", name);
		} else {
			sprintf(buf,"%s %s%s%s", (typeData->flags & ITEM_AN)  ? "an" : "a",
				adjective ? adjective : "", adjective ? " ": "", typeName);
		}
	} else {
		int cnt=count > 1 ? count : stack.size()+1;
		const char *nameToUse = name ? name : typeName;
		bool es = (nameToUse[strlen(nameToUse)-1] == 's');
		sprintf(buf,"%d %s%s%s%s",cnt,
			adjective ? adjective : "", adjective ? " ": "",
			nameToUse, es ? "es":"s");
	}
	return buf;
}

const char *Item::TheName() const {
	static char buf[64];
	if ( count == 1 && (!isSoftStackable() || stack.size() == 0) ) {
		sprintf(buf,"The %s%s%s", adjective ? adjective : "", adjective ? " ": "",
			name ? name : typeName);
	} else {
		int cnt=count > 1 ? count : stack.size()+1;
		const char *nameToUse = name ? name : typeName;
		bool es = (nameToUse[strlen(nameToUse)-1] == 's');
		sprintf(buf,"The %d %s%s%s%s",cnt,
			adjective ? adjective : "", adjective ? " ": "",
			nameToUse, es ? "es":"s");
	}
	return buf;
}

const char *Item::theName() const {
	static char buf[64];
	if ( count == 1 && (!isSoftStackable() || stack.size() == 0) ) {
		sprintf(buf,"the %s%s%s", adjective ? adjective : "", adjective ? " ": "",
			name ? name : typeName);
	} else {
		int cnt=count > 1 ? count : stack.size()+1;
		const char *nameToUse = name ? name : typeName;
		bool es = (nameToUse[strlen(nameToUse)-1] == 's');
		sprintf(buf,"the %d %s%s%s%s",cnt,
			adjective ? adjective : "", adjective ? " ": "",
			nameToUse, es ? "es":"s");
	}
	return buf;
}

#define ITEM_CHUNK_VERSION 7

bool Item::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip *zip) {
	if ( chunkVersion != ITEM_CHUNK_VERSION ) return false;
	x=zip->getFloat();
	y=zip->getFloat();
	itemClass=(ItemClass)zip->getInt();
	if ( itemClass < 0 || itemClass >= NB_ITEM_CLASSES ) return false;
	col=zip->getColor();
	typeName=strdup(zip->getString());
	const char *fname=zip->getString();
	if ( fname ) name=strdup(fname);
	count=zip->getInt();
	ch = zip->getInt();
	article = (Article)zip->getChar();
	life=zip->getFloat();

	int nbItems=zip->getInt();
	bool soft=isSoftStackable();
	while ( nbItems > 0 ) {
		const char * itemTypeName=zip->getString();
		ItemType *itemType=ItemType::getType(itemTypeName);
		if (!itemType) return false;
		uint32_t itemChunkId ,itemChunkVersion;
		saveGame.loadChunk(&itemChunkId, &itemChunkVersion);
		Item *it=Item::getItem(itemType, 0,0);
		if (!it->loadData(itemChunkId, itemChunkVersion, zip)) return false;
		if (soft) stack.push(it);
		else it->putInContainer(this);
		nbItems--;
	}


	ItemFeature *feat=getFeature(ItemFeature::ATTACK);
	if (feat ) {
		castDelay=zip->getFloat();
		reloadDelay=zip->getFloat();
		damages=zip->getFloat();
	}

	int nbModifiers=zip->getInt();
	while ( nbModifiers > 0) {
		ItemModifierId id = (ItemModifierId)zip->getInt();
		if ( id < 0 || id >= ITEM_MOD_NUMBER ) return false;
		float value=zip->getFloat();
		ItemModifier *mod=new ItemModifier(id,value);
		modifiers.push(mod);
	}
	return true;
}

void Item::saveData(uint32_t chunkId, TCODZip *zip) {
	saveGame.saveChunk(ITEM_CHUNK_ID,ITEM_CHUNK_VERSION);
	zip->putFloat(x);
	zip->putFloat(y);
	zip->putInt(itemClass);
	zip->putColor(&col);
	zip->putString(typeName);
	zip->putString(name);
	zip->putInt(count);
	zip->putInt(ch);
	zip->putChar(article);
	zip->putFloat(life);
	// save items inside this item or soft stacks
	zip->putInt(stack.size());
	for (Item **it=stack.begin(); it != stack.end(); it++) {
		zip->putString((*it)->typeData->name);
		(*it)->saveData(ITEM_CHUNK_ID,zip);
	}
	ItemFeature *feat=getFeature(ItemFeature::ATTACK);
	if (feat ) {
		zip->putFloat(castDelay);
		zip->putFloat(reloadDelay);
		zip->putFloat(damages);
	}

	zip->putInt(modifiers.size());
	for ( ItemModifier ** it = modifiers.begin(); it != modifiers.end(); it++) {
		zip->putInt((*it)->id);
		zip->putFloat((*it)->value);
	}
}
