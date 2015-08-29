// Scheduled job for downloading sensor data

var http = require('http');
var path = require('path');
var fs = require('fs');

var home_path="/mnt/HD/HD_b2/Work/home_automation/data/"
//var home_path="/home/ubuntu/workspace/commandline/"

var options = {
  hostname: 'pluto.gkov.hu',
  port: 10000,
  path: '/',
  method: 'GET'
};

//or as a Number prototype method:
Number.prototype.padLeft = function(n, str) {
  return Array(n - String(this).length + 1).join(str || '0') + this;
}

var mkdirSync = function(path) {
  try {
    fs.mkdirSync(path);
  }
  catch (e) {
    if (e.code != 'EEXIST') throw e;
  }
}

function storeJSON(data) {
  data = data.replace(/([a-z][a-zA-Z0-9]+):/g, '"$1":')

  // console.log(data);
  data = JSON.parse(data);

  var ttt = new Date(Date.parse(data.time));
  console.log(ttt.getDate().padLeft(2, '0') + "/" + ttt.getHours().padLeft(2, '0'));

  mkdirSync(path.join(home_path, (ttt.getYear() + 1900).padLeft(4, '0')));
  mkdirSync(path.join(home_path, (ttt.getYear() + 1900).padLeft(4, '0'), (ttt.getMonth() + 1).padLeft(2, '0')));
  var ppp = path.join(home_path, (ttt.getYear() + 1900).padLeft(4, '0'), (ttt.getMonth() + 1).padLeft(2, '0'), ttt.getDate().padLeft(2, '0'));
  mkdirSync(ppp);

  //console.log("--------");
  //console.log(data);
  var fname = ttt.getHours().padLeft(2, '0') + ttt.getMinutes().padLeft(2, '0') + ".json";
  ppp = path.join(ppp, fname);
  fs.writeFile(ppp, JSON.stringify(data), function(err) {
    if (err) {
      console.log(err);
    }
    else {
      console.log(ppp);
    }
  });
}

/* Date-based scheduler */
function runOnDate(date, job){
  console.log("Scheduled for: " + date);
  
	var now = (new Date()).getTime();
	var then = date.getTime();
	
	if (then < now)
    {
        process.nextTick(job);
        return null;
    }
	
	return setTimeout(job, (then - now));
}

function nextEvenMinute() {
  var date = new Date();
  date.setMilliseconds(0);
  date.setSeconds(0);
  date.setMinutes(date.getMinutes() + (2 - date.getMinutes() % 2));
  return date;
}


function readSensors() {
      console.log('Download started...');
      var res = http.get("http://192.168.1.44/", function(res) {
        console.log("Got response: " + res.statusCode);
        res.setEncoding('utf8');
        res.on('data', function(data) {
          console.log("Got data: " + data);
          storeJSON(data);
          runOnDate(nextEvenMinute(), readSensors);
        });
      }).on('error', function(e) {
        console.log("Got error: " + e.message);
        runOnDate(nextEvenMinute(), readSensors);
      });
}

runOnDate(nextEvenMinute(), readSensors);

