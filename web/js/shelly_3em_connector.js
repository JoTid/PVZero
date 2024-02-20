var timerRefresh;
var countNA = 0;

window.addEventListener("DOMContentLoaded", function(event) {
    // document.getElementById('check_now').addEventListener("mousedown", function(){
    //     call("/check");
    //     clearTimeout(timerRefresh);
    //     timerRefresh = setTimeout(getJSON.bind(null, "/pvzero/state.json", "fillPVZeroState"), 1000);
    // }, false);

    setTimeout(getJSON.bind(null, "/pvzero/state.json", "fillPVZeroState"), 1000);
    // getJSON("/pvzero/state.json", "fillPVZeroState");
});

function call(url) {
    var oReq = new XMLHttpRequest();
    oReq.open("get", url, true);
    oReq.send();
}

function setTrueFalse(elname, value, strTrue="Leer", colorTrue="red", strFalse="Voll", colorFalse="green") {
    element = document.getElementById(elname);
    if (element == undefined)
        return;
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
    minutes =  Math.floor(secs / 60);
    secs %= 60;
    result = "";
    if (hours > 0) { result += hours + 'h'; }
    if (minutes > 0) { result += minutes + 'm'; }
    if (secs > 0) { result += secs + 's'; }
    if (value == 0) { result = '-'; }
    return result;
}

function setTrueFalseSVG(svgcycle, elname, value, strTrue="Leer", colorTrue="red", strFalse="Voll", colorFalse="green") {
    element = svgcycle.getElementById(elname);
    if (element == undefined)
        return;
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
    document.getElementById("consumption_power").innerHTML = '<label id="info_consumption_power">' + data["consumption_power"] + "</label>";
    
}

function fillPVZeroState(data, url) {
    clearTimeout(timerRefresh);
    document.getElementById("pvzero_title").innerText = data["name"];
    document.getElementById("version").innerText = 'v' + data["version"];
    document.getElementById("consumption_power").innerText = 'Aktueller Verbrauch: ' + data["consumption_power"] + ' W';
    document.getElementById("feed_in_power").innerText =
      "Einspeisung: " + data["feed_in_power"] + " W";
    document.getElementById("check_interval").innerText =
      "Prüfintervall: " + data["check_interval"] + "s";
    document.getElementById("info_check_info").innerText = data["check_info"];

    if (data["consumption_power"] == 0) {
        countNA = 0;
        timerRefresh = setTimeout(getJSON.bind(null, "/pvzero/state.json", "fillPVZeroState"), 1000);
    } else if ( countNA < 3) {
        countNA = countNA + 1;
        timerRefresh = setTimeout(getJSON.bind(null, "/pvzero/state.json", "fillPVZeroState"), 1000);
    } else {
        timerRefresh = setTimeout(getJSON.bind(null, "/pvzero/state.json", "fillPVZeroState"), 10000);
    }
}
