{
  "variables": {
    "module_path": "./app",
    "PRODUCT_DIR": "./build/Release"
  },
  'target_defaults': {
    'defines': [
      'CBC=1',
      'AES256=1'
    ]
  },
  'targets': [
    {
      'target_name': 'main',
      'sources': [
        'src/main.cpp',
        'src/aes/aes.c'
      ],
      'includes': [
        './common.gypi'
      ]
    },
    {
      'target_name': 'renderer',
      'sources': [
        'src/main.cpp',
        'src/aes/aes.c'
      ],
      'includes': [
        './common.gypi'
      ],
      'defines':[
        '_TARGET_ELECTRON_RENDERER_'
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
