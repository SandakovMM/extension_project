// Class reaction work with on click user actions.
function Reaction() {  }
	//function clickFileCheck(clickedElem)
Reaction.clickFileCheck = function (clickedElem)
{
	// Now geting domain and file like this
	var value = clickedElem.value;
	var destPoints = value.split(' ');

	if (!clickedElem.checked) {
		//SocketWorker.stopReadSend(clickedElem.value) // Sending file names to server like that.
		$('setupAll').checked = false;
	}
	else {
		alert(destPoints[0] + ' and ' + destPoints[1])
		SocketWorker("ws://127.0.0.1:10000/", destPoints[0], destPoints[1]); // Try to connect
		//SocketWorker.startReadSend(destPoints[0], destPoints[1]) // Sending file names to server like that.
	}
}

Reaction.clickAllFiles = function(clickedElem)
{
	// found our label 
	//var elemId = clickedElem.id;
	//var allLabels = $$('label[for=' + elemId + ']')[0]; // sirios sheet need to remember it
	if (!clickedElem.checked) // if we uncheck box
		return;
	var elemValue = clickedElem.value;
	for (var i = 1; i <= elemValue; i++) {
		var logName = $('log' + i);
		logName.checked = true;
		//SocketWorker.startReadSend(logName.value) // Sending file names to server like that.
	}/**/
	//alert("See me!");
	EntryWorker.addEntry('Entry!');
}

// Class entry worker add entryes to list and add some css styles to entryes
function EntryWorker() {  }

EntryWorker.addEntry = function(entry)
{
	var list = $('logList');
	var newOption = document.createElement('option');
	newOption.innerHTML = entry;
	//newOption.style.backgroundColor = '#ff3019';
	newOption.className = 'good';
	list.appendChild(newOption);
}

// Class sender work with web socket connections
function SocketWorker(addr, host, filename) 
{ 
	if ("WebSocket" in window) {
		alert("WebSocket is supported by your Browser!");
     	// Start connection
	    var workSocket = new WebSocket(addr);
	    workSocket.onopen = function()
	    {
	        // Web Socket is connected, send data using send()
	        alert("Start send this: " + host + filename);
	        workSocket.send('/var/www/vhosts/' + host + '/logs/' + filename);
	        alert("Message is sent...");
	    };
	    workSocket.onmessage = function (evt) 
	    { 
	        var received_msg = evt.data;
	        alert("Message is received...");
	    };
	    workSocket.onclose = function()
	    { 
	        // websocket is closed.
	        alert("Connection is closed..."); 
	    };
	}
	else {
		alert("Web socket is not supported!");
	}
}
SocketWorker.startReadSend = function(host, filename) // function send server file name and start reading logs
{
	//workSocket.send('start'); //Send somthing meens that we need to start reading;
	workSocket.send('/var/www/vhosts/' + host + '/logs/' + filename); // some like this
}
SocketWorker.stopReadSend = function(host, filename) // function send server file name and start reading logs
{
	//workSocket.send('stop'); //Send somthing meens that we need to start reading;
	workSocket.send('/var/www/vhosts/' + host + '/logs/' + filename); // some like this
}
