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
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include "main.hpp"

Dungeon::Dungeon(int width, int height) : level(0), ambient(TCODColor::black) {
	this->width=width;
	this->height=height;
	initData( NULL );
	clouds = new CloudBox(width*2,height*2);
	canopy=new TCODImage(width*2,height*2);
}

void Dungeon::computeSpawnSources() {
	static int spawnSourceRange=config.getIntProperty("config.aidirector.spawnSourceRange");
	// generate spawn sources
	spawnSources.clear();
	for (int x=spawnSourceRange/2; x < width; x+=spawnSourceRange ) {
		for (int y=spawnSourceRange/2; y < height; y+=spawnSourceRange ) {
			if ( map->isWalkable(x,y) ) spawnSources.push(x | (y<<16));
		}
	}
}

// allocate all data
void Dungeon::initData(CaveGenerator *caveGen) {
	cells=new Cell[width*height];
	subcells=new SubCell[width*height*4];
	stairx=stairy=-1;
	if ( caveGen ) {
		map=caveGen->map;
		map2x=caveGen->map2x;
		for (int x=0; x < width*2; x++) {
			for (int y=0; y< height*2; y++) {
				setGroundColor(x,y,caveGen->ground->getPixel(x,y));
			}
		}
	} else {
		gameEngine->displayProgress(0.05f);
		map = new TCODMap(width,height);
		map2x=new TCODMap(width*2,height*2);
	}
	hmap=new TCODHeightMap(width*2,height*2);
	smap=new TCODHeightMap(width*2,height*2);
	smapBeforeTree=new TCODHeightMap(width*2,height*2);
	if (! caveGen ) gameEngine->displayProgress(0.1f);
	isUpdatingItems=false;
	isUpdatingCreatures=false;
}

// deallocate data
void Dungeon::cleanData() {
	if (map ) delete map;
	if ( map2x ) delete map2x;
	if ( cells ) delete [] cells;
	if ( subcells ) delete[] subcells;
	if ( hmap ) delete hmap;
	if ( smap ) delete smap;
	if ( smapBeforeTree) delete smapBeforeTree;
	if ( clouds ) delete clouds;
	map=NULL;
	map2x=NULL;
	cells=NULL;
	subcells=NULL;
	hmap=NULL;
	smap=NULL;
	smapBeforeTree=NULL;
	clouds=NULL;
}

Dungeon::Dungeon(int level, CaveGenerator *caveGen) : level(level), ambient(TCODColor::black){
	// get dungeons min/max size from config
	static int nbLevels=config.getIntProperty("config.gameplay.nbLevels");
	static int minSize=config.getIntProperty("config.gameplay.dungeonMinSize");
	static int maxSize=config.getIntProperty("config.gameplay.dungeonMaxSize");
	static bool debug=config.getBoolProperty("config.debug");

	clouds=NULL;
	width=height=minSize + (maxSize-minSize)*(level+1)/nbLevels;
	do {
		caveGen->generate();
		initData( caveGen );
	} while (!setPlayerStartingPosition());
	computeSpawnSources();

    	finalizeMap(level >= 1,true);

	// put some random lights
	static TCODColor lightCols[9] = {
		TCODColor(60,20,20),
		TCODColor(20,60,20),
		TCODColor(20,20,60),
		TCODColor(60,80,60),
		TCODColor(80,60,60),
		TCODColor(60,60,80),
		TCODColor(40,40,60),
		TCODColor(40,60,40),
		TCODColor(60,40,40),
	};
	// generate some random lights
	for ( int i=0; i < spawnSources.size()/4; i++ ) {
		Light *l=new Light();
		l->range=TCODRandom::getInstance()->getInt(10,40);
		l->color=lightCols[TCODRandom::getInstance()->getInt(0,8)];
		int offset=spawnSources.get(i*4);
		int sx=offset&0xFFFF;
		int sy=offset>>16;
		l->x = sx * 2;
		l->y = sy * 2;
		addLight(l);
	}
	// always one light at player starting position
	Light *l=new Light();
	l->range=TCODRandom::getInstance()->getInt(10,40);
	l->color=lightCols[TCODRandom::getInstance()->getInt(0,8)];
	l->x = gameEngine->player->x * 2;
	l->y = gameEngine->player->y * 2;
	addLight(l);

	// compute light map
	LightMap *temp=new LightMap(width*2,height*2);
	gameEngine->dungeon=this;
	gameEngine->xOffset=0;
	gameEngine->yOffset=0;
	renderLightsToLightMap(temp,NULL,NULL,NULL,NULL,false,false);
	// apply radiosity
	temp->diffuse(1.5f,0.4f,5);
	// put result back in the map subcells
	for (int x=0; x < width*2; x++) {
		for (int y=0; y< height*2; y++) {
			SubCell *sc=getSubCell(x,y);
			HDRColor col=temp->getHdrColor2x(x,y);
			sc->r = std::min(255.0f,col.r);
			sc->g = std::min(255.0f,col.g);
			sc->b = std::min(255.0f,col.b);

		}
	}
	delete temp;
	lights.clearAndDelete();
	if ( debug ) {
		Player *player=gameEngine->player;
		saveMap((int)player->x,(int)player->y);
	}
}

