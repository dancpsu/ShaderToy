#include <stdio.h>

#include "os_generic.h"

#define CNFGOGL
#define CNFG_BATCH
#define CNFG_IMPLEMENTATION
#define NO_OPENGL_HEADERS
#include "rawdraw_sf.h"
#include "chew.c"

typedef struct _Uniforms Uniforms;
short width, height, pressed;

struct Callback_ptrs{
};

struct _Uniforms {
  GLfloat  u_time;
  GLfloat  u_resolution[2];
  GLfloat  u_mouse[2];
};

Uniforms uniforms;
GLuint renderedTexture;

void HandleKey( int keycode, int bDown )
{
  if( keycode == 65307 ) exit( 0 );
  printf( "Key: %d -> %d\n", keycode, bDown );
}

void HandleButton( int x, int y, int button, int bDown )
{
  pressed = bDown;
}

void HandleMotion( int x, int y, int mask )
{
  if (pressed) {
    uniforms.u_mouse[0] = (double)x;
    uniforms.u_mouse[1] = (double)(height - y);
  }
}

void HandleDestroy()
{
}

GLuint
shader_compile(const char* vs_raw, const char* fs_raw)
{
  GLuint vs = 0;
  GLuint fs = 0;
  GLuint shader = 0;
  GLenum err;
  int len;
  char log[500];
  vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &(vs_raw), NULL);
  glCompileShader(vs);

  fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &(fs_raw), NULL);
  glCompileShader(fs);
  glGetShaderInfoLog(fs, 500, &len, log);
  if (len > 0) printf("Error: %s\n", log);

  shader = glCreateProgram();
  glAttachShader(shader, fs);
  glAttachShader(shader, vs);
  glLinkProgram(shader);
  return shader;
}

GLuint
shader_setup(char* fs)
{
  const char* vs =
    "#version 130\n"
    "in vec2 position;\n"
    "void main() {\n"
    "  gl_Position = vec4(position, 0, 1);\n"
    "}\n";

  return shader_compile(vs, fs);
}

void shader_draw(GLuint shader)
{
  glColor3f(0.0, 1.0, 0.0);
  glUseProgram(shader);
  
  GLuint u_time = glGetUniformLocation(shader, "iTime");
  glUniform1f(u_time, uniforms.u_time);

  GLuint u_mouse = glGetUniformLocation(shader, "iMouse");
  glUniform2f(u_mouse, uniforms.u_mouse[0], uniforms.u_mouse[1]);

  GLuint u_resolution = glGetUniformLocation(shader, "iResolution");
  glUniform2f(u_resolution, uniforms.u_resolution[0], uniforms.u_resolution[1]);
  
  glBegin(GL_QUADS);
  glVertex3f(-1.0f, -1.0f, 0.0f);
  glVertex3f(1.0f, -1.0f, 0.0f);
  glVertex3f(1.0f, 1.0f, 0.0f);
  glVertex3f(-1.0f, 1.0f, 0.0f);
  glEnd();
  glUseProgram(0);
}

int
main(int argc, char **argv)
{
  short len;
  double starttime;
  const char* frag_shader;
  const char *frag_header = "#version 130\n\
uniform vec2 iResolution;\n\
uniform float iTime;\n\
uniform int iFrame;\n\
uniform vec2 iMouse;\n";
  const char *frag_footer = "\nvoid main( void ) {\n\
  vec4 fragColor;\n\
  mainImage(fragColor, gl_FragCoord.xy);\n\
  gl_FragColor = fragColor;\n\
}\n";
  FILE *file;
  
  if (argc < 2) {
    printf("Usage: run [filename]\n");
    return 1;
  }
  
  file = fopen (argv[1], "rb");
  if (file == NULL) {
    printf("Could not load shader from file: %s\n", argv[1]);
    return 1;
  }
  
  fseek(file, 0, SEEK_END);
  len = ftell(file);
  fseek(file, 0, SEEK_SET);
  
  frag_shader = malloc(len + strlen(frag_header) + strlen(frag_footer));
  strcpy(frag_shader, frag_header);
  
  if (fread(frag_shader + strlen(frag_header), len, 1, file) < 1)
  {
    printf("Problem loading file: %s\n", argv[1]);
    free (frag_shader);
    return 1;
  }
  fclose(file);
  
  strcpy(frag_shader + len + strlen(frag_header), frag_footer);

  CNFGSetup( "Shadertoy Example", 800, 600 );
  starttime = OGGetAbsoluteTime();

  chewInit();

  GLuint shader = shader_setup(frag_shader);
  GLenum err;
  while((err = glGetError()) != GL_NO_ERROR)
  {
    printf("OpenGL Error: %0x\n", err);
  }
  
  CNFGGetDimensions(&width, &height);

  /* Main loop */
  while(CNFGHandleInput())
  {
    /* Set the uniforms */
    CNFGGetDimensions(&width, &height);
    uniforms.u_resolution[0] = (GLfloat)width;
    uniforms.u_resolution[1] = (GLfloat)height;
    uniforms.u_time = (GLfloat)(OGGetAbsoluteTime() - starttime);

    CNFGClearFrame();
    shader_draw(shader);

    CNFGSwapBuffers();
  }
  return 0;
}
