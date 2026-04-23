project('ascorbic', 'cpp',
  version : '0.1.0',
  license : 'GPL-3.0-or-later',
  default_options : [
    'cpp_std=c++23',
    'warning_level=3',
  ])

add_project_arguments('-Wshadow', language : 'cpp')

ascorbic_include = include_directories('include')

subdir('src')