void Dungeon::finalizeMap(bool roundCorners, bool blurGround) {
	static TCODColor groundColor=config.getColorProperty("config.display.groundColor");
	static TCODColor memoryWallColor=config.getColorProperty("config.display.memoryWallColor");

	// scale the map 2x
	for (int x=0; x < width*2; x++) {
		for (int y=0; y< height*2; y++) {
			map2x->setProperties(x,y,map->isTransparent(x/2,y/2),map->isWalkable(x/2,y/2));
		}
	}

	// apply cellular automata to 2x map
	if ( roundCorners ) {
		CellularAutomata cell2x(map2x);
		cell2x.generate(&CellularAutomata::CAFunc_roundCorners,1);
		cell2x.generate(&CellularAutomata::CAFunc_removeInnerWalls,1);
		cell2x.generate(&CellularAutomata::CAFunc_roundCorners,1);
		cell2x.generate(&CellularAutomata::CAFunc_cleanIsolatedWalls,1);
		cell2x.seal();
		cell2x.apply(map2x);
		// fix walk info in normal resolution map
		for (int cx=0; cx < width; cx++) {
			for (int cy=0; cy < height; cy++) {
				int count=0;
				if ( !map2x->isWalkable(cx*2,cy*2) ) count++;
				if ( !map2x->isWalkable(cx*2+1,cy*2) ) count++;
				if ( !map2x->isWalkable(cx*2,cy*2+1) ) count++;
				if ( !map2x->isWalkable(cx*2+1,cy*2+1) ) count++;
				if ( count == 0 ) map->setProperties(cx,cy,true,true);
			}
		}
	}

	// generate ground image
    float noiseDepth = 0.05f * (level-1);
    noiseDepth = std::min(0.3f, noiseDepth);
	for (int x=0; x < width*2; x++) {
		for (int y=0; y < height*2; y++) {
		    if (map2x->isTransparent(x,y)) {
		        // ground
		        float f[2]={ x/3.0f, y/3.0f };
		        float coef = 1.0f + noiseDepth*noise2d.getFbm(f,3.0f);
                setGroundColor(x,y, groundColor * coef);
		    } else {
                // wall
                setGroundColor(x,y, memoryWallColor );
		    }
		}
	}
	// smooth it
	if ( blurGround ) {
		for (int x=0; x < width*2-1; x++) {
			for (int y=0; y < height*2-1; y++) {
				int r=0,g=0,b=0;
				TCODColor col=getGroundColor(x,y);
				r+=col.r;
				g+=col.g;
				b+=col.b;
				col=getGroundColor(x+1,y);
				r+=col.r;
				g+=col.g;
				b+=col.b;
				col=getGroundColor(x+1,y+1);
				r+=col.r;
				g+=col.g;
				b+=col.b;
				col=getGroundColor(x,y+1);
				r+=col.r;
				g+=col.g;
				b+=col.b;
				setGroundColor(x,y,TCODColor(r/4,g/4,b/4) );
			}
		}
	}

}

void Dungeon::smoothShadow() {
	// compute shadow from shadowheight
	for (int y=0; y < height*2-1; y++) {
		float z=getShadowHeight(width*2-1,y);
		float rayZ=z;
		for (int x=width*2-2; x >= 0; x--) {
			rayZ -= 1.0f;
			z=getShadowHeight(x,y);
			if ( rayZ < z ) {
				rayZ=z;
				if(  z > 0.0f ) setShadow(x,y,getShadow(x,y)*0.9f);
			} else {
				setShadow(x,y,getShadow(x,y)*0.9f);
			}
		}
	}
	// smooth shadow
	for (int x=0; x < width*2-1; x++) {
		for (int y=0; y < height*2-1; y++) {
			float shadow=0.0f;
			shadow+=getShadow(x,y);
			shadow+=getShadow(x+1,y);
			shadow+=getShadow(x+1,y+1);
			shadow+=getShadow(x,y+1);
			setShadow(x,y,0.25f * shadow);
		}
	}
}

void Dungeon::updateLights(float elapsed) {
	for (Light **it=lights.begin();it!=lights.end(); it++) {
		(*it)->update(elapsed);
	}
}

void Dungeon::renderLightsToLightMap(LightMap *lightMap, int *minx, int *miny, int *maxx, int *maxy, bool clearMap, bool withFov) {
	int minx2x=width*2-1;
	int maxx2x=0;
	int miny2x=height*2-1;
	int maxy2x=0;
	if ( clearMap ) lightMap->clear(ambient);
	for (Light **it=lights.begin();it!=lights.end(); it++) {
		int light_minx,light_maxx,light_miny,light_maxy;
		(*it)->addToLightMap(lightMap,withFov);
		(*it)->getDungeonPart(&light_minx,&light_miny,&light_maxx,&light_maxy);
		if ( minx2x > light_minx ) minx2x=light_minx;
		if ( maxx2x < light_maxx ) maxx2x=light_maxx;
		if ( miny2x > light_miny ) miny2x=light_miny;
		if ( maxy2x < light_maxy ) maxy2x=light_maxy;
	}
	if ( minx && *minx > minx2x ) *minx=minx2x;
	if ( miny && *miny > miny2x ) *miny=miny2x;
	if ( maxx && *maxx < maxx2x ) *maxx=maxx2x;
	if ( maxy && *maxy < maxy2x ) *maxy=maxy2x;
}

