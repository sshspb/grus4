const assert = require('assert');
const dgram = require('dgram');
const MongoClient = require('mongodb').MongoClient;
const request = require('request');
const config = require('./config');
const server = dgram.createSocket('udp4');

server.on('error', (err) => {
  console.log(`server error:\n${err.stack}`);
//  server.close();
  throw err;
});

server.on('message', (msg, rinfo) => {
  
  var date = new Date(Date.now());
  console.log(`\ndate ${date.toString()}`);
  console.log(`server got: ${msg} \nfrom ${rinfo.address}:${rinfo.port}`);
  var docs = {"_id": Math.trunc(Date.now()/1000)};
  var data = [];
  var msgString = msg.toString();
  // control of belonging of the received information to our sensors
  if (msgString.length < 5 ||  msgString[0] != 'G' || msgString[1] != 'r' || msgString[2] != 'u' || msgString[3] != 's') {
    console.log("Bad data - discard");
    return;
  }
  var msgStrings = msgString.split('\n');
  for (var i = 1; i < msgStrings.length; i++) {
    var msgList = msgStrings[i].split(/\s+/, 2);
    if (msgList.length == 2 && msgList[0].length > 0 && msgList[1].length > 0) {
      data.push({"dev": msgList[0], "val": msgList[1]});
    }
  }
  docs.data = data;
  // weather data in Toksovo
  request(config.openweathermapUrl, function (error, response, body) {
    if (!error && response && (response.statusCode == 200)) {
      result = JSON.parse(body);
      docs.tout = (result.main.temp - 273.15).toFixed(1);
    }
    // save to database on mLab.com
    MongoClient.connect(config.mlabDB, {useNewUrlParser: true}, function(err, client) {
      assert.equal(null, err);
      console.log("Connected successfully to server mlab");
      const db = client.db(config.dbName);
      const collection = db.collection('toksovo');
      collection.insertOne(docs, function(err, result) {
        assert.equal(err, null);
        client.close();
      });
    });
  });
});

server.on('listening', () => {
  const address = server.address();
  console.log(`UDP server listening ${address.address}:${address.port}`);
});

server.bind(config.portUDPserver, config.hostUDPserver);
