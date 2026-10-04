#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

const char* ssid = "RC CAR";
const char* password = ""; // no password

ESP8266WebServer server(80);

// ----------------------- Pins -----------------------
const int IN1_STEER_A = D0;   // steering motor +
const int IN2_STEER_B = D1;   // steering motor -
const int ENA_STEER   = D4;   // steering speed

const int IN3_DRIVE_A = D2;   // drive motor +
const int IN4_DRIVE_B = D3;   // drive motor -
const int ENB_DRIVE   = D5;   // drive speed

const int LED_FRONT  = D6; // front light
const int LED_REAR   = D7; // rear light
const int FLASH_L    = D8; // left flash
const int FLASH_R    = 3;  // GPIO3 (RX) - right flash

// ----------------------- State -----------------------
int gasValue = 0; // -5 to +5
bool steeringLeft = false;
bool steeringRight = false;

bool frontLight = false;
bool rearLight = false;
bool leftFlash = false;
bool rightFlash = false;
bool hazardFlash = false;
bool nitro = false;

bool blinkState = false;
unsigned long lastBlinkMillis = 0;

// ----------------------- setup -----------------------
void setup() {
  Serial.begin(115200);

  pinMode(IN1_STEER_A, OUTPUT);
  pinMode(IN2_STEER_B, OUTPUT);
  pinMode(ENA_STEER, OUTPUT);

  pinMode(IN3_DRIVE_A, OUTPUT);
  pinMode(IN4_DRIVE_B, OUTPUT);
  pinMode(ENB_DRIVE, OUTPUT);

  pinMode(LED_FRONT, OUTPUT);
  pinMode(LED_REAR, OUTPUT);
  pinMode(FLASH_L, OUTPUT);
  pinMode(FLASH_R, OUTPUT);

  digitalWrite(IN1_STEER_A, LOW);
  digitalWrite(IN2_STEER_B, LOW);
  digitalWrite(IN3_DRIVE_A, LOW);
  digitalWrite(IN4_DRIVE_B, LOW);
  analogWrite(ENA_STEER, 0);
  analogWrite(ENB_DRIVE, 0);

  digitalWrite(LED_FRONT, LOW);
  digitalWrite(LED_REAR, LOW);
  digitalWrite(FLASH_L, LOW);
  digitalWrite(FLASH_R, LOW);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.println("AP started");
  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", getHtmlPage());
  });

  server.on("/cmd", HTTP_GET, handleCommand);

  server.begin();
  Serial.println("Web server started");
}

void loop() {
  server.handleClient();
  updateBlinkingLights();
  updateSteeringMotor();
  updateDriveMotor();
}

// ----------------------- Command handler -----------------------
void handleCommand() {
  String cmd = server.arg("cmd");
  String val = server.arg("val");

  if (cmd == "gas") {
    gasValue = val.toInt();
    Serial.print("Gas: ");
    Serial.println(gasValue);
  }
  else if (cmd == "steer") {
    if (val == "left") {
      steeringLeft = true;
      steeringRight = false;
    } else if (val == "right") {
      steeringLeft = false;
      steeringRight = true;
    } else if (val == "stop") {
      steeringLeft = false;
      steeringRight = false;
    }
    Serial.print("Steer: ");
    Serial.println(val);
  }
  else if (cmd == "front") {
    frontLight = (val == "1");
    Serial.print("Front: ");
    Serial.println(frontLight ? "ON" : "OFF");
  }
  else if (cmd == "rear") {
    rearLight = (val == "1");
    if (rearLight) {
      frontLight = true;
    }
    Serial.print("Rear: ");
    Serial.println(rearLight ? "ON" : "OFF");
  }
  else if (cmd == "leftFlash") {
    leftFlash = (val == "1");
    if (leftFlash) {
      hazardFlash = false;
    }
    Serial.print("Left Flash: ");
    Serial.println(leftFlash ? "ON" : "OFF");
  }
  else if (cmd == "rightFlash") {
    rightFlash = (val == "1");
    if (rightFlash) {
      hazardFlash = false;
    }
    Serial.print("Right Flash: ");
    Serial.println(rightFlash ? "ON" : "OFF");
  }
  else if (cmd == "hazard") {
    hazardFlash = (val == "1");
    if (hazardFlash) {
      leftFlash = false;
      rightFlash = false;
    }
    Serial.print("Hazard: ");
    Serial.println(hazardFlash ? "ON" : "OFF");
  }
  else if (cmd == "nitro") {
    nitro = (val == "1");
    Serial.print("Nitro: ");
    Serial.println(nitro ? "ON" : "OFF");
  }

  updateActualOutputs();
  server.send(200, "text/plain", "OK");
}