void Dungeon::renderLightsToImage(TCODImage *img, int *minx, int *miny, int *maxx, int *maxy) {
	int minx2x=width*2-1;
	int maxx2x=0;
	int miny2x=height*2-1;
	int maxy2x=0;
	for (Light **it=lights.begin();it!=lights.end(); it++) {
		int light_minx,light_maxx,light_miny,light_maxy;
		(*it)->addToImage(img);
		(*it)->getDungeonPart(&light_minx,&light_miny,&light_maxx,&light_maxy);
		if ( minx2x > light_minx ) minx2x=light_minx;
		if ( maxx2x < light_maxx ) maxx2x=light_maxx;
		if ( miny2x > light_miny ) miny2x=light_miny;
		if ( maxy2x < light_maxy ) maxy2x=light_maxy;
	}
	if ( minx )	*minx=minx2x;
	if ( miny ) *miny=miny2x;
	if ( maxx ) *maxx=maxx2x;
	if ( maxy ) *maxy=maxy2x;
}

#define addToMemory(x,y) cells[(x)+(y)*width].memory=true;
void Dungeon::setMemory(int x, int y) {
	if ( cells[x+y*width].memory ) return;
	addToMemory(x,y);
	if ( ! map->isTransparent(x,y) ) {
		if ( x < width-1 && y < height-1 && ! map->isTransparent(x+1,y+1) && cells[x+1+(y+1)*width].memory ) {
			addToMemory(x+1,y);
			addToMemory(x,y+1);
		}
		if ( x > 1 && y < height -1 && ! map->isTransparent(x-1,y+1) && cells[x-1+(y+1)*width].memory ) {
			addToMemory(x-1,y);
			addToMemory(x,y+1);
		}
		if ( x < width-1 && y > 1 && ! map->isTransparent(x+1,y-1) && cells[x+1+(y-1)*width].memory ) {
			addToMemory(x+1,y);
			addToMemory(x,y-1);
		}
		if ( x > 1 && y > 1 && ! map->isTransparent(x-1,y-1) && cells[x-1+(y-1)*width].memory ) {
			addToMemory(x-1,y);
			addToMemory(x,y-1);
		}
	}
}
#undef addToMemory

Dungeon::~Dungeon() {
	items.clearAndDelete();
	creatures.clearAndDelete();
	corpses.clearAndDelete();
	cleanData();
}

void Dungeon::getClosestWalkable(int *x, int *y, bool includingStairs, bool includingCreatures, bool includingWater) const {
	if ( map->isWalkable(*x,*y)
		&& (includingStairs || *x != stairx || *y != stairy )
		&& (includingCreatures || ! hasCreature(*x,*y) )
		&& (includingWater || !hasRipples(*x,*y))
		) return;
	int dist=1000000;
	int bestx=0,besty=0;
	int range=10;
	do {
		dist=1000000;
		int minx=*x - range;
		int maxx=*x + range;
		int miny=*y - range;
		int maxy=*y + range;
		minx=std::max(0,minx);
		miny=std::max(0,miny);
		maxx=std::min(width-1,maxx);
		maxy=std::min(height-1,maxy);
		for (int cx=minx; cx <= maxx; cx++) {
			for (int cy=miny; cy <= maxy; cy++) {
				if ( map->isWalkable(cx,cy)
					&& (includingStairs || cx != stairx || cy != stairy )
					&& (includingCreatures || ! hasCreature(cx,cy) )
					&& (includingWater || !hasRipples(cx,cy) )

				) {
					int curdist=SQRDIST(*x,*y,cx,cy);
					if ( curdist < dist ) {
						dist=curdist;
						bestx=cx;
						besty=cy;
					}
				}
			}
		}
		range*=2;
	} while ( dist == 1000000 );
	*x=bestx;
	*y=besty;
}

TCODColor Dungeon::getShadedGroundColor(int x2, int y2, LightMap *lightmap) const {
	TCODColor col=getGroundColor(x2,y2);
	float cloudIntensity = 1.0f;
	bool useLightMap=false;
	if ( clouds ) {
		// cloud shadow
		cloudIntensity = getInterpolatedCloudCoef(x2,y2);
	}
	if ( lightmap ) {
		int conx2=x2-gameEngine->xOffset*2;
		int cony2=y2-gameEngine->yOffset*2;
		if ( IN_RECTANGLE(conx2,cony2,CON_W*2,CON_H*2) ) {
			col = col * lightmap->getHdrColor2x(conx2,cony2);
			useLightMap=true;
		}
	}
	if (! useLightMap) {
		// natural ground shadow (tree, house,...)
		float intensity = getShadow(x2,y2);
		intensity=std::min(intensity,cloudIntensity);
		if ( intensity < 1.0f ) {
			col = col * intensity;
		}
	}
	/*
	if (clouds && hasRipples(x2/2,y2/2)) {
		// cloud reflection
		float coef=getWaterCoef(x2,y2);
		if ( coef > 0.0f) {
			TCODColor skyCol=clouds->getColor(cloudIntensity,x2,y2);
			col = TCODColor::lerp(col,skyCol,0.5f*coef);
		}
	}
	*/
	return col;
}

TCODColor Dungeon::getGroundColor(int x2, int y2) const {
	TCODColor col = subcells[x2+y2*width*2].groundColor;
	if (clouds && hasRipples(x2/2,y2/2)) {
		// cloud reflection
		float coef=getWaterCoef(x2,y2);
		if ( coef > 0.0f) {
			float cloudIntensity = getInterpolatedCloudCoef(x2,y2);
			TCODColor skyCol=clouds->getColor(cloudIntensity,x2,y2);
			col = TCODColor::lerp(col,skyCol,0.5f*coef);
		}
	}
	return col;
}

