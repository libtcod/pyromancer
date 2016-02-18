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

#include "main.hpp"

TCODList<ConditionType *> ConditionType::list;
ConditionType *ConditionType::find(const char *name) {
	for ( ConditionType **it=list.begin(); it!=list.end(); it++) {
		if (strcmp((*it)->name,name)==0 ) return *it;
	}
	return NULL;
}

ConditionType *ConditionType::get(ConditionType::Type type) {
	for ( ConditionType **it=list.begin(); it!=list.end(); it++) {
		if ((*it)->type==type) return *it;
	}
	return NULL;
}

void ConditionType::init() {
	list.push(new ConditionType(STUNNED,"stunned"));
	list.push(new ConditionType(BLEED,"bleed"));
	list.push(new ConditionType(HEAL,"heal"));
	list.push(new ConditionType(POISONED,"poisoned"));
	list.push(new ConditionType(IMMUNE,"immune"));
	list.push(new ConditionType(PARALIZED,"paralized"));
	list.push(new ConditionType(CRIPPLED,"crippled"));
	list.push(new ConditionType(WOUNDED,"wounded"));
	list.push(new ConditionType(REGEN_MANA,"regenMana"));
}

bool ConditionType::check(Creature *cr) {
	switch ( type ) {
		case POISONED :
			// a immune creature cannot be poisoned
			return (!cr->hasCondition(IMMUNE));
		break;
		default : return true; break;
	}
}

Condition::Condition(ConditionType::Type type, float duration, float amount, const char *alias) 
		: initialDuration(duration), duration(duration), amount(amount),curAmount(0.0f), alias(alias) {
	this->type = ConditionType::get(type);
}

bool Condition::equals(ConditionType::Type type,const char *name) {
	return this->type->type == type && (name == NULL || (alias && strcmp(alias,name) == 0 ));
}

// warning : increase CREA_CHUNK_VERSION when changing this

void Condition::save(TCODZip *zip) {
	zip->putInt(type->type);
	zip->putString(alias);
	zip->putFloat(initialDuration);
	zip->putFloat(duration);
	zip->putFloat(amount);
	zip->putFloat(curAmount);
}

void Condition::load(TCODZip *zip) {
	ConditionType::Type typeId = (ConditionType::Type)zip->getInt();
	type = ConditionType::get(typeId);
	alias=zip->getString();
	if ( alias ) alias=strdup(alias);
	initialDuration=zip->getFloat();
	duration=zip->getFloat();
	amount=zip->getFloat();
	curAmount=zip->getFloat();
}


bool Condition::update(float elapsed) {
	curAmount += amount * elapsed / initialDuration;
	switch(type->type) {
		case ConditionType::POISONED :
		case ConditionType::BLEED : {
			// lose health over time
			if ( curAmount > 0 ) {
				target->takeDamage(curAmount);
				// TODO
				//GameScreen::getInstance()->addBloodStain(target->x,target->y,lostHp);
			}
			curAmount = 0;
			
		}
		break;
		case ConditionType::HEAL : {
			// gain health over time
			if ( curAmount > 0 ) {
				target->addLife(curAmount);
			}
			curAmount = 0;
		}
		break;
		case ConditionType::REGEN_MANA : {
			// gain mana over time
			if ( curAmount > 0 ) {
				target->addMana(curAmount);
			}
			curAmount = 0;
		}
		break;
		default:break;
	}
	if ( duration > 0.0f ) {
		duration -= elapsed;
		if ( duration <= 0.0f) {
			switch(type->type) {
				case ConditionType::WOUNDED : {
					// wounded decrease the max hp
					target->setMaxLife(target->getMaxLife()/ (1.0f-amount));
				}
				break;
				case ConditionType::BLEED :
					if (alias && strcmp(alias,"burning") == 0 ) target->setBurning(false); 
				break;
				default:break;
			}
			return true;
		}
	}
	return false;
}

void Condition::applyTo(Creature *cr) {
	cr->addCondition(this);
	switch (type->type) {
		case ConditionType::IMMUNE : {
			// the immune condition remove all poisoned conditions
			Condition *cond=NULL;
			do {
				cond=cr->getCondition(ConditionType::POISONED);
				if ( cond ) {
					cr->conditions.remove(cond);
					delete cond;
				}
			} while (cond);
		}
		break;
		case ConditionType::BLEED :
			if (alias && strcmp(alias,"burning") == 0 ) cr->setBurning(true); 
		break;
		case ConditionType::STUNNED :
		// !! NO-BREAK 
		case ConditionType::PARALIZED :
			cr->walkTimer=-0.1f; 
		break;
		case ConditionType::WOUNDED : {
			// wounded decrease the max hp
			float quantity=amount*cr->getMaxLife();
			cr->setMaxLife(cr->getMaxLife() - quantity);
			if ( cr->getMaxLife() <= 0 ) {
				cr->setMaxLife(0);
			}
			cr->addLife(0); // to clamp life between 0 and maxLife  
		}
		break;
		default:break;
	}
}

