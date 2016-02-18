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
#include "main.hpp"

#define DBG_GRAPH(x)
//#define DBG_GRAPH(x) printf x

TCODList<Powerup *> Powerup::list;
TCODImage *PowerupGraph::img[PW_NB]={NULL};
const char *PowerupGraph::imgName[PW_NB]={
	"data/img/skill_fireball.png",
	"data/img/skill_burst.png",
	"data/img/skill_speed.png",
	"data/img/skill_bounce.png",
	"data/img/skill_blast.png",
	"data/img/skill_incandescence.png",
	"data/img/skill_firefly.png",
	"data/img/skill_beacon.png",
	"data/img/skill_mana.png",
	"data/img/skill_life.png",
};

// cheap data initialization, but well.. 7DRL style...
// TODO : to be merged with skill system...
void Powerup::init() {
	Powerup *fb_lvl1=new Powerup(
		PW_FB,1,
		"Fireball",
		"Wand powerup",
		"Fireball damages increased to 10 and range 1"
	);
	list.push(fb_lvl1);
	fb_lvl1->enabled=true;
	Powerup *fb_lvl2=new Powerup(
		PW_FB,2,
		"Fireball 2",
		"Wand powerup",
		"Fireball damages increased to 15 and range 2",
		fb_lvl1
	);
	list.push(fb_lvl2);
	Powerup *fb_lvl3=new Powerup(
		PW_FB,3,
		"Fireball 3",
		"Wand powerup",
		"Fireball damages increased to 20 and range 3",
		fb_lvl2
	);
	list.push(fb_lvl3);
	Powerup *fb_lvl4=new Powerup(
		PW_FB,4,
		"Fireball 4",
		"Wand powerup",
		"Fireball damages increased to 25 and range 4",
		fb_lvl3
	);
	list.push(fb_lvl4);
	Powerup *fb_lvl5=new Powerup(
		PW_FB,5,
		"Fireball 5",
		"Wand powerup",
		"Fireball damages increased to 30 and range 5",
		fb_lvl4
	);
	list.push(fb_lvl5);
	Powerup *fb_burst1=new Powerup(
		PW_BURST,1,
		"Live embers",
		"Fire spell",
		"Casts a special fireball that projects deadly embers when it explodes",
		fb_lvl1
	);
	list.push(fb_burst1);
	Powerup *fb_burst2=new Powerup(
		PW_BURST,2,
		"Live embers 2",
		"Spell powerup",
		"Increase embers speed and number",
		fb_burst1
	);
	list.push(fb_burst2);
	Powerup *fb_sparkleThrough=new Powerup(
		PW_SPARKLE_THROUGH,0,
		"Fast embers",
		"Spell powerup",
		"Live embers go through creatures",
		fb_burst2
	);
	list.push(fb_sparkleThrough);
	Powerup *fb_burst3=new Powerup(
		PW_BURST,3,
		"Live embers 3",
		"Spell powerup",
		"Increase embers speed and number",
		fb_burst2
	);
	list.push(fb_burst3);
	Powerup *fb_sparkleBounce=new Powerup(
		PW_SPARKLE_BOUNCE,0,
		"Bouncing embers",
		"Spell powerup",
		"Live embers bounce against walls",
		fb_burst3
	);
	list.push(fb_sparkleBounce);
	Powerup *fb_burst4=new Powerup(
		PW_BURST,4,
		"Live embers 4",
		"Spell powerup",
		"Increase embers speed and number",
		fb_burst3
	);
	list.push(fb_burst4);
	Powerup *fb_burst5=new Powerup(
		PW_BURST,5,
		"Live embers 5",
		"Spell powerup",
		"Increase embers speed and number",
		fb_burst4
	);
	list.push(fb_burst5);
	Powerup *blast1=new Powerup(
		PW_BLAST,1,
		"Blast",
		"Fire spell",
		"A ring of burning gas that pushes nearby enemies away and sets them to fire",
		fb_burst2
	);
	list.push(blast1);
	Powerup *blast2=new Powerup(
		PW_BLAST,2,
		"Blast 2",
		"Spell powerup",
		"Increase the ring range and power",blast1
	);
	list.push(blast2);
	Powerup *blast3=new Powerup(
		PW_BLAST,3,
		"Blast 3",
		"Spell powerup",
		"Increase the ring range and power",blast1
	);
	list.push(blast3);
	Powerup *fb_incan1=new Powerup(
		PW_INCAN,1,
		"Incandescence",
		"Fire spell",
		"Casts a fire cloud that sets fire to any creature close enough",
		fb_lvl1
	);
	list.push(fb_incan1);
	Powerup *fb_incan2=new Powerup(
		PW_INCAN,2,
		"Incandescence 2",
		"Spell powerup",
		"Increases the fire cloud size and life time",
		fb_incan1
	);
	list.push(fb_incan2);
	Powerup *fb_incan3=new Powerup(
		PW_INCAN,3,
		"Incandescence 3",
		"Spell powerup",
		"Increases the fire cloud size and life time",
		fb_incan2
	);
	list.push(fb_incan3);
	Powerup *fb_incan4=new Powerup(
		PW_INCAN,4,
		"Incandescence 4",
		"Spell powerup",
		"Increases the fire cloud size and life time",
		fb_incan3
	);
	list.push(fb_incan4);
	Powerup *fb_incan5=new Powerup(
		PW_INCAN,5,
		"Incandescence 5",
		"Spell powerup",
		"Increases the fire cloud size and life time",
		fb_incan4
	);
	list.push(fb_incan5);
	Powerup *firefly1=new Powerup(
		PW_FIREFLY,1,
		"Firefly",
		"Light spell",
		"Casts an small light that temporarily illuminates a zone"
	);
	list.push(firefly1);
	Powerup *beacon1=new Powerup(
		PW_BEACON,1,
		"Beacon",
		"Light spell",
		"Casts an intense light that temporarily illuminates a zone",
		firefly1
	);
	list.push(beacon1);
	Powerup *beacon2=new Powerup(
		PW_BEACON,2,
		"Beacon 2",
		"Spell powerup",
		"Increases the light radius and duration and improves its stability",
		beacon1
	);
	list.push(beacon2);
	Powerup *beacon3=new Powerup(
		PW_BEACON,3,
		"Beacon 3",
		"Spell powerup",
		"Increases the light radius and duration and improves its stability",
		beacon2
	);
	list.push(beacon3);
	Powerup *mana=new Powerup(
		PW_MANA,1,
		"Mana",
		"Character powerup",
		"Increases your mana max level and regeneration speed"
	);
	list.push(mana);
	Powerup *mana2=new Powerup(
		PW_MANA,2,
		"Mana 2",
		"Character powerup",
		"Increases your mana max level and regeneration speed",
		mana
	);
	list.push(mana2);
	Powerup *mana3=new Powerup(
		PW_MANA,3,
		"Mana 3",
		"Character powerup",
		"Increases your mana max level and regeneration speed",
		mana2
	);
	list.push(mana3);
	Powerup *life=new Powerup(
		PW_LIFE,1,
		"Life",
		"Character powerup",
		"Increases your health max level and regeneration speed"
	);
	list.push(life);
	Powerup *life2=new Powerup(
		PW_LIFE,2,
		"Life 2",
		"Character powerup",
		"Increases your health max level and regeneration speed",
		life
	);
	list.push(life2);
	Powerup *life3=new Powerup(
		PW_LIFE,3,
		"Life 3",
		"Character powerup",
		"Increases your health max level and regeneration speed",
		life2
	);
	list.push(life3);
}

