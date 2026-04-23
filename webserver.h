#pragma once
#include <ESP8266WebServer.h>
#include "led.h"
#include "weather.h"

ESP8266WebServer server(80);

static const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="zh">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>SmartLight</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:-apple-system,sans-serif;background:#0d0d0d;color:#eee;padding:14px;max-width:480px;margin:0 auto}
h1{text-align:center;font-size:1.3em;margin-bottom:14px;color:#fff;letter-spacing:2px}
.card{background:#1a1a1a;border-radius:14px;padding:14px;margin-bottom:12px;border:1px solid #252525}
.card-title{font-size:.75em;color:#666;margin-bottom:10px;text-transform:uppercase;letter-spacing:1px}
.info-grid{display:grid;grid-template-columns:1fr 1fr;gap:8px}
.info-grid-4{display:grid;grid-template-columns:repeat(4,1fr);gap:6px}
.info-item{background:#111;border-radius:10px;padding:10px;text-align:center}
.info-val{font-size:1.6em;font-weight:700;color:#f0a500}
.info-val.sm{font-size:1.1em}
.info-lbl{font-size:.68em;color:#555;margin-top:3px}
.power-row{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:12px}
.btn-power{padding:16px;border:none;border-radius:12px;font-size:1.05em;font-weight:600;cursor:pointer;transition:.15s}
.btn-on{background:linear-gradient(135deg,#f0a500,#e06000);color:#000}
.btn-off{background:#222;color:#888;border:1px solid #333}
.btn-power:active{transform:scale(.96)}
.slider-row{margin-bottom:10px}
.slider-label{display:flex;justify-content:space-between;font-size:.82em;color:#888;margin-bottom:4px}
input[type=range]{width:100%;height:6px;accent-color:#f0a500;cursor:pointer}
input[type=color]{width:100%;height:42px;border:none;border-radius:10px;cursor:pointer;background:none;padding:0}
.mode-grid{display:grid;grid-template-columns:repeat(4,1fr);gap:7px}
.mode-grid-3{display:grid;grid-template-columns:repeat(3,1fr);gap:7px}
.btn-mode{padding:9px 4px;border:1px solid #252525;border-radius:10px;
          background:#111;color:#aaa;font-size:.78em;cursor:pointer;transition:.15s;text-align:center}
.btn-mode:active{transform:scale(.93)}
.btn-mode.active{background:#1a3a5c;color:#4a9eff;border-color:#4a9eff}
.btn-mode .icon{font-size:1.2em;display:block;margin-bottom:2px}
.dot{display:inline-block;width:8px;height:8px;border-radius:50%;margin-right:5px;vertical-align:middle}
.dot-on{background:#4caf50;box-shadow:0 0 6px #4caf50}.dot-off{background:#555}
.mode-badge{display:inline-block;background:#1a3a5c;color:#4a9eff;border-radius:6px;padding:2px 8px;font-size:.75em;margin-left:6px}
.power-bar-wrap{background:#111;border-radius:8px;height:8px;margin-top:6px;overflow:hidden}
.power-bar{height:100%;background:linear-gradient(90deg,#4caf50,#f0a500,#f44336);border-radius:8px;transition:width .5s}
.section-gap{margin-top:8px}
</style>
</head>
<body>
<h1>&#128161; SmartLight</h1>

<!-- 环境状态 -->
<div class="card">
  <div class="card-title">&#127777; 环境状态</div>
  <div class="info-grid">
    <div class="info-item">
      <div class="info-val" id="tempVal">--</div>
      <div class="info-lbl">室内温度 °C</div>
    </div>
    <div class="info-item">
      <div class="info-val" id="humiVal">--</div>
      <div class="info-lbl">室内湿度 %</div>
    </div>
    <div class="info-item">
      <div class="info-val" id="outTempVal">--</div>
      <div class="info-lbl">室外温度 °C</div>
    </div>
    <div class="info-item">
      <div class="info-val" style="font-size:1em;padding-top:4px" id="weatherVal">--</div>
      <div class="info-lbl">庆阳天气</div>
    </div>
  </div>
</div>

<!-- 耗电统计 -->
<div class="card">
  <div class="card-title">&#9889; 耗电统计</div>
  <div class="info-grid-4">
    <div class="info-item">
      <div class="info-val sm" id="powerVal">--</div>
      <div class="info-lbl">当前功率 W</div>
    </div>
    <div class="info-item">
      <div class="info-val sm" id="currentVal">--</div>
      <div class="info-lbl">电流 mA</div>
    </div>
    <div class="info-item">
      <div class="info-val sm" id="whVal">--</div>
      <div class="info-lbl">累计 Wh</div>
    </div>
    <div class="info-item">
      <div class="info-val sm" id="uptimeVal">--</div>
      <div class="info-lbl">运行时长</div>
    </div>
  </div>
  <div class="power-bar-wrap section-gap">
    <div class="power-bar" id="powerBar" style="width:0%"></div>
  </div>
  <div style="font-size:.7em;color:#444;margin-top:4px;text-align:right">
    最大功率: <span id="maxPower">--</span> W
  </div>
</div>

<!-- 灯光控制 -->
<div class="card">
  <div class="card-title">
    <span class="dot dot-off" id="statusDot"></span>灯光控制
    <span class="mode-badge" id="modeBadge">--</span>
  </div>
  <div class="power-row">
    <button class="btn-power btn-on"  onclick="setLight('on')">&#9728; 开灯</button>
    <button class="btn-power btn-off" onclick="setLight('off')">&#9866; 关灯</button>
  </div>
  <div class="slider-row">
    <div class="slider-label"><span>亮度</span><span id="brightNum">80</span>%</div>
    <input type="range" id="bright" min="1" max="100" value="80"
           oninput="document.getElementById('brightNum').innerText=this.value"
           onchange="setBright(this.value)">
  </div>
  <div class="slider-row">
    <div class="slider-label"><span>颜色</span></div>
    <input type="color" id="colorPick" value="#ffffff" onchange="setColor(this.value)">
  </div>
</div>

<!-- 照明模式 -->
<div class="card">
  <div class="card-title">&#128294; 照明模式</div>
  <div class="mode-grid-3">
    <div class="btn-mode" onclick="setMode('white',this)"><span class="icon">&#9728;</span>白光</div>
    <div class="btn-mode" onclick="setMode('warm',this)"><span class="icon">&#127774;</span>暖白</div>
    <div class="btn-mode" onclick="setMode('cool',this)"><span class="icon">&#10052;</span>冷白</div>
  </div>
</div>

<!-- 动态效果 -->
<div class="card">
  <div class="card-title">&#10024; 动态效果</div>
  <div class="mode-grid">
    <div class="btn-mode" onclick="setMode('rainbow',this)"><span class="icon">&#127752;</span>彩虹</div>
    <div class="btn-mode" onclick="setMode('breath',this)"><span class="icon">&#128148;</span>呼吸</div>
    <div class="btn-mode" onclick="setMode('colorbreath',this)"><span class="icon">&#127775;</span>彩呼吸</div>
    <div class="btn-mode" onclick="setMode('flow',this)"><span class="icon">&#127803;</span>流水</div>
    <div class="btn-mode" onclick="setMode('wave',this)"><span class="icon">&#127754;</span>波浪</div>
    <div class="btn-mode" onclick="setMode('meteor',this)"><span class="icon">&#9732;</span>流星</div>
    <div class="btn-mode" onclick="setMode('sparkle',this)"><span class="icon">&#10022;</span>星光</div>
    <div class="btn-mode" onclick="setMode('chase',this)"><span class="icon">&#9654;</span>跑马</div>
    <div class="btn-mode" onclick="setMode('fire',this)"><span class="icon">&#128293;</span>火焰</div>
    <div class="btn-mode" onclick="setMode('ocean',this)"><span class="icon">&#127754;</span>海洋</div>
    <div class="btn-mode" onclick="setMode('candle',this)"><span class="icon">&#128367;</span>烛光</div>
    <div class="btn-mode" onclick="setMode('rainbowstatic',this)"><span class="icon">&#127752;</span>彩虹静</div>
  </div>
</div>

<!-- 场景模式 -->
<div class="card">
  <div class="card-title">&#127968; 场景模式</div>
  <div class="mode-grid">
    <div class="btn-mode" onclick="setMode('ambient',this)"><span class="icon">&#127777;</span>氛围</div>
    <div class="btn-mode" onclick="setMode('segment',this)"><span class="icon">&#127912;</span>分段</div>
    <div class="btn-mode" onclick="setMode('police',this)"><span class="icon">&#128680;</span>警察灯</div>
    <div class="btn-mode" onclick="setMode('alert',this)"><span class="icon">&#128721;</span>警报</div>
  </div>
</div>

<script>
// 最大功率用于进度条
let maxPowerW = 0;

function setLight(s){fetch('/light?cmd='+s).then(()=>refreshStatus());}
function setBright(v){fetch('/light?cmd=bright&val='+v).then(()=>refreshStatus());}
function setColor(hex){
  let r=parseInt(hex.slice(1,3),16),g=parseInt(hex.slice(3,5),16),b=parseInt(hex.slice(5,7),16);
  fetch('/light?cmd=color&r='+r+'&g='+g+'&b='+b).then(()=>refreshStatus());
}
function setMode(m,el){
  document.querySelectorAll('.btn-mode').forEach(b=>b.classList.remove('active'));
  if(el)el.classList.add('active');
  fetch('/light?cmd=mode&val='+m).then(()=>refreshStatus());
}

// 格式化时长
function fmtTime(s){
  let h=Math.floor(s/3600),m=Math.floor((s%3600)/60),sec=s%60;
  if(h>0) return h+'h'+m+'m';
  if(m>0) return m+'m'+sec+'s';
  return sec+'s';
}

function refreshStatus(){
  fetch('/status').then(r=>r.json()).then(d=>{
    document.getElementById('tempVal').innerText    = d.temp;
    document.getElementById('humiVal').innerText    = d.humi;
    document.getElementById('outTempVal').innerText = d.outTemp;
    document.getElementById('weatherVal').innerText = d.weather;
    document.getElementById('bright').value         = d.bright;
    document.getElementById('brightNum').innerText  = d.bright;
    document.getElementById('modeBadge').innerText  = d.mode;
    document.getElementById('powerVal').innerText   = d.power;
    document.getElementById('currentVal').innerText = d.current;
    document.getElementById('whVal').innerText      = d.wh;
    document.getElementById('uptimeVal').innerText  = fmtTime(d.uptime);
    let dot=document.getElementById('statusDot');
    dot.className='dot '+(d.on?'dot-on':'dot-off');
    // 功率进度条
    let pw=parseFloat(d.power)||0;
    if(pw>maxPowerW) maxPowerW=pw;
    document.getElementById('maxPower').innerText=maxPowerW.toFixed(1);
    let pct=maxPowerW>0?Math.min(pw/maxPowerW*100,100):0;
    document.getElementById('powerBar').style.width=pct+'%';
  });
}
refreshStatus();
setInterval(refreshStatus,5000);
</script>
</body>
</html>
)rawhtml";

extern float temperature, humidity;

void handleRoot() { server.send_P(200,"text/html",HTML_PAGE); }

void handleLight() {
  String cmd=server.arg("cmd");
  if      (cmd=="on")     { setLightOn(true); }
  else if (cmd=="off")    { setLightOn(false); }
  else if (cmd=="bright") { brightness=constrain(server.arg("val").toInt(),1,100); setLightOn(true); }
  else if (cmd=="color")  {
    uint8_t r=server.arg("r").toInt(),g=server.arg("g").toInt(),b=server.arg("b").toInt();
    rgbColor=((uint32_t)r<<16)|((uint32_t)g<<8)|b;
    lightMode="custom"; setLightOn(true);
  }
  else if (cmd=="mode")   { lightMode=server.arg("val"); setLightOn(true); }
  server.send(200,"text/plain","ok");
}

void handleStatus() {
  unsigned long upSec = millis() / 1000UL;
  float pw  = estimatePowerW();
  float cur = estimateCurrentMA();
  float wh  = getTotalWh();
  String json="{";
  json+="\"on\":"      +(String)(lightOn?"true":"false")+",";
  json+="\"bright\":"  +String(brightness)+",";
  json+="\"mode\":\""  +lightMode+"\",";
  json+="\"temp\":"    +String(temperature,1)+",";
  json+="\"humi\":"    +String(humidity,0)+",";
  json+="\"outTemp\":" +weatherTemp+",";
  json+="\"weather\":\""+weatherToEn(weatherText)+"\",";
  json+="\"power\":\""  +String(pw,1)+"\",";
  json+="\"current\":\""+String(cur,0)+"\",";
  json+="\"wh\":\""     +String(wh,2)+"\",";
  json+="\"uptime\":"   +String(upSec);
  json+="}";
  server.send(200,"application/json",json);
}

void webServerBegin() {
  server.on("/",       handleRoot);
  server.on("/light",  handleLight);
  server.on("/status", handleStatus);
  server.begin();
}

void webServerHandle() { server.handleClient(); }
