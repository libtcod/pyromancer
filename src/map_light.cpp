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
#include <math.h>
#include "main.hpp"

void Light::addToLightMap(LightMap *lightmap, bool withFov) {
	add(lightmap,NULL,withFov);
}
void Light::addToImage(TCODImage *img) {
	add(NULL,img);
}

void Light::add(LightMap *l, TCODImage *img, bool withFov) {
	static HDRColor hdrWhite(255,255,255);
	if ( this->range == 0.0f ) return;
	// get light range in dungeon coordinates
	int minx=(int)(this->x-this->range);
	int maxx=(int)(this->x+this->range);
	int miny=(int)(this->y-this->range);
	int maxy=(int)(this->y+this->range);
	int xOffset=gameEngine->xOffset*2;
	int yOffset=gameEngine->yOffset*2;
	// clamp it to the dungeon
	minx=std::max(0,minx);
	miny=std::max(0,miny);
	maxx=std::min(gameEngine->dungeon->width*2-1,maxx);
	maxy=std::min(gameEngine->dungeon->height*2-1,maxy);
	// convert it to lightmap (console x2) coordinates
	minx-=xOffset;
	maxx-=xOffset;
	miny-=yOffset;
	maxy-=yOffset;
	// clamp it to the lightmap
	minx=std::max(0,minx);
	miny=std::max(0,miny);
	if ( l ) {
		maxx=std::min(l->width-1,maxx);
		maxy=std::min(l->height-1,maxy);
	} else {
		int iw,ih;
		img->getSize(&iw,&ih);
		maxx=std::min(iw-1,maxx);
		maxy=std::min(ih-1,maxy);
	}

	int fovmap_width=maxx-minx;
	int fovmap_height=maxy-miny;

	// watch out ! 3 different coordinates referentials here
	// dungeon (xOffset, yOffset, this->x, this->y)
	// lightmap (minx,maxx,miny,maxy)
	// fovmap : subpart of lightmap to compute light fov

	if ( fovmap_width <= 0 || fovmap_height <= 0 ) return;
	// create a small map for the light fov
	TCODMap fovmap(fovmap_width,fovmap_height);
	// copy dungeon info into it
	for (int cx=0; cx < fovmap_width; cx++ ) {
		for (int cy=0; cy < fovmap_height; cy++ ) {
			int dungeon2x=cx+minx+xOffset;
			int dungeon2y=cy+miny+yOffset;
			bool canpass=gameEngine->dungeon->map2x->isTransparent(dungeon2x,dungeon2y);
			fovmap.setProperties(cx,cy,canpass,canpass);
		}
	}
	// calculate light fov
	// the fov algo must support viewer out of the map !
	fovmap.computeFov((int)(this->x-xOffset-minx),(int)(this->y-yOffset-miny),(int)(range),
			true,FOV_BASIC);

	float squaredRange=range*range;
	TCODMap *map2x=gameEngine->dungeon->map2x;
	// get fov data and add light to lightmap
	for (int cx=0; cx < fovmap_width; cx++ ) {
		for (int cy=0; cy < fovmap_height; cy++ ) {
			if (fovmap.isInFov(cx,cy)) {
				int dungeon2x=cx+minx+xOffset;
				int dungeon2y=cy+miny+yOffset;
				if ( !withFov || map2x->isInFov(dungeon2x,dungeon2y)) {
					int dx=(int)(dungeon2x-this->x);
					int dy=(int)(dungeon2y-this->y);
					float crange=dx*dx+dy*dy;
					float coef;
					float rad=0.0f;
					if ( flags & RANDOM_RAD ) {
						float angle=atan2f(dy,dx);
						float f=angle+noiseOffset;
						float squaredRangeRnd=squaredRange*(0.5f*(1.0f+noise1d.get(&f)));
						// fix radius continuity near -PI
						float rcoef=0.0f;
						if ( angle < -7*M_PI/8) rcoef = (-7*M_PI/8-angle)/(M_PI/8);
						if ( rcoef > 1E-6f ) {
							float fpi=M_PI+noiseOffset;
							float squaredRangePi=squaredRange*(0.5f*(1.0f+noise1d.get(&fpi)));
							squaredRangeRnd=squaredRangeRnd + rcoef*(squaredRangePi-squaredRangeRnd);
						}
						if ( flags & INVSQRT ) {
							coef = 1.0f / (1.0f+40*crange * squaredRangeRnd/(squaredRange*range));
							rad=1.0f-coef;
						} else {
							rad=crange/squaredRangeRnd;
							rad=std::min(1.0f,rad);
							coef=1.0f - rad;
						}
					} else {
						if ( flags & INVSQRT ) {
							coef = 1.0f / (1.0f+40*crange/range);
							rad = 1.0f-coef;
						} else {
							rad=crange/squaredRange;
							rad=std::min(1.0f,rad);
							coef=1.0f - rad;
						}
					}
					if ( coef > 0.0f ) {
						HDRColor col=getColor(rad);

						float intensity=getIntensity();
						coef*=intensity;
						if (l) {
							HDRColor prevCol=l->getHdrColor2x(cx+minx,cy+miny);
							switch(drawMode) {
							case MODE_ADD:
								col = col * coef;
								prevCol = prevCol+col;
							break;
							case MODE_MUL:
								col = HDRColor::lerp(hdrWhite, col,coef);
								prevCol = prevCol*col;
							break;
							case MODE_MAX:
								col = col * coef;
								prevCol.r = std::max(prevCol.r,col.r);
								prevCol.g = std::max(prevCol.g,col.g);
								prevCol.b = std::max(prevCol.b,col.b);
							break;
							}
							l->setColor2x(cx+minx,cy+miny,prevCol);
						} else {
							HDRColor prevCol=img->getPixel(cx+minx,cy+miny);
							col = col * coef;
							switch(drawMode) {
							case MODE_ADD:	prevCol = prevCol+col;break;
							case MODE_MUL:	prevCol = prevCol*col;break;
							case MODE_MAX:
								prevCol.r = std::max(prevCol.r,col.r);
								prevCol.g = std::max(prevCol.g,col.g);
								prevCol.b = std::max(prevCol.b,col.b);
							break;
							}
							img->putPixel(cx+minx,cy+miny,prevCol);
						}
					}
				}
			}
		}
	}
}

