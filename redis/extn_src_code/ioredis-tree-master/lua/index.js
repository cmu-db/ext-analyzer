var fs = require('fs');
var path = require('path');

function loadScript(name) {
  return fs.readFileSync(path.join(__dirname, name + '.lua'), 'utf8');
}

var head = loadScript('_head').split('\n').filter(isNotCommend).join(' ') + ' ';
var deleteReference = loadScript('_delete_reference').split('\n').filter(isNotCommend).join(' ') + ' ';
var getPath = loadScript('_get_path').split('\n').filter(isNotCommend).join(' ') + ' ';

var commands = ['tinsert', 'tchildren', 'tparents', 'tpath', 'trem', 'tmrem',
  'tdestroy', 'texists', 'trename', 'tprune', 'tmovechildren'].map(function (command) {
  var lua = loadScript(command);
  if (command === 'trem' || command === 'tdestroy' || command === 'tmrem' || command === 'tprune') {
    lua = deleteReference + lua;
  } else if (command === 'tpath' || command === 'tinsert' || command === 'tmovechildren') {
    lua = getPath + lua;
  }
  return {
    name: command,
    lua: head + lua
  };
});

module.exports = commands;

function isNotCommend(line) {
  return line.trim().slice(0, 2) !== '--';
}
