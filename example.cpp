#include <vector>
#include <cstdio>
#include <string>
// #include <glm/glm.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/trigonometric.hpp>
// Include GLM extensions
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/transform.hpp>

#include <GL/glew.h>
#include <cstdint>
#include <SDL2/SDL.h>

using glm::float32_t;
using glm::float64_t;
using glm::vec3;
using glm::vec4;
using glm::mat4;

using std::string;
using std::fopen;
using std::size;
using std::data;

//////////////////////////////
// opengl utility functions //
//////////////////////////////

void debugCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int32_t length, const char* message, const void* userPointer) {
  if(severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
    return;
  }

  puts("gl-debug-callback:");
  printf("  message: %s\n", message);  //"

  switch(source) {
  case GL_DEBUG_SOURCE_API:
    puts("  source: api");
    break;
  case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
    puts("  source: window system");
    break;
  case GL_DEBUG_SOURCE_SHADER_COMPILER:
    puts("  source: shader compiler");
    break;
  case GL_DEBUG_SOURCE_THIRD_PARTY:
    puts("  source: third party");
    break;
  case GL_DEBUG_SOURCE_APPLICATION:
    puts("  source: application");
    break;
  case GL_DEBUG_SOURCE_OTHER:
    puts("  source: other");
    break;
  default:
    printf("  source: %d ???", int(source)); //"
  }

  switch(type) {
  case GL_DEBUG_TYPE_ERROR:
    puts("  type: error");
    break;
  case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
    puts("  type: deprecated behavior");
    break;
  case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
    puts("  type: undefined behavior"); //"
    break;
  case GL_DEBUG_TYPE_PORTABILITY:
    puts("  type: portability");
    break;
  case GL_DEBUG_TYPE_PERFORMANCE:
    puts("  type: performance");
    break;
  case GL_DEBUG_TYPE_MARKER:
    puts("  type: marker");
    break;
  case GL_DEBUG_TYPE_PUSH_GROUP:
    puts("  type: push group");
    break;
  case GL_DEBUG_TYPE_POP_GROUP:
    puts("  type: pop group");
    break;
  case GL_DEBUG_TYPE_OTHER:
    puts("  type: other");
    break;
  default:
    printf("  type:  %d ???", type);
  }

  printf("  id: %d", id); //"
  switch(severity) {
  case GL_DEBUG_SEVERITY_LOW:
    puts("  severity: low");
    break;
  case GL_DEBUG_SEVERITY_MEDIUM:
    puts("  severity: medium");
    break;
  case GL_DEBUG_SEVERITY_HIGH:
    puts("  severity: high");
    break;
  case GL_DEBUG_SEVERITY_NOTIFICATION:
    puts("  severity: notification");
    break;
  default:
    printf("  severity: %d ???", int(severity));
  }
}

uint32_t compileShaderFile(uint32_t shaderType, const char* filename) {
  FILE* file = fopen(filename, "r");
  if(file == nullptr) {
    perror(filename);
    exit(EXIT_FAILURE);
  }
  string buffer;
  int character = fgetc(file);
  while(character != EOF) {
    buffer += char(character);
    character = fgetc(file);
  }
  fclose(file);
  auto result = glCreateShader(shaderType);

  const char* cstring[] = {buffer.c_str()};
  glShaderSource(result, size(cstring), cstring, nullptr);
  glCompileShader(result);


  int32_t status;
  glGetShaderiv(result, GL_COMPILE_STATUS, &status);

  if(status == GL_FALSE) {
    fprintf(stderr, "compilation failed: %s\n", filename);
    int32_t length = -1;
    glGetShaderiv(result, GL_INFO_LOG_LENGTH, &length);
    if( length > 0 ) {
      fprintf(stderr, "%d\n", length);
      char* message = (char*)malloc(length);
      glGetShaderInfoLog(result, length, nullptr, message);
      fprintf(stderr, "%s\n", message);
    } else {
      fprintf(stderr, "no log :'(\n");
    }
    exit(EXIT_FAILURE);
 }

  glObjectLabel(GL_SHADER, result, -1, filename);

  return result;
}

template <typename T>
struct attribSize {
  static const int value = 1;
};

template <>
struct attribSize<vec4> {
  static const int value = 4;
};