void Dungeon::getClosestSpawnSource(int x,int y, int *ssx, int *ssy) const {
	int dist=1000000;
	TCODList<int> bests;
	TCODPath path(map);
	// get the closest sources (straight distance)
	for (int *it=spawnSources.begin(); it != spawnSources.end(); it++) {
		int sx=(*it)&0xFFFF;
		int sy=(*it)>>16;
		if (map->isInFov(sx,sy) && getMemory(sx,sy) ) {
			// cannot spawn from a visible spawnsource
			continue;
		}
		int curdist=SQRDIST(x,y,sx,sy);
		if (curdist < dist) {
			dist=curdist;
			bests.push(*it);
		}
	}
	// now check the path distance for the 5 bests
	/*
	dist=1000000;
	TCODList<int> bests2;
	for (int i=0; i < 5; i++ ) {
		int best=bests.pop();
		int sx=best&0xFFFF;
		int sy=best>>16;
		if (path.compute(x,y,sx,sy) && path.size()<dist) {
			dist=path.size();
			bests2.push(best);
		}
	}
	*/
	// return one of the 3 bests
	int b=TCODRandom::getInstance()->getInt(1,std::min(3,bests.size()));
	int best=bests.get(bests.size()-b);
	*ssx=best&0xFFFF;
	*ssy=best>>16;
}

void Dungeon::getRandomPositionInCorner(int cornerx,int cornery, int *px, int *py) {
	int posx=rng->getInt(0,width/2-1);
	int posy=rng->getInt(0,height/2-1);
	posx+=cornerx*width/2;
	posy+=cornery*height/2;
	getClosestWalkable(&posx,&posy);
	*px=posx;
	*py=posy;
}

bool Dungeon::setPlayerStartingPosition() {
	Player *player=gameEngine->player;
	// choose a corner
	int cornerx=rng->getInt(0,1);
	int cornery=rng->getInt(0,1);
	int px,py;
	getRandomPositionInCorner(cornerx,cornery,&px,&py);
	player->setPos(px,py);
	// set the stair position in another corner
	int dist=0;
	do {
		int r=rng->getInt(1,3);
		if ( r&1 ) cornerx=1-cornerx;
		if ( r&2 ) cornery=1-cornery;
		getRandomPositionInCorner(cornerx,cornery,&stairx,&stairy);
		// check that the stair is not too close to player starting position
		TCODPath path(map);
		dist=1000000;
		if (path.compute((int)player->x,(int)player->y,stairx,stairy)) {
			dist=path.size();
		}
	} while (dist < width/3);
	return (dist != 1000000);
}

void Dungeon::saveMap(int playerX,int playerY) {
	// save the map as png
	TCODImage tmp(width*2,height*2);
	for (int x=0; x < width*2; x++) {
		for (int y=0; y< height*2; y++) {
			//tmp.putPixel(x,y,map2x->isTransparent(x,y) ? TCODColor::lightGrey:TCODColor::darkGrey);
			TCODColor col;
			if (map2x->isTransparent(x,y)) {
				col=getGroundColor(x,y);
				SubCell *sc = getSubCell(x,y);
				col.r = (int)(col.r*sc->r/255.0f);
				col.g = (int)(col.g*sc->g/255.0f);
				col.b = (int)(col.b*sc->b/255.0f);
			} else {
				col = TCODColor::lightGrey;
			}
			tmp.putPixel(x,y,col);
		}
	}
	for (int *i=spawnSources.begin(); i!= spawnSources.end(); i++) {
		int x=2*((*i)&0xFFFF);
		int y=2*((*i)>>16);
		tmp.putPixel(x,y,TCODColor::orange);
		tmp.putPixel(x+1,y,TCODColor::orange);
		tmp.putPixel(x+2,y,TCODColor::orange);
		tmp.putPixel(x,y+1,TCODColor::orange);
		tmp.putPixel(x,y+2,TCODColor::orange);
	}
	if ( playerX != -1 ) {
        tmp.putPixel(playerX*2,playerY*2,TCODColor::blue);
        tmp.putPixel(playerX*2-1,playerY*2,TCODColor::blue);
        tmp.putPixel(playerX*2+1,playerY*2,TCODColor::blue);
        tmp.putPixel(playerX*2,playerY*2-1,TCODColor::blue);
        tmp.putPixel(playerX*2,playerY*2+1,TCODColor::blue);
        tmp.putPixel(playerX*2-2,playerY*2,TCODColor::blue);
        tmp.putPixel(playerX*2+2,playerY*2,TCODColor::blue);
        tmp.putPixel(playerX*2,playerY*2-2,TCODColor::blue);
        tmp.putPixel(playerX*2,playerY*2+2,TCODColor::blue);
	}
	if ( stairx != -1 ) {
        tmp.putPixel(stairx*2,stairy*2,TCODColor::red);
        tmp.putPixel(stairx*2+1,stairy*2,TCODColor::red);
        tmp.putPixel(stairx*2-1,stairy*2,TCODColor::red);
        tmp.putPixel(stairx*2,stairy*2+1,TCODColor::red);
        tmp.putPixel(stairx*2,stairy*2-1,TCODColor::red);
        tmp.putPixel(stairx*2+2,stairy*2,TCODColor::red);
        tmp.putPixel(stairx*2-2,stairy*2,TCODColor::red);
        tmp.putPixel(stairx*2,stairy*2+2,TCODColor::red);
        tmp.putPixel(stairx*2,stairy*2-2,TCODColor::red);
	}
	char fname[128];
	sprintf(fname,"cave%02d.png",level);
	tmp.save(fname);
}

