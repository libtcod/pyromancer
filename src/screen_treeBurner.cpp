/*
* Copyright (c) 2010 Jice
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
#include <math.h>
#include <stdio.h>
#include "main.hpp"

#define FOREST_W 400
#define FOREST_H 100
#define WATER_START -0.87f

#define MAX_ENTITY_PROB 15

// probability for an entity (item or creature) to be on some terrain type
struct EntityProb {
	// item or creature type. NULL/-1 = end of list
	const char *itemTypeName;
	int creatureType;
	// how many entities (1.0 = on every cell, 0.0= never)
	float density;
	// height threshold on this terrain layer (0.0 - 1.0)
	float minThreshold, maxThreshold;
};

struct TerrainGenData {
	TerrainId terrain;
    float threshold;
	EntityProb itemData[MAX_ENTITY_PROB];
};


struct LayeredTerrain {
	const char *name;
	TerrainGenData info[5];
};

enum ForestId {
    FOREST_PINE,
    FOREST_NORTHERN,
    FOREST_OAK,
    NB_FORESTS
};

static LayeredTerrain forestTypes[NB_FORESTS] = {
	{"pine forest",
    {{TERRAIN_GRASS_LUSH,0.5f,
		{{"pine tree",-1,0.36f,0.0f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GRASS_NORMAL,0.0f,
		{{"pine tree",-1,0.28f,0.0f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GRASS_SPARSE,-0.9f,
		{{"pine tree",-1,0.1f,0.17f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GROUND,-1.25f,{{NULL,-1}}},
    {TERRAIN_GROUND,-1.5f,{{NULL,-1}}},
	}},
	{"northern forest",
	{{TERRAIN_GRASS_NORMAL,0.66f,
		{{"oak tree",-1,0.2f,0.0f,1.0f},
		{"apple tree",-1,0.05f,0.0f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GRASS_SPARSE,0.33f,
		{{"oak tree",-1,0.11f,0.0f,1.0f},
		{"apple tree",-1,0.05f,0.0f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GRASS_WITHERED,0.0f,
		{{"oak tree",-1,0.07f,0.0f,1.0f},
		{"apple tree",-1,0.03f,0.0f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GRASS_DRIED,-0.5f,
		{{"oak tree",-1,0.04f,0.0f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GROUND,-1.0f,
		{{"oak tree",-1,0.02f,0.1f,1.0f},
		{NULL,-1}}},
	}},
	{"oak forest",
	{{TERRAIN_GRASS_LUSH,0.1f,
		{{"oak tree",-1,0.36f,0.0f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GRASS_NORMAL,-0.4f,
		{{"oak tree",-1,0.14f,0.0f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GRASS_SPARSE,-0.8f,
		{{"oak tree",-1,0.06f,0.12f,1.0f},
		{NULL,-1}}},
    {TERRAIN_SHALLOW_WATER,WATER_START,
		{{NULL,-1}}},
    {TERRAIN_DEEP_WATER,-1.0f,{{NULL,-1}}},
//    {TERRAIN_DIRT,0,-1.0f,-0.75f},
//    {TERRAIN_GRASS_SPARSE,0,-1.25f,-0.75f},
	}},
};

TreeBurner::TreeBurner() {
	forestRng=NULL;
	fadeInLength=fadeOutLength=(int)(config.getFloatProperty("config.display.fadeTime")*1000);
	endTimer=0.0f;
}

void TreeBurner::render() {
	renderer->renderSubcells();
	// render boss health bar
	if ( boss->hasBeenSeen() && !boss->isDead() ) {
		float lifeper=boss->getLifeRatio();
		for (int x=70; x < 90; x++ ) {
			TCODColor col=(x-70) < (int)(lifeper*20) ? TCODColor::red : TCODColor::darkerRed;
			ground->putPixel(x,5,col);
			ground->putPixel(x,4,col);
		}
	}

	// blit it on console
	ground->blit2x(TCODConsole::root,0,0);
	renderer->renderCells();

	if ( boss->hasBeenSeen() && !boss->isDead()) {
		TCODConsole::root->printEx(40,1,TCOD_BKGND_NONE,TCOD_CENTER,boss->getName());
	}
	if ( boss->isDead() && player->getLife() > 0 ) {
		TCODConsole::root->setDefaultForeground(TCODColor::lightRed);
		TCODConsole::root->printEx(40,2,TCOD_BKGND_NONE,TCOD_CENTER,"VICTORY");
	}

	// apply sepia post-processing
	gameEngine->applySepia();

}

bool TreeBurner::update(float elapsed, TCOD_key_t &k,TCOD_mouse_t &mouse) {
	static bool debug=config.getBoolProperty("config.debug");
	static TCODColor sunColor=getColourParam("sunColor");
	static TCODColor dawnColor=getColourParam("dawnColor");

	mousex=mouse.cx;
	mousey=mouse.cy;
	if ( firstFrame ) elapsed=0.1f;
	firstFrame=false;

	GameEngine::update(elapsed,k,mouse);

	if ( k.c ==' ' && ! k.pressed && gui.mode == GUI_NONE ) {
		if (isGamePaused()) resumeGame();
		else pauseGame();
	} else if ( ! k.pressed && (k.c == 'o' || k.c =='O' ) ) {
		openCloseObjectives();
	}
	// non player related keyboard handling
	if ( debug ) {
		// debug/cheat shortcuts
		if ( k.c == 'd' && k.lalt && ! k.pressed) {
			// debug mode : Alt-d = player takes 'd'amages
			player->takeDamage(20);
		} else if ( k.c == 'v' && k.lalt && ! k.pressed) {
			// debug mode : Alt-v = go to village
			if ( player->x < cityWallX-40 ) player->x=cityWallX-15;
			else player->x = FOREST_W-20;
			player->y = FOREST_H/2;
		} else if ( k.c == 'w' && k.lalt && ! k.pressed) {
			// debug mode : Alt-w = instawin
			boss->setLife(0);
			boss->setSeen(true);
		}
	}
	if ( k.vk == TCODK_ALT || k.lalt ) lookOn=k.pressed;

	// update messages
	//log.update(k,mouse,elapsed);

	if ( isGamePaused() ) {
		if (gui.mode == GUI_NONE) gui.descriptor.setFocus(mousex,mousey,mousex+xOffset,mousey+yOffset,lookOn);
		return true;
	}

	// update player
	player->update(elapsed,k,&mouse);
	xOffset=(int)(player->x-CON_W/2);
	yOffset=(int)(player->y-CON_H/2);

	if ( player->getLife() <= 0 && fade != FADE_DOWN ) {
		setFadeOut(fadeOutLength, TCODColor::darkRed);
		fade=FADE_DOWN;
	}
	if ( boss->isDead() ) {
		endTimer+=elapsed;
		if ( endTimer < 30.0f ) dungeon->setAmbient(TCODColor::lerp(sunColor,dawnColor,endTimer/30.0f));
	}
	if ( fade != FADE_DOWN
		&& gui.objectives.sleeping.size() == 0 && gui.objectives.active.size() == 0
		&& endTimer > 30.0f ) {
		setFadeOut(fadeOutLength, TCODColor::darkRed);
		fade=FADE_DOWN;
	}
	if ( fade == FADE_DOWN && fadeLvl <= 0.0f ) {
        if ( player->getLife() <= 0 ) {
            // death
            engine.activateModule("treeBurnerGameOver");
            return false;
        } else {
            // victory
            engine.activateModule("treeBurnerVictory");
            return false;
        }
	}
	// update items
	dungeon->updateItems(elapsed,k,&mouse);

	// calculate player fov
	dungeon->computeFov((int)player->x,(int)player->y);

	// update monsters
	if ( fade != FADE_DOWN ) {
		dungeon->updateCreatures(elapsed);
		// ripples must be updated after creatures because of shoal updates
		rippleManager->updateRipples(elapsed);
		fireManager->update(elapsed);
		if ( !boss->isDead() ) aiDirector.update(elapsed);
	}
	dungeon->updateClouds(elapsed);

	HerdBehavior::updateScarePoints(elapsed);

	// update particles
	updateParticles(elapsed);

	gui.descriptor.setFocus(mousex,mousey,mousex+xOffset,mousey+yOffset,lookOn);

	return true;
}

void TreeBurner::placeTree(Dungeon *dungeon,int x, int y, const ItemType * treeType) {
	// trunk
	int dx=x/2;
	int dy=y/2;
	// no tree against a door
	if (dungeon->hasItemFlag(dx-1,dy,ITEM_BUILD_NOT_BLOCK)
		|| dungeon->hasItemFlag(dx+1,dy,ITEM_BUILD_NOT_BLOCK)
		|| dungeon->hasItemFlag(dx,dy-1,ITEM_BUILD_NOT_BLOCK)
		|| dungeon->hasItemFlag(dx,dy+1,ITEM_BUILD_NOT_BLOCK)) return;

	dungeon->addItem(Item::getItem(treeType,x/2,y/2));
	// folliage
	setCanopy(x,y,treeType);
}

void TreeBurner::generateMap(uint32_t seed) {
	DBG(("Forest generation start\n"));
	forestRng = new TCODRandom(seed);
	dungeon = new Dungeon(FOREST_W,FOREST_H);

	for (int x=1; x < FOREST_W-1; x++ ) {
		if ( x % 40 == 0 ) displayProgress(0.4f+(float)(x) / FOREST_W*0.1f);
		for (int y=1; y < FOREST_H-1; y++ ) {
			dungeon->map->setProperties(x,y,true,true);
		}
	}
	for (int x=2; x < 2*FOREST_W-2; x++ ) {
		if ( x % 40 == 0 ) displayProgress(0.5f + (float)(x) / (2*FOREST_W)*0.1f);
		for (int y=2; y < 2*FOREST_H-2; y++ ) {
			dungeon->map2x->setProperties(x,y,true,true);
		}
	}
	displayProgress(0.6f);
	dungeon->hmap->addFbm(new TCODNoise(2,forestRng),2.20*FOREST_W/400,2.20*FOREST_W/400,0,0,4.0f,1.0,2.05);
	dungeon->hmap->normalize();
	TCODNoise terrainNoise(2,0.5f,2.0f,forestRng);
#ifndef NDEBUG
	float t0=TCODSystem::getElapsedSeconds();
#endif
	Packer packer(FOREST_W-100,20, 80, FOREST_H-40);
	// set water as non buildable
	for (int x=FOREST_W-100; x < FOREST_W-20; x++ ) {
		for (int y=20; y < FOREST_H; y++) {
			if (dungeon->hasRipples(x,y)) packer.addForbiddenZone(Rect(x,y,1,1));
		}
	}
	// boss house
	int housex=forestRng->getInt(FOREST_W-50,FOREST_W-20);
	int housey=forestRng->getInt(20,FOREST_H-20);
	Rect house(housex,housey,forestRng->getInt(20,30),forestRng->getInt(20,30));
	if (packer.addRect(&house)) {
		Building *building=Building::generate(house.w,house.h,house.w*house.h/20,forestRng);
		building->applyTo(dungeon,(int)(house.x+building->doorx),(int)(house.y+building->doory));
	}
	// the village head
	boss=Creature::getCreature(CREATURE_VILLAGE_HEAD);
	housex = (int)(house.x+house.w/2);
	housey = (int)(house.y+house.h/2);
	dungeon->getClosestWalkable(&housex,&housey,true,true,false);
	boss->setPos(housex,housey);
	dungeon->addCreature(boss);
	for (int i=0; i < 20; i++ ) {
		int housex=forestRng->getInt(FOREST_W-100,FOREST_W-20);
		int housey=forestRng->getInt(20,FOREST_H-20);
		Rect house(housex,housey,forestRng->getInt(6,12),forestRng->getInt(6,12));
		if (packer.addRect(&house)) {
			Building *building=Building::generate(house.w,house.h,house.w*house.h/30,forestRng);
			building->applyTo(dungeon,(int)(house.x+building->doorx),(int)(house.y+building->doory));
		}
		//building->setHuntingHide(dungeon);
	}
	cityWallX = FOREST_W-130;
	Building::buildCityWalls(cityWallX,dungeon);

	dungeon->saveShadowBeforeTree();

	for (int x=2*FOREST_W-1; x >=0 ; x--) {
		float f[2];
		f[0] = 2.5f * x / FOREST_W;
		if ( x % 40 == 0 ) displayProgress(0.6f + (float)(2*FOREST_W-1-x) / (2*FOREST_W)*0.4f);
		for (int y=0; y < 2*FOREST_H-1; y++ ) {
			if ( dungeon->getCell(x/2,y/2)->terrain == TERRAIN_WOODEN_FLOOR ) continue;
			f[1] = 2.5f * y / FOREST_H;
			float height = terrainNoise.getFbm(f,5.0f);
			float forestTypeId = (dungeon->hmap->getValue(x,y) * NB_FORESTS);
			forestTypeId=MIN(NB_FORESTS-1,forestTypeId);
			LayeredTerrain *forestType1 = &forestTypes[(int)forestTypeId];
			LayeredTerrain *forestType2 = forestType1;
			if ( (int)forestTypeId < forestTypeId ) forestType2++;
            TerrainGenData *info1 = &forestType1->info[0];
            TerrainGenData *info2 = &forestType2->info[0];
            float maxThreshold1=2.0f;
            float maxThreshold2=2.0f;
            TCODColor nextColor1;
            TCODColor nextColor2;
			bool swimmable1=false;
			bool swimmable2=false;
            while (height<=info1->threshold) {
            	nextColor1 = terrainTypes[info1->terrain].color;
            	maxThreshold1 = info1->threshold;
				swimmable1=terrainTypes[info1->terrain].swimmable;
            	info1++;
            }
            while (height<=info2->threshold) {
            	nextColor2 = terrainTypes[info2->terrain].color;
            	maxThreshold2 = info2->threshold;
				swimmable2=terrainTypes[info2->terrain].swimmable;
            	info2++;
            }
			float terrainTypeCoef=forestTypeId-(int)forestTypeId;
			float layer1Height = (height - info1->threshold) / (maxThreshold1 - info1->threshold);
			float layer2Height = (height - info2->threshold) / (maxThreshold2 - info2->threshold);
			float waterCoef=0.0f;
			if ( terrainTypes[info1->terrain].swimmable || swimmable1
				|| terrainTypes[info2->terrain].swimmable || swimmable2 ) {
				waterCoef = (WATER_START-height) / (WATER_START +1);
			}
			TerrainGenData *info=NULL;
			if ( (terrainTypeCoef < 0.25f && !swimmable2 && !terrainTypes[info2->terrain].swimmable)  || swimmable1 || terrainTypes[info1->terrain].swimmable) {
		      	TCODColor groundCol1=TCODColor::lerp(terrainTypes[info1->terrain].color,
					nextColor1,
					layer1Height);
		      	dungeon->setGroundColor(x,y,groundCol1);
		      	/*
				if ( terrainTypes[info1->terrain].swimmable && swimmable1 ) waterCoef=1.0f;
				else if ( terrainTypes[info1->terrain].swimmable ) waterCoef=1.0f-layer1Height;
				else if ( swimmable1 ) waterCoef=layer1Height;
				*/
				info=info1;
			} else if ( terrainTypeCoef > 0.75f || swimmable2 || terrainTypes[info2->terrain].swimmable) {
		      	TCODColor groundCol2=TCODColor::lerp(terrainTypes[info2->terrain].color,
					nextColor2,
					layer2Height);
		      	dungeon->setGroundColor(x,y,groundCol2);
		      	/*
				if ( terrainTypes[info2->terrain].swimmable && swimmable2 ) waterCoef=1.0f;
				else if ( terrainTypes[info2->terrain].swimmable ) waterCoef=1.0f-layer2Height;
				else if ( swimmable2 ) waterCoef=layer2Height;
				*/
				info=info2;
			} else {
		      	TCODColor groundCol1=TCODColor::lerp(terrainTypes[info1->terrain].color,
					nextColor1,
					layer1Height);
					/*
				float waterCoef1=0.0f,waterCoef2=0.0f;
				if ( terrainTypes[info1->terrain].swimmable && swimmable1 ) waterCoef1=1.0f;
				else if ( terrainTypes[info1->terrain].swimmable ) waterCoef1=1.0f-layer1Height;
				else if ( swimmable1 ) waterCoef1=layer1Height;
				*/

		      	TCODColor groundCol2=TCODColor::lerp(terrainTypes[info2->terrain].color,
					nextColor2,
					layer2Height);
					/*
				if ( terrainTypes[info2->terrain].swimmable && swimmable2 ) waterCoef2=1.0f;
				else if ( terrainTypes[info2->terrain].swimmable ) waterCoef2=1.0f-layer2Height;
				else if ( swimmable2 ) waterCoef2=layer2Height;
				*/

				float coef=(terrainTypeCoef-0.25f)*2;
				if ( terrainTypes[info1->terrain].swimmable && swimmable1 ) coef = 1.0f-waterCoef;
				else if ( terrainTypes[info2->terrain].swimmable && swimmable2 ) coef=waterCoef;
		      	dungeon->setGroundColor(x,y,TCODColor::lerp(groundCol1,groundCol2,coef));
				//waterCoef=waterCoef2*coef + waterCoef1*(1.0f-coef);
				info = ( terrainTypeCoef <= 0.5f ? info1 : info2 );
			}
			if (terrainTypes[info->terrain].ripples) waterCoef=MAX(0.01f,waterCoef);
			dungeon->getSubCell(x,y)->waterCoef=waterCoef;
			if ( (x&1) == 0 && (y&1) == 0 && dungeon->getTerrainType(x/2,y/2) != TERRAIN_WOODEN_FLOOR ) {
				dungeon->setTerrainType(x/2,y/2,info->terrain);
				EntityProb *itemData=info->itemData;
				int count=MAX_ENTITY_PROB;
				while (count > 0 && (itemData->itemTypeName != NULL || itemData->creatureType != -1)) {
					if (layer1Height >= itemData->minThreshold && layer1Height < itemData->maxThreshold
						&& forestRng->getFloat(0.0,1.0) < itemData->density) {
						if ( itemData->itemTypeName ) {
							ItemType *type=ItemType::getType(itemData->itemTypeName);
							if (! type ) {
								printf ("FATAL : unknown item type '%s'\n",itemData->itemTypeName);

							} else {
								if ( type->isA("tree") ) placeTree(dungeon,x,y,type);
								else dungeon->addItem(Item::getItem(type, x/2, y/2));
							}
						} else {
							Creature *cr=Creature::getCreature((CreatureTypeId)itemData->creatureType);
							cr->setPos(x/2,y/2);
							dungeon->addCreature(cr);
						}
					}
					itemData++;
					count--;
				}
			}

		}
	}


