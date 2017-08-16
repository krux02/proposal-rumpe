#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <cstdint>


std::vector<uint16_t> indices;
std::vector<glm::vec4> positions;
std::vector<glm::vec4> normals;

enum BufferNames {
  Buffer_Indices,
  Buffer_Positions,
  Buffer_Normals,
  Buffer_Count
};

void initializeBuffers() {
  uint32_t buffers[Buffer_Count]; // handles for buffers
  glCreateBuffers(Buffer_Count, buffers);
  glNamedBufferData(buffers[Buffer_Indices],   indices.size() * sizeof(uint16_t), indices.data(), 0);
  glNamedBufferData(buffers[Buffer_Positions], positions.size() * sizeof(glm::vec4), positions.data(), 0);
  glNamedBufferData(buffers[Buffer_Normals],   normals.size() * sizeof(glm::vec4), normals.data(), 0);
}


GLenum myenum;

template <typename T>
struct Buffer {
  uint32_t handle;

  Buffer(T* data, ssize_t N) {
    glCreateBuffers(1, &handle);
    if( N > 0 ) {
      glNamedBufferData(handle, N * sizeof(T), data, 0);
    }
  }

  Buffer(const std::vector<T>& arg) : Buffer(arg.data(), arg.size()) {}

  Buffer(const std::initializer_list<T>& arg) : Buffer(arg.data(), arg.size()) {}

};