Creature *Dungeon::getCreature(int x,int y) const {
	if (!IN_RECTANGLE(x,y,width,height)) return NULL;
	if ( getCell(x,y)->nbCreatures == 0 ) return NULL;
	for ( Creature **it=creatures.begin(); it != creatures.end(); it++ ) {
		if ( (int)(*it)->x == x && (int)(*it)->y ==y ) return *it;
	}
	return NULL;
}

Creature *Dungeon::getCreature(CreatureTypeId id) const {
	for ( Creature **it=creatures.begin(); it != creatures.end(); it++ ) {
		if ( (*it)->type == id ) return *it;
	}
	return NULL;
}

Creature *Dungeon::getCreature(const char *name) const {
	if ( strcmp(name,"player") == 0 ) return gameEngine->player;
	for ( Creature **it=creatures.begin(); it != creatures.end(); it++ ) {
		if ( strcmp((*it)->getName(),name) == 0 ) return *it;
	}
	return NULL;
}

void Dungeon::moveCreature(Creature *cr,int xFrom,int yFrom, int xTo, int yTo) {
//printf ("%x %d %d -> %d %d\n",cr,xFrom,yFrom,xTo,yTo);
	getCell(xTo,yTo)->nbCreatures++;
	assert(getCell(xFrom,yFrom)->nbCreatures>0);
	getCell(xFrom,yFrom)->nbCreatures--;
}

void Dungeon::addCreature(Creature *cr) {
	if (isUpdatingCreatures) {
		creaturesToAdd.push(cr);
	} else {
		creatures.push(cr);
		getCell(cr->x,cr->y)->nbCreatures++;
	}
}

bool Dungeon::hasCreature(int x,int y) const {
	if (!IN_RECTANGLE(x,y,width,height)) return false;
	if ( getCell(x,y)->nbCreatures == 0 ) return false;
	return true;
}

void Dungeon::removeCreature(Creature *cr, bool kill) {
	Cell *cell=getCell(cr->x,cr->y);
	cell->nbCreatures--;
	if ( kill ) {
		if ( ! cell->hasCorpse ) {
			addCorpse(cr);
		} else cr->toDelete=true;
	} else {
		creatures.remove(cr);
	}
}

void Dungeon::addCorpse(Creature *cr) {
	getCell(cr->x,cr->y)->hasCorpse=true;
	corpses.push(cr);
}

void Dungeon::renderCreatures(LightMap *lightMap) {
	for ( Creature **it=creatures.begin(); it != creatures.end(); it++ ) {
		if ( (*it)->ch != 0 ) {
			if ( map->isInFov((int)(*it)->x,(int)(*it)->y) ) (*it)->render(lightMap);
			if ( (*it)->isTalking() ) (*it)->renderTalk();
		}
	}
}

void Dungeon::renderSubcellCreatures(LightMap *lightMap) {
	for ( Creature **it=creatures.begin(); it != creatures.end(); it++ ) {
		if ( (*it)->ch == 0 ) {
			assert(IN_RECTANGLE((*it)->x,(*it)->y,width,height));
			if (IN_RECTANGLE((*it)->x,(*it)->y,width,height)) {
				if ( map->isInFov((int)(*it)->x,(int)(*it)->y) ) (*it)->render(lightMap);
				if ( (*it)->isTalking() ) (*it)->renderTalk();
			}
		}
	}
}

void Dungeon::updateCreatures(float elapsed) {
	// can't use iterator because the boss update function summon creatures,
	// which may result in creatures reallocation
	TCODList<Creature *>toDelete;
	isUpdatingCreatures=true;
	for ( int i=0; i < creatures.size(); i++ ) {
		Creature *cr=creatures.get(i);
		if ( cr->toDelete ) {
			toDelete.push(cr);
		} else if ((cr->isUpdatedOffscreen() || cr->isOnScreen()) && !cr->update(elapsed)) {
			cr->die();
			toDelete.push(cr);
		}
	}
	isUpdatingCreatures=false;
	for ( Creature **it = toDelete.begin(); it != toDelete.end(); it++) {
		creatures.removeFast(*it);
		removeCreature(*it,! (*it)->toDelete);
		if ( (*it)->toDelete ) delete *it;
	}
	for ( Creature **it = creaturesToAdd.begin(); it != creaturesToAdd.end(); it++) {
		addCreature(*it);
	}
	creaturesToAdd.clear();
}

void Dungeon::killCreaturesAtRange(int radius) {
	float px=gameEngine->player->x;
	float py=gameEngine->player->y;
	int rad2=radius*radius;
	for ( Creature **it=creatures.begin(); it != creatures.end(); it++ ) {
		float dx=(*it)->x-px;
		float dy=(*it)->y-py;
		if ( dx*dx+dy*dy <= rad2 ) (*it)->setLife(0.0f);
	}
}


