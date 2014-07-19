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
		SocketWorker("127.0.0.1:10000", destPoints[0], destPoints[1]); // Try to connect
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
		alert("Start connection. Addres is " + addr);
		var workSocket = new WebSocket(addr);
		alert("Start connection in progress?");
		workSocket.onopen = function() {
			alert('soket was opened');
			workSocket.send('/var/www/vhosts/' + host + '/logs/' + filename); // some like this
			alert('name was sended');
		};
		workSocket.onclose = function(event) {alert('Socket was closed!');};
		workSocket.onmessage = function(event) {EntryWorker.addEntry(event.data);}; // adding entry on message
		workSocket.onclose = function(err) {alert('Socket error!');};
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
