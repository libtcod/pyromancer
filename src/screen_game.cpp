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
#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include "main.hpp"

Game::Game() : level(0),helpOn(false) {
}

void Game::onInitialise() {
	GameEngine::onInitialise();
	lightMap->fogRange=15.0f;
	fadeInLength=fadeOutLength=(int)(config.getFloatProperty("config.display.fadeTime")*1000);
}

void Game::onActivate() {
	//static int playerLightRange=2*config.getIntProperty("config.display.playerLightRange");
	//static TCODColor playerLightColor=config.getColorProperty("config.display.playerLightColor");
    // set keyboard mode to RELEASED + PRESSED
	renderer = new IndoorRenderer();

	init(); // init game engine
	GameEngine::onActivate();
	initLevel();
	boss=NULL;
	finalExplosion=2.0f;
	memset(&stats,0,sizeof(stats));

	// add light skill to player
	player->addSkillType("light");

	// set pyromancer specific help pages
	gui.tutorial.disableMenuPage(TUTO_ITEMS);
	gui.tutorial.disableMenuPage(TUTO_INVENTORY2);
	gui.tutorial.disableMenuPage(TUTO_CROUCH);
	gui.tutorial.enableMenuPage(TUTO_FIREBALL);
	gui.tutorial.enableMenuPage(TUTO_ACTIONS);
	// since there's no inventory in pyro, make all items
	// auto-activable (bumping = take and use)
	ItemType *bottle=ItemType::getType("bottle");
	bottle->flags |= ITEM_USE_WHEN_PICKED|ITEM_DELETE_ON_USE|ITEM_AUTOPICK;
	player->maxFovRange=player->fovRange=CON_W/2;
	// health potions produce light in pyromancer!
	bottle->features.push(new ItemFeatureLight(4,HDRColor(255,50,0),0,1.0f,"56789876",HDRColor()));
	Item *wand=Item::getItem(ItemType::getType("wand"),0,0,false);
	wand = player->addToInventory(wand);
	player->equip(wand);
	// amulet of zeepoh's light
	ItemType::addFeature("amulet",new ItemFeatureLight(10,HDRColor(330,280,20),0,
		3.0f,"noise",HDRColor(),"0123456789900079"));

	// mana & health is auto-regenerating in pyro
	Condition *regenMana=new Condition(ConditionType::REGEN_MANA,1.0f,1.0f,"");
	regenMana->duration=0.0f; // unlimited condition
	player->addCondition(regenMana);
	Condition *regenHealth=new Condition(ConditionType::HEAL,1.0f,0.3f,"");
	regenHealth->duration=0.0f; // unlimited condition
	player->addCondition(regenHealth);

	// create main objective
	Objective *obj=new Objective("Find the amulet of Zeepoh",
	"Zeepoh has your amulet. Find him. Kill him. Get back the amulet.",
	NULL,
	"if hasCreatureItemType('player','amulet') then "
	"   creatureTalk('player','See, Zeepoh ? This is what happens when you try and screw a pyromancer !') "
	"   addObjectiveStep('He was no fit. The fight was epic but he still finished as a pile of ashes...') "
	"   closeObjective(true) "
	"end ",true);
	gameEngine->gui.objectives.addObjective(obj);

}