// ----------------------- Motor logic -----------------------
void updateDriveMotor() {
  int speed = abs(gasValue);
  if (speed > 5) speed = 5;

  int pwm = map(speed, 0, 5, 0, 1023);

  if (gasValue > 0) {
    digitalWrite(IN3_DRIVE_A, HIGH);
    digitalWrite(IN4_DRIVE_B, LOW);
  } else if (gasValue < 0) {
    digitalWrite(IN3_DRIVE_A, LOW);
    digitalWrite(IN4_DRIVE_B, HIGH);
  } else {
    digitalWrite(IN3_DRIVE_A, LOW);
    digitalWrite(IN4_DRIVE_B, LOW);
    pwm = 0;
  }

  if (nitro) pwm = 1023;

  analogWrite(ENB_DRIVE, pwm);
}

void updateSteeringMotor() {
  int steerPWM = 600;

  if (steeringLeft) {
    digitalWrite(IN1_STEER_A, HIGH);
    digitalWrite(IN2_STEER_B, LOW);
    analogWrite(ENA_STEER, steerPWM);
  } else if (steeringRight) {
    digitalWrite(IN1_STEER_A, LOW);
    digitalWrite(IN2_STEER_B, HIGH);
    analogWrite(ENA_STEER, steerPWM);
  } else {
    digitalWrite(IN1_STEER_A, LOW);
    digitalWrite(IN2_STEER_B, LOW);
    analogWrite(ENA_STEER, 0);
  }
}

// ----------------------- Light logic -----------------------
void updateBlinkingLights() {
  if (millis() - lastBlinkMillis >= 250) {
    lastBlinkMillis = millis();
    blinkState = !blinkState;

    if (hazardFlash) {
      digitalWrite(FLASH_L, blinkState ? HIGH : LOW);
      digitalWrite(FLASH_R, blinkState ? HIGH : LOW);
    } else {
      digitalWrite(FLASH_L, (leftFlash && blinkState) ? HIGH : LOW);
      digitalWrite(FLASH_R, (rightFlash && blinkState) ? HIGH : LOW);
    }
  }
}

void updateActualOutputs() {
  digitalWrite(LED_FRONT, frontLight ? HIGH : LOW);

  if (rearLight) {
    digitalWrite(LED_REAR, HIGH);
    if (!frontLight) {
      frontLight = true;
      digitalWrite(LED_FRONT, HIGH);
    }
  } else {
    digitalWrite(LED_REAR, LOW);
  }

  if (hazardFlash) {
    digitalWrite(FLASH_L, blinkState ? HIGH : LOW);
    digitalWrite(FLASH_R, blinkState ? HIGH : LOW);
  } else {
    digitalWrite(FLASH_L, (leftFlash && blinkState) ? HIGH : LOW);
    digitalWrite(FLASH_R, (rightFlash && blinkState) ? HIGH : LOW);
  }
}

