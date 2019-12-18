var http = require('http');
var url = require('url');
var fs = require('fs');
var MongoClient = require('mongodb').MongoClient;
var config = require('./config');

http.createServer(function (req, res) {
  var locator = url.parse(req.url, true);
  if (locator.pathname === '/mlab') {
    MongoClient.connect(config.mlabDB, {useNewUrlParser: true}, function(err, client) {
      if (!err) {
        var collection = client.db(config.dbName).collection('grus');
        collection.find({}).sort({"_id": -1}).limit(18).toArray(function(err, docs) {
          if (!err) {
            res.end(JSON.stringify(docs));
          } else {
            res.end('Sorry, can not select collection');
          }
        });
      } else {
        res.end('Sorry, can not connect to mlabDB');
      }
      client.close();
    });
  } else {
    var filename = '.' + locator.pathname;
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
    var fileContent = file.read();
    if (fileContent && !res.write(fileContent)) {
      file.removeListener('readable', write);
      res.once('drain', function() {
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