void Dungeon::renderCorpses(LightMap *lightMap) {
	for ( Creature **it=corpses.begin(); it != corpses.end(); it++ ) {
		if ( map->isInFov((int)(*it)->x,(int)(*it)->y) ) (*it)->render(lightMap);
	}
}

bool Dungeon::hasItem(int x,int y) const {
	if (!IN_RECTANGLE(x,y,width,height)) return false;
	return getCell(x,y)->items.size() > 0;
}

bool Dungeon::hasActivableItem(int x, int y) const {
	if (!IN_RECTANGLE(x,y,width,height)) return false;
	Cell *cell=getCell(x,y);
	if (cell->items.size() != 1) return false;
	Item *it=cell->items.peek();
	return it->isActivatedOnBump();
}

bool Dungeon::hasItemType(int x, int y, const char *typeName) {
	return getItem(x,y,typeName) != NULL;
}

bool Dungeon::hasItemType(int x, int y, const ItemType *type) {
	return getItem(x,y,type) != NULL;
}

Item *Dungeon::getItem(int x, int y, const ItemType *type) {
	if (!IN_RECTANGLE(x,y,width,height)) return nullptr;
	Cell *cell=getCell(x,y);
	for (Item **it=cell->items.begin(); it != cell->items.end(); it++) {
		if ((*it)->isA(type)) return *it;
	}
	return NULL;
}

Item *Dungeon::getItem(int x, int y, const char *typeName) {
	if (!IN_RECTANGLE(x,y,width,height)) return nullptr;
	return getItem(x,y,ItemType::getType(typeName));
}

bool Dungeon::hasItemFlag(int x, int y, int flag) {
	if (!IN_RECTANGLE(x,y,width,height)) return false;
	Cell *cell=getCell(x,y);
	for (Item **it=cell->items.begin(); it != cell->items.end(); it++) {
		if (((*it)->typeData->flags & flag) != 0) return true;
	}
	return false;
}

TCODList <Item *> *Dungeon::getItems(int x, int y) const {
	if (!IN_RECTANGLE(x,y,width,height)) return NULL;
	return &cells[x+y*width].items;
}

Item *Dungeon::getFirstItem(int x, int y) const {
	if (!IN_RECTANGLE(x,y,width,height)) return NULL;
	if ( cells[x+y*width].items.size()==0) return NULL;
	return cells[x+y*width].items.get(0);
}

void Dungeon::addItem(Item *it) {
	if ( it == NULL ) return;
	if ( isUpdatingItems ) itemsToAdd.push(it);
	else {
		Item *newItem = it;
		if (! it->isVolatile()) newItem = it->addToList(&getCell(it->x,it->y)->items);
		if ( newItem == it ) {
			items.push(it);
			if ( it->light ) {
				it->light->setPos(it->x*2,it->y*2);
				addLight(it->light);
			}
			if (! it->isVolatile()) {
				bool walk=isCellWalkable((int)it->x,(int)newItem->y);
				bool transp=isCellTransparent((int)it->x,(int)newItem->y);
				walk = walk && it->isWalkable();
				transp = transp && it->isTransparent();
				setProperties((int)it->x,(int)it->y,transp,walk);
			}
		}
	}
}

void Dungeon::computeWalkTransp(int x, int y) {
	if (!IN_RECTANGLE(x,y,width,height)) return;
	Cell *cell=getCell(x,y);
	bool walk=true;
	bool transp=true;
	for (Item **it=cell->items.begin(); it != cell->items.end(); it++) {
		walk = walk && (*it)->isWalkable();
		transp = transp && (*it)->isTransparent();
	}
	setProperties(x,y,transp,walk);
}

void Dungeon::setProperties(int x, int y, bool transparent, bool walkable) {
	map->setProperties(x,y,transparent,walkable);
	map2x->setProperties(x*2,y*2,transparent,walkable);
	map2x->setProperties(x*2+1,y*2,transparent,walkable);
	map2x->setProperties(x*2,y*2+1,transparent,walkable);
	map2x->setProperties(x*2+1,y*2+1,transparent,walkable);
}

void Dungeon::setWalkable(int x, int y, bool walkable) {
	bool transp=map->isTransparent(x,y);
	map->setProperties(x,y,transp,walkable);
	map2x->setProperties(x*2,y*2,transp,walkable);
	map2x->setProperties(x*2+1,y*2,transp,walkable);
	map2x->setProperties(x*2,y*2+1,transp,walkable);
	map2x->setProperties(x*2+1,y*2+1,transp,walkable);
}

Item * Dungeon::removeItem(Item *it, int count, bool del) {
	Item *newItem = it;
	if (! it->isVolatile() ) {
		if (it->owner) {
			newItem = it->owner->removeFromInventory(it,count);
		} else if ( it->container ) {
			newItem = it->removeFromContainer(count);
		} else {
			newItem = it->removeFromList(&getCell(it->x,it->y)->items,count);
		}
	}
	if ( newItem == it ) {
		if ( it->light ) removeLight(it->light);
		if ( del ) it->toDelete=count;
		if (! it->isVolatile()) computeWalkTransp((int)it->x,(int)it->y);
	}
	return newItem;
}

void Dungeon::saveShadowBeforeTree() {
	for (int i=0;i < width*height*4; i++) {
		subcells[i].shadowBeforeTree = subcells[i].shadow;
	}
	smapBeforeTree->copy(smap);
}

