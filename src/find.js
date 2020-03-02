function findEntryModule(mainModule, exports) {
  function findModule(start, target) {
    if (start.exports === target) {
      return start;
    }
    for (var i = 0; i < start.children.length; i++) {
      var res = findModule(start.children[i], target);
      if (res) {
        return res;
      }
    }
    return null;
  }
  return findModule(mainModule, exports);
}
