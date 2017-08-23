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

using cstring = const char*;

//////////////////////////////
// opengl utility functions //
//////////////////////////////

void debugCallback(uint32_t source, uint32_t type, uint32_t id, uint32_t severity, int32_t length, cstring message, const void* userPointer) {
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
    printf("  source: %d ???", source); //"
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
    printf("  severity: %d ???", severity);
  }
}

uint32_t compileShaderFile(uint32_t shaderType, cstring filename) {
  FILE* file = fopen(filename, "r");
  if(file == nullptr) {
    perror(filename);
    exit(EXIT_FAILURE);
  }
  string buffer;
  int32_t character = fgetc(file);
  while(character != EOF) {
    buffer += char(character);
    character = fgetc(file);
  }
  fclose(file);
  auto result = glCreateShader(shaderType);

  cstring cstring[] = {buffer.c_str()};
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
struct AttribSize {
  static const int32_t value = 1;
};

template <>
struct AttribSize<vec4> {
  static const int32_t value = 4;
};

template <>
struct AttribSize<vec3> {
  static const int32_t value = 3;
};

template <typename T>
struct AttribType {
  static const int32_t value = -1;
};

template <>
struct AttribType<int8_t> {
  static const int32_t value = GL_BYTE;
};

template <>
struct AttribType<float32_t> {
  static const int32_t value = GL_FLOAT;
};

template <>
struct AttribType<float64_t> {
  static const int32_t value = GL_DOUBLE;
};

template <>
struct AttribType<vec4> {
  static const int32_t value = AttribType<vec4::value_type>::value;
};

template <>
struct AttribType<vec3> {
  static const int32_t value = AttribType<vec3::value_type>::value;
};

template <typename T>
struct AttribNormalized {
  static const bool value = false;
};

/////////////////
// buffer type //
/////////////////

template <typename T>
struct ArrayBuffer {
  uint32_t id = 0;

  int32_t valueByteSize() {
    return sizeof(T);
  }

  void operator=(std::initializer_list<T> args) {
    assert(id == 0);
    glCreateBuffers(1, &id);
    glNamedBufferData(id, size(args) * sizeof(*data(args)), data(args), GL_STATIC_DRAW);
  }

  int32_t attribSize() {
    return AttribSize<T>::value;
  }

  int32_t attribType() {
    return AttribType<T>::value;
  }

  bool attribNormalized() {
    return AttribNormalized<T>::value;
  }
};

//////////////////
// program data //
//////////////////

ArrayBuffer<vec4> vertices, colors;
ArrayBuffer<uint16_t> indices;

enum struct VertexArrayObjectId {
  HelloTriangle,
  Count,
};

uint32_t vertexArrayObjects[size_t(VertexArrayObjectId::Count)];

enum struct ProgramId {
  HelloTriangle,
  Count,
};

int32_t programLocationsList[4];
int32_t programLocationOffsets[size_t(ProgramId::Count)] = {0};
uint32_t programs[size_t(ProgramId::Count)];

void initializeRendering() {
  { // initialize Buffers that are later bound to the attributes
    vertices = {
      {-1,-1, 0, 1},
      { 1,-1, 0, 1},
      { 0, 1, 0, 1},
    };
    glObjectLabel(GL_BUFFER, vertices.id, -1, "vertices");

    colors = {
      {1,0,0,1},
      {0,1,0,1},
      {0,0,1,1},
    };
    glObjectLabel(GL_BUFFER, colors.id, -1, "colors");

    indices = {
      0, 1, 2,
    };
    glObjectLabel(GL_BUFFER, indices.id, -1, "indices");
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
    glObjectLabel(GL_PROGRAM, program, -1, "HelloTriangle");

    programs[size_t(ProgramId::HelloTriangle)] = program;

    int32_t* locations = programLocationsList + programLocationOffsets[size_t(ProgramId::HelloTriangle)];
    locations[0] = glGetUniformLocation(program, "modelView");
    locations[1] = glGetUniformLocation(program, "proj");
    locations[2] = glGetAttribLocation(program, "a_vertex");
    locations[3] = glGetAttribLocation(program, "a_color");

    uint32_t vao;
    glCreateVertexArrays(1, &vao);

    { // setup vertices attribute
      int32_t loc = locations[2];
      glVertexArrayVertexBuffer(vao, loc, vertices.id, 0, vertices.valueByteSize() );
      auto attribSizeValue = vertices.attribSize();
      auto attribTypeValue = vertices.attribType();
      auto attribNormalizedValue = vertices.attribNormalized();
      glVertexArrayAttribFormat(vao, loc, attribSizeValue, attribTypeValue, attribNormalizedValue, 0);
      glVertexArrayAttribBinding(vao, loc, 0); // bind attribute a_vertex to binding index 0
      glVertexArrayBindingDivisor(vao,     0, 0); // set binding index 0 to divisor 0 (no instancing)
      glEnableVertexArrayAttrib(vao, 0);
    }
    { // setup colors attribute
      int32_t loc = locations[3];
      glVertexArrayVertexBuffer(vao, loc, colors.id, 0, colors.valueByteSize());
      auto attribSizeValue = colors.attribSize();
      auto attribTypeValue = colors.attribType();
      auto attribNormalizedValue = colors.attribNormalized();
      glVertexArrayAttribFormat(vao, loc, attribSizeValue, attribTypeValue, attribNormalizedValue, 0);
      glVertexArrayAttribBinding(vao, loc, 1); // bind attribute to binding index 1
      glVertexArrayBindingDivisor(vao,     1, 0);  // set binding index 1 to divisor 0 (no instancing)
      glEnableVertexArrayAttrib(vao, 1);
    }
    vertexArrayObjects[size_t(VertexArrayObjectId::HelloTriangle)] = vao;
  }
}

int32_t main() {

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
  float64_t time;
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
      int32_t* locations = programLocationsList + programLocationOffsets[size_t(ProgramId::HelloTriangle)];

      glUseProgram(program);
      glBindVertexArray(vertexArrayObjects[size_t(VertexArrayObjectId::HelloTriangle)]);
      glVertexArrayElementBuffer(vao, indices.id);

      auto modelViewMat = viewMat * modelMat;
      glProgramUniformMatrix4fv(program, locations[0], 1, false, value_ptr(modelViewMat));
      glProgramUniformMatrix4fv(program, locations[1], 1, false, value_ptr(proj));

      glVertexArrayVertexBuffer(vao, 0, vertices.id, 0, sizeof(vec4));
      glVertexArrayVertexBuffer(vao, 1, colors.id, 0, sizeof(vec4));

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