void Game::render() {
	static int nbLevels=config.getIntProperty("config.gameplay.nbLevels");
	TCODConsole::root->setDefaultBackground(TCODColor::black);
	TCODConsole::root->clear();
	// draw subcell ground
	renderer->renderSubcells();

	// final explosion
	/*
	if ( bossIsDead && finalExplosion > 0.0f && finalExplosion <= 1.0f ) {
		float radius = 2*CON_W * (1.0f-finalExplosion);
		float minRadius=MAX(0,MIN(radius-3,0.6f*radius));
		float medRadius, radiusdiv=1.0f;
		int minx=dungeon->stairx*2-(int)radius;
		int miny=dungeon->stairy*2-(int)radius;
		int maxx=dungeon->stairx*2+(int)radius;
		int maxy=dungeon->stairy*2+(int)radius;
		int xOffset2=xOffset*2;
		int yOffset2=yOffset*2;
		int conExploX=dungeon->stairx*2-xOffset2;
		int conExploY=dungeon->stairy*2-yOffset2;
		minx = MAX(0,minx);
		miny = MAX(0,miny);
		maxx = MIN(dungeon->width*2-1,maxx);
		maxy = MIN(dungeon->height*2-1,maxy);
		radius=radius*radius;
		minRadius=minRadius*minRadius;
		medRadius=(radius+minRadius)*0.5f;
		if ( radius - minRadius > 1E-5) radiusdiv=2.0f/(radius-minRadius);
		minx -= xOffset2;
		miny -= yOffset2;
		maxx -= xOffset2;
		maxy -= yOffset2;
		minx=MAX(0,minx);
		miny=MAX(0,miny);
		maxx=MIN(CON_W*2-1,maxx);
		maxy=MIN(CON_H*2-1,maxy);
		minx2x=MIN(minx,minx2x);
		miny2x=MIN(miny,miny2x);
		maxx2x=MAX(maxx,maxx2x);
		maxy2x=MAX(maxy,maxy2x);
		for (int x=minx; x <= maxx; x++) {
			int dx2=(conExploX-x)*(conExploX-x);
			for (int y=miny; y <= maxy; y++) {
				if ( dungeon->map2x->isInFov(x+xOffset2,y+yOffset2)
					&& dungeon->map->isWalkable(x/2+xOffset,y/2+yOffset)) {
					int dy=conExploY-y;
					float r=dx2+dy*dy;
					if ( r<= radius && r > minRadius) {
						float midr=(r-medRadius)*radiusdiv;
						float rcoef=1.0f-ABS(midr);
						float f[2]={(float)(3*x)/CON_W,(float)(3*y)/CON_H};
						float ncoef = 0.5f*(1.0f+noise2d.getFbm(f,3.0f));
//						ground->putPixel(x,y,TCODColor::lerp(TCODColor::yellow,TCODColor::red,coef));
						TCODColor col=lightMap->getColor2x(x,y);
						col = TCODColor::lerp(col,TCODColor::lerp(TCODColor::darkRed,TCODColor::yellow,ncoef),rcoef*ncoef);
						lightMap->setColor2x(x,y,col);
					}
				}
			}
		}
	}
	*/

	// blit it on console
	ground->blit2x(TCODConsole::root,0,0);

   	// render boss health bar
	if ( boss && boss->hasBeenSeen() && ! boss->isDead() ) {
		float lifeper=boss->getLifeRatio();
		StatusPanel::drawBar(35,2,lifeper,TCODColor::red,TCODColor::darkerRed);
		int y=3;
		for (Skill **it=boss->skills.begin(); it != boss->skills.end(); it++) {
			if ( (*it)->isCasting() ) {
				float skillPer=((*it)->type->castTime - (*it)->castTime)/((*it)->type->castTime);
				StatusPanel::drawBar(35,y++,skillPer,TCODColor::blue,TCODColor::darkerBlue);
			}
		}
	}

	// render the items
	// render the creatures
	// render the player
	renderer->renderCells();


	// render the stair
	int stairx=dungeon->stairx-xOffset;
	int stairy=dungeon->stairy-yOffset;
	if ( IN_RECTANGLE(stairx,stairy,CON_W,CON_H)
		&& dungeon->getMemory(dungeon->stairx,dungeon->stairy) ) {
		if ( level < nbLevels-1 ) {
			TCODConsole::root->setChar(stairx,stairy,'<');
			TCODConsole::root->setCharForeground(stairx,stairy,TCODColor::white);
		}
	}

	// render level
	if (fade == FADE_UP) {
		TCODColor lvlCol=TCODColor::white * (1.0f-fadeLvl);
		TCODConsole::root->setDefaultForeground(lvlCol);
		TCODConsole::root->printEx(CON_W/2,CON_H-5,TCOD_BKGND_NONE,TCOD_CENTER, "Level %d",level+1);
	}

	// render items under the mouse cursor
/*
	if (! isGamePaused()) {
		int dungeonx=mousex+xOffset;
		int dungeony=mousey+yOffset;
		Item *mouseItem=dungeon->getFirstItem(dungeonx,dungeony);
		if (mouseItem) {
			mouseItem->renderDescription(mousex,mousey);
		} else if ( dungeonx == dungeon->stairx && dungeony == dungeon->stairy ) {
			int my=mousey+1;
			if ( my == CON_H ) my = CON_H-2;
			if (level < nbLevels-1 ) {
				TCODConsole::root->setDefaultForeground(TCODColor::white);
				TCODConsole::root->printEx(mousex,my,TCOD_BKGND_NONE,TCOD_CENTER,"Stairs to next level");
			} else if (bossIsDead && finalExplosion == 2.0f) {
				TCODConsole::root->setDefaultForeground(TCODColor::lightYellow);
				TCODConsole::root->printEx(mousex,my,TCOD_BKGND_NONE,TCOD_CENTER,"Amulet of Zeepoh");
			}
		}
	}
*/

	// various UI stuff
	TCODConsole::root->setDefaultForeground(TCODColor::lightYellow);
	if ( boss && boss->hasBeenSeen() && ! boss->isDead()) {
		TCODConsole::root->printEx(40,1,TCOD_BKGND_NONE,TCOD_CENTER,boss->getName());
		int y=3;
		for (Skill **it=boss->skills.begin(); it != boss->skills.end(); it++) {
			if ( (*it)->isCasting() ) {
				TCODConsole::root->printEx(40,y++,TCOD_BKGND_NONE,TCOD_CENTER,(*it)->getName());
			}
		}
	}

	// apply sepia post-processing
	applySepia();
}

