// show list of files and read/download one
var http = require('http');
var url = require('url');
var fs = require('fs');
var MongoClient = require('mongodb').MongoClient;
var config = require('./config');
var showdown  = require('showdown');
var converter = new showdown.Converter();

http.createServer(function (req, res) {
  var locator = url.parse(req.url, true);
  if (locator.pathname === '/put') {
    var query = locator.query;
    query.timestamp = Date.now();
    var readout = JSON.stringify(query);
    res.end(readout);
    fs.writeFileSync('test.json', readout);
  } else if (locator.pathname === '/test') {
    var readout = fs.readFileSync('test.json');
    res.end(readout);
  } else if (locator.pathname === '/mlab') {
    MongoClient.connect(config.mlabDB, {useNewUrlParser: true}, function(err, client) {
      var collection = client.db(config.dbName).collection('toksovo');
      collection.find({}).sort([['_id', -1]]).limit(18).toArray(function(err, docs) {
        res.end(JSON.stringify(docs));
        client.close();
      });
    });
  } else {
    var filename = '.' + locator.pathname;
    if (filename.length > 0) {
      if (filename.substr(-3) == '.md') {
        var readout = fs.readFileSync(filename, 'utf8');
        var html = converter.makeHtml(readout);
        fs.writeFileSync('temporary.html', html);
        filename = './temporary.html'; 
      } else {
        filename = '.' + locator.pathname; 
      }
    } else {
      filename = '.' + locator.pathname; 
    }
    if (filename == './' || !fs.existsSync(filename) || !fs.statSync(filename).isFile()) {
      filename = './index.html';
    }
    var file = new fs.ReadStream(filename);
    sendFile(file, res, filename == './temporary.html');
  }
}).listen(8008);

function sendFile(file, res, meta) {
  res.setHeader("Cache-Control", "no-store");
  if (meta) {
    res.write('<head><meta charset="utf-8"></head><body>');
  }
  file.on('readable', write);
  function write() {
    var fileContent = file.read(); // считать
    if (fileContent && !res.write(fileContent)) { // отправить
      file.removeListener('readable', write);
      res.once('drain', function() { // подождать
        file.on('readable', write);
        write();
      });
    }
  }
  file.on('end', function() {
    if (meta) {
      res.end('</body>');
    } else {
      res.end();
    }
  });
};