// part of the dungeon that this light can hit (2x coords)
void Light::getDungeonPart(int *minx,int *miny,int *maxx,int *maxy) {
	*minx = (int)(x-range);
	*maxx = (int)(x+range);
	*miny = (int)(y-range);
	*maxy = (int)(y+range);
}

void ExtendedLight::setup(HDRColor outColor,float intensityPatternDelay,const char *intensityPattern,const char *colorPattern) {
	this->outColor=outColor;
	this->intensityPatternDelay=intensityPatternDelay;
	this->intensityPattern=intensityPattern;
	this->colorPattern=colorPattern;
	flags |= EXTENDED;
	intensityPatternLen=intensityPattern ? strlen(intensityPattern)-1 : 0;
	colorPatternLen=colorPattern ? strlen(colorPattern)-1 : 0;
	noiseIntensity=false;
	intensityTimer=0.0f;
	if ( intensityPattern && strcmp(intensityPattern,"noise") == 0 ) {
		noiseIntensity=true;
	}
}

void ExtendedLight::update(float elapsed) {
	intensityTimer+=elapsed;
	if ( ! noiseIntensity && intensityPatternDelay ) while ( intensityTimer > intensityPatternDelay ) intensityTimer -= intensityPatternDelay ;
}

float ExtendedLight::getIntensity() {
	if (intensityPatternLen == 0 ) return 1.0f;
	if ( noiseIntensity ) {
		float f=noiseOffset+intensityTimer*intensityPatternDelay;
		return 0.5f*(1.0f+noise1d.get(&f));
	}
	float it=intensityTimer*intensityPatternLen/intensityPatternDelay;
	int iit=(int)it;
	int itchar=intensityPattern[ iit ];
	int itchar2=intensityPattern[ (iit+1)%(intensityPatternLen+1) ];
	float it1=(float)(itchar-'0')/9.0f;
	float it2=(float)(itchar2-'0')/9.0f ;
	return it1 + (it2-it1)*(it - iit);
}

HDRColor ExtendedLight::getColor(float rad) {
	if (colorPatternLen == 0 ) return color;
	float col=rad*colorPatternLen;
	int icol=(int)col;
	int colchar=colorPattern[ icol ];
	int colchar2=colorPattern[ (icol+1)%(colorPatternLen+1) ];
	float coef1=(float)(colchar-'0')/9.0f;
	float coef2=(float)(colchar2-'0')/9.0f;
	float coef=coef1+(coef2-coef1)*(col-icol);
	return HDRColor::lerp(color,outColor,coef);
}

Light *Light::clone() const {
	Light *ret=new Light();
	ret->flags=flags;
	ret->drawMode=drawMode;
	ret->range=range;
	ret->color=color;
	return ret;
}
Light *ExtendedLight::clone() const {
	ExtendedLight *ret=new ExtendedLight();
	ret->flags=flags;
	ret->drawMode=drawMode;
	ret->range=range;
	ret->color=color;
	ret->outColor=outColor;
	ret->intensityPattern=intensityPattern;
	ret->colorPattern=colorPattern;
	ret->intensityPatternDelay=intensityPatternDelay;
	ret->intensityPatternLen=intensityPatternLen;
	ret->colorPatternLen=colorPatternLen;
	ret->intensityTimer=intensityTimer;
	ret->noiseIntensity=noiseIntensity;
	return ret;
}
