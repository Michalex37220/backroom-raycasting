project('backroom-raycaster', 'c')

cc = meson.get_compiler('c')

gl_dep = dependency('gl')
glu_dep = dependency('glu')
glut_dep = dependency('glut')
m_dep = cc.find_library('m', required: true)
executable(
  'backroom',
  'src/main.c',
  dependencies: [gl_dep, glu_dep, glut_dep, m_dep ]
)
