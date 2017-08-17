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

uint32_t compileShaderFile(uint32_t shaderType, const char* filename) {
  FILE* file = fopen(filename, "r");
  if(file == nullptr) {
    perror(filename);
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
  return result;
}

// let (window, context) = defaultSetup()

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


const int32_t WindowWidth = 640;
const int32_t WindowHeight = 480;

enum struct BufferId {
  Vertices,
  Colors,
  Indices,
  Count,
};

int32_t buffers[size_t(BufferId::Count)];

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


void initializeRendering() {
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

    glEnableVertexAttribArray(locations[2]);
    uint32_t vao;
    glCreateVertexArrays(1, &vao);

    int32_t loc;
#define T vec4
    loc = locations[2];
    glVertexArrayVertexBuffer(vao, loc, buffers[size_t(BufferId::Vertices)], 0, size_t(sizeof(T)));
    glVertexArrayAttribFormat(vao, loc, attribSize<T>::value, attribType<T>::value, attribNormalized<T>::value, /* relative offset ?! */ 0);
    glVertexArrayAttribBinding(vao, loc, 0);  // bind attribute a_vertex to binding index 0
    glVertexArrayBindingDivisor(vao, 0, 0); // set binding index 0 to divisor 0 (no instancing)

    loc = locations[3];
    glVertexArrayVertexBuffer(vao, loc, buffers[size_t(BufferId::Vertices)], 0, size_t(sizeof(T)));
    glVertexArrayAttribFormat(vao, loc, attribSize<T>::value, attribType<T>::value, attribNormalized<T>::value, /* relative offset ?! */ 0);
    glVertexArrayAttribBinding(vao, loc, 1);  // bind attribute to binding index 1
    glVertexArrayBindingDivisor(vao, 1, 0);   // set binding index 1 to divisor 0 (no instancing)
#undef T

    vertexArrayObjects[size_t(VertexArrayObjectId::HelloTriangle)] = vao;
  }
}

int main() {

  SDL_Init(SDL_INIT_EVERYTHING);


  SDL_Window* window = SDL_CreateWindow("example", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                 WindowWidth, WindowHeight, SDL_WINDOW_OPENGL);

  assert(window);

  SDL_GLContext context = SDL_GL_CreateContext(window);
  SDL_GL_MakeCurrent(window, context);

  uint32_t err = glewInit();
  if (GLEW_OK != err) {
    /* Problem: glewInit failed, something is seriously wrong. */
    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    exit(EXIT_FAILURE);
  }

  initializeRendering();

  glEnable(GL_DEPTH_TEST); // I wonder why this is by default off.

  SDL_Event evt;
  bool runGame = true;

  uint64_t startTime = SDL_GetPerformanceCounter();
  float64_t frequency = float64_t(SDL_GetPerformanceFrequency());

  float32_t aspect = float32_t(WindowWidth) / float32_t(WindowHeight);
  mat4 proj = glm::perspective(90.0f, aspect, 0.01f, 1000.0f);
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

    time = double(SDL_GetPerformanceCounter() - startTime) / frequency;

    mat4 viewMat = mat4(1);
    viewMat = translate(viewMat, vec3(0,1,5));
    viewMat = inverse(viewMat);

    mat4 modelMat = mat4(1.0f);
    modelMat = rotate(modelMat, float32_t(M_PI * -0.05), vec3(0.0f, 1.0f, 0.0f));
    modelMat = scale(modelMat, vec3(3.0f, 3.0f, 3.0f));

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    // START HERE WITH THE VERTEX ARRAY OBJECT !!!


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


      glVertexArrayVertexBuffer(vao, locations[2], buffers[size_t(BufferId::Vertices)], 0, sizeof(vec4));
      glVertexArrayVertexBuffer(vao, locations[3], buffers[size_t(BufferId::Colors)], 0, sizeof(vec4));

      glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, reinterpret_cast<void *>(0));
      glBindVertexArray(0);
      glUseProgram(0);
    }

    SDL_GL_SwapWindow(window);
  }

}




#if 0

var evt: Event
var runGame: bool = true

let timer = newStopWatch(true)

let aspect = float32(window.size.x / window.size.y)
let proj : Mat4f = frustum(-aspect * 0.01f, aspect * 0.01f, -0.01f, 0.01f, 0.01f, 100.0)

while runGame:
  while pollEvent(evt):
    if evt.kind == QuitEvent:
      runGame = false
      break
    if evt.kind == KeyDown and evt.key.keysym.scancode == SDL_SCANCODE_ESCAPE:
      runGame = false

  let time = timer.time.float32

  let viewMat = mat4f(1)
    .translate(0,1,5)            # position camera at position 0,1,5
    .rotateX(Pi * -0.05)         # look a bit down
    .inverse                     # the camera matrix needs to be inverted

  let modelMat = mat4f(1)
    .rotateY(time)               # rotate the triangle
    .scale(3)                    # scale the triangle to be big enough on screen

  glClear(GL_COLOR_BUFFER_BIT or GL_DEPTH_BUFFER_BIT)

  shadingDsl:
    debug
    primitiveMode = GL_TRIANGLES
    numVertices = 3
    indices = indices
    uniforms:
      modelView = viewMat * modelMat
      proj
    attributes:
      a_vertex = vertices
      a_color  = colors
    vertexMain:
      //"""
      gl_Position = proj * modelView * a_vertex;
      v_color = a_color;
      //"""
    vertexOut:
      "out vec4 v_color"
    fragmentMain:
