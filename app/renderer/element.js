module.exports = {
  createElement: function createElement (tag, attr, children) {
    const el = document.createElement(tag)
    if (attr) {
      for (const key in attr) {
        if (key.indexOf('on') === 0) {
          el.addEventListener(key.substring(2).toLowerCase(), attr[key])
        } else {
          el.setAttribute(key, attr[key])
        }
      }
    }
    if (typeof children === 'string') {
      el.innerHTML = children
    } else if (Array.isArray(children) && children.length) {
      for (let i = 0; i < children.length; i++) {
        el.appendChild(children[i])
      }
    }
    return el
  },
  render: function render (root, el) {
    if (Array.isArray(el)) {
      for (let i = 0; i < el.length; i++) {
        root.appendChild(el[i])
      }
    } else if (typeof el === 'string') {
      root.innerHTML = el
    } else {
      root.appendChild(el)
    }
  }
}
