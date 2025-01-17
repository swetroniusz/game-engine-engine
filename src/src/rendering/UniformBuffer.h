#pragma once
#include "math/Vec.h"

typedef unsigned int GLenum;
namespace GEE
{
	struct UniformBuffer
	{
		UniformBuffer();
		void Generate(unsigned int blockBindingSlot, size_t size, const float* = nullptr);
		void Generate(unsigned int blockBindingSlot, size_t size, const float*, GLenum usage);

		void BindToSlot(unsigned int blockBindingSlot, bool isAlreadyBound);

		[[nodiscard]] bool HasBeenGenerated() const;

		void SubData1i(int, size_t offset);
		void SubData1f(float, size_t offset);
		void SubData(size_t size, const float* data, size_t offset);
		void SubData4fv(const Vec3f&, size_t offset);
		void SubData4fv(std::vector<Vec3f>, size_t offset);
		void SubData4fv(const Vec4f&, size_t offset);
		void SubData4fv(std::vector<Vec4f>, size_t offset);
		void SubDataMatrix4fv(const Mat4f&, size_t offset);
		void PadOffset();

		void Dispose();

		unsigned int UBO;
		unsigned int BlockBindingSlot;
		size_t offsetCache;
	};
}
