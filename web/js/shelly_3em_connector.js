var timerRefresh;

window.addEventListener("DOMContentLoaded", function (event) {
  // document.getElementById('check_now').addEventListener("mousedown", function(){
  //     call("/check");
  //     clearTimeout(timerRefresh);
  //     timerRefresh = setTimeout(getJSON.bind(null, "/pvzero/state.json", "fillPVZeroState"), 1000);
  // }, false);

  setTimeout(getJSON.bind(null, "/pvzero/state.json", "fillPVZeroState"), 500);
});

function call(url) {
  var oReq = new XMLHttpRequest();
  oReq.open("get", url, true);
  oReq.send();
}

function setTrueFalse(
  elname,
  value,
  strTrue = "Leer",
  colorTrue = "red",
  strFalse = "Voll",
  colorFalse = "green"
) {
  element = document.getElementById(elname);
  if (element == undefined) return;
  if (value) {
    element.innerText = strTrue;
    element.style.color = colorTrue;
  } else {
    element.innerText = strFalse;
    element.style.color = colorFalse;
  }
}

function sec2str(value) {
  secs = value;
  hours = Math.floor(secs / 3600);
  secs %= 3600;
  minutes = Math.floor(secs / 60);
  secs %= 60;
  result = "";
  if (hours > 0) {
    result += hours + "h";
  }
  if (minutes > 0) {
    result += minutes + "m";
  }
  if (secs > 0) {
    result += secs + "s";
  }
  if (value == 0) {
    result = "-";
  }
  return result;
}

function setTrueFalseSVG(
  svgcycle,
  elname,
  value,
  strTrue = "Leer",
  colorTrue = "red",
  strFalse = "Voll",
  colorFalse = "green"
) {
  element = svgcycle.getElementById(elname);
  if (element == undefined) return;
  if (value) {
    element.textContent = strTrue;
    element.style.fill = colorTrue;
  } else {
    element.textContent = strFalse;
    element.style.fill = colorFalse;
  }
}

var color_full = "rgb(64, 203, 231)";
var color_invisible = "rgb(0, 0, 0, 0.0)";
var color_grey = "rgb(113, 113, 113)";
var color_green = "rgb(34, 153, 7)";
function updateCycle(cycle, data) {
  co = data[cycle];
  // document.getElementById("info_" + cycle).innerHTML = '<label id="info_check">' + data["check_info"] + "</label>";
  document.getElementById("consumption_power").innerHTML =
    '<label id="info_consumption_power">' +
    data["consumption_power"] +
    "</label>";
}

function fillPVZeroState(data, url) {
  document.getElementById("pvzero_title").innerText = data["name"];
  document.getElementById("version").innerText = "v" + data["version"];

  document.getElementById("total_consumption").innerText =
    "Gesamtverbrauch: " + data["total_consumption"].toFixed(0) + " W";
  document.getElementById("battery_current").innerText =
    "Batteriestrom: " + data["battery_current"].toFixed(1) + " A";

  document.getElementById("consumption_power").innerText =
    "Aktueller Verbrauch: " + data["consumption_power"].toFixed(0) + " W";
  document.getElementById("feed_in_power").innerText =
    "Einspeisung: " + data["feed_in_power"].toFixed(0) + " W";
  document.getElementById("check_interval").innerText =
    "Prüfintervall: " + data["check_interval"] + "s";

  if (data["mppt_available"]) {
    document.getElementById("mppt_w").innerText = data["mppt_w"].toFixed(0) + " W";
    document.getElementById("mppt_v").innerText = data["mppt_v"].toFixed(1) + " V";
    document.getElementById("mppt_a").innerText = data["mppt_a"].toFixed(1) + " A";
  }

  if (data["psu1_available"]) {
    document.getElementById("psu1_w").innerText = data["psu1_w"].toFixed(0) + " W";
    document.getElementById("psu1_v").innerText = data["psu1_v"].toFixed(1) + " V";
    document.getElementById("psu1_a").innerText = data["psu1_a"].toFixed(1) + " A";
    document.getElementById("psu1_target_w").innerText = data["psu1_target_w"].toFixed(0) + " W";
    document.getElementById("psu1_target_v").innerText = data["psu1_target_v"].toFixed(1) + " V";
    document.getElementById("psu1_target_a").innerText = data["psu1_target_a"].toFixed(1) + " A";
  }

  if (data["psu2_available"]) {
    document.getElementById("psu2_w").innerText = data["psu2_w"].toFixed(0) + " W";
    document.getElementById("psu2_v").innerText = data["psu2_v"].toFixed(1) + " V";
    document.getElementById("psu2_a").innerText = data["psu2_a"].toFixed(1) + " A";
    document.getElementById("psu2_target_w").innerText = data["psu2_target_w"].toFixed(0) + " W";
    document.getElementById("psu2_target_v").innerText = data["psu2_target_v"].toFixed(1) + " V";
    document.getElementById("psu2_target_a").innerText = data["psu2_target_a"].toFixed(1) + " A";
  }

  if (data["analog_available"]) {
    document.getElementById("analog_v").innerText = data["analog_v"].toFixed(2) + " V";
  }

  document.getElementById("battery_state").innerText =
    "Status: " + data["battery_state"];
  document.getElementById("info_battery_state").innerText =
    data["battery_state_info"];
  document.getElementById("charge_voltage").innerText =
    "Charge Voltage: " + data["charge_voltage"].toFixed(2) + " V";
  document.getElementById("charge_current").innerText =
    "Charge Current: " + data["charge_current"].toFixed(2) + " A";
  document.getElementById("info_check_info").innerText = data["check_info"];

  if (document.getElementById("autoupdate").checked) {
    console.log(`Set timer for update in 1 sec`);
    timerRefresh = setTimeout(getJSON.bind(null, "/pvzero/state.json", "fillPVZeroState"), 1000);
  }
}