Powerup::Powerup(PowerupId id, int level, const char *name, const char *type, const char *description, Powerup *requires) :
	id(id),name(name),level(level),type(type),description(description), requires(requires) {
	enabled=false;
}

void Powerup::apply() {
	enabled=true;
	if ( id == PW_FB ) {
		ItemType *fireball=ItemType::getType("fireball");
		ItemFeatureAttack *attack=(ItemFeatureAttack *)fireball->getFeature(ItemFeature::ATTACK);
		ItemFeatureLight *light=(ItemFeatureLight *)fireball->getFeature(ItemFeature::LIGHT);
		ItemFeatureExplodeOnBounce *explode=(ItemFeatureExplodeOnBounce *)fireball->getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
		ItemFeatureHeat *heat=(ItemFeatureHeat *)fireball->getFeature(ItemFeature::HEAT);
		attack->minDamagesCoef+=3;
		attack->maxDamagesCoef+=3;
		light->range+=0.7f;
		explode->startRange+=0.7f;
		explode->endRange+=0.7f;
		heat->radius += 0.7f;			
	} else if ( id == PW_BURST ) {
		if ( level == 1 ) {
			gameEngine->player->addSkillType("embers");
		} else {
			ItemType *fireball=ItemType::getType("embers");
			ItemFeatureExplodeOnBounce *explode=(ItemFeatureExplodeOnBounce *)fireball->getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
			explode->particleType.duration += 0.3f;
			explode->particleType.speed += 3.0f;
			explode->particleType.minDamage+=2.0f;
			explode->particleType.maxDamage+=2.0f;
			explode->particleCount+=2;
		}
	} else if ( id == PW_INCAN ) {
		if ( level == 1 ) {
			gameEngine->player->addSkillType("incan");
		} else {
			ItemType *fireball=ItemType::getType("firecloud");
			ItemFeatureLight *light=(ItemFeatureLight *)fireball->getFeature(ItemFeature::LIGHT);
			ItemFeatureExplodeOnBounce *explode=(ItemFeatureExplodeOnBounce *)fireball->getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
			ItemFeatureHeat *heat=(ItemFeatureHeat *)fireball->getFeature(ItemFeature::HEAT);
			light->range += 0.6f;
			explode->delay+=2.0f;
			explode->middleRange += 1.2f;
			explode->endRange += 1.2f;
			explode->middleTime+=2.0f;
			heat->radius += 1.2f;	
			heat->intensity += 1.0f;		
		}
	} else if ( id == PW_SPARKLE_THROUGH ) {
		ItemType *fireball=ItemType::getType("embers");
		ItemFeatureExplodeOnBounce *explode=(ItemFeatureExplodeOnBounce *)fireball->getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
		explode->particleType.throughCreatures = true;
		explode->particleType.speed += 5.0f;
	} else if ( id == PW_SPARKLE_BOUNCE ) {
		ItemType *fireball=ItemType::getType("embers");
		ItemFeatureExplodeOnBounce *explode=(ItemFeatureExplodeOnBounce *)fireball->getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
		explode->particleType.bounce = true;
	} else if ( id == PW_BLAST ) {
		if ( level == 1 ) {
			gameEngine->player->addSkillType("blast");
		} else {
			ItemType *blast=ItemType::getType("blast");
			ItemFeatureExplodeOnBounce *explode=(ItemFeatureExplodeOnBounce *)blast->getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
			explode->endRange += 2.0f;
			BlastEffect *blastEffect=(BlastEffect *)explode->getEffect(Effect::BLAST);
			blastEffect->range+=2;
			ConditionEffect *burn = explode->getConditionEffect(ConditionType::BLEED);
			burn->duration += 0.5f;
			burn->amount += 2.0f;
			ConditionEffect *stun = explode->getConditionEffect(ConditionType::STUNNED);
			stun->duration+=1.0f;
		}
	} else if ( id == PW_BEACON ) {
		if ( level == 1 ) {
			gameEngine->player->addSkillType("beacon");
		} else {
			static const char *patterns[2]={
				"94567485948567469",
				"98798987898978988"
			};
			ItemType *beacon=ItemType::getType("beacon");
			ItemFeatureExplodeOnBounce *explode=(ItemFeatureExplodeOnBounce *)beacon->getFeature(ItemFeature::EXPLODE_ON_BOUNCE);
			explode->startRange += 15.0f;
			explode->middleRange += 15.0f;
			explode->delay += 5.0f;
			ItemFeatureLight *light=(ItemFeatureLight *)beacon->getFeature(ItemFeature::LIGHT);
			light->pattern=patterns[level-2];
		}
	} else if ( id == PW_FIREFLY ) {
		gameEngine->player->addSkillType("firefly");
	} else if ( id == PW_MANA ) {
		gameEngine->player->setMaxMana(gameEngine->player->getMaxMana()+10);
		gameEngine->player->getType()->setMana(gameEngine->player->getMaxMana());
		Condition *cond=gameEngine->player->getCondition(ConditionType::REGEN_MANA,"");
		if ( cond ) cond->amount += 0.3f;
	} else if ( id == PW_LIFE ) {
		gameEngine->player->setMaxLife(gameEngine->player->getMaxLife()+10);
		gameEngine->player->getType()->setLife(gameEngine->player->getMaxLife());
		Condition *cond=gameEngine->player->getCondition(ConditionType::HEAL,"");
		if ( cond ) cond->amount += 0.1f;
	}
}

