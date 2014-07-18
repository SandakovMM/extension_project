// Class reaction work with on click user actions.
function Reaction() {  }
	//function clickFileCheck(clickedElem)
Reaction.clickFileCheck = function (clickedElem)
{
	if (!clickedElem.checked) {
		//SocketWorker.stopReadSend(clickedElem.value) // Sending file names to server like that.
		$('setupAll').checked = false;
	}
	else {
		//SocketWorker.startReadSend(clickedElem.value) // Sending file names to server like that.
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
	this.workSocket = new WebSocket(addr);
	workSocket.onopen = function() {
		alert('soket was opened');
		workSocket.send('/var/www/vhosts/' + host + '/logs/' + filename); // some like this
		alert('name was sended');
	};
	workSocket.onclose = function(event) {alert('Socket was closed!');};
	workSocket.onmessage = function(event) {EntryWorker.addEntry(event.data);}; // adding entry on message
	workSocket.onclose = function(err) {alert('Socket error!');};
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
