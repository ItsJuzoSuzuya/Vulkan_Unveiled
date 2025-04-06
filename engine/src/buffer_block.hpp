#include <cstddef>
#include <cstdint>
#include <ostream>
namespace engine {
struct BufferBlock {
  uint32_t drawCallIndex;
  uint32_t vertexOffset;
  size_t vertexCount;
  uint32_t firstIndex;
  size_t indexCount;
  uint32_t vertexBufferOffset;
  uint32_t indexBufferOffset;
};

inline std::ostream &operator<<(std::ostream &os,
                                const BufferBlock &bufferBlock) {
  os << "BufferBlock { "
     << "drawCallIndex: " << bufferBlock.drawCallIndex
     << ", vertexOffset: " << bufferBlock.vertexOffset
     << ", vertexCount: " << bufferBlock.vertexCount
     << ", indexOffset: " << bufferBlock.firstIndex
     << ", indexCount: " << bufferBlock.indexCount << " }";
  return os;
}
} // namespace engine
