var json_uri = "http://pluto.gkov.hu:10000/"

if (!String.format) {
  String.format = function(format) {
    var args = Array.prototype.slice.call(arguments, 1);
    return format.replace(/{(\d+)}/g, function(match, number) { 
      return typeof args[number] != 'undefined'
        ? args[number] 
        : match
      ;
    });
  };
}

function load() {
    var html = "";
    html = html + "<table id='temptable'>";
    for (i = 0; i < 6; ++i) {
        html = html + "<tr>";
        for (j = 0; j < 4; ++j) {
            html = html + "<th>";
            hour = i * 4 + j;
            html = html + hour;
            html = html + "</th>";
        }
        html = html + "</tr>";
        html = html + "<tr>";
        for (j = 0; j < 4; ++j) {
            html = html + "<td>";
            for (k = 0; k < 4; ++k) {
                index = i * 16 + j * 4 + k;
                html = html + '<span><input onclick="sendSettings()" type="checkbox" name="option' + index + '" value="" id="check' + index + '"></span>';
            }
            html = html + "</td>";
        }
        html = html + "</tr>";
    }
    html = html + "<table>";

    document.getElementById("checkTable").innerHTML = html;

    loadXMLDoc();

    setInterval(function() {
        window.loadXMLDoc();
        //window.location.reload(1);
    }, 2000);

}

function loadXMLDoc() {
    var xmlhttp;
    if (window.XMLHttpRequest) {
        // code for IE7+, Firefox, Chrome, Opera, Safari
        xmlhttp = new XMLHttpRequest();
    }
    else {
        // code for IE6, IE5
        xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
    }
    xmlhttp.onreadystatechange = function() {
        if (xmlhttp.readyState == 4 && xmlhttp.status == 200) {
            var response = eval("(" + xmlhttp.responseText + ")");
            var contrTimeStr = response.time;
            var contrTime = new Date(Date.parse(response.time));
            document.getElementById("clock").innerHTML = contrTimeStr;
            document.getElementById("start").innerHTML = convertHumanReadableTime(response.start);

            if (response.sensors1 != null) {
                document.getElementById("lastSensorData_1").innerHTML = response.sensors1.lastReceived;
                document.getElementById("longestSensorWait_1").innerHTML = response.sensors1.longestWait;
                document.getElementById("tempDHT11_1").innerHTML = response.sensors1.tempDHT11;
                document.getElementById("humidityDHT11_1").innerHTML = response.sensors1.humidityDHT11;
                document.getElementById("tempDS18B20_1").innerHTML = parseFloat(response.sensors1.tempDS18B20) / 100;
            }

            if (response.sensors2 != null) {
                document.getElementById("lastSensorData_2").innerHTML = response.sensors2.lastReceived;
                document.getElementById("longestSensorWait_2").innerHTML = response.sensors2.longestWait;
                document.getElementById("tempDHT11_2").innerHTML = response.sensors2.tempDHT11;
                document.getElementById("humidityDHT11_2").innerHTML = response.sensors2.humidityDHT11;
                document.getElementById("tempDS18B20_2").innerHTML = parseFloat(response.sensors2.tempDS18B20) / 100;
            }

            if (response.sensors3 != null) {
                document.getElementById("lastSensorData_3").innerHTML = response.sensors3.lastReceived;
                document.getElementById("longestSensorWait_3").innerHTML = response.sensors3.longestWait;
                document.getElementById("tempDHT11_3").innerHTML = response.sensors3.tempDHT11;
                document.getElementById("humidityDHT11_3").innerHTML = response.sensors3.humidityDHT11;
                document.getElementById("tempDS18B20_3").innerHTML = parseFloat(response.sensors3.tempDS18B20) / 100;
            }

            var st = response.settings;
            for (var i = 0; i < 96; ++i) {
                if (st.charAt(i) == '1') {
                    document.getElementById("check" + i).checked = true;
                }
                else {
                    document.getElementById("check" + i).checked = false;
                }
                document.getElementById("check" + i).parentNode.style.backgroundColor = "";
            }
            
            var checkindex = contrTime.getHours() * 4 + Math.trunc(contrTime.getMinutes() / 15);
            document.getElementById("check" + checkindex).parentNode.style.backgroundColor = "red";
        }
    }
    xmlhttp.open("GET", json_uri, true);
    xmlhttp.send();
}

function sendSettings() {
    var st = "";
    for (i = 0; i < 96; ++i) {
        st = st + (document.getElementById("check" + i).checked ? "1" : "0");
    }
    //window.alert(st);
    var xmlhttp;

    if (window.XMLHttpRequest) {
        // code for IE7+, Firefox, Chrome, Opera, Safari
        xmlhttp = new XMLHttpRequest();
    }
    else {
        // code for IE6, IE5
        var xmlhttp = new ActiveXObject("Microsoft.XMLHTTP");
    }
    xmlhttp.open("GET", json_uri + "S-" + st, true);
    xmlhttp.onreadystatechange = function() {
        //window.alert("ready=" + xmlhttp.readyState + "; status=" + xmlhttp.status);
    }
    xmlhttp.send();
}

function convertHumanReadableTime(time)
{
    var seconds = time;
    var minutes = Math.trunc(time / 60); seconds -= minutes * 60;
    var hours = Math.trunc(minutes / 60); minutes -= hours * 60;
    var days = Math.trunc(hours / 24); hours -= days * 24;
    return String.format("{0} nap {1} óra {2} perc {3} másodperc", days, hours, minutes, seconds);
}