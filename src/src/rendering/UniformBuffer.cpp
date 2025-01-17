#include <rendering/UniformBuffer.h>
#include <glad/glad.h>

using namespace GEE;

UniformBuffer::UniformBuffer() :
	UBO(0),
	BlockBindingSlot(0),
	offsetCache(0)
{
}

void UniformBuffer::Generate(unsigned int blockBindingSlot, size_t size, const float* data)
{
	Generate(blockBindingSlot, size, data, GL_DYNAMIC_DRAW);
}

void UniformBuffer::Generate(unsigned int blockBindingSlot, size_t size, const float* data, GLenum usage)
{
	Dispose();
	glGenBuffers(1, &UBO);

	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBufferData(GL_UNIFORM_BUFFER, size, data, usage);
	glBindBufferBase(GL_UNIFORM_BUFFER, blockBindingSlot, UBO);

	offsetCache = 0;
	BlockBindingSlot = blockBindingSlot;
}

void UniformBuffer::BindToSlot(unsigned int blockBindingSlot, bool isAlreadyBound)
{
	if (!isAlreadyBound)
		glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBindBufferBase(GL_UNIFORM_BUFFER, blockBindingSlot, UBO);
}

bool UniformBuffer::HasBeenGenerated() const
{
	return UBO != 0;
}

void UniformBuffer::SubData1i(int data, size_t offset)
{
	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBufferSubData(GL_UNIFORM_BUFFER, offset, sizeof(int), &data);
	offsetCache = offset + sizeof(int);
}

void UniformBuffer::SubData1f(float data, size_t offset)
{
	SubData(4, &data, offset);
}

void UniformBuffer::SubData(size_t size, const float* data, size_t offset)
{
	glBindBuffer(GL_UNIFORM_BUFFER, UBO);
	glBufferSubData(GL_UNIFORM_BUFFER, offset, size, data);
	offsetCache = offset + size;
}

void UniformBuffer::SubData4fv(const Vec3f& vec, size_t offset)
{
	Vec4f bufferVector(vec, 0.0f);
	SubData(sizeof(Vec4f), Math::GetDataPtr(bufferVector), offset);
}

void UniformBuffer::SubData4fv(std::vector<Vec3f> vecs, size_t offset)
{
	offsetCache = offset;
	for (const auto& vec : vecs)
		SubData4fv(vec, offsetCache);
}

void UniformBuffer::SubData4fv(const Vec4f& vec, size_t offset)
{
	SubData(sizeof(Vec4f), Math::GetDataPtr(vec), offset);
}

void UniformBuffer::SubData4fv(std::vector<Vec4f> vecs, size_t offset)
{
	offsetCache = offset;
	for (const auto& vec : vecs)
		SubData4fv(vec, offsetCache);
}

void UniformBuffer::SubDataMatrix4fv(const Mat4f& mat, size_t offset)
{
	SubData(64, Math::GetDataPtr(mat), offset);
}

void UniformBuffer::PadOffset()
{
	if (offsetCache % 16 == 0)
		return;
	offsetCache = offsetCache - offsetCache % 16 + 16; //pad to the next multiple of 16
}

void UniformBuffer::Dispose()
{
	if (HasBeenGenerated())
		glDeleteBuffers(1, &UBO);
}