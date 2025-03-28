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

// color that can go beyond 0-255 range
struct HDRColor {
	float r,g,b;
	HDRColor() : r(0.0f),g(0.0f),b(0.0f) {}
	HDRColor(const HDRColor &col) : r(col.r),g(col.g),b(col.b) {}
	HDRColor(const TCODColor &col) : r(col.r),g(col.g),b(col.b) {}
	HDRColor(float r,float g,float b) : r(r),g(g),b(b) {}
	operator TCODColor() { return TCODColor(std::clamp((int)r,0,255),std::clamp((int)g,0,255),std::clamp((int)b,0,255));}
	bool operator == (const HDRColor & c) const {
		return (c.r == r && c.g == g && c.b == b);
	}
	bool operator != (const HDRColor & c) const {
		return (c.r != r || c.g != g || c.b != b);
	}
	HDRColor operator * (const HDRColor & a) const {
		static const float c=1.0f/255.0f;
		return HDRColor(r*a.r*c,g*a.g*c,b*a.b*c);
	}
	HDRColor operator *(float value) const {
		return HDRColor(r*value,g*value,b*value);
	}
	HDRColor operator + (const HDRColor & a) const {
		return HDRColor(r+a.r,g+a.g,b+a.b);
	}
	HDRColor operator - (const HDRColor & a) const {
		return HDRColor(r-a.r,g-a.g,b-a.b);
	}
	static HDRColor lerp(const HDRColor &a, const HDRColor &b, float coef) {
		return HDRColor((1.0f-coef)*a.r+coef*b.r,
						(1.0f-coef)*a.g+coef*b.g,
						(1.0f-coef)*a.b+coef*b.b);
	}
};
HDRColor operator *(float value, const HDRColor &c);

class LightMap {
public :
	LightMap(int width, int height);
	void clear(const TCODColor& col);
	void applyToImage(TCODImage *img,
		int minx2x=0,int miny2x=0,int maxx2x=0,int maxy2x=0, bool playerFog=true);
	void applyToImageOutdoor(TCODImage *img);
	inline TCODColor getColor2x(int x, int y) { return data2x[x+y*width]; }
	inline TCODColor getColor(int x, int y) { return data[x+y*width/2]; }
	inline HDRColor &getHdrColor2x(int x, int y) { return data2x[x+y*width]; }
	inline HDRColor &getHdrColor(int x, int y) { return data[x+y*width/2]; }
	inline void setColor2x(int x, int y, const TCODColor &col) {
		data2x[x+y*width]=col;
		if ( (x&1)==0 && (y&1) == 0 ) data[x/2+y*width/4]=col;
	}
	inline void setColor2x(int x, int y, const HDRColor &col) {
		data2x[x+y*width]=col;
		if ( (x&1)==0 && (y&1) == 0 ) data[x/2+y*width/4]=col;
	}

	inline void addColor2x(int x, int y, const HDRColor &col) {
		data2x[x+y*width]=data2x[x+y*width]+col;
		if ( (x&1)==0 && (y&1) == 0 ) data[x/2+y*width/4]=data2x[x+y*width];
	}

	float getFog(int x,int y);
	float getPlayerFog(int x,int y);
	void update(float elapsed);

	// apply some radiosity magic!
	void diffuse(float reflectivity, float selfIllumination, int steps);

	int width,height;
	float fogRange;

protected :
	HDRColor *data2x;
	HDRColor *data;

	float fogZ;
	TCODNoise *fogNoise;

	// radiosity stuff
	void propagate(float reflectivity, float selfIllumination, HDRColor *outgoing, HDRColor *emission,TCODList<int> & offsets);
	void computeLightContribution(int lx, int ly, const HDRColor &lcol, float reflectivity, float *ff);
	void computeFastLightContribution(int lx, int ly, const HDRColor &lcol, float reflectivity);
	void computeFormFactor(int x, int y, float *ff, float *ffSum) ;
};