template <>
struct attribSize<vec3> {
  static const int value = 3;
};

template <typename T>
struct attribType {
  static const int value = -1;
};

template <>
struct attribType<int8_t> {
  static const int value = GL_BYTE;
};

template <>
struct attribType<float32_t> {
  static const int value = GL_FLOAT;
};

template <>
struct attribType<float64_t> {
  static const int value = GL_DOUBLE;
};

template <>
struct attribType<vec4> {
  static const int value = attribType<vec4::value_type>::value;
};

template <>
struct attribType<vec3> {
  static const int value = attribType<vec3::value_type>::value;
};

template <typename T>
struct attribNormalized {
  static const bool value = false;
};

//////////////////
// program data //
//////////////////

vec4 vertices[] = {
  {-1,-1, 0, 1},
  { 1,-1, 0, 1},
  { 0, 1, 0, 1},
};

vec4 colors[] = {
  {1,0,0,1},
  {0,1,0,1},
  {0,0,1,1},
};

uint16_t indices[] = {
  0,1,2
};

enum struct BufferId {
  Vertices,
  Colors,
  Indices,
  Count,
};

uint32_t buffers[size_t(BufferId::Count)];

enum struct VertexArrayObjectId {
  HelloTriangle,
  Count,
};

int32_t vertexArrayObjects[size_t(VertexArrayObjectId::Count)];

enum struct ProgramId {
  HelloTriangle,
  Count,
};

uint32_t programs[size_t(ProgramId::Count)];
int helloTriangleLocations[4];

void initializeRendering() {
  { // initialize Buffers that are later bound to the attributes
    glCreateBuffers(size(buffers), buffers);

    glNamedBufferData(buffers[size_t(BufferId::Vertices)], size(vertices) * sizeof(*data(vertices)), vertices, GL_STATIC_DRAW);
    glNamedBufferData(buffers[size_t(BufferId::Colors)],   size(colors) * sizeof(*data(colors)), colors, GL_STATIC_DRAW);
    glNamedBufferData(buffers[size_t(BufferId::Indices)],  size(indices) * sizeof(*data(indices)), indices, GL_STATIC_DRAW);

    glObjectLabel(GL_BUFFER, buffers[size_t(BufferId::Vertices)], -1, "vertices");
    glObjectLabel(GL_BUFFER, buffers[size_t(BufferId::Colors)],   -1, "colors");
    glObjectLabel(GL_BUFFER, buffers[size_t(BufferId::Indices)],  -1, "indices");
  }

  { // compile HelloTriangle
    auto program = glCreateProgram();
    auto vertexShader = compileShaderFile(GL_VERTEX_SHADER, "hello_triangle.vert");
    glAttachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    auto fragmentShader = compileShaderFile(GL_FRAGMENT_SHADER, "hello_triangle.frag");
    glAttachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);
    glLinkProgram(program);

    programs[size_t(ProgramId::HelloTriangle)] = program;

    int32_t* locations = helloTriangleLocations;
    locations[0] = glGetUniformLocation(program, "modelView");
    locations[1] = glGetUniformLocation(program, "proj");
    locations[2] = glGetAttribLocation(program, "a_vertex");
    locations[3] = glGetAttribLocation(program, "a_color");

    uint32_t vao;
    glCreateVertexArrays(1, &vao);

    { // setup vertices attribute
      int32_t loc = locations[2];
      glVertexArrayVertexBuffer(vao, loc, buffers[size_t(BufferId::Vertices)], 0, sizeof(*data(vertices)));
      auto attribSizeValue = attribSize<vec4>::value;
      auto attribTypeValue = attribType<vec4>::value;
      auto attribNormalizedValue = attribNormalized<vec4>::value;
      glVertexArrayAttribFormat(vao, loc, attribSizeValue, attribTypeValue, attribNormalizedValue, 0);
      glVertexArrayAttribBinding(vao, loc, 0); // bind attribute a_vertex to binding index 0
      glVertexArrayBindingDivisor(vao,     0, 0); // set binding index 0 to divisor 0 (no instancing)
      glEnableVertexArrayAttrib(vao, 0);
    }
    { // setup colors attribute
      int32_t loc = locations[3];
      glVertexArrayVertexBuffer(vao, loc, buffers[size_t(BufferId::Colors)], 0, sizeof(decltype(*data(colors))));
      auto attribSizeValue = attribSize<vec4>::value;
      auto attribTypeValue = attribType<vec4>::value;
      auto attribNormalizedValue = attribNormalized<vec4>::value;
      glVertexArrayAttribFormat(vao, loc, attribSizeValue, attribTypeValue, attribNormalizedValue, 0);
      glVertexArrayAttribBinding(vao, loc, 1); // bind attribute to binding index 1
      glVertexArrayBindingDivisor(vao,     1, 0);  // set binding index 1 to divisor 0 (no instancing)
      glEnableVertexArrayAttrib(vao, 1);
    }
    vertexArrayObjects[size_t(VertexArrayObjectId::HelloTriangle)] = vao;
  }
}

