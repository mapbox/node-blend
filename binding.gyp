{
  'conditions': [
      ['OS=="win"', {
        'variables': {
          'copy_command%': 'copy',
          'bin_name':'call'
        },
      },{
        'variables': {
          'copy_command%': 'cp',
          'bin_name':'node'
        },
      }]
  ],
  'target_defaults': {
      'default_configuration': 'Release',
      'configurations': {
          'Debug': {
              'cflags_cc!': ['-O3', '-DNDEBUG'],
              'xcode_settings': {
                'OTHER_CPLUSPLUSFLAGS!':['-O3', '-DNDEBUG']
              },
              'msvs_settings': {
                 'VCCLCompilerTool': {
                     'ExceptionHandling': 1,
                     'RuntimeTypeInfo':'true',
                     'RuntimeLibrary': '3'  # /MDd
                 }
              }
          },
          'Release': {

          }
      },
      'include_dirs': [
          './src',
          './deps'
      ],
      'cflags_cc!': ['-fno-rtti', '-fno-exceptions'],
      'cflags_cc' : [
          '<!@(pkg-config libpng --cflags)',
          '-funroll-loops',
          '-fomit-frame-pointer'
          
      ],
      'libraries':[
        '<!@(pkg-config libpng --libs)',
        '-lwebp',
        '-ljpeg',
      ]
  },
  'targets': [
    {
      'target_name': 'blend',
      'sources': ["src/blend.cpp",
                  "src/reader.cpp",
                  "src/palette.cpp",
      ],
      'defines': [
       'USE_DENSE_HASH_MAP',
       'NDEBUG'
      ],
      'xcode_settings': {
        'OTHER_CPLUSPLUSFLAGS':[
           '<!@(pkg-config libpng --cflags)'
        ],
        'GCC_ENABLE_CPP_RTTI': 'YES',
        'GCC_ENABLE_CPP_EXCEPTIONS': 'YES'
      }
    },
    {
      'target_name': 'action_after_build',
      'type': 'none',
      'dependencies': [ 'blend' ],
'actions': [
        {
          'action_name': 'move_node_module',
          'inputs': [
            '<@(PRODUCT_DIR)/blend.node'
          ],
          'outputs': [
            'lib/blend.node'
          ],
          'action': ['<@(copy_command)', '<@(PRODUCT_DIR)/blend.node', 'lib/blend.node']
        }
      ]
    }
  ]
}