bool Game::update(float elapsed, TCOD_key_t &k,TCOD_mouse_t &mouse) {
	static int nbLevels=config.getIntProperty("config.gameplay.nbLevels");
	static bool debug=config.getBoolProperty("config.debug");
	static float finalExplosionTime=config.getFloatProperty("config.display.finalExplosionTime");

	mousex=mouse.cx;
	mousey=mouse.cy;

	GameEngine::update(elapsed,k,mouse);

	if ( finalExplosion <= 1.0f ) finalExplosion -= elapsed/finalExplosionTime;


	if ( helpOn && (k.c == '?' || k.c == ' ') && ! k.pressed ) {
		helpOn=false;
		resumeGame();
		return true;
	}
	if ( k.lalt && k.c=='p' && ! k.pressed ) {
		// Alt-P : export ascii-animator file
		TCODConsole::root->saveApf("pyro.apf");
	}
	if ( k.c ==' ' && ! k.pressed && gui.mode == GUI_NONE ) {
		if (isGamePaused()) resumeGame();
		else pauseGame();
	} else if ( ! k.pressed && (k.c == 'o' || k.c =='O' ) ) {
		openCloseObjectives();
	}

	// update player
	if ( fade != FADE_DOWN ) player->update(elapsed,k,&mouse);
	if ( isGamePaused() ) {
		if (gui.mode == GUI_NONE) gui.descriptor.setFocus(mousex,mousey,mousex+xOffset,mousey+yOffset,lookOn);
		return true;
	}
	// update items
	dungeon->updateItems(elapsed,k,&mouse);

	xOffset=(int)(player->x-CON_W/2);
	yOffset=(int)(player->y-CON_H/2);

	if ( player->getLife() <= 0 && fade != FADE_DOWN ) {
		setFadeOut(fadeOutLength, TCODColor::darkRed);
		fade=FADE_DOWN;
		fadeLvl=1.0f;
	}

	// update fading
	if ( fade == FADE_DOWN && fadeLvl <= 0.0f ) {
        if ( player->getLife() <= 0 ) {
            // death
            engine.activateModule("pyroGameOver");
            return false;
        }
        if ( level < nbLevels-1 ) {
            // go to next level
            fade=FADE_UP;
            termLevel();
            level++;
            initLevel();
        } else {
            // victory
            engine.activateModule("pyroVictory");
            return false;
        }
	} else if ( fade == FADE_OFF ) {
		if (finalExplosion <= 0.0f ) {
			setFadeOut(fadeOutLength, TCODColor::white);
			fade=FADE_DOWN;
			fadeLvl=1.0f;
		}
	}

	// level ending condition
	if ( fade == FADE_OFF ) {
		if ( level == nbLevels-1 ) {
			if ( win ) {
				if (finalExplosion==2.0f) {
					// triggers final explosion
					finalExplosion=1.0f;
				}
			}
		} else if (player->x == dungeon->stairx && player->y == dungeon->stairy ) {
			fade=FADE_DOWN;
			fadeLvl=1.0f;
		}
	}

	// calculate player fov
	dungeon->computeFov((int)player->x,(int)player->y);

	// update monsters
	if ( fade != FADE_DOWN ) {
		if ( boss && boss->isDead() && finalExplosion > 0.0f && finalExplosion <= 1.0f) {
			int radius = (int)(2*CON_H * (1.0f-finalExplosion))-10;
			if ( radius > 0 ) dungeon->killCreaturesAtRange(radius);
		}

		aiDirector.update(elapsed);
		dungeon->updateCreatures(elapsed);
	}

	// non player related keyboard handling
	if ( debug ) {
		// debug/cheat shortcuts
		if ( k.c == 'd' && k.lalt && ! k.pressed) {
			// debug mode : Alt-d = player takes 'd'amages
			player->takeDamage(20);
		}
		if ( k.c == 'b' && k.lalt && ! k.pressed) {
			// debug mode : Alt-b = burn
			for (Creature **cr=dungeon->creatures.begin(); cr != dungeon->creatures.end(); cr++) {
				(*cr)->setBurning(true);
			}
		}
		if ( k.c == 'i' && k.lalt && ! k.pressed) {
			// debug mode : Alt-i = item
			aiDirector.dropItem((int)player->x,(int)player->y-1);
		}
		if ( k.c == 'm' && k.lalt && ! k.pressed) {
			// debug mode : Alt-m : max spells
			TCODList<Powerup *> list;
			bool again=true;
			do {
				Powerup::getAvailable(&list);
				again=false;
				while (!list.isEmpty()) {
					Powerup *sel=list.pop();
					sel->apply();
					again=true;
				}
			} while (again);
			k.c=0;
		}
		if ( k.c == 's' && k.lalt && ! k.pressed) {
			// debug mode : Alt-s = go to stairs
			player->setPath(dungeon->stairx,dungeon->stairy,false);
		}
		if ( k.c == 'f' && k.lalt && ! k.pressed) {
			// debug mode : Alt-f = final explosion
			if ( boss ) boss->setLife(0.0f);
			finalExplosion=1.0f;
			k.c=0;
		}
		if ( k.c == 'l' && ! k.pressed ) {
			// debug mode : Alt-l : create light
			ExtendedLight *l=new ExtendedLight();
			l->range=10;
			l->color=TCODColor::lighterRed;
			l->x = gameEngine->player->x * 2;
			l->y = gameEngine->player->y * 2;
			//l->flags|=Light::RANDOM_RAD;
			//l->flags|=Light::INVSQRT;
			l->setup(TCODColor::darkBlue,0.0f,NULL,"09090");
			dungeon->addLight(l);

		}
		// debug : change level with numpad +/-
		if ( k.vk == TCODK_KPSUB && level > 0 && ! k.pressed) {
			termLevel();
			level--;
			initLevel();
		} else if (k.vk == TCODK_KPADD && level < nbLevels-1 && ! k.pressed ) {
			termLevel();
			level++;
			initLevel();
		}
	}

	// update lightmap (fog)
	lightMap->update(elapsed);

	// update lights
	dungeon->updateLights(elapsed);

	// update particles
	updateParticles(elapsed);

	gui.descriptor.setFocus(mousex,mousey,mousex+xOffset,mousey+yOffset,lookOn);

	return true;
}

