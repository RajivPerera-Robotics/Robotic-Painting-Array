#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

const char webpage[] PROGMEM = R"rawstring(
<!DOCTYPE html>
<html>
<head><meta name="viewport" content="width=device-width, initial-scale=1"></head>
<body style="font-family: sans-serif; max-width: 320px; margin: 0 auto; padding: 1.5rem 1rem;">
  <p style="font-size: 18px; font-weight: 500;">Painting array control</p>

  <p style="font-size: 12px; color: gray; text-transform: uppercase; margin: 1.5rem 0 0.5rem;">Homing</p>
  <button onclick="send('cascade home')">Cascade home</button>
  <button onclick="send('homeall')">Home all</button>

  <p style="font-size: 12px; color: gray; text-transform: uppercase; margin: 1.5rem 0 0.5rem;">Cascade move</p>
  <button onclick="send('cascade 90 rel')">Cascade 90 relative</button>
  <button onclick="send('cascade 90 abs')">Cascade 90 absolute</button>
  <button onclick="send('cascade 45 rel')">Cascade 45 relative</button>
  <button onclick="send('cascade 45 abs')">Cascade 45 absolute</button>
  <button onclick="send('cascade rev')">Cascade reverse</button>

  <p style="font-size: 12px; color: gray; text-transform: uppercase; margin: 1.5rem 0 0.5rem;">All — forward</p>
  <button onclick="send('fwd 90 rel')">Forward 90 relative</button>
  <button onclick="send('fwd 90 abs')">Forward 90 absolute</button>
  <button onclick="send('fwd 45 rel')">Forward 45 relative</button>
  <button onclick="send('fwd 45 abs')">Forward 45 absolute</button>

  <p style="font-size: 12px; color: gray; text-transform: uppercase; margin: 1.5rem 0 0.5rem;">All — reverse</p>
  <button onclick="send('rev 90 rel')">Reverse 90 relative</button>
  <button onclick="send('rev 90 abs')">Reverse 90 absolute</button>
  <button onclick="send('rev 45 rel')">Reverse 45 relative</button>
  <button onclick="send('rev 45 abs')">Reverse 45 absolute</button>

  <p id="status" style="font-size: 13px; color: gray; text-align: center; margin-top: 1rem;"></p>

  <style>
    button {
      display: block; width: 100%; padding: 1.2rem;
      font-size: 16px; margin-bottom: 10px;
      border: 1px solid #ccc; border-radius: 10px;
      background: white; cursor: pointer;
    }
    button:active { background: #eee; }
  </style>

  <script>
    function send(cmd) {
      fetch('/command?cmd=' + encodeURIComponent(cmd))
        .then(r => r.text())
        .then(() => {
          document.getElementById('status').textContent = 'Sent: ' + cmd;
          setTimeout(() => document.getElementById('status').textContent = '', 2000);
        });
    }
  </script>
</body>
</html>
)rawstring";

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);

  WiFi.softAP("PaintingControl", "password123");
  Serial.println(WiFi.softAPIP());

  server.on("/", []() {
    server.send(200, "text/html", webpage);
  });

  server.on("/command", []() {
    String cmd = server.arg("cmd");
    Serial2.println(cmd);
    server.send(200, "text/plain", "ok");
  });

  server.begin();
}

void loop() {
  server.handleClient();
}