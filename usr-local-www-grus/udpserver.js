const dgram = require('dgram');
const MongoClient = require('mongodb').MongoClient;
const request = require('request');
const config = require('./config');
const server = dgram.createSocket('udp4');

function savedata(docs) {
    // save to database on mLab.com { useUnifiedTopology: true } 
    MongoClient.connect(config.mlabDB, {useNewUrlParser: true, useUnifiedTopology: true}, function(err, client) {
      console.log("Connected successfully to server mlab");
      const db = client.db(config.dbName);
      const collection = db.collection('grus');
      collection.insertOne(docs, function(err, result) {
        client.close();
      });
    });
}

server.on('error', (err) => {
  console.log(`server error:\n${err.stack}`);
//  server.close();
  throw err;
});

server.on('message', (msg, rinfo) => {
  var date = new Date();
  console.log(`\ndate ${date.toString()}`);
  console.log(`server got: ${msg} \nfrom ${rinfo.address}:${rinfo.port}`);
  var msgString = msg.toString();
  // control of belonging of the received information to our sensors
  if (msgString.length < 5 ||  msgString[0] != 'G' || msgString[1] != 'r' || msgString[2] != 'u' || msgString[3] != 's') {
    console.log("Bad data - discard");
    return;
  }
  var docs = {
    "_id": Math.trunc(Date.now()/1000),
    "date": date.toLocaleString()
  };
  var msgStrings = msgString.split('\n');
  for (var i = 1; i < msgStrings.length; i++) {
    var msgList = msgStrings[i].split(/\s+/, 2);
    if (msgList.length == 2 && msgList[0].length > 0 && msgList[1].length > 0) {
      if (i == 1) docs["tin"] = msgList[1];
      else docs[msgList[0]] = msgList[1];
    }
  }
  var result;
  // Toksovo
  request(config.openweathermapUrlToksovo, function (error, response, body) {
    if (!error && response && (response.statusCode == 200)) {
      result = JSON.parse(body);
      docs.tout = (result.main.temp - 273.15).toFixed(1);
      docs.name = result.name;
      savedata(docs);
    } else {
      // Oselki
      request(config.openweathermapUrlOselki, function (error, response, body) {
        if (!error && response && (response.statusCode == 200)) {
          result = JSON.parse(body);
          docs.tout = (result.main.temp - 273.15).toFixed(1);
          docs.name = result.name;
          savedata(docs);
        } else {
          // Leskolovo
          request(config.openweathermapUrlLeskolovo, function (error, response, body) {
            if (!error && response && (response.statusCode == 200)) {
              result = JSON.parse(body);
              docs.tout = (result.main.temp - 273.15).toFixed(1);
              docs.name = result.name;
              savedata(docs);
            } else {
              // Murino
              request(config.openweathermapUrlMurino, function (error, response, body) {
                if (!error && response && (response.statusCode == 200)) {
                  result = JSON.parse(body);
                  docs.tout = (result.main.temp - 273.15).toFixed(1);
                  docs.name = result.name;
                } else {
                  docs.tout = null;
                  docs.name = null;
                }
                savedata(docs);
              });
            }
          });
        }
      });
    }
  });
});

server.on('listening', () => {
  const address = server.address();
  console.log(`UDP server listening ${address.address}:${address.port}`);
});

server.bind(config.portUDPserver, config.hostUDPserver);
