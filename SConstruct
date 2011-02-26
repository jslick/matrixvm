env     = Environment()
debug   = env.Clone(CCFLAGS='-Wall -g')
release = env.Clone(CCFLAGS='-Wall')

env = debug
Export('env')
debug.SConscript(
    'SConscript',
    variant_dir='build',
    exports=['env'],
    duplicate=0
)
