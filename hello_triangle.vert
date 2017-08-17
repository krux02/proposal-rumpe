#version 330
#extension GL_ARB_enhanced_layouts : enable
#define M_PI 3.1415926535897932384626433832795
uniform mat4x4 modelView;
uniform mat4x4 proj;
in vec4 a_vertex;
in vec4 a_color;
out vec4 v_color;
void main() {
////////////////////////////////////////////////////////////////////////////////
//                            user code begins here                           //
      gl_Position = proj * modelView * a_vertex;
      v_color = a_color;

//                            user code ends here                             //
////////////////////////////////////////////////////////////////////////////////

}
