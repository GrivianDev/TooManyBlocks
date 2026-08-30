#ifndef TOOMANYBLOCKS_COMPATABILITY_H
#define TOOMANYBLOCKS_COMPATABILITY_H

#include <GL/glew.h>

#include <climits>

// Open GL type compatability
static_assert(sizeof(float) == sizeof(GLfloat), "Default typesize mismatch for GLfloat.");
static_assert(sizeof(double) == sizeof(GLdouble), "Default typesize mismatch for GLdouble.");
static_assert(sizeof(int) == sizeof(GLint), "Default typesize mismatch for GLint.");
static_assert(sizeof(unsigned int) == sizeof(GLuint), "Default typesize mismatch for GLuint.");
static_assert(sizeof(short) == sizeof(GLshort), "Default typesize mismatch for GLshort.");
static_assert(sizeof(unsigned short) == sizeof(GLushort), "Default typesize mismatch for GLushort.");
static_assert(sizeof(char) == sizeof(GLbyte), "Default typesize mismatch for GLbyte.");
static_assert(sizeof(unsigned char) == sizeof(GLubyte), "Default typesize mismatch for GLubyte.");

// Gurantee that char is indeed 8 bit wide
static_assert(CHAR_BIT == 8, "Character type is not 8 bit wide.");

// Ensure default type sizes
static_assert(sizeof(char) == 1, "Char is not 1 byte.");
static_assert(sizeof(short) == 2, "Short is not 2 bytes.");
static_assert(sizeof(int) == 4, "Int is not 4 bytes.");
static_assert(sizeof(unsigned char) == 1, "Unsigned char is not 1 byte.");
static_assert(sizeof(unsigned short) == 2, "Unsigned short is not 2 bytes.");
static_assert(sizeof(unsigned int) == 4, "Unsigned int is not 4 bytes.");
static_assert(sizeof(unsigned long) == sizeof(long), "Unsigned long does not match long.");
static_assert(sizeof(unsigned long long) == sizeof(long long), "Unsigned long long does not match long long.");

#endif