#version 330
#extension GL_ARB_enhanced_layouts : enable
#define M_PI 3.1415926535897932384626433832795
uniform mat4x4 modelView;
uniform mat4x4 proj;
in vec4 v_color;
layout(location = 0) out vec4 color;
void main() {
////////////////////////////////////////////////////////////////////////////////
//                            user code begins here                           //
      color = v_color;

//                            user code ends here                             //
////////////////////////////////////////////////////////////////////////////////

}