int main() {

  SDL_Init(SDL_INIT_EVERYTHING);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  int32_t contextFlags = SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG | SDL_GL_CONTEXT_DEBUG_FLAG;
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS        , contextFlags);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK , SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE         , 8);

  SDL_Window* window = SDL_CreateWindow("example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                 -1, -1, SDL_WINDOW_OPENGL |  SDL_WINDOW_FULLSCREEN_DESKTOP);

  if (not window) {
    puts(SDL_GetError());
    exit(EXIT_FAILURE);
  }

  SDL_GLContext context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, context);

  uint32_t err = glewInit();
  if (GLEW_OK != err) {
    /* Problem: glewInit failed, something is seriously wrong. */
    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    exit(EXIT_FAILURE);
  }

  glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB);
  glDebugMessageCallbackARB(debugCallback, nullptr);

  initializeRendering();

  glEnable(GL_DEPTH_TEST); // I wonder why this is by default off.

  SDL_Event evt;
  bool runGame = true;

  uint64_t startTime = SDL_GetPerformanceCounter();
  float64_t frequency = float64_t(SDL_GetPerformanceFrequency());

  mat4 proj;
  {
    int32_t windowWidth, windowHeight;
    SDL_GetWindowSize(window, &windowWidth, &windowHeight);
    float32_t aspect = float32_t(windowWidth) / float32_t(windowHeight);
    proj = glm::perspective(90.0f, aspect, 0.01f, 1000.0f);
  }
  double time;
  while(runGame) {

    while(SDL_PollEvent(&evt)) {
      if(evt.type == SDL_QUIT) {
        runGame = false;
        break;
      }
      if(evt.type == SDL_KEYDOWN and evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE) {
        runGame = false;
        break;
      }
    }

    time = float64_t(SDL_GetPerformanceCounter() - startTime) / frequency;

    mat4 viewMat = mat4(1);
    viewMat = translate(viewMat, vec3(0,1,5));
    viewMat = inverse(viewMat);

    mat4 modelMat = mat4(1.0f);
    modelMat = rotate(modelMat, float32_t(time), vec3(0.0f, 1.0f, 0.0f));
    modelMat = scale(modelMat, vec3(3.0f, 3.0f, 3.0f));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    {
      auto program = programs[size_t(ProgramId::HelloTriangle)];
      auto vao = vertexArrayObjects[size_t(VertexArrayObjectId::HelloTriangle)];
      int32_t* locations = helloTriangleLocations;

      glUseProgram(program);
      glBindVertexArray(vertexArrayObjects[size_t(VertexArrayObjectId::HelloTriangle)]);
      glVertexArrayElementBuffer(vao, buffers[size_t(BufferId::Indices)]);

      auto modelViewMat = viewMat * modelMat;
      glProgramUniformMatrix4fv(program, locations[0], 1, false, value_ptr(modelViewMat));
      glProgramUniformMatrix4fv(program, locations[1], 1, false, value_ptr(proj));

      glVertexArrayVertexBuffer(vao, 0, buffers[size_t(BufferId::Vertices)], 0, sizeof(vec4));
      glVertexArrayVertexBuffer(vao, 1, buffers[size_t(BufferId::Colors)], 0, sizeof(vec4));

      glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, reinterpret_cast<void *>(0));
      glBindVertexArray(0);
      glUseProgram(0);
    }

    if(auto err = glGetError()) {
      printf("Error: %s\n", glGetString(err));
    }

    SDL_GL_SwapWindow(window);
  }
}