//"""
      color = v_color;
//"""

  glSwapWindow(window)



var vao1119988: VertexArrayObject
  var program1119990: Program
  var locations1119994: array[4, Location]
  glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, 1, 10, "shadingDsl")
  if isNil(program1119990):

    if not linkStatus(program1119990):
      echo ["Log: ", infoLog(program1119990)]
      glDeleteProgram(program1119990.handle)
    glUseProgram(program1119990.handle)
    locations1119994[0].index = glGetUniformLocation(program1119990.handle,
        "modelView")
    if locations1119994[0].index < 0:
      writeLine stderr, ["hello_triangle.nim(56,26)(139937079190920, 139937079191368) Hint: unused uniform: modelView"]
    locations1119994[1].index = glGetUniformLocation(program1119990.handle, "proj")
    if locations1119994[1].index < 0:
      writeLine stderr, ["hello_triangle.nim(57,6)(139937079281984, 139937079282096) Hint: unused uniform: proj"]
    vao1119988 = newVertexArrayObject(nil)
    locations1119994[2] = attributeLocation(program1119990, "a_vertex")
    if 0 <= locations1119994[2].index:
      enableAttrib(vao1119988, locations1119994[2])
      glVertexArrayBindingDivisor(vao1119988.handle, binding(locations1119994[2]).index,
                                  0)
    else:
      writeLine stderr, ["hello_triangle.nim(59,17)(139937078918728, 139937078916432) Hint: unused attribute: a_vertex"]
    bindAndAttribPointer(vao1119988, vertices, locations1119994[2])
    locations1119994[3] = attributeLocation(program1119990, "a_color")
    if 0 <= locations1119994[3].index:
      enableAttrib(vao1119988, locations1119994[3])
      glVertexArrayBindingDivisor(vao1119988.handle, binding(locations1119994[3]).index,
                                  0)
    else:
      writeLine stderr, ["hello_triangle.nim(60,17)(139937079199728, 139937079201576) Hint: unused attribute: a_color"]
    bindAndAttribPointer(vao1119988, colors, locations1119994[3])
  glUseProgram(program1119990.handle)
  glBindVertexArray(vao1119988.handle)
  glVertexArrayElementBuffer(vao1119988.handle, indices.handle)
  uniform(program1119990, locations1119994[0], viewMat * modelMat)
  uniform(program1119990, locations1119994[1], proj)
  if 0 <= locations1119994[2].index:
    let loc1120130 = GLuint(locations1119994[2].index)
    glVertexArrayVertexBuffer(vao1119988.handle, loc1120130, vertices.handle, 0,
                              GLsizei(sizeof(T)))
  if 0 <= locations1119994[3].index:
    let loc1120141 = GLuint(locations1119994[3].index)
    glVertexArrayVertexBuffer(vao1119988.handle, loc1120141, colors.handle, 0,
                              GLsizei(sizeof(T)))
  glDrawElements(0x00000004, GLsizei(3), GL_UNSIGNED_SHORT, cast[pointer](0))
  glBindVertexArray(0)
  glUseProgram(0)
  glPopDebugGroup()
/home/arne/proj/nim/fancygl/examples/hello_triangle.nim(3, 14) Hint: 'context' is declared but not used [XDeclaredButNotUsed]
CC: fancygl_hello_triangle
CC: fancygl_fancygl
Hint:  [Link]
Hint: operation successful (57943 lines compiled; 2.202 sec total; 167.098MiB peakmem; Debug Build) [SuccessX]
Hint: /home/arne/proj/nim/fancygl/examples/hello_triangle  [Exec]
extensions loaded

nim-compile finished at Wed Aug 16 19:42:20

enum BufferNames {
  Buffer_Indices,
  Buffer_Positions,
  Buffer_Normals,
  Buffer_Count
};

void initializeBuffers() {
  uint32_t buffers[Buffer_Count]; // handles for buffers
  glCreateBuffers(Buffer_Count, buffers);
  glNamedBufferStorage(buffers[Buffer_Indices],     indices.size() * sizeof(decltype(  indices)::value_type), indices.data(), 0);
  glNamedBufferStorage(buffers[Buffer_Positions], positions.size() * sizeof(decltype(positions)::value_type), positions.data(), 0);
  glNamedBufferStorage(buffers[Buffer_Normals],     normals.size() * sizeof(decltype(  normals)::value_type), normals.data(), 0);



  glNamedBufferData(buffers[Buffer_Indices],   indices.size() * sizeof(uint16_t), indices.data(), 0);
  glNamedBufferData(buffers[Buffer_Positions], positions.size() * sizeof(vec4), positions.data(), 0);



  glNamedBufferData(buffers[Buffer_Normals],   normals.size() * sizeof(vec4), normals.data(), 0);
}

GLenum myenum;

#include <cstdio>

int main(int argc, char* argv[], char** env) {
  switch(argc) {
  case 0:
    puts("0");
    break;
  case 1:
    puts("1");
    break;
  case 2:
    puts("2");
    break;
  case 3:
    puts("2 or 3");
    break;
  default:
    puts("default\n");
  };

  for(ssize_t i = 0; env[i]; ++i) {
    puts(env[i]);
  }
}

#endif
