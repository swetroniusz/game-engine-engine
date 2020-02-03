#pragma once
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Shader
{
	unsigned int Program;
	std::vector <std::pair<unsigned int, std::string>> MaterialTextureUnits;

	void DebugShader(unsigned int);
public:
	Shader();
	Shader(const char*, const char*, const char* = nullptr);
	std::vector<std::pair<unsigned int, std::string>>* GetMaterialTextureUnits();
	void SetTextureUnitNames(std::vector<std::pair<unsigned int, std::string>>);
	unsigned int LoadShader(GLenum, const char*);
	void LoadShaders(const char*, const char*, const char* = nullptr);
	void Uniform1i(std::string, int);
	void Uniform1f(std::string, float);
	void Uniform2fv(std::string, glm::vec2);
	void Uniform3fv(std::string, glm::vec3);
	void Uniform4fv(std::string, glm::vec4);
	void UniformMatrix3fv(std::string, glm::mat3);
	void UniformMatrix4fv(std::string, glm::mat4);
	void UniformBlockBinding(std::string, unsigned int);
	void UniformBlockBinding(unsigned int, unsigned int);
	unsigned int GetUniformBlockIndex(std::string);
	void Use();
};

unsigned int textureFromFile(std::string, bool);