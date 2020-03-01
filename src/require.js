function makeRequireFunction(mod) {
  const Module = mod.constructor;

  function validateString (value, name) { if (typeof value !== 'string') throw new TypeError('The \"' + name + '\" argument must be of type string. Received type ' + typeof value); }

  const require = function require(path) {
    return mod.require(path);
  };

  function resolve(request, options) {
    validateString(request, 'request');
    return Module._resolveFilename(request, mod, false, options);
  }

  require.resolve = resolve;

  function paths(request) {
    validateString(request, 'request');
    return Module._resolveLookupPaths(request, mod);
  }

  resolve.paths = paths;

  require.main = process.mainModule;

  require.extensions = Module._extensions;

  require.cache = Module._cache;

  return require;
}
