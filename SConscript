Import(['env'])

basiccpu = SharedLibrary('basiccpu', ['mycpu.cpp'])

matrixvm = env.Program(target='matrixvm',
                       source=['main.cpp', 'motherboard.cpp', 'dladapter.cpp'],
                       LINKFLAGS='-ldl'
                      )
