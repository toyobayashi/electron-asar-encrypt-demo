{
  "variables": {
    "module_path": "./app",
    "PRODUCT_DIR": "./build/Release"
  },
  'target_defaults': {
    'include_dirs': [
      "<!@(node -p \"require('node-addon-api').include\")"
    ],
    'dependencies': [
      "<!(node -p \"require('node-addon-api').gyp\")"
    ],
    'cflags!': [ '-fno-exceptions' ],
    'cflags_cc!': [ '-fno-exceptions' ],
    'defines':[
      ['CBC', '1'],
      ['AES256', '1']
    ],
    'conditions':[
      ['OS=="mac"', {
        'cflags+': ['-fvisibility=hidden'],
        'xcode_settings': {
          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',
          'CLANG_CXX_LIBRARY': 'libc++',
          'MACOSX_DEPLOYMENT_TARGET': '10.7',
          'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES', # -fvisibility=hidden
        }
      }],
      ['OS=="win"', { 
        'msvs_settings': {
          'VCCLCompilerTool': { 'ExceptionHandling': 1, 'AdditionalOptions': ['/source-charset:utf-8'] },
        },
        'defines':[
          'NOMINMAX'
        ]
      }]
    ]
  },
  'targets': [
    {
      'target_name': 'main',
      'sources': [
        'src/main.cpp',
        'src/aes/aes.c'
      ]
    },
    {
      'target_name': 'renderer',
      'sources': [
        'src/main.cpp',
        'src/aes/aes.c'
      ],
      'defines':[
        ['_TARGET_ELECTRON_RENDERER_', '1']
      ]
    },
    {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "main", "renderer" ],
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/main.node", "<(PRODUCT_DIR)/renderer.node" ],
          "destination": "<(module_path)"
        }
      ]
    }
  ]
}