//	static float lightDir[3]={0.2f,0.0f,1.0f};
//	dungeon->computeOutdoorLight(lightDir, sunColor);
	dungeon->smoothShadow();
	dungeon->computeSpawnSources();
//	dungeon->applyShadowMap();
#ifndef NDEBUG
	float t1=TCODSystem::getElapsedSeconds();
	DBG(("Forest generation end. %g sec\n",t1-t0));
#endif
}


void TreeBurner::onActivate() {
	renderer = new OutdoorRenderer();
	TCODConsole::root->setDefaultBackground(TCODColor::black);
	TCODConsole::root->clear();
	// disable fading (to see the progress bar)
	TCODConsole::setFade(255,TCODColor::black);
	GameEngine::onActivate();
	init();
	TCODConsole::mapAsciiCodeToFont(TCOD_CHAR_BOLT_N,27,3);
	TCODConsole::mapAsciiCodeToFont(TCOD_CHAR_BOLT_NNE,28,3);
	TCODConsole::mapAsciiCodeToFont(TCOD_CHAR_BOLT_NE,29,3);
	TCODConsole::mapAsciiCodeToFont(TCOD_CHAR_BOLT_ENE,30,3);
	TCODConsole::mapAsciiCodeToFont(TCOD_CHAR_BOLT_E,31,3);
	TCODConsole::mapAsciiCodeToFont(TCOD_CHAR_BOLT_ESE,27,4);
	TCODConsole::mapAsciiCodeToFont(TCOD_CHAR_BOLT_SE,28,4);
	TCODConsole::mapAsciiCodeToFont(TCOD_CHAR_BOLT_SSE,29,4);

	generateMap(TCODRandom::getInstance()->getInt(0,0x7FFFFFFF));

	/*
	Item *staff=Item::getRandomWeapon("staff",ITEM_CLASS_STANDARD);
	staff->name=strdup("pyromancer staff");
	player->addToInventory(staff);
	player->equip(staff);
	*/
	player->x=20;
	player->y=FOREST_H/2;
	dungeon->setAmbient(getColourParam("sunColor"));
	int px,py;
	px=(int)player->x;
	py=(int)player->y;
	dungeon->getClosestWalkable(&px,&py,true,false);
	player->x=px;
	player->y=py;
	player->setName("You");
	// make player uber powerful
	Powerup::init();
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

	// re-enable fading
	TCODConsole::setFade(0,TCODColor::black);
	fade=FADE_UP;
	fadeLvl=0.0f;
	player->maxFovRange=player->fovRange=8;
	timefix=1.0f;
	gui.log.critical ("Welcome to %s ! %c?%c for help.",getStringParam("windowTitle"),TCOD_COLCTRL_2,TCOD_COLCTRL_STOP);
	lookOn=false;
	rippleManager = new RippleManager(dungeon);
	fireManager = new FireManager(dungeon);
	aiDirector.setBaseCreature(CREATURE_VILLAGER);

	Objective *obj=new Objective("Find the village",
			"They must be hiding in this forest. Find the village. It must be destroyed!",
			NULL,
		"local x=creaturePos('player')"
		"if x > cityWallX then "
		"   addObjectiveStep('As expected, their laughable village was hidden deep in the woods. Now there\\'s only one thing to do... Burn them all!')"
		"   closeObjective(true)"
		"	activateObjective('Kill the village head')"
		"end ");
	obj->onSuccess->setFloatVariable("cityWallX",cityWallX);
	gui.objectives.addObjective(obj);

	obj=new Objective("Kill the village head",
			"The old man is probably in the village! Find him and turn him into ashes.",
			"",
		"local l=creatureLife('village head')"
		"if l == nil or l <= 0 then "
		"   addObjectiveStep('The old fool! His weak powers were not match...')"
		"   closeObjective(true)"
		"end ");
	gui.objectives.addObjective(obj);

	gui.tutorial.disableMenuPage(TUTO_ITEMS);
	gui.tutorial.disableMenuPage(TUTO_INVENTORY2);
	gui.tutorial.enableMenuPage(TUTO_FIREBALL);
	gui.tutorial.startLiveTuto(TUTO_FOOD);
}

void TreeBurner::onDeactivate() {
	GameEngine::onDeactivate();
}

void TreeBurner::onFontChange() {
	float oldAspectRatio=aspectRatio;
	GameEngine::onFontChange();
	// recompute canopy if aspect ratio has changed (we want round trees!)
	if ( dungeon->canopy && oldAspectRatio != aspectRatio ) {
		recomputeCanopy();
	}
}