// protected stuff
void Game::initLevel() {
	static int nbLevels=config.getIntProperty("config.gameplay.nbLevels");
	static TCODColor playerLightColor=config.getColorProperty("config.display.playerLightColor");
	static TCODColor playerLightColorEnd=config.getColorProperty("config.display.playerLightColorEnd");

	SkillType *light=SkillType::find("light");
	LightEffect *lightEffect=(LightEffect *)light->getEffect(Effect::LIGHT);
	lightEffect->light.color=TCODColor::lerp(playerLightColor,playerLightColorEnd,(float)(level+1)/nbLevels);
	CaveGenerator caveGen(level);
	dungeon=new Dungeon(level,&caveGen);
	AiDirector::instance->setLevelCoef((float)(level)/(nbLevels-1));
	if ( level == nbLevels-1 ) {
		// boss
		boss=(Boss *)Creature::getCreature(CREATURE_ZEEPOH);
		int bx=dungeon->stairx;
		int by=dungeon->stairy;
		dungeon->getClosestWalkable(&bx,&by,false);
		boss->setPos(bx,by);
		dungeon->addCreature(boss);
	}
	player->initLevel();
}

void Game::termLevel() {
	// clear all creatures
	for (int i=0; i < NB_CREATURE_TYPES; i++) Creature::creatureByType[i].clear();
	player->termLevel();
	particles.clearAndDelete();
	delete dungeon;
}