void Dungeon::restoreShadowBeforeTree() {
	for (int i=0;i < width*height*4; i++) {
		subcells[i].shadow = subcells[i].shadowBeforeTree;
	}
	smap->copy(smapBeforeTree);
}

void Dungeon::renderItems(LightMap *lightMap, bool subCellPhase) {
	Player *player=gameEngine->player;
	float aspectRatio=gameEngine->aspectRatio;
	// static items in fov
	for (int tx = (int)(-player->fovRange); tx <= (int)(player->fovRange); tx++) {
		int x=(int)(player->x+tx);
		if ( (unsigned)x < (unsigned)width ) {
			int dy=(int)(sqrtf(player->fovRange*player->fovRange - tx*tx)*aspectRatio);
			for (int ty=-dy; ty <= dy; ty++) {
				int y=(int)(player->y+ty);
				if ( (unsigned)y < (unsigned)height && map->isInFov(x,y) && ! terrainTypes[getTerrainType(x,y)].swimmable) {
					Item *it=getFirstItem(x,y);
					if ( it && it->speed == 0.0f ) {
						it->render(lightMap);
					}
				}
			}
		}
	}
	// moving items
	for ( Item **it = items.begin(); it != items.end(); it++) {
		if ( (*it)->speed > 0.0f && map->isInFov((int)(*it)->x,(int)(*it)->y)) {
			(*it)->render(lightMap,subCellPhase);
		}
	}
}

void Dungeon::updateItems(float elapsed, TCOD_key_t k,TCOD_mouse_t *mouse) {
	TCODList<Item *>toDelete;
	isUpdatingItems=true;
	for ( Item **it = items.begin(); it != items.end(); it++) {
		if ( (*it)->toDelete ) {
			toDelete.push(*it);
		} else if (!(*it)->update(elapsed,k,mouse)) {
			toDelete.push(*it);
			(*it)->toDelete=1;
		}
	}
	isUpdatingItems=false;
	for ( Item **it = toDelete.begin(); it != toDelete.end(); it++) {
		removeItem(*it, (*it)->count); // from item map
		items.removeFast(*it); // from item list
		if ( (*it)->typeData->isA("tree") ) {
			gameEngine->recomputeCanopy(*it);
		}
		delete (*it);
	}
	for ( Item **it = itemsToAdd.begin(); it != itemsToAdd.end(); it++) {
		addItem(*it);
	}
	itemsToAdd.clear();
}

bool Dungeon::hasLos(int xFrom, int yFrom, int xTo,int yTo, bool ignoreCreatures) const {
	TCODLine::init(xFrom,yFrom,xTo,yTo);
	while (! TCODLine::step(&xFrom,&xTo) ) {
		if ( ! map->isTransparent(xFrom,yFrom) ) return false;
		if ( !ignoreCreatures && hasCreature(xFrom,yFrom)) return false;
	}
	return true;
}

void Dungeon::computeFov(int x, int y) {
	// compute fov on 2x map, then copy info to 1x map
	map2x->computeFov(2*x,2*y,CON_W,true,FOV_RESTRICTIVE);
	// dungeon rectangle corresponding to console
	int minx=gameEngine->xOffset;
	int miny=gameEngine->yOffset;
	int maxx=gameEngine->xOffset+CON_W-1;
	int maxy=gameEngine->yOffset+CON_H-1;
	// clamp if to the dungeon map
	minx=std::max(0,minx);
	miny=std::max(0,miny);
	maxx=std::min(width-1,maxx);
	maxy=std::min(height-1,maxy);
	for (int cx=minx; cx <= maxx; cx++) {
		for (int cy=miny; cy <= maxy; cy++) {
			map->setInFov(cx,cy,map2x->isInFov(cx*2,cy*2)
				|| map2x->isInFov(cx*2+1,cy*2)
				|| map2x->isInFov(cx*2,cy*2+1)
				|| map2x->isInFov(cx*2+1,cy*2+1)
				);
		}
	}
}


void Dungeon::applyShadowMap() {
	for (int x=0; x < width*2; x++ ) {
		for (int y=0; y < height*2; y++ ) {
			TCODColor col=getGroundColor(x,y);
			col = col * getShadow(x,y);
			setGroundColor(x,y,col);
		}
	}
}

#define SQR(x) ((x)*(x))
void Dungeon::computeOutdoorLight(float lightDir[3], TCODColor lightColor) {
	float min=1.0f,max=0.0f;
	// normalize light vector
	float len=SQR(lightDir[0])+SQR(lightDir[1])+SQR(lightDir[2]);
	len = 1.0f/sqrt(len);
	lightDir[0] *= len;
	lightDir[1] *= len;
	lightDir[2] *= len;
	// compute light coef min/max
	for (int x=0; x < width*2; x++ ) {
		for (int y=0; y < height*2; y++ ) {
			float n[3];
			hmap->getNormal(x,y,n);
			float lightCoef = (n[0]*lightDir[0]+n[1]*lightDir[1]+n[2]*lightDir[2]+1.0f)*0.5f;
			if ( lightCoef > max ) max=lightCoef;
			else if (lightCoef < min)  min=lightCoef;
		}
	}
	float normcoef=1.0f/(max-min);
	// apply normalized light coef to color
	for (int x=0; x < width*2; x++ ) {
		for (int y=0; y < height*2; y++ ) {
			TCODColor col=getGroundColor(x,y);
			float n[3];
			hmap->getNormal(x,y,n);
			float lightCoef = (n[0]*lightDir[0]+n[1]*lightDir[1]+n[2]*lightDir[2]+1.0f)*0.5f;
			lightCoef = (lightCoef-min) * normcoef;
//			if ( lightCoef < 0.5f ) lightCoef *= 0.8f;
//			else lightCoef *= 1.2f;
			col = col * (lightColor*lightCoef);
			setGroundColor(x,y,col);
		}
	}
}

