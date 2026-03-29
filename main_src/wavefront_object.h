#pragma once

#include <vector>
#include <map>
#include <windows.h>
#include <GL/glcorearb.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <glm/glm.hpp>

struct Vertex {

	glm::vec3 position;
	glm::vec2 texture;

	Vertex(GLfloat x, GLfloat y, GLfloat z, GLfloat u, GLfloat v) {

		position = glm::vec3(x, y, z);
		texture = glm::vec2(u, v);
		return;

	}

};

struct Couple {

	int x;
	int y;

	bool operator==(const Couple& x) {

		return (this->x == x.x) && (this->y == x.y);

	}

};

class Wavefront_Object {

	public:

		Wavefront_Object();
		~Wavefront_Object();

		std::vector<GLint> index_list;
		std::vector<Vertex> vertex_list;

	private:

};