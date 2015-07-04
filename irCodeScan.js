var child_process = require('child_process');

var baseCode = "10AF";
var startCode = parseInt("F7", 16);
var knownCodes = {
	"10AF8877":"On/Off",
	"10AF609F":"Timer",
	"10AF807F":"Fan Speed +",
	"10AF20DF":"Fan Speed -",
	"10AF708F":"Temp/Timer +",
	"10AFB04F":"Temp/Timer -",
	"10AF906F":"Cool",
	"10AF40BF":"Energy Saver",
	"10AFF00F":"Auto Fan",
	"10AFE01F":"Fan Only",
	"10AF00FF":"Sleep"
};

var pad = function(chr, str, width) {
	while (str.length < width) {
		str = chr + str;
	}
	return str.substring(str.length - width, str.length).toUpperCase();
}

for (var i = startCode; i < 256; i++) {
	var cmd = pad("0", i.toString(16), 2);
	var inv = pad("0", ((~i)>>>0).toString(16), 2)
	var fullCode = baseCode + cmd + inv;
	if (knownCodes[fullCode]) {
		console.log(fullCode + " - " + knownCodes[fullCode]);
	} else {
		for (var t = 0; t < 3; t++) {
			console.log(fullCode);
			child_process.execSync('particle call ac-controller-1 sendNEC ' + fullCode);
		}
	}
}
