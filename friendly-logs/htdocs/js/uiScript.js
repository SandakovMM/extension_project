// Call init socket function ol load of document
document.observe('dom:loaded', function(){ SocketWorker(); });

// Class reaction work with on click user actions.
function Reaction() {  }
	//function clickFileCheck(clickedElem)
Reaction.clickFileCheck = function (clickedElem)
{
	// Now geting domain and file like this
	var fileName = clickedElem.value;
	var savedVars = $('someInformation').value;
	var domainName = savedVars.split(' ')[0];
	if (!clickedElem.checked) {
		//SocketWorker.stopReadSend(separated[0], fileName)) // Sending file names to server like that.
		$('setupAll').checked = false;
	}
	else {
		//alert(separated[0] + ' on adress ' + separated[1] + ' and ' + clickedElem.value);
		if (!workSocket) {
			alert('Socket is not initialyze yet');
			//SocketWorker("ws://" + separated[1] + ":10030/"); // Try init socket. // change later
		}
		else {
			SocketWorker.startReadSend(domainName, fileName) // Sending file names to server like that.
		}
	}
}

Reaction.clickAllFiles = function(clickedElem)
{
	// found our label 
	//var elemId = clickedElem.id;
	//var allLabels = $$('label[for=' + elemId + ']')[0]; // sirios sheet need to remember it
	if (!clickedElem.checked) // if we uncheck box
		return;

	var domainName = $('someInformation');
	var elemValue = clickedElem.value;
	for (var i = 1; i <= elemValue; i++) {
		var logName = $('log' + i);
		if(!logName.checked) {
			logName.checked = true;
			SocketWorker.startReadSend(logName.value); // Sending file names to server like that.
		}
	}/**/
}

// Class entry worker add entryes to list and add some css styles to entryes
function EntryWorker() {  }

EntryWorker.addEntry = function(entry)
{
	var list = $('logList');
	var allEntryes = entry.split('\n');
	//alert(allEntryes[0] + ' ' + allEntryes[1]);
	for (var i = 0; i < allEntryes.length - 1; i++) {
		var newOption = document.createElement('option');
		newOption.innerHTML = allEntryes[i];
		// choosing style of entry
		var rand = Math.random() * 10 + 1;
		if (rand < 3)
			newOption.className = 'good';
		//	newOption.style.background = 'linear-gradient(to right, red, #ffffff';
		else if (rand < 6)
			newOption.className = 'warning';
		//	newOption.style.background = 'linear-gradient(to right, yellow, #ffffff)';
		else 
			newOption.className = 'error';
		//	newOption.style.background = 'linear-gradient(to right, green, #ffffff)';
		
		//newOption.className = 'good';
		list.add(newOption, list[0]);
	}
}

// Class sender work with web socket connections
var workSocket;
function SocketWorker() // Initialyze socket
{ 
	if ("WebSocket" in window) {
		//alert("WebSocket is supported by your Browser!");
		var savedVars = $('someInformation').value;
		var ipAdress = savedVars.split(' ')[1];
     	// Start connection
	    workSocket = new WebSocket("wss://" + ipAdress + ":10020/");
	    workSocket.onopen = function()
	    {
	        // Web Socket is connected, send data using send()
	        alert("Socket is opened");
	        //alert("Start send this: " + host + filename);
	        //workSocket.send('/var/www/vhosts/' + host + '/logs/' + filename);
	        //alert("Message is sent...");
	    };
	    workSocket.onmessage = function (evt) 
	    { 
	    	
	        var received_msg = evt.data;
	        //alert("Message is received: " + received_msg);
	        EntryWorker.addEntry(received_msg);        
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
	workSocket.send('add /var/www/vhosts/system/' + host + '/logs/' + filename); // some like this
}
SocketWorker.stopReadSend = function(host, filename) // function send server file name and start reading logs
{
	//workSocket.send('stop'); //Send somthing meens that we need to start reading;
	workSocket.send('del /var/www/vhosts/system/' + host + '/logs/' + filename); // some like this
}
