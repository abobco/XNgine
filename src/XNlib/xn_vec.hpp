#pragma once
#include <iostream>
#include <stdio.h>
#include <cmath>

#define SMEPSILON 1E-16f

#define DUMP(a) { std::cout << #a " = " << (a) << std::endl; } 

namespace xn {
    template <class numtype>
    numtype clampt(numtype n, numtype minval, numtype maxval) {
        return std::max( minval, std::min( n, maxval));
    }

    struct vec3 {
        float x;
        float y;
        float z;

        static inline  float dist_sqr(const vec3& l, const vec3& r) {
            float x = l.x - r.x;
            float y = l.y - r.y;
            float z = l.z - r.z;
            // printf("d=(%f, %f, %f)\n", x,y,z);
            return x*x + y*y + z*z;
        } 

        static inline float dist(const vec3& l, const vec3& r) {
            return sqrt(dist_sqr(l,r));
        }

        static inline float dot(const vec3& l, const vec3& r) {
            return l.x*r.x + l.y*r.y + l.z*r.z;
        }

        static inline vec3 cross(const vec3& l, const vec3& r) {
            return { 
                l.y*r.z - l.z*r.y,
                l.z*r.x - l.x*r.z,
                l.x*r.y - l.y*r.x
            };
        }

        static inline vec3 rotate_axis(vec3 point, vec3 axis, float angle) {
            float cos_ang = cos(angle);
            float sin_ang = sin(angle);

            return point * cos_ang + cross(axis, point)*sin_ang + axis*dot(axis, point)*(1-cos_ang);
        }

        inline float mag() { return dist(*this, {0, 0, 0}); }

        inline float mag_sqr() { return dist_sqr(*this, {0,0,0}); }

        inline void normalize() {  
            *this = *this/mag(); 
        }

        inline vec3 abs() {
            return {fabsf(x), fabsf(y), fabsf(z)};
        }

        inline vec3 operator+(const vec3& r) {
            return { this->x + r.x, this->y + r.y, this->z + r.z };
        }

        inline vec3 operator-(const vec3& r) {
            return { this->x - r.x, this->y - r.y, this->z - r.z };
        }

        inline vec3 operator*(const vec3& r) {
            return { this->x * r.x, this->y * r.y, this->z * r.z };
        }
        
        inline vec3 operator*(const float& r) {
            return { this->x * r, this->y * r, this->z * r };
        }

        inline vec3 operator/(vec3& r) {
            if ( fabs(r.x) < SMEPSILON || fabs(r.y) < SMEPSILON || fabs(r.z) < SMEPSILON ) {
                printf("attempted divide by zero!\n");
                std::cout << toString() << " / " << r.toString() << '\n';
                return *this;
            }

            return { this->x / r.x, this->y / r.y, this->z / r.z };
        }
        
        inline vec3 operator/(const float& r) {
            // printf("%f\n", fabs(r));
            if ( fabs(r) < SMEPSILON ) {
                printf("attempted divide by zero!\n");
                std::cout << toString() << " / " << r << '\n';
                // return *this;
            }

            float inv = 1/r;
            return { this->x * inv, this->y * inv, this->z * inv };
        }

        void print() { printf("(%f, %f, %f)\n", x, y, z); }

        std::string toString() {
            char buf[256];
            int n = sprintf(buf, "(%f, %f, %f)", x, y, z);
            return std::string(buf);
        }

#ifdef PIO_VIRTUAL
        glm::vec3 toGLM() {
            return glm::vec3(x,y,z);
        }
#endif
    };
    std::ostream &operator<<(std::ostream &os, const vec3& r) { 
        return os << "(" << r.x << ", " << r.y << ", " << r.z << ")";
    }

    struct vec2 {
        float x;
        float y;

        static inline  float dist_sqr(const vec2& l, const vec2& r) {
            float x = l.x - r.x;
            float y = l.y - r.y;
            // printf("d=(%f, %f, %f)\n", x,y,z);
            return x*x + y*y;
        } 

        static inline float dist(const vec2& l, const vec2& r) {
            return sqrt(dist_sqr(l,r));
        }

        static inline float dot(const vec2& l, const vec2& r) {
            return l.x*r.x + l.y*r.y;
        }

        inline float mag() { return dist(*this, {0, 0}); }

        inline float mag_sqr() { return dist_sqr(*this, {0,0}); }

        inline void normalize() {  
            *this = *this/mag(); 
        }

        inline vec2 abs() {
            return {fabs(x), fabs(y)};
        }

        inline vec2 operator+(const vec2& r) {
            return { this->x + r.x, this->y + r.y };
        }

        inline vec2 operator-(const vec2& r) {
            return { this->x - r.x, this->y - r.y };
        }

        inline vec2 operator*(const vec2& r) {
            return { this->x * r.x, this->y * r.y };
        }
        
        inline vec2 operator*(const float& r) {
            return { this->x * r, this->y * r };
        }

        inline vec2 operator/(vec2& r) {
            if ( fabs(r.x) < SMEPSILON || fabs(r.y) < SMEPSILON  ) {
                printf("attempted divide by zero!\n");
                std::cout << toString() << " / " << r.toString() << '\n';
                return *this;
            }

            return { this->x / r.x, this->y / r.y };
        }
        
        inline vec2 operator/(const float& r) {
            // printf("%f\n", fabs(r));
            if ( fabs(r) < SMEPSILON ) {
                printf("attempted divide by zero!\n");
                std::cout << toString() << " / " << r << '\n';
                // return *this;
            }

            float inv = 1/r;
            return { this->x * inv, this->y * inv };
        }

        void print() { printf("(%f, %f)\n", x, y); }

        std::string toString() {
            char buf[256];
            int n = sprintf(buf, "(%f, %f)", x, y);
            return std::string(buf);
        }
    };

    std::ostream &operator<<(std::ostream &os, const vec2& r) { 
        return os << "(" << r.x << ", " << r.y << ")";
    }
}