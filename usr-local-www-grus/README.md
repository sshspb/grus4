Папка /usr/local/www/grus

После изменения файлов из этой папки следует перезагрузить LaunchDaemons
```
besarab: shs$ cd /Library/LaunchDaemons/
besarab:LaunchDaemons shs$ sudo launchctl unload -w ./besarab.webserver.plist
besarab:LaunchDaemons shs$ sudo launchctl load -w ./besarab.webserver.plist
besarab:LaunchDaemons shs$ sudo launchctl unload -w ./besarab.udpserver.plist
besarab:LaunchDaemons shs$ sudo launchctl load -w ./besarab.udpserver.plist
```

Содержимое Property List
```
besarab:~ shs$ cat /Library/LaunchDaemons/besarab.udpserver.plist

<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key>
    <string>besarab.udpserver</string>
  <key>Disabled</key>
    <false/>
  <key>ProgramArguments</key>
    <array>
      <string>/opt/local/bin/node</string>
      <string>udpserver.js</string>
    </array>
  <key>WorkingDirectory</key>
    <string>/usr/local/www/grus</string>
  <key>RunAtLoad</key>
    <true/>
</dict>
</plist>

besarab:~ shs$ cat /Library/LaunchDaemons/besarab.webserver.plist

<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple Computer//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
  <key>Label</key>
    <string>besarab.webserver</string>
  <key>Disabled</key>
    <false/>
  <key>ProgramArguments</key>
    <array>
      <string>/opt/local/bin/node</string>
      <string>webserver.js</string>
    </array>
  <key>WorkingDirectory</key>
    <string>/usr/local/www/grus</string>
  <key>RunAtLoad</key>
    <true/>
</dict>
</plist>
```

17.12.2019 transform collection "toksovo" to collection "grus"
```
const fs = require('fs');
const MongoClient = require('mongodb').MongoClient;
const config = require('./config');
MongoClient.connect(config.mlabDB, {useNewUrlParser: true, useUnifiedTopology: true}, function(err, client) {
  const db = client.db(config.dbName);
  const toksovo = db.collection('toksovo');
  toksovo.find({}).sort({"_id": -1}).limit(18).toArray(function(err, docs) {
    fs.writeFileSync('toksovo.json', JSON.stringify(docs), 'utf8');
    for (var i = 0; i < docs.length; i++) {
      docs[i].tin = docs[i].data[0].val;
      for (var j = 1; j < docs[i].data.length; j++) {
        docs[i][docs[i].data[j].dev] = docs[i].data[j].val;
      }
      if (!docs[i].tout) {
        docs[i].tout = null; 
      }
    }
    const grus = db.collection('grus');
    grus.insertMany(docs, function(err, result) {
      console.log(err);
      fs.writeFileSync('grus.json', JSON.stringify(result), 'utf8');
      client.close();
    });
  });
});
```