{
  "variables": {
    "module_name": "main",
    "module_path": "./app",
    "PRODUCT_DIR": "./build/Release"
  },
  'targets': [
    {
      'target_name': '<(module_name)',
      'sources': [
        'src/main.cpp',
        'src/aes/aes.c'
      ],
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
    {
      'target_name': 'renderer',
      'sources': [
        'src/main.cpp',
        'src/aes/aes.c'
      ],
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
        ['AES256', '1'],
        ['_TARGET_ELECTRON_RENDERER_', '1']
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
    {
      "target_name": "action_after_build",
      "type": "none",
      "dependencies": [ "<(module_name)", "renderer" ],
      "copies": [
        {
          "files": [ "<(PRODUCT_DIR)/<(module_name).node", "<(PRODUCT_DIR)/renderer.node" ],
          "destination": "<(module_path)"
        }
      ]
    }
  ]
}