// ----------------------- Web UI -----------------------
String getHtmlPage() {
  String html = R"=====(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover, user-scalable=no">
  <title>RC CAR</title>
  <style>
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
      -webkit-user-select: none;
      user-select: none;
      -webkit-touch-callout: none;
    }

    html, body {
      width: 100%;
      height: 100%;
      overflow: hidden;
      background: #0c1017;
      font-family: Arial, sans-serif;
      color: #e6edf7;
    }

    body {
      display: flex;
      flex-direction: column;
    }

    .topbar {
      height: 76px;
      background: linear-gradient(180deg, #161d2b 0%, #0d1119 100%);
      border-bottom: 2px solid #ff6b35;
      display: flex;
      align-items: center;
      justify-content: space-between;
      padding: 10px 18px;
      gap: 16px;
    }

    .gear-display {
      min-width: 106px;
      padding: 10px 16px;
      border-radius: 12px;
      background: #10161f;
      border: 2px solid #ff6b35;
      color: #ff9d66;
      font-weight: 900;
      font-size: 32px;
      letter-spacing: 2px;
      text-align: center;
      box-shadow: 0 0 18px rgba(255, 107, 53, 0.2);
    }

    .indicators {
      display: flex;
      justify-content: center;
      align-items: center;
      flex: 1;
      gap: 12px;
    }

    .indicator {
      width: 40px;
      height: 40px;
      display: flex;
      align-items: center;
      justify-content: center;
      border-radius: 10px;
      font-size: 18px;
      background: #1c2432;
      border: 2px solid #2f394a;
      color: #5f6f86;
      transition: all 0.15s ease;
    }

    .indicator.active {
      background: linear-gradient(135deg, #ff7a45 0%, #ffb067 100%);
      border-color: #ffb067;
      color: white;
      box-shadow: 0 0 12px rgba(255, 122, 69, 0.7);
    }

    .main {
      flex: 1;
      display: grid;
      grid-template-columns: 1fr 1.2fr 0.9fr;
      gap: 18px;
      padding: 16px;
      height: calc(100vh - 76px);
    }

    .panel {
      background: linear-gradient(180deg, #1a212d 0%, #111822 100%);
      border-radius: 20px;
      border: 2px solid rgba(255,255,255,0.06);
      box-shadow: inset 0 1px 0 rgba(255,255,255,0.05);
    }

    .left-panel, .right-panel {
      display: flex;
      align-items: center;
      justify-content: center;
    }

    .steer-area {
      display: flex;
      flex-direction: row;
      gap: 16px;
      align-items: center;
      justify-content: center;
      width: 100%;
      height: 100%;
    }

    .steer-btn {
      width: 110px;
      height: 110px;
      border-radius: 50%;
      border: 3px solid #ff6b35;
      background: linear-gradient(135deg, #2b3340 0%, #181f2b 100%);
      color: #ff9d66;
      font-size: 42px;
      font-weight: bold;
      cursor: pointer;
      touch-action: manipulation;
      transition: transform 0.1s ease, box-shadow 0.1s ease;
      box-shadow: 0 10px 18px rgba(0,0,0,0.4);
      user-select: none;
    }

    .steer-btn:active {
      transform: scale(0.96);
      box-shadow: 0 6px 12px rgba(255, 107, 53, 0.35);
      background: linear-gradient(135deg, #ff7b45 0%, #ff9d66 100%);
      color: white;
    }

    .center-panel {
      display: flex;
      justify-content: center;
      align-items: center;
      padding: 14px;
    }

    .control-grid {
      width: min(100%, 300px);
      display: grid;
      grid-template-columns: repeat(2, minmax(120px, 1fr));
      gap: 14px;
    }

    .control-btn {
      width: 100%;
      aspect-ratio: 1;
      border: 2px solid rgba(255,255,255,0.08);
      border-radius: 50%;
      background: linear-gradient(135deg, #2a3445 0%, #171f2a 100%);
      color: #eef4ff;
      font-weight: 700;
      font-size: 12px;
      letter-spacing: 0.5px;
      cursor: pointer;
      touch-action: manipulation;
      box-shadow: 0 8px 16px rgba(0,0,0,0.45);
      transition: all 0.15s ease;
      display: flex;
      align-items: center;
      justify-content: center;
      text-align: center;
      padding: 8px;
      user-select: none;
    }

    .control-btn:active {
      transform: scale(0.95);
    }

    .control-btn.active {
      background: linear-gradient(135deg, #ff7b45 0%, #ffb067 100%);
      border-color: #ffc390;
      box-shadow: 0 0 16px rgba(255, 123, 69, 0.5);
      color: white;
    }

    .right-panel {
      padding: 12px;
    }

    .throttle-wrap {
      width: 100%;
      height: 100%;
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      gap: 10px;
    }

    .throttle-label {
      font-size: 16px;
      font-weight: 700;
      color: #9aa8bb;
      letter-spacing: 1px;
    }

    input[type=range] {
      writing-mode: bt-lr;
      -webkit-appearance: slider-vertical;
      appearance: slider-vertical;
      width: 48px;
      height: 360px;
      accent-color: #ff7b45;
      background: transparent;
      transform: rotate(180deg);
    }

    @media (max-width: 850px) {
      .main {
        grid-template-columns: 1fr 1.2fr 0.7fr;
      }
      .steer-btn {
        width: 86px;
        height: 86px;
      }
    }
  </style>
</head>
<body>
  <div class="topbar">
    <div class="gear-display" id="gearDisplay">N</div>
    <div class="indicators">
      <div class="indicator" id="headlightInd">🔦</div>
      <div class="indicator" id="rearInd">🔴</div>
      <div class="indicator" id="leftSignalInd">⬅</div>
      <div class="indicator" id="rightSignalInd">➡</div>
      <div class="indicator" id="hazardInd">⚠</div>
      <div class="indicator" id="nitroInd">⚡</div>
    </div>
  </div>

  <div class="main">
    <div class="panel left-panel">
      <div class="steer-area">
        <button class="steer-btn" id="leftBtn">◀</button>
        <button class="steer-btn" id="rightBtn">▶</button>
      </div>
    </div>

    <div class="panel center-panel">
      <div class="control-grid">
        <button class="control-btn" id="headlightBtn">HIGH BEAM</button>
        <button class="control-btn" id="lightBtn">LIGHT</button>
        <button class="control-btn" id="leftSignalBtn">LEFT SIGNAL</button>
        <button class="control-btn" id="rightSignalBtn">RIGHT SIGNAL</button>
        <button class="control-btn" id="hazardBtn">HAZARD</button>
        <button class="control-btn" id="nitroBtn">NITRO</button>
      </div>
    </div>

    <div class="panel right-panel">
      <div class="throttle-wrap">
        <div class="throttle-label">D5</div>
        <input id="throttleSlider" type="range" min="-5" max="5" step="1" value="0" orient="vertical">
        <div class="throttle-label">R5</div>
      </div>
    </div>
  </div>

  <script>
    const slider = document.getElementById('throttleSlider');
    const gearDisplay = document.getElementById('gearDisplay');

    const headlightBtn = document.getElementById('headlightBtn');
    const lightBtn = document.getElementById('lightBtn');
    const leftSignalBtn = document.getElementById('leftSignalBtn');
    const rightSignalBtn = document.getElementById('rightSignalBtn');
    const hazardBtn = document.getElementById('hazardBtn');
    const nitroBtn = document.getElementById('nitroBtn');

    const leftBtn = document.getElementById('leftBtn');
    const rightBtn = document.getElementById('rightBtn');

    const headlightInd = document.getElementById('headlightInd');
    const rearInd = document.getElementById('rearInd');
    const leftSignalInd = document.getElementById('leftSignalInd');
    const rightSignalInd = document.getElementById('rightSignalInd');
    const hazardInd = document.getElementById('hazardInd');
    const nitroInd = document.getElementById('nitroInd');

    let frontLight = false;
    let rearLight = false;
    let leftFlash = false;
    let rightFlash = false;
    let hazardFlash = false;
    let nitro = false;

    function sendCommand(cmd, val) {
      fetch('/cmd?cmd=' + cmd + '&val=' + val).catch(() => {});
    }

    function updateDisplay() {
      const mapValue = {
        '-5': 'R5', '-4': 'R4', '-3': 'R3', '-2': 'R2', '-1': 'R1',
        '0': 'N',
        '1': 'D1', '2': 'D2', '3': 'D3', '4': 'D4', '5': 'D5'
      };
      gearDisplay.textContent = mapValue[String(parseInt(slider.value))] || 'N';
    }

    function updateIndicators() {
      headlightInd.classList.toggle('active', frontLight);
      rearInd.classList.toggle('active', rearLight);
      leftSignalInd.classList.toggle('active', leftFlash);
      rightSignalInd.classList.toggle('active', rightFlash);
      hazardInd.classList.toggle('active', hazardFlash);
      nitroInd.classList.toggle('active', nitro);
    }

    slider.addEventListener('input', () => {
      const val = parseInt(slider.value);
      sendCommand('gas', val);
      updateDisplay();
    });

    slider.addEventListener('change', () => {
      slider.value = 0;
      sendCommand('gas', 0);
      updateDisplay();
    });

    leftBtn.addEventListener('pointerdown', () => sendCommand('steer', 'left'));
    leftBtn.addEventListener('pointerup', () => sendCommand('steer', 'stop'));
    leftBtn.addEventListener('pointerleave', () => sendCommand('steer', 'stop'));

    rightBtn.addEventListener('pointerdown', () => sendCommand('steer', 'right'));
    rightBtn.addEventListener('pointerup', () => sendCommand('steer', 'stop'));
    rightBtn.addEventListener('pointerleave', () => sendCommand('steer', 'stop'));

    headlightBtn.addEventListener('pointerdown', () => {
      frontLight = true;
      headlightBtn.classList.add('active');
      sendCommand('front', '1');
      updateIndicators();
    });
    headlightBtn.addEventListener('pointerup', () => {
      frontLight = false;
      headlightBtn.classList.remove('active');
      sendCommand('front', '0');
      updateIndicators();
    });
    headlightBtn.addEventListener('pointerleave', () => {
      frontLight = false;
      headlightBtn.classList.remove('active');
      sendCommand('front', '0');
      updateIndicators();
    });

    lightBtn.addEventListener('click', () => {
      rearLight = !rearLight;
      lightBtn.classList.toggle('active', rearLight);

      if (rearLight) {
        frontLight = true;
        headlightBtn.classList.add('active');
      } else {
        frontLight = false;
        headlightBtn.classList.remove('active');
      }

      sendCommand('rear', rearLight ? '1' : '0');
      updateIndicators();
    });

    leftSignalBtn.addEventListener('click', () => {
      if (hazardFlash) {
        hazardFlash = false;
        hazardBtn.classList.remove('active');
        sendCommand('hazard', '0');
      }
      leftFlash = !leftFlash;
      leftSignalBtn.classList.toggle('active', leftFlash);
      sendCommand('leftFlash', leftFlash ? '1' : '0');
      updateIndicators();
    });

    rightSignalBtn.addEventListener('click', () => {
      if (hazardFlash) {
        hazardFlash = false;
        hazardBtn.classList.remove('active');
        sendCommand('hazard', '0');
      }
      rightFlash = !rightFlash;
      rightSignalBtn.classList.toggle('active', rightFlash);
      sendCommand('rightFlash', rightFlash ? '1' : '0');
      updateIndicators();
    });

    hazardBtn.addEventListener('click', () => {
      hazardFlash = !hazardFlash;
      if (hazardFlash) {
        leftFlash = false;
        rightFlash = false;
        leftSignalBtn.classList.remove('active');
        rightSignalBtn.classList.remove('active');
      }
      hazardBtn.classList.toggle('active', hazardFlash);
      sendCommand('hazard', hazardFlash ? '1' : '0');
      updateIndicators();
    });

    nitroBtn.addEventListener('click', () => {
      nitro = !nitro;
      nitroBtn.classList.toggle('active', nitro);
      sendCommand('nitro', nitro ? '1' : '0');
      updateIndicators();
    });

    updateDisplay();
    updateIndicators();
  </script>
</body>
</html>
  )=====";
  return html;
}