#define DUNG_CHUNK_VERSION 4
void Dungeon::saveData(uint32_t chunkId, TCODZip *zip) {
	saveGame.saveChunk(DUNG_CHUNK_ID,DUNG_CHUNK_VERSION);
	// save the map
	zip->putInt(width);
	zip->putInt(height);
	for (int i=0; i < width*height; i++) {
		cells[i].saveData(zip);
	}
	for (int i=0; i < width*height*4; i++) {
		subcells[i].saveData(zip);
	}
	zip->putData(hmap->w*hmap->h*sizeof(float),hmap->values);
	zip->putData(smap->w*smap->h*sizeof(float),smap->values);
	zip->putData(smapBeforeTree->w*smapBeforeTree->h*sizeof(float),smapBeforeTree->values);
	zip->putImage(canopy);

	// save the creatures
	int nbCreaturesToSave=0;
	for (Creature **it=creatures.begin(); it != creatures.end(); it++ ) {
		if ( (*it)->mustSave() ) nbCreaturesToSave++;
	}
	zip->putInt(nbCreaturesToSave);
	for (Creature **it=creatures.begin(); it != creatures.end(); it++ ) {
		if ( (*it)->mustSave() ) {
			zip->putInt((*it)->type);
			(*it)->saveData(CREA_CHUNK_ID,zip);
		}
	}
	// save the corpses
	zip->putInt(corpses.size());
	for (Creature **it=corpses.begin(); it != corpses.end(); it++ ) {
		zip->putInt((*it)->type);
		(*it)->saveData(CREA_CHUNK_ID,zip);
	}
	// save the items on ground
	int nbItemsToSave=0;
	for (int i=0; i < width*height; i++) {
		nbItemsToSave += cells[i].items.size();
	}
	zip->putInt(nbItemsToSave);
	for (int i=0; i < width*height; i++) {
		for (Item **it = cells[i].items.begin(); it != cells[i].items.end(); it++) {
			zip->putString((*it)->typeData->name);
			(*it)->saveData(ITEM_CHUNK_ID,zip);
		}
	}
}

bool Dungeon::loadData(uint32_t chunkId, uint32_t chunkVersion, TCODZip *zip) {
	if ( chunkVersion != DUNG_CHUNK_VERSION ) return false;
	// load the map
	width = zip->getInt();
	height = zip->getInt();
	gameEngine->displayProgress(0.4f);
	for (int i=0; i < width*height; i++) {
		cells[i].loadData(zip);
	}
	gameEngine->displayProgress(0.5f);
	for (int i=0; i < width*height*4; i++) {
		subcells[i].loadData(zip);
	}
	gameEngine->displayProgress(0.6f);
	zip->getData(hmap->w*hmap->h*sizeof(float),hmap->values);
	zip->getData(smap->w*smap->h*sizeof(float),smap->values);
	zip->getData(smapBeforeTree->w*smapBeforeTree->h*sizeof(float),smapBeforeTree->values);
	canopy = zip->getImage();
	gameEngine->displayProgress(0.7f);

	// load the creatures
	int nbCreatures=zip->getInt();
	while ( nbCreatures > 0 ) {
		CreatureTypeId creatureId=(CreatureTypeId)zip->getInt();
		if ( creatureId < 0 || creatureId >= NB_CREATURE_TYPES ) {
			return false;
		}
		uint32_t creaChunkId ,creaChunkVersion;
		saveGame.loadChunk(&creaChunkId,&creaChunkVersion);
		Creature *crea=Creature::getCreature(creatureId);
		if (!crea->loadData(creaChunkId, creaChunkVersion, zip)) return false;
		addCreature(crea);
		nbCreatures--;
	}

	// load the corpses
	int nbCorpses=zip->getInt();
	while ( nbCorpses > 0 ) {
		CreatureTypeId creatureId=(CreatureTypeId)zip->getInt();
		if ( creatureId < 0 || creatureId >= NB_CREATURE_TYPES ) {
			return false;
		}
		uint32_t creaChunkId ,creaChunkVersion;
		saveGame.loadChunk(&creaChunkId,&creaChunkVersion);
		Creature *crea=Creature::getCreature(creatureId);
		if (!crea->loadData(creaChunkId, creaChunkVersion, zip)) return false;
		addCorpse(crea);
		nbCorpses--;
	}
	gameEngine->displayProgress(0.8f);

	// load the items
	int nbItems = zip->getInt();
	while (nbItems > 0 ) {
		const char *itemTypeName=zip->getString();
		ItemType *itemType=ItemType::getType(itemTypeName);
		if (!itemType) return false;
		uint32_t itemChunkId ,itemChunkVersion;
		saveGame.loadChunk(&itemChunkId, &itemChunkVersion);
		Item *it=Item::getItem(itemType, 0,0);
		if (!it->loadData(itemChunkId, itemChunkVersion, zip)) return false;
		addItem(it);
		nbItems--;
	}
	gameEngine->displayProgress(0.9f);

	return true;
}
