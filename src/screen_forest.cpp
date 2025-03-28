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
#define FOREST_H 400
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
		{NULL,CREATURE_DEER,0.01f,0.0f,1.0f},
		{"wolfsbane",-1,0.001f,0.5f,1.0f},
		{"ginko",-1,0.001f,0.5f,1.0f},
		{"klamath",-1,0.001f,0.0f,1.0f},
		{"yarrow",-1,0.001f,0.0f,1.0f},
		{"chamomile",-1,0.001f,0.0f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GRASS_NORMAL,0.0f,
		{{"pine tree",-1,0.28f,0.0f,1.0f},
		{"klamath",-1,0.001f,0.0f,1.0f},
		{"ephedra",-1,0.001f,0.0f,1.0f},
		{"yarrow",-1,0.001f,0.0f,1.0f},
		{"chamomile",-1,0.001f,0.0f,1.0f},
		{"passiflora",-1,0.001f,0.0f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GRASS_SPARSE,-0.9f,
		{{"pine tree",-1,0.1f,0.17f,1.0f},
		{"ephedra",-1,0.001f,0.8f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GROUND,-1.25f,{{NULL,-1}}},
    {TERRAIN_GROUND,-1.5f,{{NULL,-1}}},
	}},
	{"northern forest",
	{{TERRAIN_GRASS_NORMAL,0.66f,
		{{"oak tree",-1,0.2f,0.0f,1.0f},
		{"apple tree",-1,0.05f,0.0f,1.0f},
		{"klamath",-1,0.001f,0.0f,1.0f},
		{"ephedra",-1,0.001f,0.0f,1.0f},
		{"acanthopax",-1,0.001f,0.0f,1.0f},
		{"yarrow",-1,0.001f,0.0f,1.0f},
		{"chamomile",-1,0.001f,0.0f,1.0f},
		{"passiflora",-1,0.001f,0.0f,1.0f},
		{"psyllium",-1,0.001f,0.5f,1.0f},
		{"wolfsbane",-1,0.001f,0.5f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GRASS_SPARSE,0.33f,
		{{"oak tree",-1,0.11f,0.0f,1.0f},
		{"apple tree",-1,0.05f,0.0f,1.0f},
		{"ephedra",-1,0.001f,0.8f,1.0f},
		{"chamomile",-1,0.001f,0.8f,1.0f},
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
		{"ginko",-1,0.001f,0.5f,1.0f},
		{"klamath",-1,0.001f,0.0f,1.0f},
		{"yarrow",-1,0.001f,0.0f,1.0f},
		{"chamomile",-1,0.001f,0.0f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GRASS_NORMAL,-0.4f,
		{{"oak tree",-1,0.14f,0.0f,1.0f},
		{"klamath",-1,0.001f,0.0f,1.0f},
		{"ephedra",-1,0.001f,0.0f,1.0f},
		{"acanthopax",-1,0.001f,0.0f,1.0f},
		{"yarrow",-1,0.001f,0.0f,1.0f},
		{"chamomile",-1,0.001f,0.0f,1.0f},
		{"passiflora",-1,0.001f,0.0f,1.0f},
		{NULL,-1}}},
    {TERRAIN_GRASS_SPARSE,-0.8f,
		{{"oak tree",-1,0.06f,0.12f,1.0f},
		{"stone",-1,0.1f,0.0f,0.1f},
		{"broom",-1,0.001f,0.0f,0.5f},
		{"chaparral",-1,0.001f,0.0f,0.5f},
		{"dill",-1,0.001f,0.0f,0.5f},
		{"ephedra",-1,0.001f,0.8f,1.0f},
		{"chamomile",-1,0.001f,0.8f,1.0f},
		{NULL,-1}}},
    {TERRAIN_SHALLOW_WATER,WATER_START,
		{{"stone",-1,0.25f,0.2f,1.0f},
		{"broom",-1,0.001f,0.2f,1.0f},
		{"chaparral",-1,0.001f,0.2f,1.0f},
		{"dill",-1,0.001f,0.2f,1.0f},
		{NULL,-1}}},
    {TERRAIN_DEEP_WATER,-1.0f,{{NULL,-1}}},
//    {TERRAIN_DIRT,0,-1.0f,-0.75f},
//    {TERRAIN_GRASS_SPARSE,0,-1.25f,-0.75f},
	}},
};

ForestScreen::ForestScreen() {
	forestRng=NULL;
	fadeInLength=fadeOutLength=(int)(config.getFloatProperty("config.display.fadeTime")*1000);
}

void ForestScreen::render() {
	// draw subcell ground
	renderer->renderSubcells();
	// blit it on console
	ground->blit2x(TCODConsole::root,0,0);
	// render the items
	// render the creatures
	// render the player
	renderer->renderCells();
	// apply sepia post-processing
	applySepia();

//TCODConsole::root->print(0,2,"player pos %d %d\nfriend pos %d %d\n",player->x,player->y,fr->x,fr->y);
	if ( player->getName()[0] == 0 ) textInput.render(CON_W/2,2);
}

bool ForestScreen::update(float elapsed, TCOD_key_t &k,TCOD_mouse_t &mouse) {
	static bool debug=config.getBoolProperty("config.debug");

	mousex=mouse.cx;
	mousey=mouse.cy;
	if ( firstFrame ) elapsed=0.1f;
	firstFrame=false;

	GameEngine::update(elapsed,k,mouse);

	if ( player->getName()[0] == 0 ) {
		if (!textInput.update(elapsed,k)) {
			player->setName(textInput.getText());
			TextGenerator::addGlobalValue("PLAYER_NAME",player->getName());
			resumeGame();
		} else return true;
	}

	if ( (k.c == 'm' || k.c =='M') && ! k.pressed && k.lalt) {
			// ALT-M : switch control type
			userPref.mouseOnly=!userPref.mouseOnly;
			gui.log.warn(userPref.mouseOnly ? "Mouse only" : "Mouse + keyboard");
			k.c=0;k.vk=TCODK_NONE;
	} else if ( k.c ==' ' && ! k.pressed && gui.mode == GUI_NONE ) {
		if (isGamePaused()) resumeGame();
		else pauseGame();
	} else if ( ! k.pressed && (k.c == 'i' || k.c =='I' ) ) {
		openCloseInventory();
	} else if ( ! k.pressed && (k.c == 'o' || k.c =='O' ) ) {
		openCloseObjectives();
	} else if ( ! k.pressed && (k.c == 'c' || k.c =='C' ) ) {
		openCloseCraft();
	}
	// non player related keyboard handling
	if ( debug ) {
		// debug/cheat shortcuts
		if ( k.c == 'd' && k.lalt && ! k.pressed) {
			// debug mode : Alt-d = player takes 'd'amages
			player->takeDamage(20);
		} else if ( k.c == 'i' && k.lalt && ! k.pressed) {
			// debug mode : Alt-i = item
			dungeon->addItem(Item::getItem("short bronze blade",player->x,player->y-1));
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

	// update items
	dungeon->updateItems(elapsed,k,&mouse);

	// calculate player fov
	dungeon->computeFov((int)player->x,(int)player->y);

	// update monsters
	if ( fade != FADE_DOWN ) {
		dungeon->updateCreatures(elapsed);
		// ripples must be after creatures because of shoal updates
		rippleManager->updateRipples(elapsed);
		fireManager->update(elapsed);
	}
	dungeon->updateClouds(elapsed);

	HerdBehavior::updateScarePoints(elapsed);

	// update particles
	updateParticles(elapsed);

	gui.descriptor.setFocus(mousex,mousey,mousex+xOffset,mousey+yOffset,lookOn);

	return true;
}

void ForestScreen::placeHouse(Dungeon *dungeon,int doorx, int doory,Entity::Direction dir) {
	Building *building=Building::generate(9,7,2,forestRng);
	building->applyTo(dungeon,doorx,doory);
	building->setHuntingHide(dungeon);
}

void ForestScreen::placeTree(Dungeon *dungeon,int x, int y, const ItemType * treeType) {
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
	if (treeType->hasFeature(ItemFeature::PRODUCE) ) {
		float odds=forestRng->getFloat(0.0,30.0);
		if (odds <= 1.0) {
			// drop some fruit/twig
			int dx=x/2;
			int dy=y/2;
			dungeon->getClosestWalkable(&dx,&dy);
			Item *it = treeType->produce(odds);
			if (it) {
				it->setPos(dx,dy);
				dungeon->addItem(it);
			}
		}
	}

}

int housex,housey;
void ForestScreen::generateMap(uint32_t seed) {
	DBG(("Forest generation start\n"));
	forestRng = new TCODRandom(seed);
	dungeon = new Dungeon(FOREST_W,FOREST_H);

	saveGame.registerListener(CHA1_CHUNK_ID,PHASE_START,this);
    saveGame.registerListener(DUNG_CHUNK_ID,PHASE_START,dungeon);
	saveGame.registerListener(PLAY_CHUNK_ID,PHASE_START,player);

	//lightMap->clear(sunColor);
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
	housex=forestRng->getInt(20,dungeon->width-20);
	housey=forestRng->getInt(20,dungeon->height-20);
	// don't put the house on water
	while ( dungeon->hasWater(housex,housey) ) {
		housex += 4;
		if ( housex > dungeon->width-20 ) {
			housex=20;
			housey += 4;
			if ( housey > dungeon->height-20 ) housey = 20;
		}
	}
	placeHouse(dungeon,housex,housey,Entity::NORTH);
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
			forestTypeId=std::min<float>(NB_FORESTS-1,forestTypeId);
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
			if (terrainTypes[info->terrain].ripples) waterCoef=std::max(0.01f,waterCoef);
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

// SaveListener
#define CHA1_CHUNK_VERSION 1
bool ForestScreen::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip *zip) {
	if ( chunkVersion != CHA1_CHUNK_VERSION ) return false;
	float lastDate=zip->getFloat();
	dateDelta = lastDate - TCODSystem::getElapsedSeconds();
	return true;
}

void ForestScreen::saveData(uint32_t chunkId, TCODZip *zip) {
	saveGame.saveChunk(CHA1_CHUNK_ID,CHA1_CHUNK_VERSION);
	zip->putFloat(gameEngine->getCurrentDate());
}

void ForestScreen::loadMap(uint32_t seed) {
	DBG(("Forest loading start\n"));
	//lightMap->clear(sunColor);
	forestRng = new TCODRandom(seed);
	dungeon = new Dungeon(FOREST_W,FOREST_H);

	saveGame.registerListener(CHA1_CHUNK_ID,PHASE_START,this);
	saveGame.registerListener(DUNG_CHUNK_ID,PHASE_START,dungeon);
	saveGame.registerListener(PLAY_CHUNK_ID,PHASE_START,player);

	for (int x=1; x < FOREST_W-1; x++ ) {
		for (int y=1; y < FOREST_H-1; y++ ) {
			dungeon->map->setProperties(x,y,true,true);
		}
	}
	for (int x=2; x < 2*FOREST_W-2; x++ ) {
		for (int y=2; y < 2*FOREST_H-2; y++ ) {
			dungeon->map2x->setProperties(x,y,true,true);
		}
	}
#ifndef NDEBUG
	float t0=TCODSystem::getElapsedSeconds();
#endif
	saveGame.load(PHASE_START);

	dungeon->computeSpawnSources();
	fr = (Friend *)dungeon->getCreature(CREATURE_FRIEND);
#ifndef NDEBUG
	float t1=TCODSystem::getElapsedSeconds();
	DBG(("Forest loading end. %g sec\n",t1-t0));
#endif

}

void ForestScreen::onActivate() {
	renderer = new OutdoorRenderer();
	TCODConsole::root->setDefaultBackground(TCODColor::black);
	TCODConsole::root->clear();
	// disable fading (to see the progress bar)
	TCODConsole::setFade(255,TCODColor::black);
	GameEngine::onActivate();
	init();
	MainMenu::instance->waitForForestGen();

	if ( newGame ) {
		generateMap(saveGame.seed);
		dungeon->setPlayerStartingPosition();
		int fx,fy;
		fr = new Friend();
		Item *knife=Item::getRandomWeapon("knife",ITEM_CLASS_STANDARD);
		knife->addComponent(Item::getItem("emerald",0,0,false));
		knife->name=strdup("emerald pocketknife");
		knife->article=Item::AN;
		player->addToInventory(knife);
		player->x=housex;
		player->y=housey+10;
		int px,py;
		px=(int)player->x;
		py=(int)player->y;
		dungeon->getClosestWalkable(&px,&py,true,false);
		player->x=px;
		player->y=py;
		dungeon->getClosestSpawnSource(player->x,player->y, &fx, &fy);
		dungeon->getClosestWalkable(&fx,&fy,true,false);
		fr->setPos(fx,fy);
		dungeon->addCreature(fr);
	} else {
		displayProgress(0.02f);
		loadMap(saveGame.seed);
	}
	dungeon->setAmbient(getColourParam("sunColor"));
	// re-enable fading
	TCODConsole::setFade(0,TCODColor::black);
	fade=FADE_UP;
	fadeLvl=0.0f;
	player->maxFovRange=player->fovRange=8;
	timefix=1.0f;
	if ( newGame ) gui.log.critical ("Welcome to the %s ! %c?%c for help.",getStringParam("windowTitle"),TCOD_COLCTRL_2,TCOD_COLCTRL_STOP);
	lookOn=false;
	rippleManager=new RippleManager(dungeon);
	fireManager = new FireManager(dungeon);
	if ( player->getName()[0] == 0 ) {
		if ( userPref.nbLaunches == 1 ) {
			textInput.init("Welcome to The Cave !","PageUp/PageDown to change font size\nPlease enter your name :",60);
		} else {
			textInput.init("Welcome to The Cave !","Please enter your name :",60);
		}

		pauseGame();
	}
}

void ForestScreen::onDeactivate() {
	GameEngine::onDeactivate();
}

void ForestScreen::onFontChange() {
	float oldAspectRatio=aspectRatio;
	GameEngine::onFontChange();
	// recompute canopy if aspect ratio has changed (we want round trees!)
	if ( dungeon->canopy && oldAspectRatio != aspectRatio ) {
		recomputeCanopy();
	}
}
