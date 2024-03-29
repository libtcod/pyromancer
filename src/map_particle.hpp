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

class ParticleType {
public :
	HDRColor startColor;
	HDRColor endColor;
	float minDamage;
	float maxDamage;
	float duration;
	float speed;
	bool bounce;
	bool throughCreatures;
	ParticleType(const HDRColor& startColor,const HDRColor &endColor,float minDamage,
		float maxDamage,float duration, float speed,bool bounce=false, bool throughCreatures=false) :
		startColor(startColor),endColor(endColor),minDamage(minDamage),maxDamage(maxDamage),
		duration(duration),speed(speed),bounce(bounce),throughCreatures(throughCreatures) {}
};

class Particle : public DynamicEntity {
private :
	const ParticleType *type;
public :
	Particle(const ParticleType *type);
	void render(LightMap *lightMap);
	void render(TCODImage *ground);
	bool update(float elapsed);
};