void Powerup::getAvailable(TCODList<Powerup *> *l) {
	for (Powerup **it=list.begin(); it!= list.end(); it++) {
		if ( ! (*it)->enabled && ((*it)->requires == NULL || (*it)->requires->enabled ) ) {
			l->push(*it);
		}
	}
}

PowerupGraph::PowerupGraph() {
	needRefresh=true;
	flags=DIALOG_MODAL;
}

void PowerupGraph::setFontSize(int fontSize) {
	static bool firstTime=true;
	// load or reload powerup icons
	// unused ascii code / font bitmap part
	static int asciiRanges[] = {1,8,11,15,19,23,127,175,181,184,207,216,233,255};
	int range=1;
	int ascii=2;
	int fontx=1;
	int fonty=9;
	for (int typ=0; typ < PW_NB; typ++ ) {
		// load image
		if ( !firstTime ) delete img[typ];
		img[typ]=new TCODImage(imgName[typ]);
		int iw,ih;
		img[typ]->getSize(&iw,&ih);
		iconWidth=1;
		while ( iconWidth*fontSize <= iw ) iconWidth++;
		iconWidth--;
		iconHeight=1;
		while ( iconHeight*fontSize <= ih ) iconHeight++;
		iconHeight--;
		iw=iconWidth*fontSize;
		ih=iconHeight*fontSize;
		// resize them according to font size
		img[typ]->scale(iw,ih);
		for (int x= 0; x < iconWidth; x++ ) {
			for (int y= 0; y < iconHeight; y++ ) {
				// map them to some unused ascii codes
				TCODConsole::mapAsciiCodeToFont(ascii,fontx,fonty);
				TCODSystem::updateChar(ascii,fontx,fonty,img[typ],x*fontSize,y*fontSize);
				fontx++;
				if ( fontx == 32 ) { fontx=0; fonty++; }
				ascii++;
				if ( ascii == asciiRanges[range] ) {
					ascii=asciiRanges[range+1];
					range+=2;
				}
			}
		}
	}
	selected=NULL;
	needRefresh=true;
	firstTime=false;
}

