ascorbic_lib_sources = files(
  'lexer.cpp',
  'parser.cpp',
  'diagnostic.cpp',
  'linter.cpp',
  'rules/unused_var.cpp',
  'rules/unused_import.cpp',
  'rules/unreachable_code.cpp',
  'rules/missing_return.cpp',
  'rules/empty_block.cpp',
)

ascorbic_lib = static_library('ascorbic',
  ascorbic_lib_sources,
  include_directories : ascorbic_include,
)

executable('ascorbic',
  'main.cpp',
  include_directories : ascorbic_include,
  link_with : ascorbic_lib,
  install : true,
)
