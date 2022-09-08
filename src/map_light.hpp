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

class Light : public Entity, public NoisyThing {
public :
	enum LightFlags {
		RANDOM_RAD = 1,
		EXTENDED = 2, // this is an extended light
		INVSQRT = 4,
	};
	enum LightDrawMode {
		MODE_ADD,  // ground color += light
		MODE_MUL,  // ground color *= light
		MODE_MAX,  // ground color = max(ground color, light)
	};
	Light() : flags(0),drawMode(MODE_ADD),range(0.0f),color(TCODColor::white) {}
	Light(float range, LightDrawMode mode=MODE_ADD, TCODColor color=TCODColor::white, int flags=0 ) : flags(flags),
		drawMode(mode), range(range), color(color) {}
	void addToLightMap(LightMap *map, bool withFov=true);
	void addToImage(TCODImage *img);
	void getDungeonPart(int *minx,int *miny,int *maxx,int *maxy);
	void add(LightMap *l,TCODImage *i, bool withFov=true);
	virtual void update(float elapsed) {}
	virtual Light *clone() const;
	int flags;
	LightDrawMode drawMode;
	float range;
	HDRColor color;
protected :
	virtual float getIntensity() { return 1.0f; }
	virtual HDRColor getColor(float rad) { return color; }
	float getFog(int x, int y);

};

class ExtendedLight : public Light {
public :
	void setup(HDRColor outColor,float intensityPatternDelay,const char *intensityPattern,const char *colorPattern);
	void update(float elapsed);
	Light *clone() const;
protected :
	HDRColor outColor;
	const char *intensityPattern;
	const char *colorPattern;
	float intensityPatternDelay;
	int intensityPatternLen;
	int colorPatternLen;
	float intensityTimer;
	bool noiseIntensity;

	float getIntensity();
	HDRColor getColor(float rad);
};
