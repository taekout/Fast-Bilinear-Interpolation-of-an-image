#pragma  once

const size_t BYTES_PER_PIXEL = 3; // 24 bit color depth :D

struct vec3 {
	unsigned char r, g, b;
	vec3(unsigned char _r, unsigned char _g, unsigned char _b) : r(_r), g(_g), b(_b) {}
	vec3() : r(0), g(0), b(0) {}

	void Set(const unsigned char * d) {
		b = d[0];
		g = d[1];
		r = d[2];
	}

	const vec3 operator +(const vec3 & rhs) const {
		return vec3(this->r + rhs.r, this->g + rhs.g, this->b + rhs.b);
	}

	const vec3 operator *(float rhs) const {
		return vec3(this->r * rhs, this->g * rhs, this->b * rhs);
	}
};