typedef struct _PwGraphNode {
	Powerup *pw;
	struct _PwGraphNode *requires;
	int x,y,w,pos,nbSons;
	PowerupId getRequiredId();
} PwGraphNode;

PowerupId PwGraphNode::getRequiredId() {
	Powerup *req=pw->requires;
	if (! req) return PW_NONE;
	PowerupId id=req->id;
	while (id == pw->id) {
		req=req->requires;
		if (! req ) return PW_NONE;
		id=req->id;
	}
	return id;
}

void PowerupGraph::render() {
	static TCODConsole pwscreen(CON_W-20,CON_H-20);
	static PwGraphNode pw[PW_NB];
	if (needRefresh) {
		needRefresh=false;
		int graphHeight=0;
		pwscreen.setDefaultBackground(TCODColor::darkerRed);
		pwscreen.rect(0,0,CON_W-20,CON_H-20,true,TCOD_BKGND_SET);
		for (int x=0; x < CON_W-20; x++ ) {
			pwscreen.setCharForeground(x,0,TCODColor::lightRed);
			pwscreen.setChar(x,0,TCOD_CHAR_HLINE);
			pwscreen.setCharForeground(x,CON_H-21,TCODColor::lightRed);
			pwscreen.setChar(x,CON_H-21,TCOD_CHAR_HLINE);
		}
		for (PowerupId id=PW_FIRST; id < PW_NB; id = (PowerupId)(id+1)) {
			pw[id].pw=NULL;
			// get the first available powerup for this type
			for (Powerup **it=Powerup::list.begin(); it!= Powerup::list.end(); it++) {
				if ( (*it)->id == id && !(*it)->enabled ) {
					pw[id].pw=*it;
					break;
				}
			}
			if (pw[id].pw==NULL) {
				// no powerup available. get the last level
				for (Powerup **it=Powerup::list.begin(); it!= Powerup::list.end(); it++) {
					if ( (*it)->id == id ) {
						pw[id].pw=*it;
					}
				}
			}
		}
		// build tree
		pw[0].w=0;
		pw[0].nbSons=0;
		// compute dependance tree (requires / nbSons fields)
		for (int i=1; i < PW_NB;i++) {
			PwGraphNode *current=&pw[i];
			current->requires=NULL;
			current->w=0;
			current->pos=0;
			current->nbSons=0;
			int nbRequired=0;
			PowerupId id=current->getRequiredId();
			if ( id != PW_NONE) {
				for (int j=0; j <PW_NB ;j++) {
					if ( pw[j].pw->id == id ) current->requires=&pw[j];
					else if (pw[j].getRequiredId() == id) {
						nbRequired++;
						if ( j < i ) current->pos++;
					}
				}
			}
			if (current->requires) current->requires->nbSons=nbRequired;
		}
		// compute nodes size (w field)
		for (int i=PW_NB-1; i >= 0;i--) {
			PwGraphNode *current=&pw[i];
			if ( current->nbSons == 0 ) current->w = iconWidth;
			else {
				for (int j=0; j <PW_NB ;j++) {
					if ( pw[j].requires == current ) {
						if ( current->w > 0 ) current->w++;
						current->w+=pw[j].w;
					}
				}
			}
			DBG_GRAPH(("%s width : %d\n",current->pw->name,current->w));			
		}
		// compute nodes position (x & y fields)
		pw[0].x=CON_W/2-20;
		pw[0].y=0;
		int lastRoot=0;
		DBG_GRAPH(("%s pos : %d %d\n",pw[0].pw->name,pw[0].x,pw[0].y));			
		for (int i=1; i < PW_NB;i++) {
			PwGraphNode *current=&pw[i];
			if ( current->requires ) {
				current->x = current->requires->x-current->requires->w/2;
			} else {
				current->x = pw[lastRoot].x + pw[lastRoot].w+1;
				lastRoot=i;
			}
			int pos=current->pos;
			if ( pos > 0 ) {
				for (int j=0; j < i;j++) {
					if ( pw[j].requires == current->requires ) {
						current->x += pw[j].w+1;
						pos--;
						if ( pos == 0 ) break;
					}
				}
			}
			current->x+=iconWidth/2;
			current->y = current->requires ? current->requires->y -1 -iconHeight : 0;
			if ( -current->y > graphHeight ) graphHeight=-current->y;
			DBG_GRAPH(("%s pos : %d %d\n",current->pw->name,current->x,current->y));			
		}
		// position the graph vertically
		graphHeight+=iconHeight;
		for (int i=0; i < PW_NB;i++) {
			pw[i].y+=graphHeight;
		}
		static int asciiRanges[] = {1,8,11,15,19,23,127,175,181,184,207,216,233,255};
		int range=1;
		int ascii=2;
		for (int i=0; i < PW_NB;i++) {
			// draw icon i
			PwGraphNode *current=&pw[i];
			int startx=current->x-iconWidth/2;
			int starty=current->y-iconHeight/2;
			for (int ix=startx;ix < startx+iconWidth; ix++) {
				for (int iy=starty;iy < starty+iconHeight; iy++) {
					pwscreen.setChar(ix,iy,ascii);
					ascii++;
					if ( ascii == asciiRanges[range] ) {
						ascii=asciiRanges[range+1];
						range+=2;
					}
				}
			}
			pwscreen.setDefaultForeground(TCODColor::lightOrange);
			if ( current->requires ) {
				int liney=current->y-iconHeight/2+iconHeight;
				if ( current->requires->nbSons > 1 ) {
					if ( current->pos == 0 ) {
						pwscreen.putChar(current->x,liney,TCOD_CHAR_SW,TCOD_BKGND_NONE);

					} else if ( current->pos == current->requires->nbSons-1 ) {
						pwscreen.putChar(current->x,liney,TCOD_CHAR_SE,TCOD_BKGND_NONE);
					} else {
						pwscreen.putChar(current->x,liney,TCOD_CHAR_TEEN,TCOD_BKGND_NONE);
					}
				} else pwscreen.putChar(current->x,liney,TCOD_CHAR_VLINE,TCOD_BKGND_NONE);
			}
			if ( current->nbSons > 1 ) {
				// not the same as -iconWidth for odd icon widths
				int linew=current->w - iconWidth/2 -iconWidth/2;
				int startx=current->x-linew/2;
				int liney=current->y-iconHeight/2-1;
				pwscreen.hline(startx,liney,linew,TCOD_BKGND_NONE);
				pwscreen.putChar(current->x,liney,TCOD_CHAR_TEES,TCOD_BKGND_NONE);
			}
		}
	}
	selected=NULL;
	for (int i=0; i < PW_NB;i++) {
		TCODColor col;
		PwGraphNode *current=&pw[i];
		// check if mouse cursor in this icon
		int startx=current->x-iconWidth/2;
		int starty=current->y-iconHeight/2;
		bool active = IN_RECTANGLE(mousecx-startx,mousecy-starty,iconWidth,iconHeight);
		if (active) selected=current->pw;
		// set color
		if ( current->pw->enabled ) col=TCODColor::lighterBlue;
		else if ( !current->pw->requires || current->pw->requires->enabled ) col=active ? TCODColor::orange : TCODColor::lightBlue;
		else col=active ? TCODColor::lightGrey : TCODColor::grey;
		// draw icon i
		for (int ix=startx;ix < startx+iconWidth; ix++) {
			for (int iy=starty;iy < starty+iconHeight; iy++) {
				pwscreen.setCharForeground(ix,iy,col);
			}
		}
	}
	// draw details on selected powerup
	int y=pw[0].y+iconHeight;
	pwscreen.rect(0,y,CON_W-20,CON_H-21-y,true,TCOD_BKGND_NONE);
	if ( selected ) {
		TCODColor col;
		if (! selected->enabled && selected->requires && ! selected->requires->enabled ) {
			col=TCODColor::lightGrey;
		} else {
			col=TCODColor::lightOrange;
		}
		pwscreen.setDefaultForeground(col);
		pwscreen.printEx(CON_W/2-10,y++,TCOD_BKGND_NONE,TCOD_CENTER,selected->name);
		y++;
		pwscreen.print(CON_W/2-30,y++,selected->type);
		if (! selected->enabled && selected->requires && ! selected->requires->enabled ) {
			pwscreen.setDefaultForeground(TCODColor::red);
			pwscreen.print(CON_W/2-30,y++,"Requires %s",selected->requires->name);
			pwscreen.setDefaultForeground(col);
		}
		y++;
		pwscreen.printRect(CON_W/2-30,y++,40,0,selected->description);
	}
	blitTransparent(&pwscreen,0,0,CON_W-20,CON_H-20,TCODConsole::root,10,5);
}

bool PowerupGraph::update(float elapsed, TCOD_key_t &key,TCOD_mouse_t &mouse) {
	mousecx=mouse.cx-10;
	mousecy=mouse.cy-5;
	if ( selected && mouse.lbutton_pressed
		&& ! selected->enabled && (!selected->requires || selected->requires->enabled) ) {
		selected->apply();
		mouse.lbutton_pressed=false; // do not fire when selecting a powerup
		gameEngine->gui.setMode(GUI_NONE);
		needRefresh=true;
		return false;
	}
	return true;
}


