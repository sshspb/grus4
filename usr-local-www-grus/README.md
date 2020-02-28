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
